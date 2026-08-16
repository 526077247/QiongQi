import * as UE from "ue";
import { Log } from "../../../../Mono/Module/Log/Log";
import { ServerConfigManager } from "../ServerConfigManager";
import { UpdateRes } from "../UpdateRes";
import { UpdateTask } from "../UpdateTask";
import { UpdateProcess } from "./UpdateProcess";
import { CacheManager } from "../../Player/CacheManager";
import { Define } from "../../../../Mono/Define";
import { AppConfig } from "../../../../Mono/Module/Resource/VersionManifest";

/**
 * App 版本检查流程
 * 依据 CDN 更新列表（update_{platform}.list 的 AppList）判断是否存在新 App 版本。
 * 有则弹窗提示，确认后打开浏览器下载；强制更新时取消则返回 Quit。
 */
export class AppUpdateProcess extends UpdateProcess {
    public async process(task: UpdateTask): Promise<UpdateRes> {
        // 渠道来自打包时写入 DefaultGame.ini 的 [QiongQi] ChannelName（无则回退 Default）
        const appChannel = ServerConfigManager.instance.getChannel();
        const channelAppUpdateList: AppConfig = ServerConfigManager.instance.getAppUpdateListByChannel(appChannel);
        if (channelAppUpdateList == null || channelAppUpdateList.AppVer == null) {
            Log.info("[Update] App 更新列表为空，跳过 App 更新检查");
            return UpdateRes.Over;
        }

        const version = ServerConfigManager.instance.findMaxUpdateAppVer(appChannel);
        Log.info(`[HotUpdate] FindMaxUpdateAppVer = ${version}`);
        if (version < 0) {
            Log.info("[HotUpdate] CheckAppUpdate maxVer is nil");
            return UpdateRes.Over;
        }

        const appVer = task.appVer;
        const flag = appVer - version;
        Log.info(`[HotUpdate] CheckAppUpdate AppVer:${appVer} maxVer:${version}`);
        if (flag >= 0) {
            Log.info(`[HotUpdate] CheckAppUpdate AppVer is Most Max Version, so return; flag = ${flag}`);
            return UpdateRes.Over;
        }

        const appURL = channelAppUpdateList.AppUrl;
        const verInfo = channelAppUpdateList.AppVer[String(appVer)] ?? null;
        Log.info(`[HotUpdate] CheckAppUpdate app_url = ${appURL}`);

        // 不强制更新时, ForceUpdate=-1 直接不提示
        const forceUpdateGlobal = Define.ForceUpdate;
        if (!forceUpdateGlobal && verInfo && verInfo.ForceUpdate === -1) {
            return UpdateRes.Over;
        }

        let forceUpdate = forceUpdateGlobal;
        if (verInfo && verInfo.ForceUpdate !== 0) {
            forceUpdate = true;
        }

        // 非强制更新时, 检查是否已跳过过此版本
        const checkKey = `CheckAppUpdate${version}`;
        const check = CacheManager.instance.getInt(checkKey, 0);
        if (check !== 0 && !forceUpdate) {
            return UpdateRes.Over;
        }

        const cancelBtnText = forceUpdate ? "退出" : "进入游戏";
        const contentUpdate = forceUpdate ? "需要重新下载" : "有新版本可下载";
        const btnState = await task.showMsgBoxView(contentUpdate, "确认", cancelBtnText);
        // 暂时直接跳过
        Log.info(`[HotUpdate] App update available: ${appVer} → ${version}, url: ${appURL}, force: ${forceUpdate}`);
        if (btnState) {
            Log.info(`[Update] 打开浏览器下载 App: ${appURL}`);
            try {
                UE.KismetSystemLibrary.LaunchURL(appURL);
            } catch (e) {
                Log.error("[Update] 打开浏览器失败:", e);
            }
            return await this.process(task);
        } else if (forceUpdate) {
            return UpdateRes.Quit;
        } else {
            CacheManager.instance.setInt(checkKey, 1);
        }
        return UpdateRes.Over;
    }
}
