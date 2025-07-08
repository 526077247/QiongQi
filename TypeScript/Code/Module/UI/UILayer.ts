import { CanvasPanel, PanelWidget } from "ue";
import { IManager } from "../../../Mono/Core/Manager/IManager"
import { Define } from "../../../Mono/Define";
import { UILayerDefine, UILayerNames, UIManager } from "./UIManager"

export class UILayer implements IManager<UILayerDefine, PanelWidget>{

    public name :UILayerNames;

    public widget: PanelWidget
    private canvas: CanvasPanel;

    public init(p1?:UILayerDefine, p2?:PanelWidget){
        this.name = p1.name;
        this.widget = p2;
        //canvas
        this.canvas = this.widget as CanvasPanel
    }
    
    public destroy(){
        this.name = null;
        this.widget = null;
        this.canvas = null;
    }
}