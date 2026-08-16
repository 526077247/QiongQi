import { UIBaseView, uiView } from "../../../Module/UI/UIBaseView";
import { IOnCreate } from "../../../Module/UI/IOnCreate";
import { IOnEnable } from "../../../Module/UI/IOnEnable";
import { UIProgressBar } from "../../../Module/UIComponent/UIProgressBar";
import { Log } from "../../../../Mono/Module/Log/Log";
import { UpdateTask } from "../../../Module/Update/UpdateTask";
import { UpdateRes } from "../../../Module/Update/UpdateRes";
import { SetUpdateListProcess } from "../../../Module/Update/UpdateProcess/SetUpdateListProcess";
import { AppUpdateProcess } from "../../../Module/Update/UpdateProcess/AppUpdateProcess";
import { UpdateIsSHProcess } from "../../../Module/Update/UpdateProcess/UpdateIsSHProcess";
import { BundleUpdateProcess } from "../../../Module/Update/UpdateProcess/BundleUpdateProcess";

/**
 * 热更流程控制视图：
 * 打开后自动执行"拉取更新列表 → App 检查 → SH 标记 → Bundle 检查/确认/下载"，
 * 进度展示在 UILoading 预制体的 Slider 上。
 *  - Restart：已下载新代码 → 重启整个 JS 虚拟机（C++ RestartJsEnv）
 *  - Over/Fail/Quit：回退当前 Code，回调进入游戏
 * onEnable 的 func 参数由 UIManager.openWindow 的 p1 透传（进入游戏回调）。
 */
@uiView("UIUpdateView")
export class UIUpdateView extends UIBaseView implements IOnCreate, IOnEnable<() => void> {

    public static readonly PrefabPath: string = "/Game/AssetsPackage/UI/UILoading/Prefabs/UILoadingView.UILoadingView_C";

    private slider: UIProgressBar;
    private onOver: () => void;

    public getConstructor() {
        return UIUpdateView;
    }

    public onCreate() {
        this.slider = this.addComponent(UIProgressBar, "Slider");
    }

    public onEnable(func: () => void) {
        this.onOver = func;
        if (this.slider != null) this.slider.setValue(0);
        this.startCheckUpdate();
    }

    /** 启动热更流程（init(updateProgress, ...processes) 顺序串联） */
    private async startCheckUpdate() {
        try {
            const task = new UpdateTask();
            await task.init(
                this.updateProgress.bind(this),
                new SetUpdateListProcess(),
                new AppUpdateProcess(),
                new UpdateIsSHProcess(),
                new BundleUpdateProcess()
            );
            const res = await task.process();
            if (res === UpdateRes.Restart) {
                Log.info("[Update] 已下载新代码，重启 JS 虚拟机加载新 Code");
                UpdateTask.restartGame(); // 不调用 onOver，等待 C++ 异步重启
            } else {
                Log.info(`[Update] 更新流程结束(res=${res})，进入游戏`);
                this.onOver?.(); // Over/Fail/Quit 均回退当前 Code 进入游戏
            }
        } catch (e) {
            Log.error("[Update] 更新流程异常，回退本地资源进入游戏:", e);
            this.onOver?.();
        }
    }

    /** 下载进度回调（total 总字节，current 当前字节） */
    private updateProgress(total: number, current: number) {
        const percent = total > 0 ? current / total : 0;
        if (this.slider != null) this.slider.setNormalizedValue(percent);
        Log.info(`[Update] 下载进度: ${(percent * 100).toFixed(1)}% (${current}/${total})`);
    }
}
