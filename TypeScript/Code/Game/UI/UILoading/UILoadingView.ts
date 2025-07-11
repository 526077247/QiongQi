import { IOnCreate } from "../../../Module/UI/IOnCreate";
import { UIBaseView } from "../../../Module/UI/UIBaseView";
import { UIProgressBar } from "../../../Module/UIComponent/UIProgressBar";

export class UILoadingView extends UIBaseView implements IOnCreate{

    public static readonly PrefabPath:string = "/Game/AssetsPackage/UI/UILoading/Prefabs/UILoadingView.UILoadingView_C";
    private slider: UIProgressBar
    public getConstructor()
    {
        return UILoadingView;
    }

    public onCreate()
    {
        this.slider = this.addComponent(UIProgressBar,"Slider");
    }

    public setProgress(value: number)
    {
        this.slider.setValue(value);
    }
}