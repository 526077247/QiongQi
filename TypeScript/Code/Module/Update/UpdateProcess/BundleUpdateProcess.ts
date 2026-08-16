import { UeDownloadHelper } from "ue";
import { HttpManager } from "../../../../Mono/Module/Http/HttpManager";
import { JsonHelper } from "../../../../Mono/Helper/JsonHelper";
import { Log } from "../../../../Mono/Module/Log/Log";
import { Define } from "../../../../Mono/Define";
import { ServerConfigManager } from "../ServerConfigManager";
import { CdnFileInfo, CdnVersionManifest } from "../VersionManifest";
import { UpdateRes } from "../UpdateRes";
import { UpdateTask } from "../UpdateTask";
import { UpdateProcess } from "./UpdateProcess";

/** 更新检查结果 */
export class UpdateCheckResult {
    /** 是否需要下载更新 */
    public needUpdate: boolean = false;
    /** 最新版本号 */
    public version: number = 0;
    /** 待下载总大小（字节） */
    public totalSize: number = 0;
    /** 待下载文件列表 */
    public downloadFiles: CdnFileInfo[] = [];
    /** 最新版本全部文件清单 */
    public allFiles: CdnFileInfo[] = [];
}

/**
 * CDN 资源热更流程：
 *  检查（{版本号}.json → md5 对比）→ 用户确认 → 下载（断点续传 + 失败重试）
 *  → 校验 md5 → 挂载 pak → 记录本地版本 → 成功返回 Restart（重启 JS 虚拟机加载新 Code）。
 *
 * CDN 目录结构（打包面板 CopyResultsToRelease 展开输出，无版本/平台子目录）：
 *  {CDN}/{渠道}_{平台}/{版本号}.json
 *  {CDN}/{渠道}_{平台}/xxx.pak
 *
 * 
 *  - 构造参数 cacheDownload：true=下载并缓存；false=仅检查，需要更新时直接 Restart
 *  - forceUpdate：由 Define.ForceUpdate 或更新列表 ForceUpdate=1 决定，取消时返回 Quit
 *  - task.remoteManifest：写入远端版本清单
 */
export class BundleUpdateProcess extends UpdateProcess {

    public versionManifest: CdnVersionManifest = null;
    public checkResult: UpdateCheckResult = null;


    /** 是否强制更新 */
    private forceUpdate: boolean;

    /** 构造 BundleUpdateProcess */
    public constructor() {
        super();
    }

    /** 流程入口：检查 → 确认 → 下载 */
    public async process(task: UpdateTask): Promise<UpdateRes> {
        const channel = ServerConfigManager.instance.getChannel();
        const maxAppResVer = ServerConfigManager.instance.findMaxUpdateResVerThisAppVer(channel, task.appVer);

        const version = ServerConfigManager.instance.getLocalVersion();
        this.forceUpdate = Define.ForceUpdate;
        var verInfo = ServerConfigManager.instance.getResVerInfo(channel, version);
        if (verInfo != null && verInfo.ForceUpdate == 1)
            this.forceUpdate = true;

        // Step 2: 通过 ServerConfigManager 查找当前渠道的最大资源版本号
        let maxVer = ServerConfigManager.instance.findMaxUpdateResVer(channel, "", maxAppResVer);
        if (!maxVer) {
            Log.warning("[HotUpdate] No remote version found, using built-in.");
            return UpdateRes.Over;
        }

        if (!maxAppResVer)
        {
            maxVer = version;
        }

        Log.info(`[HotUpdate] Max version: ${maxVer}`);

        // Step 3: 拉取 CDN 版本清单（文件名用版本号命名：{maxVer}.json）
        const routerListUrl = ServerConfigManager.instance.getUpdateListUrl();
        const platform = ServerConfigManager.instance.getPlatformName();
        const versionUrl = `${routerListUrl}/${channel}_${platform}/${maxVer}.json`;
        JsonHelper.registerClass(CdnFileInfo, "CdnFileInfo");
        JsonHelper.registerClass(CdnVersionManifest, "CdnVersionManifest");
        this.versionManifest = await HttpManager.instance.httpGetResult(CdnVersionManifest, versionUrl, null, null);
        if (!this.versionManifest) {
            Log.error("[HotUpdate] Failed to fetch CDN manifest.");
            return await this.updateFail(task);
        }
        task.remoteManifest = this.versionManifest;

        // Step 4: 全量进首包短路：本地版本已不低于最新版本时，资源已随包内置，无需下载 CDN 累积增量。
        // 新装包本地版本已由 C++ 初始化 SyncLocalVersionFromPackage 对齐到包内版本，此处直接跳过下载。
        const helper = UeDownloadHelper.GetInstance();
        if (helper.IsFullInFirstPak() && version >= this.versionManifest.version) {
            Log.info(`[HotUpdate] 全量进首包，本地版本(${version})已是最新(${this.versionManifest.version})，跳过下载`);
            return UpdateRes.Over;
        }

        // 需要更新但未开启缓存下载 → 直接 Restart
        const result = new UpdateCheckResult();

        // 2. md5 对比：本地已有且完整（md5 一致）的文件跳过，其余进入下载列表
        const pakDir = helper.GetCdnPakDir();
        let totalSize = 0;
        for (const file of this.versionManifest.files) {
            // CDN pak 保留原始文件名，本地路径直接用清单中的 name
            const localPath = `${pakDir}/${this.localFileName(file.name)}`;
            const localMd5 = helper.CalcFileMd5(localPath);
            if (localMd5.length > 0 && localMd5 === file.md5) {
                continue; // 已下载且完整，文件级断点
            }
            result.downloadFiles.push(file);
            totalSize += file.size;
        }

        result.needUpdate = result.downloadFiles.length > 0;
        result.version = this.versionManifest.version;
        result.totalSize = totalSize;
        result.allFiles = this.versionManifest.files;
        // 关键：execute() 读取 this.checkResult 作为下载清单，必须在此赋值。
        // 若此处不赋值（checkResult 恒为 null），execute() 会直接返回 true（假成功），
        // 文件从未下载，重启 JsEnv 后再次触发更新 → 无限循环。
        this.checkResult = result;

        if(!result.needUpdate){
            return UpdateRes.Over;
        }

        
        const localVersion = ServerConfigManager.instance.getLocalVersion();
        Log.info(
            `[Update] 本地版本=${localVersion < 0 ? "(无)" : localVersion} 最新版本=${result.version} ` +
            `待下载=${result.downloadFiles.length} 个文件，共 ${(totalSize / 1024 / 1024).toFixed(2)}MB`
        );

        // Step 5: 提示用户下载大小
        const sizeMb = result.totalSize / (1024 * 1024);
        const displayMb = sizeMb > 0 && sizeMb < 0.01 ? 0.01 : sizeMb;
        Log.info(`[HotUpdate] Download size: ${displayMb.toFixed(2)} MB (${result.totalSize} bytes, ${result.downloadFiles.length} bundles)`);

        const content = `需要下载 ${displayMb.toFixed(2)} MB 资源，是否继续？`;
        const confirmed = await task.showMsgBoxView(content, "确认", this.forceUpdate ? "退出" : "跳过");
        if (!confirmed) {
            Log.info("[Update] 用户选择稍后再说，跳过本次更新");
            return this.forceUpdate ? UpdateRes.Quit : UpdateRes.Over;
        }

        // Step 6: 下载并挂载（本项目实现）
        const ok = await this.execute(task);
        if (ok) {
            Log.info("[Update] 更新完成，重启 JS 虚拟机加载新代码");
            return UpdateRes.Restart;
        }
        return await this.updateFail(task);
    }

    /** 执行下载并挂载 */
    public async execute(task: UpdateTask): Promise<boolean> {
        if (this.checkResult == null || !this.checkResult.needUpdate) {
            return true;
        }

        const channel = ServerConfigManager.instance.getChannel();
        const routerListUrl  = ServerConfigManager.instance.getUpdateListUrl();
        const platform = ServerConfigManager.instance.getPlatformName();
        const version = this.checkResult.version;
        const helper = UeDownloadHelper.GetInstance();
        const pakDir = helper.GetCdnPakDir();
        const totalSize = Math.max(this.checkResult.totalSize, 1);
        let doneSize = 0;

        // 1. 并发下载（断点续传 + 失败重试）：将待下载列表按大小降序排序（副本，不影响挂载顺序），
        //    每轮取 1 个大文件（数组头）+ 4 个小文件（数组尾）同批并行下载：
        //    大文件耗时长、小文件耗时短，混搭并行可摊平总耗时，避免“大文件拖尾”或“小文件空转”。
        const downloadQueue = this.checkResult.downloadFiles.slice().sort((a, b) => b.size - a.size);
        const urlPrefix = `${routerListUrl}/${channel}_${platform}`;
        const BIG_COUNT = 1;   // 每批从数组头（大文件）取的数量
        const SMALL_COUNT = 4; // 每批从数组尾（小文件）取的数量
        while (downloadQueue.length > 0) {
            // 每轮：数组头 BIG_COUNT 个大文件 + 数组尾 SMALL_COUNT 个小文件
            const batch: CdnFileInfo[] = [];
            for (let i = 0; i < BIG_COUNT && downloadQueue.length > 0; i++) {
                const big = downloadQueue.shift();
                if (big) batch.push(big);
            }
            for (let i = 0; i < SMALL_COUNT && downloadQueue.length > 0; i++) {
                const small = downloadQueue.pop();
                if (small) batch.push(small);
            }

            const results = await Promise.all(batch.map((file) => this.downloadOne(
                task, file, helper, pakDir, urlPrefix,
                (p) => {
                    // 通过 task.setDownloadSize 上报总大小/当前大小（已完成部分 + 当前文件实时进度）
                    const currentSize = doneSize + file.size * p;
                    task.setDownloadSize(totalSize, Math.floor(currentSize));
                }
            )));

            // 本批任一文件下载/校验失败即整体失败
            if (results.some((ok) => !ok)) {
                return false;
            }

            // 本批全部成功，累加已下载大小
            for (const file of batch) {
                doneSize += file.size;
            }
        }

        // 2. 挂载 pak（PakOrder 递增，保证高版本/后下载的资源优先；顺序仍按清单原序，与下载排序无关）
        //    IoStore 容器文件（.utoc/.ucas）只下载不挂载：UE5.5 挂载 .pak 时自动关联同名 .utoc/.ucas，
        //    对容器文件调用 MountPak 会因扩展名不符而失败，故在此跳过。
        const downloadList = this.checkResult.downloadFiles;
        for (let i = 0; i < downloadList.length; i++) {
            const saveName = this.localFileName(downloadList[i].name);
            if (!saveName.toLowerCase().endsWith(".pak")) {
                Log.info(`[Update] 跳过挂载（IoStore 容器文件）: ${saveName}`);
                continue;
            }
            const pakPath = `${pakDir}/${saveName}`;
            const order = UpdateTask.CDN_PAK_ORDER + i;
            if (helper.MountPak(pakPath, order)) {
                Log.info(`[Update] 挂载成功: ${saveName} (PakOrder=${order})`);
            } else {
                Log.error(`[Update] 挂载失败: ${saveName}`);
                return false;
            }
        }

        // 3. 记录本地版本
        ServerConfigManager.instance.saveLocalVersion(version);
        return true;
    }

    /**
     * 下载单个文件（断点续传 + 失败重试），完成后校验 md5。
     * @param urlPrefix  远端 URL 前缀（不含文件名）
     * @param onProgress 进度回调（0~1）
     * @returns 是否下载成功且 md5 校验通过
     */
    private async downloadOne(
        task: UpdateTask,
        file: CdnFileInfo,
        helper: UeDownloadHelper,
        pakDir: string,
        urlPrefix: string,
        onProgress: (progress01: number) => void
    ): Promise<boolean> {
        const url = `${urlPrefix}/${file.name}`;
        const saveName = this.localFileName(file.name);

        let success = false;
        for (let retry = 0; retry < UpdateTask.MAX_RETRY; retry++) {
            success = await UpdateTask.download(url, saveName, onProgress);
            if (success) break;
            Log.warning(`[Update] 下载失败(${retry + 1}/${UpdateTask.MAX_RETRY})，稍后重试: ${url}`);
            await new Promise<void>((resolve) => setTimeout(resolve, UpdateTask.retryDelayMs(retry)));
        }
        if (!success) {
            Log.error(`[Update] 下载失败，已重试 ${UpdateTask.MAX_RETRY} 次: ${url}`);
            return false;
        }

        // 下载完成后校验 md5（文件级完整性校验）
        const localPath = `${pakDir}/${saveName}`;
        const localMd5 = helper.CalcFileMd5(localPath);
        if (localMd5.length === 0 || localMd5 !== file.md5) {
            Log.error(`[Update] 文件校验失败（md5 不一致）: ${saveName}`);
            return false;
        }
        return true;
    }

    /**
     * 下载失败处理
     * 提示用户重试或跳过/退出
     */
    private async updateFail(task: UpdateTask): Promise<UpdateRes> {
        const btnState = await task.showMsgBoxView("更新失败，请检查网络后重试", "重试", this.forceUpdate ? "退出" : "跳过");
        if (btnState) {
            // 用户选择重试 → 重新执行 process
            return await this.process(task);
        } else if (this.forceUpdate) {
            // 强制更新但用户选择退出
            return UpdateRes.Quit;
        }

        // 非强制更新, 用户选择跳过
        return UpdateRes.Over;
    }

    /** 本地保存文件名（取相对路径最后一段，避免嵌套目录） */
    public localFileName(fileName: string): string {
        const idx = fileName.lastIndexOf("/");
        return idx >= 0 ? fileName.substring(idx + 1) : fileName;
    }
}
