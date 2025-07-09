import { IOnCreate } from "../../../Module/UI/IOnCreate";
import { UIBaseView } from "../../../Module/UI/UIBaseView";
import { UISlider } from "../../../Module/UIComponent/UISlider";

export class UILoadingView extends UIBaseView implements IOnCreate{

    public static readonly PrefabPath:string = "/Game/AssetsPackage/UI/UILoading/Prefabs/UILoadingView.UILoadingView_C";
    private slider: UISlider
    public getConstructor()
    {
        return UILoadingView;
    }

    public onCreate()
    {
        this.slider = this.addComponent(UISlider,"Slider");
    }

    public setProgress(value: number)
    {
        this.slider.setValue(value);
    }
}