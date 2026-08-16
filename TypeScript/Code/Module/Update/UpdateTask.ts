import { ETTask } from "../../../ThirdParty/ETTask/ETTask";
import { $Delegate, BuiltinString, NewMap, TMap, UeDownloadHelper, UeHttpHelper } from "ue";
import { Define } from "../../../Mono/Define";
import { Log } from "../../../Mono/Module/Log/Log";
import { CdnVersionManifest } from "./VersionManifest";
import { UpdateRes } from "./UpdateRes";
import { UpdateProcess } from "./UpdateProcess/UpdateProcess";
import { MsgBoxPara, UIMsgBoxWin } from "../../Game/UI/UICommon/UIMsgBoxWin";
import { UIManager } from "../UI/UIManager";
import { UIBaseView } from "../UI/UIBaseView";
import { UILayerNames } from "../UI/UILayerNames";
import { ObjectPool } from "../../../Mono/Core/ObjectPool";
import { ManagerProvider } from "../../../Mono/Core/Manager/ManagerProvider";


/**
 * 热更任务：流程上下文 + 下载封装 + CDN 更新设置（原 UpdateSetting，已合并至此）。
 * CDN 目录结构（与打包面板 CopyResultsToRelease 产出一致，资源直接展开到渠道平台根目录）：
 *   {routerListUrl}/{channel}_{platform}/{版本号}.json
 *   {routerListUrl}/{channel}_{platform}/xxx.pak
 *  - init(updateProgress, ...processes) + process()：顺序串联各流程
 *  - setDownloadSize(total, current)：通知下载进度
 *  - fetchText(url)：拉取远端文本
 *  - restartGame()：热更完成后通知 C++ 重启整个 JS 虚拟机（require 缓存清空，加载新 Code）
 */
export class UpdateTask {
    // ================= CDN 更新设置 =================

    /** 是否启用 CDN 更新流程 */
    public static readonly enabled: boolean = true;

    /** CDN pak 挂载优先级基数（高于首包默认 0，保证 CDN 新资源优先） */
    public static readonly CDN_PAK_ORDER: number = 100;

    /** 单次下载超时（秒） */
    public static readonly DOWNLOAD_TIMEOUT: number = 30;

    /** 下载失败最大重试次数 */
    public static readonly MAX_RETRY: number = 3;

    /** 失败重试基础等待（毫秒） */
    public static readonly RETRY_BASE_DELAY_MS: number = 1000;
    // ================= 任务上下文 =================

    /** App 版本号 */
    public appVer: number = 1;

    /** 同时下载数量上限 */
    public downloadingMaxNum: number = 10;

    /** 远端版本清单 */
    public remoteManifest: CdnVersionManifest = null;

    /** 跳过热更标记（未配置 CDN 时由 SetUpdateListProcess 置 true） */
    public skipUpdate: boolean = false;

    private list: UpdateProcess[] = [];

    private onDownloadSize: (totalSize: number, currentSize: number) => void = null;

    /**
     * 初始化流程列表
     * @param downloadSizeCallback 下载总大小/当前大小回调（由 UIUpdateView 转发至进度条）
     * @param processes           按序执行的更新流程
     */
    public async init(downloadSizeCallback: (totalSize: number, currentSize: number) => void, ...processes: UpdateProcess[]): Promise<void> {
        this.onDownloadSize = downloadSizeCallback;
        this.list = processes;
    }

    /** 顺序执行各流程，任一返回 Fail/Quit/Restart 立即返回该结果 */
    public async process(): Promise<UpdateRes> {
        if (this.list == null || this.list.length === 0) {
            Log.error("[Update] UpdateTask not initialized.");
            return UpdateRes.Fail;
        }
        for (const p of this.list) {
            const result = await p.process(this);
            if (result !== UpdateRes.Over) {
                return result;
            }
        }
        return UpdateRes.Over;
    }

    /** 上报下载总大小与当前大小 */
    public setDownloadSize(totalSize: number, currentSize: number): void {
        if (this.onDownloadSize != null) {
            this.onDownloadSize(totalSize, currentSize);
        }
    }

    /** 拉取远端文本（HttpGet），失败返回 null */
    public async fetchText(url: string): Promise<string> {
        if (url == null || url === "") {
            return null;
        }
        try {
            const response = ETTask.create<string>();
            const onResult = ((bSuccess: boolean, statusCode: number, responseText: string) => {
                response.setResult(bSuccess ? responseText : null);
            }) as unknown as $Delegate<(bSuccess: boolean, StatusCode: number, ResponseText: string) => void>;

            UeHttpHelper.GetInstance().HttpGet(
                url,
                NewMap(BuiltinString, BuiltinString) as unknown as TMap<string, string>,
                UpdateTask.DOWNLOAD_TIMEOUT,
                onResult
            );

            const text = await (response as unknown as Promise<string>);
            if (text == null || text === "") {
                Log.warning(`[Update] fetchText 失败: ${url}`);
                return null;
            }
            return text;
        } catch (e) {
            Log.error("[Update] fetchText 异常:", e);
            return null;
        }
    }

    /**
     * 重启游戏。
     * UE 环境重启整个 JS 虚拟机（Puerts JsEnv）。
     * 调用 C++ UQiongQiGameInstance::RestartJsEnv（AsyncTask 异步重建，require 缓存清空）。
     */
    public static async restartGame(): Promise<any> {
        try {
            await UIManager.instance.destroyAllWindow();
            ObjectPool.instance.dispose();
            ManagerProvider.clear();
            (Define.Game as any).RestartJsEnv();
        } catch (e) {
            Log.error("[Update] 重启 JS 虚拟机失败:", e);
        }
    }

    /** 本地 App 版本（UE 环境无独立 App 版本接口，固定 1，由 task.appVer 传入使用） */
    public static get UpdateAppVer(): number {
        return 1;
    }

    /**
     * 下载单个文件到 Saved/Paks/{saveName}。
     * @param url        远端地址
     * @param saveName   本地保存文件名
     * @param onProgress 进度回调（0~1，含续传量）
     * @returns 是否下载成功
     */
    public static download(url: string, saveName: string, onProgress: (progress01: number) => void): ETTask<boolean> {
        const response = ETTask.create<boolean>();
        const helper = UeDownloadHelper.GetInstance();

        const onCompleted = ((bSuccess: boolean, filePath: string) => {
            response.setResult(bSuccess);
        }) as unknown as $Delegate<(bSuccess: boolean, FilePath: string) => void>;

        const onProgressDelegate = ((progress01: number) => {
            if (onProgress != null) {
                onProgress(progress01);
            }
        }) as unknown as $Delegate<(Progress01: number, BytesReceived: bigint, TotalBytes: bigint) => void>;

        helper.DownloadFile(url, saveName, UpdateTask.DOWNLOAD_TIMEOUT, onCompleted, onProgressDelegate);
        return response;
    }

    /** 重试等待（毫秒） */
    public static retryDelayMs(retryIndex: number): number {
        return UpdateTask.RETRY_BASE_DELAY_MS * (retryIndex + 1);
    }

    // === MsgBox ===

    private msgBoxPara: MsgBoxPara = new MsgBoxPara();

    /**
     * 显示提示窗 (参考 TaoWu UpdateTask.ShowMsgBoxView)
     * @param content 内容
     * @param confirmText 确认按钮文本
     * @param cancelText 取消按钮文本
     * @returns true=确认, false=取消
     */
    public async showMsgBoxView(content: string, confirmText: string, cancelText: string): Promise<boolean> {
        return new Promise<boolean>(async (resolve) => {
            this.msgBoxPara.content = content;
            this.msgBoxPara.confirmText = confirmText;
            this.msgBoxPara.cancelText = cancelText;
            this.msgBoxPara.confirmCallback = (win: UIBaseView) => {
                resolve(true);
                UIManager.instance.closeBox(win);
            };
            this.msgBoxPara.cancelCallback = (win: UIBaseView) => {
                resolve(false);
                UIManager.instance.closeBox(win);
            };
            await UIManager.instance.openBox<UIMsgBoxWin, MsgBoxPara>(
                UIMsgBoxWin, UIMsgBoxWin.PrefabPath,
                this.msgBoxPara, null, null, null, UILayerNames.TipLayer
            );
        });
    }
}
