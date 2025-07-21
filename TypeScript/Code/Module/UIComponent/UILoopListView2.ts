import { Class, PanelWidget, ScrollBox, ScrollBoxSlot, Widget, WidgetLayoutLibrary } from "ue";
import { Log } from "../../../Mono/Module/Log/Log";
import { IUpdate } from "../../../Mono/Module/Update/IUpdate";
import { ItemPrefabConfData, LoopListView2, LoopListViewInitParam } from "../../../ThirdParty/SuperScrollView/ListView/LoopListView2";
import { LoopListViewItem2 } from "../../../ThirdParty/SuperScrollView/ListView/LoopListViewItem2";
import { I18NManager } from "../I18N/I18NManager";
import { IOnDestroy } from "../UI/IOnDestroy";
import { UIBaseContainer } from "../UI/UIBaseContainer";
import * as string from "../../../Mono/Helper/StringHelper"

export class UILoopListView2 extends UIBaseContainer implements IOnDestroy,IUpdate{
    public getConstructor(){
        return UILoopListView2;
    }
    private loopListView: LoopListView2;

    public onDestroy()
    {

    }

    private activatingComponent()
    {
        if (this.loopListView == null)
        {
            const widget = this.getWidget();
            if (!(widget instanceof ScrollBox))
            {
                Log.error(`添加UI侧组件UILoopListView2时，物体${widget.GetName()}不是ScrollBox组件`);
            }
            else
            {
                this.loopListView = new LoopListView2(widget as ScrollBox);
            }
        }
    }

    public update(){
        this.loopListView?.update();
    }

    /**
     * 初始化列表视图
     * @param itemTotalCount 列表项总数
     * @param onGetItemByIndex 获取列表项的回调函数
     * @param initParam 初始化参数
     */
    public initListView(onGetItemByIndex: (list: LoopListView2, index: number) => LoopListViewItem2,
        initParam: LoopListViewInitParam = null): void {
        this.activatingComponent();
        // 暂只支持从数量0初始化
        this.loopListView.initListView(0, onGetItemByIndex, initParam);
    }

    /**
     * 添加预制体配置数据
     * @param data 
     * @param autoLength 是否自适应宽度（垂直列表） 或高度（水平列表）
     */
    public addItemPrefabConfData(data: ItemPrefabConfData| Class| string, autoLength:boolean = true){
        this.activatingComponent();
        if(data instanceof ItemPrefabConfData) {
            this.loopListView.addItemPrefabConfData(data);
        }else{

            let configData = new ItemPrefabConfData();
            configData.mPadding = 0;
            configData.mStartPosOffset = 0;
            configData.mInitCreateCount = 0;
            if(data instanceof Class) {
                configData.mItemPrefab = data;
                this.loopListView.addItemPrefabConfData(configData);
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
                this.loopListView.addItemPrefabConfData(configData);
            }else{
                //子节点
                const child: Widget = this.findChild(this.getWidget(), data);
                if(child instanceof PanelWidget){
                    Log.error("不支持PanelWidget作为子节点")
                    return null;
                }
                
                configData.mItemPrefab = child.GetClass();
                const slot = WidgetLayoutLibrary.SlotAsCanvasSlot(child);
                if(slot != null){
                    if(autoLength){
                        if(!this.loopListView.isVertList){
                            configData.mSizeX = slot.GetSize().X;
                        }else{
                            configData.mSizeY = slot.GetSize().Y;
                        }
                    }else{
                        configData.mSizeX = slot.GetSize().X;
                        configData.mSizeY = slot.GetSize().Y;
                    }
                }
                this.loopListView.addItemPrefabConfData(configData);
            }

           
        }
    }
    /**
     * 为列表项添加UI组件
     * @param item 列表项
     * @returns 添加的UI组件
     */
    public addItemViewComponent<T extends UIBaseContainer>(type: new () => T,item: LoopListViewItem2): T {
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
     * 根据列表项获取UI组件
     * @param item 列表项
     * @returns 对应的UI组件
     */
    public getUIItemView<T extends UIBaseContainer>(type: new () => T,item: LoopListViewItem2): T {
        return this.getComponent<T>(type, item.widget.GetName());
    }

    /**
     * 设置列表项数量（注意！无限列表需要修改编译ScrollView引擎源码以支持修改滑动速度,否则滑动惯性会有问题）
     * @param itemCount 列表项数量
     * @param resetPos 是否重置位置
     */
    public setListItemCount(itemCount: number, resetPos: boolean = true): void {
        this.activatingComponent();
        this.loopListView.setListItemCount(itemCount, resetPos);
    }

    /**
     * 获取指定索引的可见列表项
     * @param itemIndex 列表项索引
     * @returns 可见的列表项，如果不存在则返回null
     */
    public getShownItemByItemIndex(itemIndex: number): LoopListViewItem2 {
        this.activatingComponent();
        return this.loopListView.getShownItemByItemIndex(itemIndex);
    }

    /**
     * 刷新所有可见的列表项
     */
    public refreshAllShownItem(): void {
        this.activatingComponent();
        this.loopListView.refreshAllShownItem();
    }

    // /**
    //  * 设置开始拖拽回调
    //  * @param callback 回调函数
    //  */
    // public setOnBeginDragAction(callback: (event: EventTouch) => void): void {
    //     this.activatingComponent();
    //     this.loopListView.onBeginDragAction = callback;
    // }

    // /**
    //  * 设置拖拽中回调
    //  * @param callback 回调函数
    //  */
    // public setOnDragingAction(callback: (event: EventTouch) => void): void {
    //     this.activatingComponent();
    //     this.loopListView.onDragingAction = callback;
    // }

    // /**
    //  * 设置结束拖拽回调
    //  * @param callback 回调函数
    //  */
    // public setOnEndDragAction(callback: (event: EventTouch) => void): void {
    //     this.activatingComponent();
    //     this.loopListView.onEndDragAction = callback;
    // }

    /**
     * 移动面板到指定索引的列表项
     * @param index 列表项索引
     * @param offset 偏移量
     */
    public movePanelToItemIndex(index: number, offset: number = 0): void {
        this.activatingComponent();
        this.loopListView.movePanelToItemIndex(index, offset);
    }

    /**
     * 获取循环列表组件
     * @returns 循环列表组件
     */
    public getLoopListView(): LoopListView2 {
        this.activatingComponent();
        return this.loopListView;
    }

    /**
     * 获取滚动组件
     * @returns 滚动组件
     */
    public getScrollView(): ScrollBox {
        this.activatingComponent();
        return this.loopListView.scrollRect;
    }

    /**
     * 移除UI项的所有组件
     * @param obj 游戏对象
     */
    public removeUIItemAllComponent(obj: Widget): void {
        this.removeAllComponent(obj.GetName());
    }

    /**
     * 清理资源
     * @param name 名称
     */
    public cleanUp(name: string): void {
        if (!this.loopListView) return;
        this.loopListView.cleanUp(name, this.removeUIItemAllComponent.bind(this));
    }    
}