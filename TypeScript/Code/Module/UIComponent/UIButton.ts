import { Log } from "../../../Mono/Module/Log/Log";
import { IOnDestroy } from "../UI/IOnDestroy";
import { UIBaseContainer } from "../UI/UIBaseContainer";
import * as string from "../../../Mono/Helper/StringHelper"
import { Button,Color, LinearColor, SlateColor } from "ue";
export class UIButton extends UIBaseContainer implements IOnDestroy {

    public getConstructor(){
        return UIButton;
    }
    private button: Button;
    private onClick: () => void;

    private onClickSelf:any;
    public onDestroy(){
        this.removeOnClick();
    }

    private activatingComponent()
    {
        if (this.button == null)
        {
            const widget = this.getWidget();
            if (!(widget instanceof Button))
            {
                Log.error(`添加UI侧组件UIButton时，物体${widget.GetName()}不是Button组件`);
            }
            else
            {
                this.button = widget as Button;
            }
        }
    }

    public setOnClick(callback: () => void)
    {
        this.activatingComponent();
        this.removeOnClick();
        this.onClick = callback;
        if(!this.onClickSelf){
            this.onClickSelf = this.onClickEvent.bind(this)
        }
        this.button.OnClicked.Add(this.onClickSelf);
    }

    public removeOnClick()
    {
        if (!!this.onClick)
        {
            this.button.OnClicked.Remove(this.onClickSelf);
            this.onClick = null;
        }
    }

    private onClickEvent(){
        //SoundComponent.Instance.PlaySound("Audio/Common/Click.mp3");
        if(!!this.onClick){
            this.onClick()
        }
    }

    public setInteractable(flag: boolean)
    {
        this.activatingComponent();
        this.button.SetIsEnabled(flag);
    }

    public setColor(color: string | Color | LinearColor| SlateColor)
    {
        if(color instanceof Color){
            this.activatingComponent();
            this.button.SetColorAndOpacity(new LinearColor(color));
            return;
        }
        if(color instanceof LinearColor){
            this.activatingComponent();
            this.button.SetColorAndOpacity(color);
            return;
        }
        if(color instanceof SlateColor){
            this.activatingComponent();
            this.button.SetColorAndOpacity(color.SpecifiedColor);
            return;
        }
        if(string.isNullOrEmpty(color)) return;
        this.activatingComponent();
        const colorRgb = Color.FromHex(color);
        this.button.SetColorAndOpacity(new LinearColor(colorRgb));
    }

    public getColor(): Color
    {
        this.activatingComponent();
        return this.button.ColorAndOpacity.ToRGBE();
    }

    public setImageAlpha(a: number)
    {
        this.activatingComponent();
        const color = this.button.ColorAndOpacity;
        color.A = a;
        this.button.SetColorAndOpacity(color);
    }
}