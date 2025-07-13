import { Log } from "../../../Mono/Module/Log/Log";
import { I18NKey } from "../Const/I18NKey";
import { I18NManager } from "../I18N/I18NManager";
import { II18N } from "../I18N/II18N";
import { UIBaseComponent } from "../UI/UIBaseComponent";
import * as string from "../../../Mono/Helper/StringHelper"
import { Color, LinearColor, SlateColor, TextBlock } from "ue";

export class UIText extends UIBaseComponent implements II18N {

    public getConstructor(){
        return UIText;
    }
    private text: TextBlock;
    private textKey: I18NKey|null = null;
    private keyParams : any;

    private activatingComponent()
    {
        if (this.text == null)
        {
            const widget = this.getWidget();
            if (!(widget instanceof TextBlock))
            {
                Log.error(`添加UI侧组件UIText时，物体${widget.GetName()}不是TextBlock组件`);
            }
            else
            {
                this.text = widget as TextBlock;
            }
        }
    }

    public onLanguageChange()
    {
        this.activatingComponent();
        if (!!this.textKey)
        {
            let text = I18NManager.instance.i18NGetText(this.textKey);
            if (!string.isNullOrEmpty(text) && this.keyParams != null)
                text = string.format(text, this.keyParams);
            this.text.SetText(text);
        }
    }

    public getText(): string
    {
        this.activatingComponent();
        return this.text.Text;
    }

    public setText(text: string)
    {
        this.activatingComponent();
        if(text === undefined){
            Log.error("SetText undefined")
        }
        this.textKey = null;
        this.text.SetText(text);
    }

    public setI18NKey(key: I18NKey, ...paras: any[])
    {
        if (key == null)
        {
            this.setText("");
            return;
        }
        this.textKey = key;
        this.setI18NText(paras);
    }

    public setI18NText(...paras: any[])
    {
        if (this.textKey == null)
        {
            Log.error("there is not key ");
        }
        else
        {
            this.keyParams = paras;
            let text = I18NManager.instance.i18NGetText(this.textKey);
            if (!string.isNullOrEmpty(text) && paras != null)
                text = string.format(text, paras);
            this.text.SetText(text);
        }
    }

    public setTextColor(color: Color | string | LinearColor| SlateColor)
    {
        const slateColor = this.text.ColorAndOpacity;
        if(color instanceof Color){
            this.activatingComponent();
            slateColor.SpecifiedColor = new LinearColor(color);
            this.text.SetColorAndOpacity(slateColor);
            return;
        }
        if(color instanceof LinearColor){
            this.activatingComponent();
            slateColor.SpecifiedColor = color;
            this.text.SetColorAndOpacity(slateColor);
            return;
        }
        if(color instanceof SlateColor){
            this.activatingComponent();
            this.text.SetColorAndOpacity(color);
            return;
        }
        if(string.isNullOrEmpty(color)) return;
        this.activatingComponent();
        const colorRgb = Color.FromHex(color);
        slateColor.SpecifiedColor = new LinearColor(colorRgb);
        this.text.SetColorAndOpacity(slateColor);
    }
}