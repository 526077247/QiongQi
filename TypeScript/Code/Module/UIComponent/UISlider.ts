import { Slider } from "ue";
import { Log } from "../../../Mono/Module/Log/Log";
import { IOnDestroy } from "../UI/IOnDestroy";
import { UIBaseComponent } from "../UI/UIBaseComponent";

export class UISlider extends UIBaseComponent implements IOnDestroy {

    public getConstructor(){
        return UISlider;
    }
    private slider: Slider;
    private onValueChanged: (val:number) => void;
    private isWholeNumbers: boolean;
    private valueList: [];

    private get sliderValue():number{
        return this.slider.Value
    }

    public onDestroy(){
        this.removeOnValueChanged()
    }

    private activatingComponent()
    {
        if (this.slider == null)
        {
            const widget = this.getWidget();
            if (!(widget instanceof Slider))
            {
                Log.error(`添加UI侧组件UISlider时，物体${widget.GetName()}不是Slider组件`);
            }
            else
            {
                this.slider = widget as Slider;
            }
        }
    }

    public setOnValueChanged(callback: (val:number) => void)
    {
        this.activatingComponent();
        this.removeOnValueChanged();
        this.onValueChanged = callback;
        this.slider.OnValueChanged.Add(this.onValueChanged)
    }

    public removeOnValueChanged()
    {
        if (this.onValueChanged != null)
        {
            this.slider.OnValueChanged.Remove(this.onValueChanged)
            this.onValueChanged = null;
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
        this.slider.SetStepSize(1)
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
                this.slider.SetValue(i);
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
        if(this.isWholeNumbers){
            this.slider.SetValue(Math.floor(value));
        }else{
            this.slider.SetValue(value);
        }
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
        this.slider.SetValue((this.slider.MaxValue - this.slider.MinValue) * value);
    }
    
    public getNormalizedValue(): number
    {
        this.activatingComponent();
        return (this.slider.Value - this.slider.MinValue) / (this.slider.MaxValue - this.slider.MinValue);
    }

    public setMaxValue(value: number)
    {
        this.activatingComponent();
        this.slider.SetMaxValue(value);
    }

    public setMinValue(value: number)
    {
        this.activatingComponent();
        this.slider.SetMinValue(value);
    }
}