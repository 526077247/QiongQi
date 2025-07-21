import { CanvasPanel, Class, UeBridgeHelper, EOrientation, ESlateVisibility, ScrollBox, SizeBox, Vector2D, Widget, CanvasPanelSlot } from "ue";
import { ListItemArrangeType } from "../Common/CommonDefine";
import { ItemPosMgr } from "../Common/ItemPosMgr";
import { ItemPool } from "./LoopListItemPool";
import { LoopListViewItem2 } from "./LoopListViewItem2";

export class ItemPrefabConfData {
    public name: string
    mItemPrefab: Class = null;
    mPadding: number = 0;
    mInitCreateCount: number = 0;
    mStartPosOffset: number = 0;
    mSizeX: number | null
    mSizeY: number | null
}

/** 列表初始化参数 */
export class LoopListViewInitParam {
    // 所有默认值
    public distanceForRecycle0: number = 300; // mDistanceForRecycle0 应大于 mDistanceForNew0
    public distanceForNew0: number = 200;
    public distanceForRecycle1: number = 300; // mDistanceForRecycle1 应大于 mDistanceForNew1
    public distanceForNew1: number = 200;
    // public smoothDumpRate: number = 0.3;
    public itemDefaultWithPaddingSize: number = 20; // 带间距的项目默认大小

    /** 复制默认初始化参数 */
    public static copyDefaultInitParam(): LoopListViewInitParam {
        return new LoopListViewInitParam();
    }
}

export class LoopListView2 
{
    public constructor(root: ScrollBox){
        this.mScrollRect = root;
    }

    public arrangeType: ListItemArrangeType = ListItemArrangeType.TopToBottom;
    public mSupportScrollBar: boolean = true;

    // 私有变量
    private mItemPoolDict: Map<string, ItemPool> = new Map();
    private mItemPoolList: ItemPool[] = [];
    private mItemList: LoopListViewItem2[] = [];
    private mContainerSize: SizeBox = null;
    private mContainerTrans: CanvasPanel = null;
    private mScrollRect: ScrollBox = null;
    private viewPortRectTransform: Widget = null;
    private itemDefaultWithPaddingSize: number = 20;
    private mItemTotalCount: number = 0;
    private mIsVertList: boolean = false;
    private onGetItemByIndex: (list: LoopListView2, index: number) => LoopListViewItem2 = null;
    private itemWorldCorners: number[] = [0, 0, 0, 0];
    private viewPortRectLocalCorners: number[] = [0, 0, 0, 0];
    private curReadyMinItemIndex: number = 0;
    private curReadyMaxItemIndex: number = 0;
    private needCheckNextMinItem: boolean = true;
    private needCheckNextMaxItem: boolean = true;
    private itemPosMgr: ItemPosMgr = null;
    private distanceForRecycle0: number = 300;
    private distanceForNew0: number = 200;
    private distanceForRecycle1: number = 300;
    private distanceForNew1: number = 200;
    private mIsDraging: boolean = false;
    // private pointerEventData: EventTouch = null;
    private lastItemIndex: number = 0;
    private lastItemPadding: number = 0;
   

    private mListViewInited: boolean = false;
    private listUpdateCheckFrameCount: number = 0;

    private mListViewSizeInited: boolean = false;

    // public onBeginDragAction: (event: EventTouch) => void = null;
    // public onDragingAction: (event: EventTouch) => void = null;
    // public onEndDragAction: (event: EventTouch) => void = null;

    // 公共属性访问器
    get itemList(): LoopListViewItem2[] { return this.mItemList; }
    get isVertList(): boolean { return this.mIsVertList; }
    get itemTotalCount(): number { return this.mItemTotalCount; }
    get containerTrans(): CanvasPanel { return this.mContainerTrans; }
    get scrollRect(): ScrollBox { return this.mScrollRect; }
    get isDraging(): boolean { return this.mIsDraging; }
    get isListViewInited(): boolean { return this.mListViewInited; }
    get supportScrollBar(): boolean { return this.mSupportScrollBar; }
    set supportScrollBar(value: boolean) { this.mSupportScrollBar = value; }


    // 添加预制体配置数据
    public addItemPrefabConfData(config: ItemPrefabConfData){
        if(!this.isListViewInited) {
            console.error("initListView firset");
            return;
        }
        if(config == null)  {
            console.error("config is null");
            return;
        }
        let data: ItemPrefabConfData = config;

        if (!data.mItemPrefab) {
            console.error("A item prefab is null");
            return;
        }

        const prefab = data.mItemPrefab;
        const prefabName = config.name || prefab.GetName();
        if (this.mItemPoolDict.has(prefabName)) {
            console.error(`A item prefab with name ${prefabName} has existed!`);
            return;
        }

        const pool = new ItemPool();
        pool.init(prefab, prefabName, data.mPadding, data.mStartPosOffset, data.mInitCreateCount, this.mContainerTrans);
        pool.SizeX = config.mSizeX;
        pool.SizeY = config.mSizeY;
       
        this.mItemPoolDict.set(prefabName, pool);
        this.mItemPoolList.push(pool);
        console.info("addItemPrefabConfData "+ prefabName)
    }

    // 设置回收距离
    public setDistanceForRecycle(distanceForRecycle0: number, distanceForRecycle1: number): void {
        this.distanceForRecycle0 = distanceForRecycle0;
        this.distanceForRecycle1 = distanceForRecycle1;
    }

    // 设置新建距离
    public setDistanceForNew(distanceForNew0: number, distanceForNew1: number): void {
        this.distanceForNew0 = distanceForNew0;
        this.distanceForNew1 = distanceForNew1;
    }

    // 清理列表视图
    public clearListView(): void {
        this.setListItemCount(0, true);
        for (const pool of this.mItemPoolList) {
            pool.clearPool();
        }
        this.mItemPoolList = [];
        this.mItemPoolDict.clear();
        this.onGetItemByIndex = null;
        this.mListViewInited = false;
    }

    public cleanUp(name: string = null, beforeDestroy:(node)=> void = null)
    {
        if (name == null)
        {
            let count = this.mItemPoolList.length;
            for (let i = 0; i < count; ++i)
            {
                this.mItemPoolList[i].cleanUp(beforeDestroy);
            }
        }
        else
        {
            const pool = this.mItemPoolDict.get(name);
            if(!!pool) pool.cleanUp(beforeDestroy);
        }
    }

    // 初始化列表视图
    public initListView(
        itemTotalCount: number,
        onGetItemByIndex: (list: LoopListView2, index: number) => LoopListViewItem2,
        initParam: LoopListViewInitParam = null
    ): void {
        if (initParam) {
            this.distanceForRecycle0 = initParam.distanceForRecycle0;
            this.distanceForNew0 = initParam.distanceForNew0;
            this.distanceForRecycle1 = initParam.distanceForRecycle1;
            this.distanceForNew1 = initParam.distanceForNew1;
            // this.smoothDumpRate = initParam.smoothDumpRate;
            this.itemDefaultWithPaddingSize = initParam.itemDefaultWithPaddingSize;
        }

        if (!this.mScrollRect) {
            console.error("ListView Init Failed! ScrollRect component not found!");
            return;
        }

        if (this.distanceForRecycle0 <= this.distanceForNew0) {
            console.error("distanceForRecycle0 should be bigger than distanceForNew0");
        }
        if (this.distanceForRecycle1 <= this.distanceForNew1) {
            console.error("distanceForRecycle1 should be bigger than distanceForNew1");
        }

        this.itemPosMgr = new ItemPosMgr(this.itemDefaultWithPaddingSize);
        this.mContainerSize = this.mScrollRect.GetChildAt(0) as SizeBox;
        this.mContainerTrans = this.mContainerSize.GetChildAt(0) as CanvasPanel;

        this.viewPortRectTransform = this.mScrollRect;
        this.mIsVertList = this.scrollRect.Orientation != EOrientation.Orient_Horizontal;
        if(this.mIsVertList){
            this.arrangeType = ListItemArrangeType.TopToBottom;
        }else{
            this.arrangeType = ListItemArrangeType.LeftToRight;
        }
        this.mSupportScrollBar = this.mScrollRect.ScrollBarVisibility == ESlateVisibility.Visible||
                                    this.mScrollRect.ScrollBarVisibility == ESlateVisibility.Hidden;
        this.mScrollRect.SetOrientation(this.mIsVertList?EOrientation.Orient_Vertical:EOrientation.Orient_Horizontal);
        this.mScrollRect.SetAnimateWheelScrolling(false);//暂不支持
        this.setScrollbarListener();
        // this.adjustPivot();
        // this.adjustAnchor(this.mContainerTrans);
        // this.adjustContainerPivot(this.mContainerTrans);

        this.clearListView();
        this.onGetItemByIndex = onGetItemByIndex;

        if (this.mListViewInited) {
            console.error("LoopListView2.InitListView method can be called only once.");
        }
        this.mListViewInited = true;
        this.mListViewSizeInited = false;
        this.resetListView();
        this.mItemTotalCount = itemTotalCount;

        if (this.mItemTotalCount < 0) {
            this.mSupportScrollBar = false;
            this.mScrollRect.SetScrollBarVisibility(ESlateVisibility.Collapsed);
        }
        if (this.mSupportScrollBar) {
            this.itemPosMgr.setItemMaxCount(this.mItemTotalCount);
        } else {
            this.itemPosMgr.setItemMaxCount(0);
        }

        this.curReadyMaxItemIndex = 0;
        this.curReadyMinItemIndex = 0;
        this.needCheckNextMaxItem = true;
        this.needCheckNextMinItem = true;
        this.updateContentSize();
    }

    // 设置滚动条监听
    private setScrollbarListener(): void {
        this.scrollRect.OnScrollBarVisibilityChanged.Add(this.onScrollBarVisibilityChanged.bind(this))
    }

    private onScrollBarVisibilityChanged(visibility: ESlateVisibility) {
        if(visibility == ESlateVisibility.Visible){
            this.onPointerDownInScrollBar();
        }else{
            this.onPointerUpInScrollBar();
        }
    }

    private onPointerDownInScrollBar(): void {

    }

    private onPointerUpInScrollBar(): void {

    }

        
    // 重置列表视图
    public resetListView(resetPos: boolean = true): void {
        if(this.mListViewSizeInited) return;
        const size =  UeBridgeHelper.GetInstance().GetGeometryLocalSize(this.mScrollRect.GetCachedGeometry());
        const width = size.X;
        const height = size.Y;
        if(width==0 && height == 0){
            return;
        }
        this.mListViewSizeInited = true;

        // 顺序：下、上、左、右
        this.viewPortRectLocalCorners[0]= height;        // 下 
        this.viewPortRectLocalCorners[1] = 0,             // 上
        this.viewPortRectLocalCorners[2] = 0;         // 左
        this.viewPortRectLocalCorners[3]= width;     // 右

        if (resetPos) {
            this.scrollRect.SetScrollOffset(0);
        }
    }

    // 设置列表项数量
    public setListItemCount(itemCount: number, resetPos: boolean = true): void {
        if (itemCount === this.mItemTotalCount) return;
        this.mItemTotalCount = itemCount;
        if (this.mItemTotalCount < 0)
        {
            this.mSupportScrollBar = false;
            this.scrollRect.SetScrollBarVisibility(ESlateVisibility.Collapsed);
        }
        if (this.mSupportScrollBar) {
            this.itemPosMgr.setItemMaxCount(this.mItemTotalCount);
        } else {
            this.itemPosMgr.setItemMaxCount(0);
        }

        if (this.mItemTotalCount === 0) {
            this.curReadyMaxItemIndex = 0;
            this.curReadyMinItemIndex = 0;
            this.needCheckNextMaxItem = false;
            this.needCheckNextMinItem = false;
            this.recycleAllItem();
            this.clearAllTmpRecycledItem();
            this.updateContentSize();
            return;
        }

        if (this.curReadyMaxItemIndex >= this.mItemTotalCount) {
            this.curReadyMaxItemIndex = this.mItemTotalCount - 1;
        }

        this.needCheckNextMaxItem = true;
        this.needCheckNextMinItem = true;

        if (resetPos) {
            this.movePanelToItemIndex(0, 0);
            return;
        }

        if (this.mItemList.length === 0) {
            this.movePanelToItemIndex(0, 0);
            return;
        }

        const maxItemIndex = this.mItemTotalCount - 1;
        const lastItemIndex = this.mItemList[this.mItemList.length - 1].itemIndex;
        if (lastItemIndex <= maxItemIndex) {
            this.updateContentSize();
            this.updateAllShownItemsPos();
            return;
        }

        this.movePanelToItemIndex(maxItemIndex, 0);
    }

    // 根据索引获取显示的项
    public getShownItemByItemIndex(itemIndex: number): LoopListViewItem2 {
        const count = this.mItemList.length;
        if (count === 0) return null;
        if (itemIndex < this.mItemList[0].itemIndex || itemIndex > this.mItemList[count - 1].itemIndex) {
            return null;
        }
        const i = itemIndex - this.mItemList[0].itemIndex;
        return this.mItemList[i];
    }

    // 获取最近的显示项
    public getShownItemNearestItemIndex(itemIndex: number): LoopListViewItem2 {
        const count = this.mItemList.length;
        if (count === 0) return null;
        if (itemIndex < this.mItemList[0].itemIndex) {
            return this.mItemList[0];
        }
        if (itemIndex > this.mItemList[count - 1].itemIndex) {
            return this.mItemList[count - 1];
        }
        const i = itemIndex - this.mItemList[0].itemIndex;
        return this.mItemList[i];
    }

    // 获取显示项数量
    get shownItemCount(): number {
        return this.mItemList.length;
    }

    // 获取视口大小
    get viewPortSize(): number {
        return this.mIsVertList ? this.viewPortHeight : this.viewPortWidth;
    }

    get viewPortWidth(): number {
        return UeBridgeHelper.GetInstance().GetGeometryLocalSize(this.viewPortRectTransform.GetCachedGeometry()).X;
    }

    get viewPortHeight(): number {
        return UeBridgeHelper.GetInstance().GetGeometryLocalSize(this.viewPortRectTransform.GetCachedGeometry()).Y;
    }

    // 根据索引获取显示项
    public getShownItemByIndex(index: number): LoopListViewItem2 {
        return (index >= 0 && index < this.mItemList.length) ? this.mItemList[index] : null;
    }

    // 在显示列表中获取项的索引
    public getIndexInShownItemList(item: LoopListViewItem2): number {
        if (!item) return -1;
        for (let i = 0; i < this.mItemList.length; i++) {
            if (this.mItemList[i] === item) return i;
        }
        return -1;
    }

    // 创建新列表项
    public newListViewItem(itemPrefabName: string, index: number = null): LoopListViewItem2 {
        const pool = this.mItemPoolDict.get(itemPrefabName);
        if (!pool) {
            for (const [key,val] of this.mItemPoolDict) {
                if(key.indexOf(itemPrefabName) >= 0){
                    console.error("not found "+itemPrefabName+" from pool, it might be called "+ key);
                    return null;
                }
            }
            console.error("not found "+itemPrefabName+" from pool, please addItemPrefabConfData first!");
            return null;
        }

        const item = pool.getItem(index);
        item.canvasPanelSlot = this.mContainerTrans.AddChildToCanvas(item.widget);
        item.widget.SetRenderScale(Vector2D.UnitVector);
        item.canvasPanelSlot.SetPosition(Vector2D.UnitVector);
        this.adjustPivot(item.canvasPanelSlot);
        this.adjustAnchor(item.canvasPanelSlot,!pool.SizeX,!pool.SizeY);
        const size = item.canvasPanelSlot.GetSize();
        if(pool.SizeX) {
            size.X = pool.SizeX
        }
        if(pool.SizeY) {
            size.Y = pool.SizeY
        }
        item.canvasPanelSlot.SetSize(size)
        item.parentListView = this;
        return item;
    }

    // 项大小变化时更新
    public onItemSizeChanged(itemIndex: number): void {
        const item = this.getShownItemByItemIndex(itemIndex);
        if (!item) return;

        if (this.mSupportScrollBar) {
            const rect = item.canvasPanelSlot;
            if (this.mIsVertList) {
                this.setItemSize(itemIndex, rect.GetSize().Y, item.padding);
            } else {
                this.setItemSize(itemIndex, rect.GetSize().X, item.padding);
            }
        }

        this.updateContentSize();
        this.updateAllShownItemsPos();
    }

    // 刷新指定项
    public refreshItemByItemIndex(itemIndex: number): void {
        const count = this.mItemList.length;
        if (count === 0) return;
        if (itemIndex < this.mItemList[0].itemIndex || itemIndex > this.mItemList[count - 1].itemIndex) {
            return;
        }

        const firstItemIndex = this.mItemList[0].itemIndex;
        const i = itemIndex - firstItemIndex;
        const curItem = this.mItemList[i];
        const pos = curItem.canvasPanelSlot.GetPosition();

        this.recycleItemTmp(curItem);
        const newItem = this.getNewItemByIndex(itemIndex);
        if (!newItem) {
            this.refreshAllShownItemWithFirstIndex(firstItemIndex);
            return;
        }

        this.mItemList[i] = newItem;
        if (this.mIsVertList) {
            pos.X = newItem.startPosOffset;
        } else {
            pos.Y = newItem.startPosOffset;
        }
        newItem.canvasPanelSlot.SetPosition(pos);
        this.onItemSizeChanged(itemIndex);
        this.clearAllTmpRecycledItem();
    }


    // 移动到指定项
    /**
     * 移动面板到指定索引的列表项
     * @param itemIndex 列表项索引
     * @param offset 偏移量，范围从0到滚动视图视口大小
     */
    public movePanelToItemIndex(itemIndex: number, offset: number): void {
        // 获取当前滚动位置
        const currentOffset = this.mScrollRect.GetScrollOffset();
        // 设置回相同位置（立即停止）
        this.mScrollRect.SetScrollOffset(currentOffset);
        
    
        if (this.mItemTotalCount === 0) return;
        if (itemIndex < 0 && this.mItemTotalCount > 0) return;
        
        if (itemIndex >= this.mItemTotalCount) {
            itemIndex = this.mItemTotalCount - 1;
        }
        
        if (offset < 0) offset = 0;
        
        const viewPortSize = this.viewPortSize;
        if (offset > viewPortSize) {
            offset = viewPortSize;
        }
        
        let pos = 0;
        const containerPos = this.scrollRect.GetScrollOffset();
        
        switch (this.arrangeType) {
            case ListItemArrangeType.TopToBottom:
                const topPosY = Math.max(containerPos, 0);
                pos = -topPosY - offset;
                break;
                
            // case ListItemArrangeType.BottomToTop:
            //     const bottomPosY = Math.min(containerPos, 0);
            //     pos = -bottomPosY + offset;
            //     break;
                
            case ListItemArrangeType.LeftToRight:
                const leftPosX = Math.min(containerPos, 0);
                pos = -leftPosX + offset;
                break;
                
            // case ListItemArrangeType.RightToLeft:
            //     const rightPosX = Math.max(containerPos, 0);
            //     pos = -rightPosX - offset;
            //     break;
        }
        
        this.recycleAllItem();
        const newItem = this.getNewItemByIndex(itemIndex);
        if (!newItem) {
            this.clearAllTmpRecycledItem();
            return;
        }
        var postion = newItem.canvasPanelSlot.GetPosition();
        if (this.mIsVertList) {
            postion.X = newItem.startPosOffset;
            postion.Y = pos;
        } else {
            postion.X = pos;
            postion.Y = newItem.startPosOffset;
        }
        
        newItem.canvasPanelSlot.SetPosition(postion);
        
        if (this.mSupportScrollBar) {
            const rect = newItem.canvasPanelSlot;
            if (this.mIsVertList) {
                this.setItemSize(itemIndex, rect.GetSize().Y, newItem.padding);
            } else {
                this.setItemSize(itemIndex, rect.GetSize().X, newItem.padding);
            }
        }
        
        this.mItemList.push(newItem);
        this.updateContentSize();
        this.updateListView(viewPortSize + 100, viewPortSize + 100, viewPortSize, viewPortSize);
        this.adjustPanelPos();
        this.clearAllTmpRecycledItem();
    }

    // 回收临时项
    private recycleItemTmp(item: LoopListViewItem2): void {
        if (!item || !item.itemPrefabName) return;
        const pool = this.mItemPoolDict.get(item.itemPrefabName);
        if (pool) pool.recycleItem(item);
    }

    // 清除所有临时回收项
    private clearAllTmpRecycledItem(): void {
        for (const pool of this.mItemPoolList) {
            pool.clearTmpRecycledItem();
        }
    }

    // 回收所有项
    private recycleAllItem(): void {
        for (const item of this.mItemList) {
            this.recycleItemTmp(item);
        }
        this.mItemList = [];
    }

    // 调整轴点
    private adjustPivot(rtf: CanvasPanelSlot): void {
        const pivot = rtf.GetAlignment();
        switch (this.arrangeType) {
            // case ListItemArrangeType.BottomToTop:
            //     pivot.Set(rtf.GetAlignment().X, 1);
            //     break;
            case ListItemArrangeType.TopToBottom:
                pivot.Set(rtf.GetAlignment().X, 0);
                break;
            case ListItemArrangeType.LeftToRight:
                pivot.Set(0, rtf.GetAlignment().Y);
                break;
            // case ListItemArrangeType.RightToLeft:
            //     pivot.Set(1, rtf.GetAlignment().Y);
            //     break;
        }
        rtf.SetAlignment(pivot);
    }

    // 调整锚点
    private adjustAnchor(rtf: CanvasPanelSlot,autoX:boolean,autoY:boolean): void {
        let anchor = rtf.GetAnchors();
        switch (this.arrangeType) {
            // case ListItemArrangeType.BottomToTop:
            //     anchor.Minimum.Y = 1;
            //     anchor.Maximum.Y = 1;
            //     if(autoX){
            //         anchor.Minimum.X = 0;
            //         anchor.Maximum.X = 1;
            //     }
            //     break;
            case ListItemArrangeType.TopToBottom:
                anchor.Minimum.Y = 0;
                anchor.Maximum.Y = 0;
                if(autoX){
                    anchor.Minimum.X = 0;
                    anchor.Maximum.X = 1;
                }
                break;
            case ListItemArrangeType.LeftToRight:
                anchor.Minimum.X = 0;
                anchor.Maximum.X = 0;
                if(autoY){
                    anchor.Minimum.Y = 0;
                    anchor.Maximum.Y = 1;
                }
                break;
            // case ListItemArrangeType.RightToLeft:
            //     anchor.Minimum.X = 1;
            //     anchor.Maximum.X = 1;
            //     if(autoY){
            //         anchor.Minimum.Y = 0;
            //         anchor.Maximum.Y = 1;
            //     }
            //     break;
        }
        rtf.SetAnchors(anchor);
    }


    private adjustPanelPos(): void {
        const count = this.mItemList.length;
        if (count === 0) {
            return;
        }
    
        this.updateAllShownItemsPos();
        const viewPortSize = this.viewPortSize;
        const contentSize = this.getContentPanelSize();
    
        switch (this.arrangeType) {
            case ListItemArrangeType.TopToBottom:
                if (contentSize <= viewPortSize) {
                    this.scrollRect.SetScrollOffset(0);
                    var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
                    pos.Set(this.mItemList[0].startPosOffset,0);
                    this.mItemList[0].canvasPanelSlot.SetPosition(pos);
                    this.updateAllShownItemsPos();
                    return;
                }
    
                const tViewItem0 = this.mItemList[0];
                tViewItem0.getViewCorners(this.itemWorldCorners);
                const topPos0 = this.itemWorldCorners[1];
                
                if (topPos0 < this.viewPortRectLocalCorners[1]) {
                    this.scrollRect.SetScrollOffset(0);
                    var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
                    pos.Set(this.mItemList[0].startPosOffset,0);
                    this.mItemList[0].canvasPanelSlot.SetPosition(pos);
                    this.updateAllShownItemsPos();
                    return;
                }
    
                const tViewItem1 = this.mItemList[this.mItemList.length - 1];
                tViewItem1.getViewCorners(this.itemWorldCorners);
                const downPos1 = this.itemWorldCorners[0];
                const d = downPos1 - this.viewPortRectLocalCorners[0];
                
                if (d > 0) {
                    const pos = this.mItemList[0].canvasPanelSlot.GetPosition();
                    pos.Y -= d;
                    this.mItemList[0].canvasPanelSlot.SetPosition(pos);
                    this.updateAllShownItemsPos();
                }
                break;
    
            // case ListItemArrangeType.BottomToTop:
            //     if (contentSize <= viewPortSize) {
            //         this.scrollRect.SetScrollOffset(0);
            //         var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
            //         pos.Set(this.mItemList[0].startPosOffset,0);
            //         this.mItemList[0].canvasPanelSlot.SetPosition(pos);
            //         this.updateAllShownItemsPos();
            //         return;
            //     }
    
            //     const btViewItem0 = this.mItemList[0];
            //     btViewItem0.getViewCorners(this.itemWorldCorners);
            //     const downPos0 = this.itemWorldCorners[0];
                
            //     if (downPos0 > this.viewPortRectLocalCorners[0]) {
            //         this.scrollRect.SetScrollOffset(0);
            //         var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
            //         pos.Set(this.mItemList[0].startPosOffset,0);
            //         this.mItemList[0].canvasPanelSlot.SetPosition(pos);
            //         this.updateAllShownItemsPos();
            //         return;
            //     }
    
            //     const btViewItem1 = this.mItemList[this.mItemList.length - 1];
            //     btViewItem1.getViewCorners(this.itemWorldCorners);
            //     const topPos1 = this.itemWorldCorners[1];
            //     const bd = this.viewPortRectLocalCorners[1] - topPos1;
                
            //     if (bd > 0) {
            //         const pos = this.mItemList[0].canvasPanelSlot.GetPosition();
            //         pos.Y += bd;
            //         this.mItemList[0].canvasPanelSlot.SetPosition(pos);
            //         this.updateAllShownItemsPos();
            //     }
            //     break;
    
            case ListItemArrangeType.LeftToRight:
                if (contentSize <= viewPortSize) {
                    this.scrollRect.SetScrollOffset(0);
                    var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
                    pos.Set(0,this.mItemList[0].startPosOffset);
                    this.mItemList[0].canvasPanelSlot.SetPosition(pos);
                    this.updateAllShownItemsPos();
                    return;
                }
    
                const lrViewItem0 = this.mItemList[0];
                lrViewItem0.getViewCorners(this.itemWorldCorners);
                const leftPos0 = this.itemWorldCorners[2];
                
                if (leftPos0 > this.viewPortRectLocalCorners[2]) {
                    this.scrollRect.SetScrollOffset(0);
                    var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
                    pos.Set(0,this.mItemList[0].startPosOffset);
                    this.mItemList[0].canvasPanelSlot.SetPosition(pos);
                    this.updateAllShownItemsPos();
                    return;
                }
    
                const lrViewItem1 = this.mItemList[this.mItemList.length - 1];
                lrViewItem1.getViewCorners(this.itemWorldCorners);
                const rightPos1 = this.itemWorldCorners[3];
                const lrd = this.viewPortRectLocalCorners[3] - rightPos1;
                
                if (lrd > 0) {
                    const pos = this.mItemList[0].canvasPanelSlot.GetPosition();
                    pos.X += lrd;
                    this.mItemList[0].canvasPanelSlot.SetPosition(pos);
                    this.updateAllShownItemsPos();
                }
                break;
    
            // case ListItemArrangeType.RightToLeft:
            //     if (contentSize <= viewPortSize) {
            //         this.scrollRect.SetScrollOffset(0);
            //         var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
            //         pos.Set(0,this.mItemList[0].startPosOffset);
            //         this.mItemList[0].canvasPanelSlot.SetPosition(pos);
            //         this.updateAllShownItemsPos();
            //         return;
            //     }
    
            //     const rlViewItem0 = this.mItemList[0];
            //     rlViewItem0.getViewCorners(this.itemWorldCorners);
            //     const rightPos0 = this.itemWorldCorners[3];
                
            //     if (rightPos0 < this.viewPortRectLocalCorners[3]) {
            //         this.scrollRect.SetScrollOffset(0);
            //         var pos = this.mItemList[0].canvasPanelSlot.GetPosition();
            //         pos.Set(0,this.mItemList[0].startPosOffset);
            //         this.mItemList[0].canvasPanelSlot.SetPosition(pos);
            //         this.updateAllShownItemsPos();
            //         return;
            //     }
    
            //     const rlViewItem1 = this.mItemList[this.mItemList.length - 1];
            //     rlViewItem1.getViewCorners(this.itemWorldCorners);
            //     const leftPos1 = this.itemWorldCorners[2];
            //     const rld = leftPos1 - this.viewPortRectLocalCorners[2];
                
            //     if (rld > 0) {
            //         const pos = this.mItemList[0].canvasPanelSlot.GetPosition();
            //         pos.X -= rld;
            //         this.mItemList[0].canvasPanelSlot.SetPosition(pos);
            //         this.updateAllShownItemsPos();
            //     }
            //     break;
        }
    }

    // 更新方法
    update(): void {
        if (!this.mListViewInited) return;
        this.resetListView();
        if(!this.mListViewSizeInited) return;

        if (this.mSupportScrollBar) {
            this.itemPosMgr.update(false);
        }

        this.updateListView(
            this.distanceForRecycle0,
            this.distanceForRecycle1,
            this.distanceForNew0,
            this.distanceForNew1
        );
        this.clearAllTmpRecycledItem();
    }
    lockThis = false;
    // 核心更新逻辑
    private updateListView(
        distanceForRecycle0: number,
        distanceForRecycle1: number,
        distanceForNew0: number,
        distanceForNew1: number
    ): void {
        if(this.lockThis) return;
        this.listUpdateCheckFrameCount++;
        if (this.mIsVertList) {
            let needContinueCheck = true;
            let checkCount = 0;
            const maxCount = 9999;
            while (needContinueCheck) {
                checkCount++;
                if(checkCount >= maxCount)
                {
                    this.lockThis = true;
                    console.error("UpdateListView Vertical while loop " + checkCount + " times! something is wrong!");
                    break;
                }
                needContinueCheck = this.updateForVertList(
                    distanceForRecycle0,
                    distanceForRecycle1,
                    distanceForNew0,
                    distanceForNew1
                );
            }
        } else {
            let needContinueCheck = true;
            let checkCount = 0;
            const maxCount = 9999;
            while (needContinueCheck) {
                checkCount++;
                if(checkCount >= maxCount)
                {
                    this.lockThis = true;
                    console.error("UpdateListView Horizontal while loop " + checkCount + " times! something is wrong!");
                    break;
                }
                needContinueCheck = this.updateForHorizontalList(
                    distanceForRecycle0,
                    distanceForRecycle1,
                    distanceForNew0,
                    distanceForNew1
                );
            }
        }
    }

    // 垂直列表更新
    private updateForVertList(
        distanceForRecycle0: number,
        distanceForRecycle1: number,
        distanceForNew0: number,
        distanceForNew1: number
    ): boolean {
        if (this.mItemTotalCount === 0) {
            if (this.mItemList.length > 0) {
                this.recycleAllItem();
            }
            return false;
        }

        if (this.arrangeType === ListItemArrangeType.TopToBottom) {
            const itemListCount = this.mItemList.length;
            if (itemListCount === 0) {
                let curY = this.scrollRect.GetScrollOffset();
                if (curY < 0) {
                    curY = 0;
                }
    
                let index = 0;
                let pos = -curY;
                
                if (this.mSupportScrollBar) {
                    const res = this.getPlusItemIndexAndPosAtGivenPos(curY);
                    if (!res) {
                        return false;
                    }
                    index = res.index;
                    pos = res.itemPos;
                    pos = -pos;
                }
    
                const newItem = this.getNewItemByIndex(index);
                if (!newItem) {
                    return false;
                }
    
                if (this.mSupportScrollBar) {
                    const rect = newItem.canvasPanelSlot;
                    this.setItemSize(index, rect.GetSize().Y, newItem.padding);
                }
    
                this.mItemList.push(newItem);
                var position = newItem.canvasPanelSlot.GetPosition();
                position.Set(newItem.startPosOffset, pos);
                newItem.canvasPanelSlot.SetPosition(position);
                this.updateContentSize();
                return true;
            }
    
            const tViewItem0 = this.mItemList[0];
            tViewItem0.getViewCorners(this.itemWorldCorners);
            
            const topPos0 = this.itemWorldCorners[1];
            const downPos0 = this.itemWorldCorners[0];
    

            if (!this.mIsDraging && 
                tViewItem0.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
                this.viewPortRectLocalCorners[1] - downPos0 > distanceForRecycle0) {
                this.mItemList.splice(0, 1);
                this.recycleItemTmp(tViewItem0);

                if (!this.mSupportScrollBar) {
                    this.updateContentSize();
                    this.checkIfNeedUpdateItemPos();
                }
                return true;
            }
    
            const tViewItem1 = this.mItemList[this.mItemList.length - 1];
            tViewItem1.getViewCorners(this.itemWorldCorners);
            
            const topPos1 = this.itemWorldCorners[1];
            const downPos1 = this.itemWorldCorners[0];

            if (!this.mIsDraging && 
                tViewItem1.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
                topPos1 - this.viewPortRectLocalCorners[0] > distanceForRecycle1) {
                this.mItemList.pop();
                this.recycleItemTmp(tViewItem1);

                if (!this.mSupportScrollBar) {
                    this.updateContentSize();
                    this.checkIfNeedUpdateItemPos();
                }
                return true;
            }
            // 检查是否需要添加新的底部项
            if (downPos1 - this.viewPortRectLocalCorners[0] < distanceForNew1) {
                if (tViewItem1.itemIndex > this.curReadyMaxItemIndex) {
                    this.curReadyMaxItemIndex = tViewItem1.itemIndex;
                    this.needCheckNextMaxItem = true;
                }
                
                const nIndex = tViewItem1.itemIndex + 1;
                if (nIndex <= this.curReadyMaxItemIndex || this.needCheckNextMaxItem) {
                    const newItem = this.getNewItemByIndex(nIndex);
                    if (!newItem) {
                        this.curReadyMaxItemIndex = tViewItem1.itemIndex;
                        this.needCheckNextMaxItem = false;
                        this.checkIfNeedUpdateItemPos();
                    } else {
                        if (this.mSupportScrollBar) {
                            this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().Y, newItem.padding);
                        }
                        
                        this.mItemList.push(newItem);
                        const y = tViewItem1.canvasPanelSlot.GetPosition().Y + newItem.canvasPanelSlot.GetSize().Y + newItem.padding;
                        var position = newItem.canvasPanelSlot.GetPosition();
                        position.Set(newItem.startPosOffset, y);
                        newItem.canvasPanelSlot.SetPosition(position);
                        
                        this.updateContentSize();
                        this.checkIfNeedUpdateItemPos();
                        
                        if (nIndex > this.curReadyMaxItemIndex) {
                            this.curReadyMaxItemIndex = nIndex;
                        }
                        return true;
                    }
                }
            }

            // 检查是否需要添加新的顶部项
            if (this.viewPortRectLocalCorners[1] - topPos0 < distanceForNew0) {
                if (tViewItem0.itemIndex < this.curReadyMinItemIndex) {
                    this.curReadyMinItemIndex = tViewItem0.itemIndex;
                    this.needCheckNextMinItem = true;
                }
                const nIndex = tViewItem0.itemIndex - 1;
                if (nIndex >= this.curReadyMinItemIndex || this.needCheckNextMinItem) {
                    const newItem = this.getNewItemByIndex(nIndex);
                    if (!newItem) {
                        this.curReadyMinItemIndex = tViewItem0.itemIndex;
                        this.needCheckNextMinItem = false;
                    } else {
                        if (this.mSupportScrollBar) {
                            this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().Y, newItem.padding);
                        }
                        
                        this.mItemList.unshift(newItem);
                  
                        const y = tViewItem0.canvasPanelSlot.GetPosition().Y - newItem.canvasPanelSlot.GetSize().Y - newItem.padding;
                        var position = newItem.canvasPanelSlot.GetPosition();
                        position.Set(newItem.startPosOffset, y);
                        newItem.canvasPanelSlot.SetPosition(position);
                        this.updateContentSize();
                        this.checkIfNeedUpdateItemPos();
                        
                        if (nIndex < this.curReadyMinItemIndex) {
                            this.curReadyMinItemIndex = nIndex;
                        }
                        return true;
                    }
                }
            }
        } 
        // else { // BottomToTop 排列
        //     if (this.mItemList.length === 0) {
        //         let curY = this.scrollRect.GetScrollOffset();
        //         if (curY > 0) {
        //             curY = 0;
        //         }
                
        //         let index = 0;
        //         let pos = -curY;
                
        //         if (this.mSupportScrollBar) {
        //             const res = this.getPlusItemIndexAndPosAtGivenPos(-curY);
        //             if (!res) {
        //                 return false;
        //             }
        //             index = res.index;
        //             pos = res.itemPos;
        //         }
                
        //         const newItem = this.getNewItemByIndex(index);
        //         if (!newItem) {
        //             return false;
        //         }
                
        //         if (this.mSupportScrollBar) {
        //             this.setItemSize(index, newItem.canvasPanelSlot.GetSize().Y, newItem.padding);
        //         }
                
        //         this.mItemList.push(newItem);
        //         var position = newItem.canvasPanelSlot.GetPosition();
        //         position.Set(newItem.startPosOffset, pos);
        //         newItem.canvasPanelSlot.SetPosition(position);
        //         this.updateContentSize();
        //         return true;
        //     }
            
        //     const tViewItem0 = this.mItemList[0];
        //     tViewItem0.getViewCorners(this.itemWorldCorners);
            
        //     const topPos0 = this.itemWorldCorners[1];
        //     const downPos0 = this.itemWorldCorners[0];
    
        //     if (!this.mIsDraging && 
        //         tViewItem0.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
        //         topPos0 - this.viewPortRectLocalCorners[0] > distanceForRecycle0) {
                
        //         this.mItemList.splice(0, 1);
        //         this.recycleItemTmp(tViewItem0);
                
        //         if (!this.mSupportScrollBar) {
        //             this.updateContentSize();
        //             this.checkIfNeedUpdateItemPos();
        //         }
        //         return true;
        //     }
    
        //     const tViewItem1 = this.mItemList[this.mItemList.length - 1];
        //     tViewItem1.getViewCorners(this.itemWorldCorners);
            
        //     const topPos1 = this.itemWorldCorners[1];
        //     const downPos1 = this.itemWorldCorners[0];
    
        //     if (!this.mIsDraging && 
        //         tViewItem1.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
        //         this.viewPortRectLocalCorners[1] - downPos1 > distanceForRecycle1) {
                
        //         this.mItemList.pop();
        //         this.recycleItemTmp(tViewItem1);
                
        //         if (!this.mSupportScrollBar) {
        //             this.updateContentSize();
        //             this.checkIfNeedUpdateItemPos();
        //         }
        //         return true;
        //     }
    
        //     // 检查是否需要添加新的底部项 (BottomToTop)
        //     if (this.viewPortRectLocalCorners[1] - topPos1 < distanceForNew1) {
        //         if (tViewItem1.itemIndex > this.curReadyMaxItemIndex) {
        //             this.curReadyMaxItemIndex = tViewItem1.itemIndex;
        //             this.needCheckNextMaxItem = true;
        //         }
                
        //         const nIndex = tViewItem1.itemIndex + 1;
        //         if (nIndex <= this.curReadyMaxItemIndex || this.needCheckNextMaxItem) {
        //             const newItem = this.getNewItemByIndex(nIndex);
        //             if (!newItem) {
        //                 this.needCheckNextMaxItem = false;
        //                 this.checkIfNeedUpdateItemPos();
        //             } else {
        //                 if (this.mSupportScrollBar) {
        //                     this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().Y, newItem.padding);
        //                 }
                        
        //                 this.mItemList.push(newItem);
        //                 const y = tViewItem1.canvasPanelSlot.GetPosition().Y - tViewItem1.canvasPanelSlot.GetSize().Y - tViewItem1.padding;
        //                 var position = newItem.canvasPanelSlot.GetPosition();
        //                 position.Set(newItem.startPosOffset, y);
        //                 newItem.canvasPanelSlot.SetPosition(position);
                        
        //                 this.updateContentSize();
        //                 this.checkIfNeedUpdateItemPos();
                        
        //                 if (nIndex > this.curReadyMaxItemIndex) {
        //                     this.curReadyMaxItemIndex = nIndex;
        //                 }
        //                 return true;
        //             }
        //         }
        //     }
    
        //     // 检查是否需要添加新的顶部项 (BottomToTop)
        //     if (downPos0 - this.viewPortRectLocalCorners[0] < distanceForNew0) {
        //         if (tViewItem0.itemIndex < this.curReadyMinItemIndex) {
        //             this.curReadyMinItemIndex = tViewItem0.itemIndex;
        //             this.needCheckNextMinItem = true;
        //         }

        //         const nIndex = tViewItem0.itemIndex - 1;
        //         if (nIndex >= this.curReadyMinItemIndex || this.needCheckNextMinItem) {
        //             const newItem = this.getNewItemByIndex(nIndex);
        //             if (!newItem) {
        //                 this.needCheckNextMinItem = false;
        //                 return false;
        //             } else {
        //                 if (this.mSupportScrollBar) {
        //                     this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().Y, newItem.padding);
        //                 }
                        
        //                 this.mItemList.unshift(newItem);
        //                 const y = tViewItem0.canvasPanelSlot.GetPosition().Y + newItem.canvasPanelSlot.GetSize().Y + newItem.padding;
        //                 var position = newItem.canvasPanelSlot.GetPosition();
        //                 position.Set(newItem.startPosOffset, y);
        //                 newItem.canvasPanelSlot.SetPosition(position);
                        
        //                 this.updateContentSize();
        //                 this.checkIfNeedUpdateItemPos();
                        
        //                 if (nIndex < this.curReadyMinItemIndex) {
        //                     this.curReadyMinItemIndex = nIndex;
        //                 }
        //                 return true;
        //             }
        //         }
        //     }
        // }
    
        return false;
    }

    // 水平列表更新
    private updateForHorizontalList(
        distanceForRecycle0: number,
        distanceForRecycle1: number,
        distanceForNew0: number,
        distanceForNew1: number
    ): boolean {
        if (this.mItemTotalCount === 0) {
            if (this.mItemList.length > 0) {
                this.recycleAllItem();
            }
            return false;
        }
    
        if (this.arrangeType === ListItemArrangeType.LeftToRight) {
            if (this.mItemList.length === 0) {
                let curX = this.scrollRect.GetScrollOffset();
                if (curX > 0) {
                    curX = 0;
                }
    
                let index = 0;
                let pos = -curX;
                
                if (this.mSupportScrollBar) {
                    const res = this.getPlusItemIndexAndPosAtGivenPos(-curX);
                    if (!res) {
                        return false;
                    }
                    index = res.index;
                    pos = res.itemPos;
                }
    
                const newItem = this.getNewItemByIndex(index);
                if (!newItem) {
                    return false;
                }
    
                if (this.mSupportScrollBar) {
                    this.setItemSize(index, newItem.canvasPanelSlot.GetSize().X, newItem.padding);
                }
    
                this.mItemList.push(newItem);
                var postion = newItem.canvasPanelSlot.GetPosition();
                postion.Set(pos, newItem.startPosOffset);
                newItem.canvasPanelSlot.SetPosition(postion);
                this.updateContentSize();
                return true;
            }
    
            const tViewItem0 = this.mItemList[0];
            tViewItem0.getViewCorners(this.itemWorldCorners);
            
            const leftPos0 = this.itemWorldCorners[2];
            const rightPos0 = this.itemWorldCorners[3];
    
            if (!this.mIsDraging && 
                tViewItem0.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
                this.viewPortRectLocalCorners[2] - rightPos0 > distanceForRecycle0) {
                
                this.mItemList.splice(0, 1);
                this.recycleItemTmp(tViewItem0);
                
                if (!this.mSupportScrollBar) {
                    this.updateContentSize();
                    this.checkIfNeedUpdateItemPos();
                }
                return true;
            }
    
            const tViewItem1 = this.mItemList[this.mItemList.length - 1];
            tViewItem1.getViewCorners(this.itemWorldCorners);
            
            const leftPos1 = this.itemWorldCorners[2];
            const rightPos1 = this.itemWorldCorners[3];
    
            if (!this.mIsDraging && 
                tViewItem1.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
                leftPos1 - this.viewPortRectLocalCorners[3] > distanceForRecycle1) {
                
                this.mItemList.pop();
                this.recycleItemTmp(tViewItem1);
                
                if (!this.mSupportScrollBar) {
                    this.updateContentSize();
                    this.checkIfNeedUpdateItemPos();
                }
                return true;
            }
    
            // 检查是否需要添加新的右侧项
            if (rightPos1 - this.viewPortRectLocalCorners[3] < distanceForNew1) {
                if (tViewItem1.itemIndex > this.curReadyMaxItemIndex) {
                    this.curReadyMaxItemIndex = tViewItem1.itemIndex;
                    this.needCheckNextMaxItem = true;
                }
                
                const nIndex = tViewItem1.itemIndex + 1;
                if (nIndex <= this.curReadyMaxItemIndex || this.needCheckNextMaxItem) {
                    const newItem = this.getNewItemByIndex(nIndex);
                    if (!newItem) {
                        this.curReadyMaxItemIndex = tViewItem1.itemIndex;
                        this.needCheckNextMaxItem = false;
                        this.checkIfNeedUpdateItemPos();
                    } else {
                        if (this.mSupportScrollBar) {
                            this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().X, newItem.padding);
                        }
                        
                        this.mItemList.push(newItem);
                        const x = tViewItem1.canvasPanelSlot.GetPosition().X + tViewItem1.canvasPanelSlot.GetSize().X + tViewItem1.padding;
                        var postion = newItem.canvasPanelSlot.GetPosition();
                        postion.Set(x, newItem.startPosOffset);
                        newItem.canvasPanelSlot.SetPosition(postion);
                        
                        this.updateContentSize();
                        this.checkIfNeedUpdateItemPos();
                        
                        if (nIndex > this.curReadyMaxItemIndex) {
                            this.curReadyMaxItemIndex = nIndex;
                        }
                        return true;
                    }
                }
            }
    
            // 检查是否需要添加新的左侧项
            if (this.viewPortRectLocalCorners[2] - leftPos0 < distanceForNew0) {
                if (tViewItem0.itemIndex < this.curReadyMinItemIndex) {
                    this.curReadyMinItemIndex = tViewItem0.itemIndex;
                    this.needCheckNextMinItem = true;
                }
                
                const nIndex = tViewItem0.itemIndex - 1;
                if (nIndex >= this.curReadyMinItemIndex || this.needCheckNextMinItem) {
                    const newItem = this.getNewItemByIndex(nIndex);
                    if (!newItem) {
                        this.curReadyMinItemIndex = tViewItem0.itemIndex;
                        this.needCheckNextMinItem = false;
                    } else {
                        if (this.mSupportScrollBar) {
                            this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().X, newItem.padding);
                        }
                        
                        this.mItemList.unshift(newItem);
                        const x = tViewItem0.canvasPanelSlot.GetPosition().X - newItem.canvasPanelSlot.GetSize().X - newItem.padding;
                        var postion = newItem.canvasPanelSlot.GetPosition();
                        postion.Set(x, newItem.startPosOffset);
                        newItem.canvasPanelSlot.SetPosition(postion);
                        
                        this.updateContentSize();
                        this.checkIfNeedUpdateItemPos();
                        
                        if (nIndex < this.curReadyMinItemIndex) {
                            this.curReadyMinItemIndex = nIndex;
                        }
                        return true;
                    }
                }
            }
        } 
        // else { // RightToLeft 排列
        //     if (this.mItemList.length === 0) {
        //         let curX = this.scrollRect.GetScrollOffset();
        //         if (curX < 0) {
        //             curX = 0;
        //         }
                
        //         let index = 0;
        //         let pos = -curX;
                
        //         if (this.mSupportScrollBar) {
        //             const res = this.getPlusItemIndexAndPosAtGivenPos(curX);
        //             if (!res) {
        //                 return false;
        //             }
        //             index = res.index;
        //             pos = res.itemPos;
        //             pos = -pos;
        //         }
        //         const newItem = this.getNewItemByIndex(index);
        //         if (!newItem) {
        //             return false;
        //         }
                
        //         if (this.mSupportScrollBar) {
        //             this.setItemSize(index, newItem.canvasPanelSlot.GetSize().X, newItem.padding);
        //         }
                
        //         this.mItemList.push(newItem);
        //         var postion = newItem.canvasPanelSlot.GetPosition();
        //         postion.Set(pos, newItem.startPosOffset);
        //         newItem.canvasPanelSlot.SetPosition(postion);
        //         this.updateContentSize();
        //         return true;
        //     }
            
        //     const tViewItem0 = this.mItemList[0];
        //     tViewItem0.getViewCorners(this.itemWorldCorners);
            
        //     const leftPos0 = this.itemWorldCorners[2];
        //     const rightPos0 = this.itemWorldCorners[3];
    
        //     if (!this.mIsDraging && 
        //         tViewItem0.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
        //         leftPos0 - this.viewPortRectLocalCorners[3] > distanceForRecycle0) {
                
        //         this.mItemList.splice(0, 1);
        //         this.recycleItemTmp(tViewItem0);
                
        //         if (!this.mSupportScrollBar) {
        //             this.updateContentSize();
        //             this.checkIfNeedUpdateItemPos();
        //         }
        //         return true;
        //     }
    
        //     const tViewItem1 = this.mItemList[this.mItemList.length - 1];
        //     tViewItem1.getViewCorners(this.itemWorldCorners);
            
        //     const leftPos1 = this.itemWorldCorners[2];
        //     const rightPos1 = this.itemWorldCorners[3];
    
        //     if (!this.mIsDraging && 
        //         tViewItem1.itemCreatedCheckFrameCount !== this.listUpdateCheckFrameCount &&
        //         this.viewPortRectLocalCorners[2] - rightPos1 > distanceForRecycle1) {
                
        //         this.mItemList.pop();
        //         this.recycleItemTmp(tViewItem1);
                
        //         if (!this.mSupportScrollBar) {
        //             this.updateContentSize();
        //             this.checkIfNeedUpdateItemPos();
        //         }
        //         return true;
        //     }

        //     // 检查是否需要添加新的左侧项 (RightToLeft)
        //     if (this.viewPortRectLocalCorners[2] - leftPos1 < distanceForNew1) {
        //         if (tViewItem1.itemIndex > this.curReadyMaxItemIndex) {
        //             this.curReadyMaxItemIndex = tViewItem1.itemIndex;
        //             this.needCheckNextMaxItem = true;
        //         }
                
        //         const nIndex = tViewItem1.itemIndex + 1;
        //         if (nIndex <= this.curReadyMaxItemIndex || this.needCheckNextMaxItem) {
        //             const newItem = this.getNewItemByIndex(nIndex);
        //             if (!newItem) {
        //                 this.curReadyMaxItemIndex = tViewItem1.itemIndex;
        //                 this.needCheckNextMaxItem = false;
        //                 this.checkIfNeedUpdateItemPos();
        //             } else {
        //                 if (this.mSupportScrollBar) {
        //                     this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().X, newItem.padding);
        //                 }
                        
        //                 this.mItemList.push(newItem);
        //                 const x = tViewItem1.canvasPanelSlot.GetPosition().X - tViewItem1.canvasPanelSlot.GetSize().X - tViewItem1.padding;
        //                 var postion = newItem.canvasPanelSlot.GetPosition();
        //                 postion.Set(x, newItem.startPosOffset);
        //                 newItem.canvasPanelSlot.SetPosition(postion);
                        
        //                 this.updateContentSize();
        //                 this.checkIfNeedUpdateItemPos();
                        
        //                 if (nIndex > this.curReadyMaxItemIndex) {
        //                     this.curReadyMaxItemIndex = nIndex;
        //                 }
        //                 return true;
        //             }
        //         }
        //     }
    
        //     // 检查是否需要添加新的右侧项 (RightToLeft)
        //     if (rightPos0 - this.viewPortRectLocalCorners[3] < distanceForNew0) {
        //         if (tViewItem0.itemIndex < this.curReadyMinItemIndex) {
        //             this.curReadyMinItemIndex = tViewItem0.itemIndex;
        //             this.needCheckNextMinItem = true;
        //         }
                
        //         const nIndex = tViewItem0.itemIndex - 1;
        //         if (nIndex >= this.curReadyMinItemIndex || this.needCheckNextMinItem) {
        //             const newItem = this.getNewItemByIndex(nIndex);
        //             if (!newItem) {
        //                 this.curReadyMinItemIndex = tViewItem0.itemIndex;
        //                 this.needCheckNextMinItem = false;
        //             } else {
        //                 if (this.mSupportScrollBar) {
        //                     this.setItemSize(nIndex, newItem.canvasPanelSlot.GetSize().X, newItem.padding);
        //                 }
                        
        //                 this.mItemList.unshift(newItem);
        //                 const x = tViewItem0.canvasPanelSlot.GetPosition().X + newItem.canvasPanelSlot.GetSize().X + newItem.padding;
        //                 var postion = newItem.canvasPanelSlot.GetPosition();
        //                 postion.Set(x, newItem.startPosOffset);
        //                 newItem.canvasPanelSlot.SetPosition(postion);
                        
        //                 this.updateContentSize();
        //                 this.checkIfNeedUpdateItemPos();
                        
        //                 if (nIndex < this.curReadyMinItemIndex) {
        //                     this.curReadyMinItemIndex = nIndex;
        //                 }
        //                 return true;
        //             }
        //         }
        //     }
        // }
    
        return false;
    }

    // 更新内容大小
    private updateContentSize(): void {
        const size = this.getContentPanelSize();
        if (this.mIsVertList) {
            if (this.mContainerSize.HeightOverride !== size) {
                this.mContainerSize.SetHeightOverride(size);
            }
        } else {
            if (this.mContainerSize.WidthOverride !== size) {
                this.mContainerSize.SetWidthOverride(size);
            }
        }
    }

    // 获取内容面板大小
    private getContentPanelSize(): number {
        if (this.mSupportScrollBar) {
            const totalSize = this.itemPosMgr.mTotalSize > 0 ? this.itemPosMgr.mTotalSize - this.lastItemPadding : 0;
            return Math.max(0, totalSize);
        }

        const count = this.mItemList.length;
        if (count === 0) return 0;
        if (count === 1) return this.mItemList[0].itemSize;
        if (count === 2) return this.mItemList[0].itemSizeWithPadding + this.mItemList[1].itemSize;

        let size = 0;
        for (let i = 0; i < count - 1; i++) {
            size += this.mItemList[i].itemSizeWithPadding;
        }
        size += this.mItemList[count - 1].itemSize;
        return size;
    }

    // 更新所有显示项位置
    private updateAllShownItemsPos(): void {
        const count = this.mItemList.length;
        if (count === 0) {
            return;
        }
    
        if (this.arrangeType === ListItemArrangeType.TopToBottom) {
            let pos = 0;
            if (this.mSupportScrollBar) {
                pos = this.getItemPos(this.mItemList[0].itemIndex);
            }
            
            const pos1 = this.mItemList[0].canvasPanelSlot.GetPosition();
            const d = pos + pos1.Y;
            let curY = pos;
            for (let i = 0; i < count; i++) {
                const item = this.mItemList[i];
                pos1.Set(item.startPosOffset, curY);
                item.canvasPanelSlot.SetPosition(pos1);
                curY += item.canvasPanelSlot.GetSize().Y + item.padding;
            }
            
            if (d !== 0) {
                const p = this.mScrollRect.GetScrollOffset();
                this.mScrollRect.SetScrollOffset(p - d);
            }
        } 
        // else if (this.arrangeType === ListItemArrangeType.BottomToTop) {
        //     let pos = 0;
        //     if (this.mSupportScrollBar) {
        //         pos = this.getItemPos(this.mItemList[0].itemIndex);
        //     }
            
        //     const pos1 = this.mItemList[0].canvasPanelSlot.GetPosition();
        //     const d = pos + pos1.Y;
        //     let curY = pos;
            
        //     for (let i = 0; i < count; i++) {
        //         const item = this.mItemList[i];
        //         pos1.Set(item.startPosOffset, curY)
        //         item.canvasPanelSlot.SetPosition(pos1);
        //         curY -= item.canvasPanelSlot.GetSize().Y + item.padding;
        //     }
            
        //     if (d !== 0) {
        //         const p = this.mScrollRect.GetScrollOffset();
        //         this.mScrollRect.SetScrollOffset(p - d);
        //     }
        // } 
        else if (this.arrangeType === ListItemArrangeType.LeftToRight) {
            let pos = 0;
            if (this.mSupportScrollBar) {
                pos = this.getItemPos(this.mItemList[0].itemIndex);
            }
            
            const pos1 = this.mItemList[0].canvasPanelSlot.GetPosition();
            const d = pos - pos1.X;
            let curX = pos;
            
            for (let i = 0; i < count; i++) {
                const item = this.mItemList[i];
                pos1.Set(curX, item.startPosOffset);
                item.canvasPanelSlot.SetPosition(pos1);
                curX += item.canvasPanelSlot.GetSize().X + item.padding;
            }
            
            if (d !== 0) {
                const p = this.mScrollRect.GetScrollOffset();
                this.mScrollRect.SetScrollOffset(p - d);
            }
        } 
        // else if (this.arrangeType === ListItemArrangeType.RightToLeft) {
        //     let pos = 0;
        //     if (this.mSupportScrollBar) {
        //         pos = this.getItemPos(this.mItemList[0].itemIndex);
        //     }
            
        //     const pos1 = this.mItemList[0].canvasPanelSlot.GetPosition();
        //     const d = pos - pos1.X;
        //     let curX = pos;
            
        //     for (let i = 0; i < count; i++) {
        //         const item = this.mItemList[i];
        //         pos1.Set(curX, item.startPosOffset);
        //         item.canvasPanelSlot.SetPosition(pos1);
        //         curX -= item.canvasPanelSlot.GetSize().X + item.padding;
        //     }
            
        //     if (d !== 0) {
        //         const p = this.mScrollRect.GetScrollOffset();
        //         this.mScrollRect.SetScrollOffset(p - d);
        //     }
        // }
    }

    private getPlusItemIndexAndPosAtGivenPos(pos: number): {index: number, itemPos: number} {
        return this.itemPosMgr.getItemIndexAndPosAtGivenPos(pos);
    }
    
    private getItemPos(itemIndex: number): number {
        return this.itemPosMgr.getItemPos(itemIndex);
    }
    
    private checkIfNeedUpdateItemPos(): void {
        const count = this.mItemList.length;
        if (count === 0) return;
    
        const viewMaxSize = this.getContentPanelSize();
        const firstItem = this.mItemList[0];
        const lastItem = this.mItemList[count - 1];
    
        switch (this.arrangeType) {
            case ListItemArrangeType.TopToBottom:
                if (firstItem.topY < 0 || 
                    (firstItem.itemIndex === this.curReadyMinItemIndex && firstItem.topY !== 0)) {
                    this.updateAllShownItemsPos();
                } else if (lastItem.bottomY > viewMaxSize || 
                    (lastItem.itemIndex === this.curReadyMaxItemIndex && lastItem.bottomY !== viewMaxSize)) {
                    this.updateAllShownItemsPos();
                }
                break;
                
            // case ListItemArrangeType.BottomToTop:
            //     if (firstItem.bottomY > 0 || 
            //         (firstItem.itemIndex === this.curReadyMinItemIndex && firstItem.bottomY !== 0)) {
            //         this.updateAllShownItemsPos();
            //     } else if (-lastItem.topY > viewMaxSize || 
            //         (lastItem.itemIndex === this.curReadyMaxItemIndex && -lastItem.topY !== viewMaxSize)) {
            //         this.updateAllShownItemsPos();
            //     }
            //     break;
                
            case ListItemArrangeType.LeftToRight:
                if (firstItem.leftX < 0 || 
                    (firstItem.itemIndex === this.curReadyMinItemIndex && firstItem.leftX !== 0)) {
                    this.updateAllShownItemsPos();
                } else if (lastItem.rightX > viewMaxSize || 
                    (lastItem.itemIndex === this.curReadyMaxItemIndex && lastItem.rightX !== viewMaxSize)) {
                    this.updateAllShownItemsPos();
                }
                break;
                
            // case ListItemArrangeType.RightToLeft:
            //     if (firstItem.rightX > 0 || 
            //         (firstItem.itemIndex === this.curReadyMinItemIndex && firstItem.rightX !== 0)) {
            //         this.updateAllShownItemsPos();
            //     } else if (-lastItem.leftX > viewMaxSize || 
            //         (lastItem.itemIndex === this.curReadyMaxItemIndex && -lastItem.leftX !== viewMaxSize)) {
            //         this.updateAllShownItemsPos();
            //     }
            //     break;
        }
    }
    
    // private adjustContainerPivot(rtf: CanvasPanel): void {
        // const pivot = new Vector2D();
        // switch (this.arrangeType) {
        //     case ListItemArrangeType.BottomToTop:
        //         pivot.Set(rtf.anchorPoint.x, 0);
        //         break;
        //     case ListItemArrangeType.TopToBottom:
        //         pivot.Set(rtf.anchorPoint.x, 1);
        //         break;
        //     case ListItemArrangeType.LeftToRight:
        //         pivot.Set(0, rtf.anchorPoint.Y);
        //         break;
        //     case ListItemArrangeType.RightToLeft:
        //         pivot.Set(1, rtf.anchorPoint.Y);
        //         break;
        // }
        // rtf.setAnchorPoint(pivot.X, pivot.Y);
    // }

    public refreshAllShownItem()
    {
        const count = this.mItemList.length;
        if (count == 0)
        {
            return;
        }
       
        this.refreshAllShownItemWithFirstIndex(this.mItemList[0].itemIndex);
    }
    
    public refreshAllShownItemWithFirstIndex(firstItemIndex: number): void {

        const count = this.mItemList.length;
        if (count === 0) return;
    
        const firstItem = this.mItemList[0];
        const pos = firstItem.canvasPanelSlot.GetPosition();
        this.recycleAllItem();
       
        for (let i = 0; i < count; i++) {
            const curIndex = firstItemIndex + i;
            const newItem = this.getNewItemByIndex(curIndex);
            if (!newItem) break;
    
            if (this.mIsVertList) {
                pos.X = newItem.startPosOffset;
            } else {
                pos.Y = newItem.startPosOffset;
            }
    
            newItem.canvasPanelSlot.SetPosition(pos);
            if (this.mSupportScrollBar) {
                if (this.mIsVertList) {
                    this.setItemSize(curIndex, newItem.canvasPanelSlot.GetSize().Y, newItem.padding);
                } else {
                    this.setItemSize(curIndex, newItem.canvasPanelSlot.GetSize().X, newItem.padding);
                }
            }
            this.mItemList.push(newItem);
        }
    
        this.updateContentSize();
        this.updateAllShownItemsPos();
        this.clearAllTmpRecycledItem();
    }

    private setItemSize(itemIndex: number, itemSize: number, padding: number): void {
        if (!this.mSupportScrollBar || itemIndex < 0) return;
        this.itemPosMgr.setItemSize(itemIndex, itemSize + padding);
        
        if (itemIndex >= this.lastItemIndex) {
            this.lastItemIndex = itemIndex;
            this.lastItemPadding = padding;
        }
    }

    // cacheDragPointerEventData(eventData: EventTouch)
    // {
    //     if (this.pointerEventData == null)
    //     {
    //         this.pointerEventData = eventData;
    //     }
    // }

    private getNewItemByIndex(index: number): LoopListViewItem2 {
        if (this.supportScrollBar && index < 0) return null;
        if (this.mItemTotalCount > 0 && index >= this.mItemTotalCount) return null;
        if (this.mItemTotalCount > 0 && index < 0) return null;

        const newItem = this.onGetItemByIndex(this, index);
        if (!newItem) return null;
        
        newItem.itemIndex = index;
        newItem.itemCreatedCheckFrameCount = this.listUpdateCheckFrameCount;
        return newItem;
    }
}