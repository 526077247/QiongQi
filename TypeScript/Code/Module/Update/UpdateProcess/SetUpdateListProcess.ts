import { Log } from "../../../../Mono/Module/Log/Log";
import { JsonHelper } from "../../../../Mono/Helper/JsonHelper";
import { ServerConfigManager } from "../ServerConfigManager";
import { UpdateRes } from "../UpdateRes";
import { UpdateTask } from "../UpdateTask";
import { UpdateProcess } from "./UpdateProcess";
import { AppConfig, Resver, UpdateListConfig } from "../../../../Mono/Module/Resource/VersionManifest";

/**
 * 更新列表前置流程：
 * 从 CDN 拉取 update_{platform}.list 写入 ServerConfigManager，
 * 供后续 AppUpdateProcess / UpdateIsSHProcess / BundleUpdateProcess 查询。
 * 同时将下载所需上下文写入 task（UE 适配）。
 */
export class SetUpdateListProcess extends UpdateProcess {
    public async process(task: UpdateTask): Promise<UpdateRes> {
        if (!UpdateTask.enabled) {
            Log.info("[HotUpdate] Disabled, skip.");
            return UpdateRes.Over;
        }

        try {
            // Step 1: 拉取 CDN update_{platform}.list (URL 由 ServerConfigManager 决定)
            const listText = await task.fetchText(ServerConfigManager.instance.getUpdateListCdnUrl());
            if (!listText) {
                Log.warning("[HotUpdate] Failed to fetch update list, using built-in bundles.");
                return UpdateRes.Over;
            }
            JsonHelper.registerClass(AppConfig, 'AppConfig');
            JsonHelper.registerClass(Resver, 'Resver');
            const updateList = JsonHelper.fromJson(UpdateListConfig, listText);
            ServerConfigManager.instance.setUpdateList(updateList);

            return UpdateRes.Over;
        } catch (e: any) {
            Log.error("[HotUpdate] SetUpdateListProcess error:", e);
            return UpdateRes.Over;
        }
    }
}
