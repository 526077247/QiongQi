import { ProgressBar } from "ue";
import { Log } from "../../../Mono/Module/Log/Log";
import { IUpdate } from "../../../Mono/Module/Update/IUpdate";
import { IOnDestroy } from "../UI/IOnDestroy";
import { UIBaseComponent } from "../UI/UIBaseComponent";

export class UISlider extends UIBaseComponent implements IOnDestroy,IUpdate {

    public getConstructor(){
        return UISlider;
    }
    private slider: ProgressBar;
    private onValueChanged: (val:number) => void;
    private isWholeNumbers: boolean;
    private valueList: [];

    private min: number = 0;
    private max: number = 1;

    private isSetting: boolean = false;

    private _lastValue:number;
    private get sliderValue():number{
        return this.min + this.slider.Percent*(this.max - this.min)
    }

    public onDestroy(){
        this.removeOnValueChanged()
    }

    private activatingComponent()
    {
        if (this.slider == null)
        {
            this.slider = this.getWidget() as ProgressBar;
            if (this.slider == null)
            {
                Log.error(`添加UI侧组件UISlider时，物体${this.getWidget().GetName()}不是ProgressBar组件`);
            }
        }
    }
    public setOnValueChanged(callback: (val:number) => void)
    {
        this.activatingComponent();
        this.removeOnValueChanged();
        this.onValueChanged = callback;
    }

    public removeOnValueChanged()
    {
        if (this.onValueChanged != null)
        {
            this.onValueChanged = null;
        }
    }

    public update()
    {
        if(!!this.slider)
        {
            if(!!this._lastValue && this.onValueChanged)
            {
                if(this._lastValue!= this.slider.Percent)
                {
                    this.onValueChangedEvent();
                }
            }
            this._lastValue = this.slider.Percent;
        }
    }

    
    private onValueChangedEvent(){
        if(this.isSetting) return;
        if(this.isWholeNumbers){
            this.isSetting = true;
            let val = this.sliderValue;
            val = Math.floor(val+0.5)
            this.setValue(val)
            this.isSetting= false;
        }
        if(this.onValueChanged){
            this.onValueChanged(this.sliderValue)
        }
    }

    public setWholeNumbers(wholeNumbers: boolean)
    {
        this.activatingComponent();
        this.isWholeNumbers = wholeNumbers;
    }

    public setValueList(valueList: [])
    {
        this.valueList = valueList;
        this.setWholeNumbers(true);
        this.setMinValue(0);
        this.setMaxValue(valueList.length - 1);
    }

    public getValueList():[]
    {
        return this.valueList;
    }
   
    public setWholeNumbersValue(value)
    {
        this.activatingComponent();
        if (!this.isWholeNumbers)
        {
            Log.warning("请先设置WholeNumbers为true");
            return;
        }

        for (let i = 0; i < this.valueList.length; i++)
        {
            if (this.valueList[i] == value)
            {
                this.slider.SetPercent(i/(this.valueList.length-1));
                return;
            }
        }
    }

    public getWholeNumbersValue(): any
    {
        this.activatingComponent();
        if (!this.isWholeNumbers)
        {
            Log.warning("请先设置WholeNumbers为true");
            return null;
        }
        var index = this.sliderValue;
        return this.valueList[index];
    }
   
    /**
     * 设置进度
     * @param value wholeNumbers 时value是ui侧的index
     */
    public setValue(value: number)
    {
        this.activatingComponent();
        
        if (!this.isWholeNumbers){
            this.slider.SetPercent((value - this.min) / (this.max - this.min));
        }else{
            this.slider.SetPercent((Math.floor(value) - this.min) / (this.max - this.min));
        }
        Log.info("setValue"+this.slider.Percent);
    }

    public getValue(): number
    {
        this.activatingComponent();
        return this.sliderValue;
    }

    /**
     * 设置进度
     * @param value 
     * @return
     */
    public setNormalizedValue(value: number)
    {
        this.activatingComponent();
        this.slider.SetPercent(value);
    }
    
    public getNormalizedValue(): number
    {
        this.activatingComponent();
        return this.slider.Percent;
    }

    public setMaxValue(value: number)
    {
        this.activatingComponent();
        this.max = value;
    }

    public setMinValue(value: number)
    {
        this.activatingComponent();
        this.min = value;
    }
}