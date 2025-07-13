import { Log } from "../../../Mono/Module/Log/Log";
import { I18NManager } from "../I18N/I18NManager";
import { IOnDestroy } from "../UI/IOnDestroy";
import { UIBaseContainer } from "../UI/UIBaseContainer";
import * as UE from 'ue';
import * as string from "../../../Mono/Helper/StringHelper";
import { Define } from "../../../Mono/Define";
export class UICopyGameObject extends UIBaseContainer implements IOnDestroy{
    public getConstructor(){
        return UICopyGameObject;
    }
    
    private onGetItemCallback: (index:number, node: UE.Widget)=>void
    private showCount: number = 0
    private itemViewList: UE.Widget[] = []
    
    private template: string;
    private comp: UE.PanelWidget;

    public onDestroy()
    {
        this.clear();
    }

    private activatingComponent()
    {
        if (this.comp == null)
        {
            const widget = this.getWidget() as UE.PanelWidget;
            if(!widget.GetAllChildren)
            {
                Log.error(`添加UI侧组件UICopyGameObject时，物体${widget.GetName()}不是PanelWidget组件`);
            }
            else
            {
                this.comp = widget;
            }
        }
    }
    

    public initListView(template:string, totalCount: number, onGetItemCallback:(index:number, node: UE.Widget)=> void = null)
    {
        this.template = template;
        this.activatingComponent();
        this.onGetItemCallback = onGetItemCallback;
        this.setListItemCount(totalCount);
    }

    /**
     * item是Cocos侧的item对象，在这里创建相应的UI对象
     * @param type
     * @param item
     * @returns
     */
    public addItemViewComponent<T extends UIBaseContainer>(type: new () => T,item: UE.Widget)
    {
        //保证名字不能相同 不然没法cache
        const t:T = this.addComponentNotCreate<T>(type, item.GetName());
        if(item instanceof UE.UserWidget && !!item.WidgetTree){
            t.setWidget(item.WidgetTree.RootWidget);
        }else{
            t.setWidget(item);
        }
        const componentAny = t as any;
        if (!!componentAny.onCreate)
            componentAny.onCreate();
        if (this.activeSelf)
            t.setActive(true);
        if (!!componentAny.onLanguageChange)
            I18NManager.instance?.registerI18NEntity(componentAny);
        return t;
    }

    /**
     * 根据Cocos侧item获取UI侧的item
     * @param type
     * @param item
     * @returns
     */
    public getUIItemView<T extends UIBaseContainer>(type: new () => T, item: UE.Widget):T
    {
        return this.getComponent<T>(type, item.GetName());
    }

    public setListItemCount(totalCount: number) {
        if (totalCount > 10) Log.info("total_count 不建议超过10个");
        if (this.template == null) Log.error("item is Null!!!");
        this.showCount = totalCount;
        var count = this.itemViewList.length > totalCount ? this.itemViewList.length : totalCount;
        for (let i = 0; i < count; i++) {
            if (i < this.itemViewList.length && i < totalCount) {
                this.itemViewList[i].SetVisibility(UE.ESlateVisibility.Visible);
                this.onGetItemCallback?.(i, this.itemViewList[i]);
            } else if (i < totalCount) {
                const item: UE.UserWidget = this.NewItemView();
                item.AddToViewport();
                item.SetVisibility(UE.ESlateVisibility.Visible);
                if(this.comp instanceof UE.VerticalBox){
                    const slot = this.comp.AddChildToVerticalBox(item);
                    const size =  slot.Size;
                    size.SizeRule = UE.ESlateSizeRule.Fill;
                    slot.SetSize(size);
                }else{
                    this.comp.AddChild(item);
                }
                
                this.itemViewList[this.itemViewList.length] = item;
                item.SetVisibility(UE.ESlateVisibility.Visible);
                this.onGetItemCallback?.(i, item);
            } else if (i < this.itemViewList.length) {
                this.itemViewList[i].SetVisibility(UE.ESlateVisibility.Collapsed);
            }
        }
    }

    private NewItemView(){
        if(string.isNullOrEmpty(this.template)){
            Log.error("this.template == null!")
            return null;
        }
        
        if(this.template.startsWith("/")){
            //预制体
            let itemClass = UE.Class.Find(this.template);
            if(!itemClass)
            {
                itemClass = UE.Class.Load(this.template);
                if(!itemClass) {
                    Log.error("UIRoot class not found at path:" + this.template);
                    return null;
                }
            }
            return UE.WidgetBlueprintLibrary.Create(Define.Game, itemClass, null) as UE.UserWidget;
        }else{
            //子节点
            const child: UE.Widget = this.findChild(this.getWidget(), this.template);
            if(child instanceof UE.PanelWidget){
                Log.error("不支持PanelWidget作为子节点")
                return null;
            }
            return UE.WidgetBlueprintLibrary.Create(Define.Game, child.GetClass(), null) as UE.UserWidget;
        }
    }
    public refreshAllShownItem()
    {
        for (let i = 0; i < this.showCount; i++)
        {
            this.onGetItemCallback?.(i, this.itemViewList[i]);
        }
    }

    public getItemByIndex(index: number): UE.Widget
    {
        return this.itemViewList[index];
    }

    public getListItemCount(): number
    {
        return this.showCount;
    }

    private clear()
    {
        for (let i = this.itemViewList.length - 1; i >= 0; i--)
        {
            this.itemViewList[i].RemoveFromParent();
        }
        this.itemViewList.length = 0;
    }
}