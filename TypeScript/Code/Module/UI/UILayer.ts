import { CanvasPanel, CanvasPanelSlot, PanelWidget, Widget } from "ue";
import { IManager } from "../../../Mono/Core/Manager/IManager"
import { UILayerNames } from "./UILayerNames";
import { UILayerDefine } from "./UILayerDefine";

export class UILayer implements IManager<UILayerDefine, PanelWidget, CanvasPanelSlot>{

    public name :UILayerNames;

    public canvas: CanvasPanel;

    private define: UILayerDefine;

    public init(p1?:UILayerDefine, p2?:PanelWidget, p3?: CanvasPanelSlot){
        this.name = p1.name;
        this.define = p1;
        //canvas
        this.canvas = p2 as CanvasPanel
        p3.SetZOrder(this.define.zOrder);
    }

    
    public destroy(){
        this.name = null;
        this.canvas = null;
    }

}