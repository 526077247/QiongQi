import { ServerConfigManager } from "../ServerConfigManager";
import { UpdateRes } from "../UpdateRes";
import { UpdateTask } from "../UpdateTask";
import { UpdateProcess } from "./UpdateProcess";
import { CacheManager } from "../../Player/CacheManager";
import { Define } from "../../../../Mono/Define";

/**
 * SH 服标记流程：
 * 依据 CDN 更新列表判断当前渠道/版本是否属于 SH 服，写入 Define.isSH。
 * 调试期可通过 CacheManager "DEBUG_IsSH" 覆盖（1=是，2=否）。
 */
export class UpdateIsSHProcess extends UpdateProcess {
    public async process(task: UpdateTask): Promise<UpdateRes> {
        const channel = ServerConfigManager.instance.getChannel();

        // 调试覆盖：DEBUG_IsSH 0=自动，1=是，2=否
        const setVal = CacheManager.instance.getInt("DEBUG_IsSH", 0);
        if (setVal === 0) {
            Define.isSH = ServerConfigManager.instance.findMaxUpdateResVerThisAppVer(channel, task.appVer) != null;
        } else {
            Define.isSH = setVal === 1;
        }
        return UpdateRes.Over;
    }
}
