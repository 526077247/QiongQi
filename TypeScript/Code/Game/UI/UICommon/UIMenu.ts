import { IOnCreate } from "../../../Module/UI/IOnCreate";
import { UIBaseContainer } from "../../../Module/UI/UIBaseContainer";
import { UICopyGameObject } from "../../../Module/UIComponent/UICopyGameObject";
import { UIMenuItem } from "./UIMenuItem";
import { Widget, SizeBox } from "ue";
import { Log } from "../../../../Mono/Module/Log/Log"
export class MenuPara
{
    public id: number;
    public name: string;
    public imgPath: string;
}
export class UIMenu extends UIBaseContainer implements IOnCreate<string>
{
    public getConstructor(){
        return UIMenu;
    }
    public space: UICopyGameObject;

    public paras: MenuPara[];
    public uiMenuItems: UIMenuItem[];
    public activeIndex;
    private onActiveIndexChanged: (para: MenuPara)=>void;

    public onCreate(templateName: string)
    {
        this.space = this.addComponent<UICopyGameObject>(UICopyGameObject,"Box");
        this.space.initListView(templateName,0,this.onGetItemByIndex.bind(this));
    }


    public onGetItemByIndex(index:number, go: Widget)
    {
        var para = this.paras[index];
        if (this.space.getUIItemView<UIMenuItem>(UIMenuItem,go) == null)
        {
            this.space.addItemViewComponent<UIMenuItem>(UIMenuItem,go);
        }
        var item = this.space.getUIItemView<UIMenuItem>(UIMenuItem,go);
        this.uiMenuItems[index] = item;
        item.setData(para, index, (type, inx) =>
        {
            this.setActiveIndex(inx);
        }, index == this.activeIndex);
    }


    public setData(paras: MenuPara[], onActiveIndexChanged: (para: MenuPara)=>void, activeIndex:number = -1)
    {
        this.onActiveIndexChanged = onActiveIndexChanged;
        this.paras = paras;
        this.uiMenuItems = [];
        this.space.setListItemCount(this.paras.length);
        this.space.refreshAllShownItem();
        this.setActiveIndex(activeIndex);
        const size = this.getWidget() as SizeBox;
        size.SetHeightOverride(100 * this.paras.length);
    }

    public setActiveIndex(index:number, force: boolean = false)
    {
        if (!force && (index < 0 || this.activeIndex == index)) return;
        if (this.activeIndex >= 0)
            this.uiMenuItems[this.activeIndex].setIsActive(false);
        this.activeIndex = index;
        this.uiMenuItems[this.activeIndex].setIsActive(true);
        this.onActiveIndexChanged(this.paras[index]);
    }
}
