import { Class, PanelWidget, ScrollBox, Vector2D, Widget } from "ue";
import { Log } from "../../../Mono/Module/Log/Log";
import { GridViewItemPrefabConfData, LoopGridView, LoopGridViewSettingParam } from "../../../ThirdParty/SuperScrollView/GridView/LoopGridView";
import { LoopGridViewItem } from "../../../ThirdParty/SuperScrollView/GridView/LoopGridViewItem";
import * as string from "../../../Mono/Helper/StringHelper"
import { I18NManager } from "../I18N/I18NManager";
import { IOnDestroy } from "../UI/IOnDestroy";
import { UIBaseContainer } from "../UI/UIBaseContainer";
import { IUpdate } from "../../../Mono/Module/Update/IUpdate";

export class UILoopGridView extends UIBaseContainer implements IOnDestroy, IUpdate{
    public getConstructor(){
        return UILoopGridView;
    }
    private loopGridView: LoopGridView;

    public onDestroy()
    {
        if (this.loopGridView != null)
        {
            this.loopGridView.clearListView();
            this.loopGridView = null;
        }
    }

    public update(){
        this.loopGridView?.update();
    }

    private activatingComponent()
    {
        if (this.loopGridView == null)
        {
            const widget = this.getWidget();
            if (!(widget instanceof ScrollBox))
            {
                Log.error(`添加UI侧组件UILoopGridView时，物体${widget.GetName()}不是ScrollBox组件`);
            }
            else
            {
                this.loopGridView = new LoopGridView(widget as ScrollBox);
            }
        }
    }

    /**
     * 初始化网格视图
     * @param itemTotalCount 
     * @param onGetItemByRowColumn 
     * @param settingParam 
     * @param initParam 
     */
    public initGridView(
        itemTotalCount: number,
        onGetItemByRowColumn: (gridView: LoopGridView, itemIndex: number, row: number, column: number) => LoopGridViewItem | null,
        settingParam?: LoopGridViewSettingParam|number
    ): void {
        this.activatingComponent();
        if (this.loopGridView) {
            if(!!settingParam){
                if(!(settingParam instanceof LoopGridViewSettingParam)){
                    const mFixedRowOrColumnCount = settingParam;
                    settingParam = new LoopGridViewSettingParam();
                    settingParam.mFixedRowOrColumnCount = mFixedRowOrColumnCount;
                    this.loopGridView.initGridView(itemTotalCount, onGetItemByRowColumn, settingParam);
                }else{
                    this.loopGridView.initGridView(itemTotalCount, onGetItemByRowColumn, settingParam);
                }
            }
            else{
                this.loopGridView.initGridView(itemTotalCount, onGetItemByRowColumn);
            }
        }
    }

    
    /**
     * 添加预制体配置数据
     * @param data 
     */
    public addItemPrefabConfData(data: GridViewItemPrefabConfData| Class| string){
        this.activatingComponent();
        if(data instanceof GridViewItemPrefabConfData) {
            this.loopGridView.addItemPrefabConfData(data);
        }else{

            let configData = new GridViewItemPrefabConfData();
            configData.mInitCreateCount = 0;
            if(data instanceof Class) {
                configData.mItemPrefab = data;
                this.loopGridView.addItemPrefabConfData(configData);
                return;
            }
            if(string.isNullOrEmpty(data)){
                Log.error("data == null!")
                return null;
            }
            if(data.startsWith("/")){
                //预制体
                let itemClass = Class.Find(data);
                if(!itemClass)
                {
                    itemClass = Class.Load(data);
                    if(!itemClass) {
                        Log.error("UIRoot class not found at path:" + data);
                        return null;
                    }
                }
                configData.mItemPrefab = itemClass;
                this.loopGridView.addItemPrefabConfData(configData);
            }else{
                //子节点
                const child: Widget = this.findChild(this.getWidget(), data);
                if(child instanceof PanelWidget && child.GetChildrenCount() > 0){
                    Log.error("不支持PanelWidget作为子节点")
                    return null;
                }
                configData.name = child.GetName();
                configData.mItemPrefab = child.GetClass();
                this.loopGridView.addItemPrefabConfData(configData);
            }

           
        }
    }
    
    /**
     * 在这里创建相应的UI对象
     * @param type 
     * @param item 
     * @returns 
     */
    public addItemViewComponent<T extends UIBaseContainer>(type: new () => T,item: LoopGridViewItem)
    {
        //保证名字不能相同 不然没法cache
        const t:T = this.addComponentNotCreate<T>(type, item.widget.GetName());
        t.setWidget(item.widget);
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
     * 根据tem获取UI侧的item
     * @param type 
     * @param item 
     * @returns 
     */
    public getUIItemView<T extends UIBaseContainer>(type: new () => T, item: LoopGridViewItem):T 
    {
        return this.getComponent<T>(type, item.widget.GetName());
    }

    /**
     * 设置列表项目数量
     * @param itemCount 
     * @param resetPos 
     */
    public setListItemCount(itemCount: number, resetPos: boolean = true): void {
        this.activatingComponent();
        if (this.loopGridView) {
            this.loopGridView.setListItemCount(itemCount, resetPos);
        }
    }

    /**
     * 获取显示的项目
     * @param itemIndex 
     * @returns 
     */
    public getShownItemByItemIndex(itemIndex: number): LoopGridViewItem | null {
        this.activatingComponent();
        return this.loopGridView ? this.loopGridView.getShownItemByItemIndex(itemIndex) : null;
    }

    /**
     * 移动面板到指定行列
     * @param row 
     * @param column 
     * @param offsetX 
     * @param offsetY 
     */
    public movePanelToItemByRowColumn(row: number, column: number, offsetX: number = 0, offsetY: number = 0): void {
        this.activatingComponent();
        if (this.loopGridView) {
            this.loopGridView.movePanelToItemByRowColumn(row, column, offsetX, offsetY);
        }
    }

    /**
     * 刷新所有显示的项目
     */
    public refreshAllShownItem(): void {
        this.activatingComponent();
        if (this.loopGridView) {
            this.loopGridView.refreshAllShownItem();
        }
    }

    /**
     * 设置项目尺寸
     * @param sizeDelta 
     */
    public setItemSize(sizeDelta: Vector2D): void {
        this.activatingComponent();
        if (this.loopGridView) {
            this.loopGridView.itemSize = sizeDelta;
        }
    }

    // /**
    //  * 设置开始拖动事件
    //  * @param callback 
    //  */
    // public setOnBeginDragAction(callback: (event: EventTouch) => void): void {
    //     this.activatingComponent();
    //     if (this.loopGridView) {
    //         this.loopGridView.mOnBeginDragAction = callback;
    //     }
    // }

    // /**
    //  * 设置拖动中事件
    //  * @param callback 
    //  */
    // public setOnDragingAction(callback: (event: EventTouch) => void): void {
    //     this.activatingComponent();
    //     if (this.loopGridView) {
    //         this.loopGridView.mOnDragingAction = callback;
    //     }
    // }

    // /**
    //  * 设置结束拖动事件
    //  * @param callback 
    //  */
    // public setOnEndDragAction(callback: (event: EventTouch) => void): void {
    //     this.activatingComponent();
    //     if (this.loopGridView) {
    //         this.loopGridView.mOnEndDragAction = callback;
    //     }
    // }
    
    /**
     * 移除UI项目所有组件
     * @param node 
     */
    public removeUIItemAllComponent(widget: Widget): void {
        this.removeAllComponent(widget.GetName());
    }
                        
    /**
     * 清理资源
     * @param name 
     * @returns 
     */
    public cleanUp(name: string): void {
        if (!this.loopGridView) return;
        this.loopGridView.cleanUp(name, this.removeUIItemAllComponent.bind(this));
    }
}