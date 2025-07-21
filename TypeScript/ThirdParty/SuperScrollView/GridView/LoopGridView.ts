
import { CanvasPanel, CanvasPanelSlot, Class, EOrientation, ESlateVisibility, ScrollBox, SizeBox, UeBridgeHelper, Vector2D, Widget, WidgetBlueprintLibrary } from 'ue';
import { GridFixedType, GridItemArrangeType, RowColumnPair } from '../Common/CommonDefine';
import { GridItemGroup } from './GridItemGroup';
import { GridItemPool } from './GridItemPool';
import { LoopGridViewItem } from './LoopGridViewItem';

export class GridViewItemPrefabConfData {
    public name: string
    public mItemPrefab: Class = null;
    public mInitCreateCount: number = 0;
}

export class LoopGridViewSettingParam {
    public mItemSize: number[]|null;
    public mPadding: number[]|null = null;
    public mItemPadding: number[]|null = null;
    public mFixedRowOrColumnCount: number|null = null;
}


class ItemRangeData {
    public mMaxRow: number = 0;
    public mMinRow: number = 0;
    public mMaxColumn: number = 0;
    public mMinColumn: number = 0;
    public mCheckedPosition: Vector2D = Vector2D.ZeroVector;
}



export class LoopGridView {

    public constructor(root: ScrollBox){
        this.mscrollView = root;
    }

    private mitemPoolDict: Map<string, GridItemPool> = new Map();
    private mitemPoolList: GridItemPool[] = [];
    

    private mitemPrefabDataList: GridViewItemPrefabConfData[] = [];
    

    private marrangeType: GridItemArrangeType = GridItemArrangeType.TopLeftToBottomRight;
    
    private mcontainerTrans: CanvasPanel | null = null;
    private mscrollView: ScrollBox | null = null;
    private mviewPortTransform: Widget | null = null;
    private mContainerSize: SizeBox = null;

    private mitemTotalCount: number = 0;

    private mfixedRowOrColumnCount: number|null = null;
    

    private mpadding: any = { left: 0, right: 0, top: 0, bottom: 0 };
    

    private mitemPadding: Vector2D = new Vector2D(0, 0);

    private mitemSize: Vector2D = new Vector2D(100, 100);
    
    private mitemRecycleDistance: Vector2D = new Vector2D(50, 50);
    
    private mitemSizeWithPadding: Vector2D = new Vector2D(0, 0);
    private mstartPadding: Vector2D = new Vector2D(0, 0);
    private mendPadding: Vector2D = new Vector2D(0, 0);
    
    private monGetItemByRowColumn: (gridView: LoopGridView, index: number, row: number, column: number) => LoopGridViewItem | null = null;
    private mitemGroupObjPool: GridItemGroup[] = [];
    private mitemGroupList: GridItemGroup[] = [];
    
    private mIsDraging: boolean = false;
    private mrowCount: number = 0;
    private mcolumnCount: number = null;
    private isVertical : boolean;
    
    public mOnBeginDragAction: (eventData: any) => void = null;
    public mOnDragingAction: (eventData: any) => void = null;
    public mOnEndDragAction: (eventData: any) => void = null;

    
    private mgridFixedType: GridFixedType = GridFixedType.ColumnCountFixed;


    private mlistViewInited: boolean = false;
    private mListViewSizeInited: boolean = false;

    private mcurFrameItemRangeData: ItemRangeData = new ItemRangeData();
    private mneedCheckContentPosLeftCount: number = 1;

    
    public get itemPrefabDataList(): GridViewItemPrefabConfData[] {
        return this.mitemPrefabDataList;
    }
    
    public get itemTotalCount(): number {
        return this.mitemTotalCount;
    }
    
    public get containerTrans(): CanvasPanel | null {
        return this.mcontainerTrans;
    }
    
    get viewPortWidth(): number {
        return UeBridgeHelper.GetInstance().GetGeometryLocalSize(this.mviewPortTransform.GetCachedGeometry()).X;
    }

    get viewPortHeight(): number {
        return UeBridgeHelper.GetInstance().GetGeometryLocalSize(this.mviewPortTransform.GetCachedGeometry()).Y;
    }

    
    public get scrollView(): ScrollBox | null {
        return this.mscrollView;
    }
    
    public get isDraging(): boolean {
        return this.mIsDraging;
    }
    
    public get itemSize(): Vector2D {
        return this.mitemSize;
    }
    
    public set itemSize(value: Vector2D) {
        this.setItemSize(value);
    }
    
    public get itemPadding(): Vector2D {
        return this.mitemPadding;
    }
    
    public set itemPadding(value: Vector2D) {
        this.setItemPadding(value);
    }
    
    public get itemSizeWithPadding(): Vector2D {
        return this.mitemSizeWithPadding;
    }
    
    public get padding(): any {
        return this.mpadding;
    }
    
    public set padding(value: any) {
        this.setPadding(value);
    }


    // 添加预制体配置数据
    public addItemPrefabConfData(config: GridViewItemPrefabConfData){
        if(!this.mlistViewInited) {
            console.error("initGridView firset");
            return;
        }
        if(config == null)  {
            console.error("config is null");
            return;
        }
        let data: GridViewItemPrefabConfData = config;

        if (!data.mItemPrefab) {
            console.error("A item prefab is null");
            return;
        }

        const prefab = data.mItemPrefab;
        const prefabName = config.name || prefab.GetName();
        if (this.mitemPoolDict.has(prefabName)) {
            console.error(`A item prefab with name ${prefabName} has existed!`);
            return;
        }

        const pool = new GridItemPool();
        pool.init(prefab, prefabName, data.mInitCreateCount, this.mcontainerTrans);
    
        this.mitemPoolDict.set(prefabName, pool);
        this.mitemPoolList.push(pool);
        console.info("addItemPrefabConfData "+ prefabName)
    }

    
    public initGridView(itemTotalCount: number, 
                        onGetItemByRowColumn: (gridView: LoopGridView, index: number, row: number, column: number) => LoopGridViewItem | null, 
                        settingParam: LoopGridViewSettingParam = null): void {
        if (this.mlistViewInited) {
            console.error("LoopGridView.InitListView method can be called only once.");
            return;
        }
        
        this.mlistViewInited = true;
        
        if (itemTotalCount < 0) {
            console.error("itemTotalCount is < 0");
            itemTotalCount = 0;
        }
        
        if (!!settingParam) {
            this.updateFromSettingParam(settingParam);
        }
        
        if (!this.mscrollView) {
            console.error("ListView Init Failed! ScrollView component not found!");
            return;
        }

        this.mContainerSize = this.mscrollView.GetChildAt(0) as SizeBox;
        this.mcontainerTrans = this.mContainerSize.GetChildAt(0) as CanvasPanel;

        this.mviewPortTransform = this.mscrollView;
        this.isVertical = this.mscrollView.Orientation != EOrientation.Orient_Horizontal;
        this.mgridFixedType = this.isVertical?GridFixedType.ColumnCountFixed:GridFixedType.RowCountFixed;
        this.mscrollView.SetOrientation(this.isVertical?EOrientation.Orient_Vertical:EOrientation.Orient_Horizontal);
        this.mscrollView.SetAnimateWheelScrolling(false);//暂不支持

        this.setScrollbarListener();

        // this.adjustViewPortPivot();
        // this.adjustContainerAnchorAndPivot();
        this.monGetItemByRowColumn = onGetItemByRowColumn;
        this.mneedCheckContentPosLeftCount = 4;

        this.mitemTotalCount = itemTotalCount;

        this.resetListView();

        this.updateAllGridSetting();
    }

     // 设置滚动条监听
     private setScrollbarListener(): void {
        this.scrollView.OnScrollBarVisibilityChanged.Add(this.onScrollBarVisibilityChanged.bind(this))
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
    
    public clearListView(): void {
        this.setListItemCount(0, true);
        
        for (const pool of this.mitemPoolList) {
            pool.clearPool();
        }
        
        this.mitemPoolList.length = 0;
        this.mitemPoolDict.clear();
        this.monGetItemByRowColumn = null;
        this.mlistViewInited = false;
    }
    
    public cleanUp(name: string = null, beforeDestroy: (node: Widget) => void = null): void {
        if (!name) {
            for (const pool of this.mitemPoolList) {
                pool.cleanUp(beforeDestroy);
            }
        } else if (this.mitemPoolDict.has(name)) {
            const pool = this.mitemPoolDict.get(name);
            pool.cleanUp(beforeDestroy);
        }
    }

            
    // 重置列表视图
    public resetListView(): void {
        if(this.mListViewSizeInited) return;
        if(!!this.mfixedRowOrColumnCount) {
            this.mListViewSizeInited = true;
            return;
        }
        const size =  UeBridgeHelper.GetInstance().GetGeometryLocalSize(this.mscrollView.GetCachedGeometry());
        const width = size.X;
        const height = size.Y;
        if(width==0 && height == 0){
            return;
        }
        this.mListViewSizeInited = true;

        if(this.isVertical){
            this.mfixedRowOrColumnCount = Math.floor(width/this.itemSize.X);
        }else{
            this.mfixedRowOrColumnCount = Math.floor(height/this.itemSize.Y);
        }
        this.forceToCheckContentPos();
        this.updateColumnRowCount();
        this.updateContentSize();
        console.log("auto set fixedRowOrColumnCount = " + this.mfixedRowOrColumnCount);
    }
    
    public setListItemCount(itemCount: number, resetPos: boolean = true): void {
        if (itemCount < 0) {
            return;
        }
        
        if (itemCount === this.mitemTotalCount) {
            return;
        }
        
        this.mitemTotalCount = itemCount;
        this.updateColumnRowCount();
        this.updateContentSize();
        this.forceToCheckContentPos();
        
        if (this.mitemTotalCount === 0) {
            this.recycleAllItem();
            this.clearAllTmpRecycledItem();
            return;
        }
        
        this.vaildAndSetContainerPos();
        this.updateGridViewContent();
        this.clearAllTmpRecycledItem();
        
        if (resetPos) {
            this.movePanelToItemByRowColumn(0, 0);
        }
    }
    
    public newListViewItem(itemPrefabName: string, index: number = null): LoopGridViewItem | null {
        const pool = this.mitemPoolDict.get(itemPrefabName);
        if (!pool) {
            for (const [key,val] of this.mitemPoolDict) {
                if(key.indexOf(itemPrefabName) >= 0){
                    console.error("not found "+itemPrefabName+" from pool, it might be called "+ key);
                    return null;
                }
            }
            console.error("not found "+itemPrefabName+" from pool, please addItemPrefabConfData first!");
            return null;
        }

        const item = pool.getItem(index);
        item.canvasPanelSlot = this.mcontainerTrans.AddChildToCanvas(item.widget);
        item.widget.SetRenderScale(Vector2D.UnitVector);
        item.canvasPanelSlot.SetPosition(Vector2D.UnitVector);
        this.adjustItemAnchorAndPivot(item.canvasPanelSlot);
        item.canvasPanelSlot.SetSize(this.itemSize)
        item.parentGridView = this;
        return item;
    }
    
    public refreshItemByItemIndex(itemIndex: number): void {
        if (itemIndex < 0 || itemIndex >= this.itemTotalCount) {
            return;
        }
        
        const count = this.mitemGroupList.length;
        if (count === 0) {
            return;
        }
        
        const val = this.getRowColumnByItemIndex(itemIndex);
        this.refreshItemByRowColumn(val.mRow, val.mColumn);
    }
    
    public refreshItemByRowColumn(row: number, column: number): void {
        const count = this.mitemGroupList.length;
        if (count === 0) {
            return;
        }
        
        if (this.mgridFixedType === GridFixedType.ColumnCountFixed) {
            const group = this.getShownGroup(row);
            if (!group) {
                return;
            }
            
            const curItem = group.getItemByColumn(column);
            if (!curItem) {
                return;
            }
            
            const newItem = this.getNewItemByRowColumn(row, column);
            if (!newItem) {
                return;
            }
            
            const pos = curItem.canvasPanelSlot.GetPosition();
            group.replaceItem(curItem, newItem);
            this.recycleItemTmp(curItem);
            newItem.canvasPanelSlot.SetPosition(pos);
            this.clearAllTmpRecycledItem();
        } else {
            const group = this.getShownGroup(column);
            if (!group) {
                return;
            }
            
            const curItem = group.getItemByRow(row);
            if (!curItem) {
                return;
            }
            
            const newItem = this.getNewItemByRowColumn(row, column);
            if (!newItem) {
                return;
            }
            
            const pos = curItem.canvasPanelSlot.GetPosition();
            group.replaceItem(curItem, newItem);
            this.recycleItemTmp(curItem);
            newItem.canvasPanelSlot.SetPosition(pos);
            this.clearAllTmpRecycledItem();
        }
    }
    
    
    public forceToCheckContentPos(): void {
        if (this.mneedCheckContentPosLeftCount <= 0) {
            this.mneedCheckContentPosLeftCount = 1;
        }
    }
    
    public movePanelToItemByIndex(itemIndex: number, offsetX: number = 0, offsetY: number = 0): void {
        if (this.itemTotalCount === 0) {
            return;
        }
        
        if (itemIndex >= this.itemTotalCount) {
            itemIndex = this.itemTotalCount - 1;
        }
        
        if (itemIndex < 0) {
            itemIndex = 0;
        }
        
        const val = this.getRowColumnByItemIndex(itemIndex);
        this.movePanelToItemByRowColumn(val.mRow, val.mColumn, offsetX, offsetY);
    }
    
    public movePanelToItemByRowColumn(row: number, column: number, offsetX: number = 0, offsetY: number = 0): void {
        
        if (this.mitemTotalCount === 0) {
            return;
        }
        
        const itemPos = this.getItemPos(row, column);
        let pos = this.scrollView.GetScrollOffset();
        
        if (!this.isVertical) {
            const maxCanMoveX = Math.max(this.mContainerSize.WidthOverride - this.viewPortWidth, 0);
            if (maxCanMoveX > 0) {
                let x = -itemPos.X + offsetX;
                x = Math.min(Math.abs(x), maxCanMoveX) * Math.sign(x);
                if(pos!= x){
                    pos = x;
                    this.scrollView.SetScrollOffset(pos);
                }
            }
        }
        else {
            const maxCanMoveY = Math.max(this.mContainerSize.HeightOverride - this.viewPortHeight, 0);
            if (maxCanMoveY > 0) {
                let y = -itemPos.Y + offsetY;
                y = Math.min(Math.abs(y), maxCanMoveY) * Math.sign(y);
                if(pos!= y){
                    pos = y;
                    this.scrollView.SetScrollOffset(pos);
                }
            }
        }
        
        this.vaildAndSetContainerPos();
        this.forceToCheckContentPos();
    }
    
    public refreshAllShownItem(): void {
        const count = this.mitemGroupList.length;
        if (count === 0) {
            return;
        }
        
        this.forceToCheckContentPos();
        this.recycleAllItem();
        this.updateGridViewContent();
    }
    
    
    public getItemIndexByRowColumn(row: number, column: number): number {
        if (this.mgridFixedType === GridFixedType.ColumnCountFixed) {
            return row * this.mfixedRowOrColumnCount + column;
        } else {
            return column * this.mfixedRowOrColumnCount + row;
        }
    }
    
    public getRowColumnByItemIndex(itemIndex: number): RowColumnPair {
        if (itemIndex < 0) {
            itemIndex = 0;
        }
        
        if (this.mgridFixedType === GridFixedType.ColumnCountFixed) {
            const row = Math.floor(itemIndex / this.mfixedRowOrColumnCount);
            const column = itemIndex % this.mfixedRowOrColumnCount;
            return new RowColumnPair(row, column);
        } else {
            const column = Math.floor(itemIndex / this.mfixedRowOrColumnCount);
            const row = itemIndex % this.mfixedRowOrColumnCount;
            return new RowColumnPair(row, column);
        }
    }
    
    
    public getItemPos(row: number, column: number,pos: Vector2D = null): Vector2D  {
        if(!pos) pos = new Vector2D();
        const x = this.mstartPadding.X + column * this.mitemSizeWithPadding.X;
        const y = this.mstartPadding.Y + row * this.mitemSizeWithPadding.Y;
        
        if (this.marrangeType === GridItemArrangeType.TopLeftToBottomRight) {
            pos.Set(x,y)
        } else if (this.marrangeType === GridItemArrangeType.BottomLeftToTopRight) {
            pos.Set(x,-y)
        } else if (this.marrangeType === GridItemArrangeType.TopRightToBottomLeft) {
            pos.Set(-x,y)
        } else if (this.marrangeType === GridItemArrangeType.BottomRightToTopLeft) {
            pos.Set(-x,-y)
        }
        return pos;
    }
    
    public getShownItemByItemIndex(itemIndex: number): LoopGridViewItem | null {
        if (itemIndex < 0 || itemIndex >= this.itemTotalCount) {
            return null;
        }
        
        if (this.mitemGroupList.length === 0) {
            return null;
        }
        
        const val = this.getRowColumnByItemIndex(itemIndex);
        return this.getShownItemByRowColumn(val.mRow, val.mColumn);
    }
    
    public getShownItemByRowColumn(row: number, column: number): LoopGridViewItem | null {
        if (this.mitemGroupList.length === 0) {
            return null;
        }
        
        if (this.mgridFixedType === GridFixedType.ColumnCountFixed) {
            const group = this.getShownGroup(row);
            if (!group) {
                return null;
            }
            return group.getItemByColumn(column);
        } else {
            const group = this.getShownGroup(column);
            if (!group) {
                return null;
            }
            return group.getItemByRow(row);
        }
    }
    
    public updateAllGridSetting(): void {
        this.updateStartEndPadding();
        this.updateItemSize();
        this.updateColumnRowCount();
        this.updateContentSize();
        this.forceToCheckContentPos();
    }
    
    public setGridFixedGroupCount(fixedType: GridFixedType, count: number): void {
        if (this.mgridFixedType === fixedType && this.mfixedRowOrColumnCount === count) {
            return;
        }
        
        this.mgridFixedType = fixedType;
        this.mfixedRowOrColumnCount = count;
        this.updateColumnRowCount();
        this.updateContentSize();
        
        if (this.mitemGroupList.length === 0) {
            return;
        }
        
        this.recycleAllItem();
        this.forceToCheckContentPos();
    }
    
    public setItemSize(newSize: Vector2D): void {
        if (newSize.Equals(this.mitemSize)) {
            return;
        }
        
        this.mitemSize = newSize;
        this.updateItemSize();
        this.updateContentSize();
        
        if (this.mitemGroupList.length === 0) {
            return;
        }
        
        this.recycleAllItem();
        this.forceToCheckContentPos();
    }
    
    public setItemPadding(newPadding: Vector2D): void {
        if (newPadding.Equals(this.mitemPadding)) {
            return;
        }
        
        this.mitemPadding = newPadding;
        this.updateItemSize();
        this.updateContentSize();
        
        if (this.mitemGroupList.length === 0) {
            return;
        }
        
        this.recycleAllItem();
        this.forceToCheckContentPos();
    }
    
    public setPadding(newPadding: any): void {
        if (newPadding === this.mpadding) {
            return;
        }
        
        this.mpadding = newPadding;
        this.updateStartEndPadding();
        this.updateContentSize();
        
        if (this.mitemGroupList.length === 0) {
            return;
        }
        
        this.recycleAllItem();
        this.forceToCheckContentPos();
    }
    
    public updateContentSize(): void {
        const width = this.mstartPadding.X + this.mcolumnCount * this.mitemSizeWithPadding.X - this.mitemPadding.X + this.mendPadding.X;
        const height = this.mstartPadding.Y + this.mrowCount * this.mitemSizeWithPadding.Y - this.mitemPadding.Y + this.mendPadding.Y;
        
        // 直接设置UITransform的内容尺寸
        this.mContainerSize.WidthOverride = width;
        this.mContainerSize.HeightOverride = height;
        // console.error(width+" "+height)
    }
    
    public vaildAndSetContainerPos(): void {
        const pos = this.scrollView.GetScrollOffset();
        const validPos = this.getContainerVaildPos(this.isVertical?0: pos, this.isVertical?pos:0);
        this.scrollView.SetScrollOffset(this.isVertical?validPos.Y:validPos.X);
    }
    
    public clearAllTmpRecycledItem(): void {
        for (const pool of this.mitemPoolList) {
            pool.clearTmpRecycledItem();
        }
    }
    
    public recycleAllItem(): void {
        for (const group of this.mitemGroupList) {
            this.recycleItemGroupTmp(group);
        }
        
        this.mitemGroupList.length = 0;
    }
    
    public updateGridViewContent(): void {
        
        if (this.mitemTotalCount === 0) {
            if (this.mitemGroupList.length > 0) {
                this.recycleAllItem();
            }
            return;
        }
        
        this.updateCurFrameItemRangeData();
        
        if (this.mgridFixedType === GridFixedType.ColumnCountFixed) {
            let groupCount = this.mitemGroupList.length;
            const minRow = this.mcurFrameItemRangeData.mMinRow;
            const maxRow = this.mcurFrameItemRangeData.mMaxRow;
            // Log.error(minRow+"-"+maxRow +" "+groupCount);
            for (let i = groupCount - 1; i >= 0; --i) {
                const group = this.mitemGroupList[i];
                if (group.groupIndex < minRow || group.groupIndex > maxRow) {
                    this.recycleItemGroupTmp(group);
                    this.mitemGroupList.splice(i, 1);
                }
            }
            
            if (this.mitemGroupList.length === 0) {
                const group = this.createItemGroup(minRow);
                this.mitemGroupList.push(group);
            }
            
            while (this.mitemGroupList[0].groupIndex > minRow) {
                const group = this.createItemGroup(this.mitemGroupList[0].groupIndex - 1);
                this.mitemGroupList.unshift(group);
            }
            
            while (this.mitemGroupList[this.mitemGroupList.length - 1].groupIndex < maxRow) {
                const group = this.createItemGroup(this.mitemGroupList[this.mitemGroupList.length - 1].groupIndex + 1);
                this.mitemGroupList.push(group);
            }
            
            for (const group of this.mitemGroupList) {
                this.updateRowItemGroupForRecycleAndNew(group);
            }
        } else {
            let groupCount = this.mitemGroupList.length;
            const minColumn = this.mcurFrameItemRangeData.mMinColumn;
            const maxColumn = this.mcurFrameItemRangeData.mMaxColumn;
            
            for (let i = groupCount - 1; i >= 0; --i) {
                const group = this.mitemGroupList[i];
                if (group.groupIndex < minColumn || group.groupIndex > maxColumn) {
                    this.recycleItemGroupTmp(group);
                    this.mitemGroupList.splice(i, 1);
                }
            }
            
            if (this.mitemGroupList.length === 0) {
                const group = this.createItemGroup(minColumn);
                this.mitemGroupList.push(group);
            }
            
            while (this.mitemGroupList[0].groupIndex > minColumn) {
                const group = this.createItemGroup(this.mitemGroupList[0].groupIndex - 1);
                this.mitemGroupList.unshift(group);
            }
            
            while (this.mitemGroupList[this.mitemGroupList.length - 1].groupIndex < maxColumn) {
                const group = this.createItemGroup(this.mitemGroupList[this.mitemGroupList.length - 1].groupIndex + 1);
                this.mitemGroupList.push(group);
            }
            
            for (const group of this.mitemGroupList) {
                this.updateColumnItemGroupForRecycleAndNew(group);
            }
        }
    }
    
    public updateStartEndPadding(): void {
        if (this.marrangeType === GridItemArrangeType.TopLeftToBottomRight) {
            this.mstartPadding.X = this.mpadding.left;
            this.mstartPadding.Y = this.mpadding.top;
            this.mendPadding.X = this.mpadding.right;
            this.mendPadding.Y = this.mpadding.bottom;
        } else if (this.marrangeType === GridItemArrangeType.BottomLeftToTopRight) {
            this.mstartPadding.X = this.mpadding.left;
            this.mstartPadding.Y = this.mpadding.bottom;
            this.mendPadding.X = this.mpadding.right;
            this.mendPadding.Y = this.mpadding.top;
        } else if (this.marrangeType === GridItemArrangeType.TopRightToBottomLeft) {
            this.mstartPadding.X = this.mpadding.right;
            this.mstartPadding.Y = this.mpadding.top;
            this.mendPadding.X = this.mpadding.left;
            this.mendPadding.Y = this.mpadding.bottom;
        } else if (this.marrangeType === GridItemArrangeType.BottomRightToTopLeft) {
            this.mstartPadding.X = this.mpadding.right;
            this.mstartPadding.Y = this.mpadding.bottom;
            this.mendPadding.X = this.mpadding.left;
            this.mendPadding.Y = this.mpadding.top;
        }
    }
    
    public updateItemSize(): void {
        let tempNode: Widget = null;
    
        try {
            if (this.mitemSize.X > 0 && this.mitemSize.Y > 0) {
                this.mitemSizeWithPadding.Set(
                    this.mitemSize.X + this.mitemPadding.X,
                    this.mitemSize.Y + this.mitemPadding.Y
                );
                return;
            }
            
            if (this.mitemPrefabDataList.length > 0) {
                const prefab = this.mitemPrefabDataList[0].mItemPrefab;
                if (prefab) {
                    tempNode = WidgetBlueprintLibrary.Create(this.containerTrans.GetWorld(), prefab, null) as Widget;
                    const slots = this.containerTrans.AddChildToCanvas(tempNode);
                    if (slots) {
                        this.mitemSize.Set(slots.GetSize().X,slots.GetSize().Y);
                        this.mitemSizeWithPadding = new Vector2D(
                            this.mitemSize.X + this.mitemPadding.X,
                            this.mitemSize.Y + this.mitemPadding.Y
                        );
                    }
                }
            }
        } finally {
            // 确保销毁临时节点
            if (tempNode) {
                tempNode.RemoveFromParent();
            }
        }
    }
    
    public updateColumnRowCount(): void {
        if (this.mgridFixedType === GridFixedType.ColumnCountFixed) {
            this.mcolumnCount = this.mfixedRowOrColumnCount;
            this.mrowCount = Math.floor(this.mitemTotalCount / this.mcolumnCount);
            
            if (this.mitemTotalCount % this.mcolumnCount > 0) {
                this.mrowCount++;
            }
            
            if (this.mitemTotalCount <= this.mcolumnCount) {
                this.mcolumnCount = this.mitemTotalCount;
            }
        } else {
            this.mrowCount = this.mfixedRowOrColumnCount;
            this.mcolumnCount = Math.floor(this.mitemTotalCount / this.mrowCount);
            
            if (this.mitemTotalCount % this.mrowCount > 0) {
                this.mcolumnCount++;
            }
            
            if (this.mitemTotalCount <= this.mrowCount) {
                this.mrowCount = this.mitemTotalCount;
            }
        }
    }
    
    
    private recycleItemGroupTmp(group: GridItemGroup): void {
        if (!group) return;
        
        while (group.first) {
            const item = group.removeFirst();
            this.recycleItemTmp(item);
        }
        
        group.clear();
        this.recycleOneItemGroupObj(group);
    }
    
    private recycleItemTmp(item: LoopGridViewItem): void {
        if (!item || !item.itemPrefabName) return;
        
        const pool = this.mitemPoolDict.get(item.itemPrefabName);
        if (pool) {
            pool.recycleItem(item);
        }
    }
    
    // private adjustViewPortPivot(): void {
    //     if (!this.mviewPortTransform) return;
        
    //     switch (this.marrangeType) {
    //         case GridItemArrangeType.TopLeftToBottomRight:
    //             this.mviewPortTransform.anchorPoint = new Vector2D(0, 1);
    //             break;
    //         case GridItemArrangeType.BottomLeftToTopRight:
    //             this.mviewPortTransform.anchorPoint = new Vector2D(0, 0);
    //             break;
    //         case GridItemArrangeType.TopRightToBottomLeft:
    //             this.mviewPortTransform.anchorPoint = new Vector2D(1, 1);
    //             break;
    //         case GridItemArrangeType.BottomRightToTopLeft:
    //             this.mviewPortTransform.anchorPoint = new Vector2D(1, 0);
    //             break;
    //     }
    // }
    
    // private adjustContainerAnchorAndPivot(): void {
    //     if (!this.mcontainerTrans) return;
        
    //     switch (this.marrangeType) {
    //         case GridItemArrangeType.TopLeftToBottomRight:
    //             this.mcontainerTrans.anchorPoint = new Vector2D(0, 1);
    //             break;
    //         case GridItemArrangeType.BottomLeftToTopRight:
    //             this.mcontainerTrans.anchorPoint = new Vector2D(0, 0);
    //             break;
    //         case GridItemArrangeType.TopRightToBottomLeft:
    //             this.mcontainerTrans.anchorPoint = new Vector2D(1, 1);
    //             break;
    //         case GridItemArrangeType.BottomRightToTopLeft:
    //             this.mcontainerTrans.anchorPoint = new Vector2D(1, 0);
    //             break;
    //     }
    // }
    
    private adjustItemAnchorAndPivot(transform: CanvasPanelSlot): void {
        const pivot = transform.GetAlignment();
        const anchor = transform.GetAnchors();
        switch (this.marrangeType) {
            case GridItemArrangeType.TopLeftToBottomRight:
                pivot.Set(0, 0);
                anchor.Minimum.Set(0, 0);
                anchor.Maximum.Set(0, 0);
                break;
            case GridItemArrangeType.BottomLeftToTopRight:
                pivot.Set(0, 1);
                anchor.Minimum.Set(0, 1);
                anchor.Maximum.Set(0, 1);
                break;
            case GridItemArrangeType.TopRightToBottomLeft:
                pivot.Set(1, 0);
                anchor.Minimum.Set(1, 0);
                anchor.Maximum.Set(1, 0);
                break;
            case GridItemArrangeType.BottomRightToTopLeft:
                pivot.Set(1, 1);
                anchor.Minimum.Set(1, 1);
                anchor.Maximum.Set(1, 1);
                break;
        }
    }
    
    
    private getNewItemByRowColumn(row: number, column: number): LoopGridViewItem | null {
        const itemIndex = this.getItemIndexByRowColumn(row, column);
        if (itemIndex < 0 || itemIndex >= this.itemTotalCount) {
            return null;
        }
        
        const newItem = this.monGetItemByRowColumn(this, itemIndex, row, column);
        if (!newItem) return null;
        
        newItem.nextItem = null;
        newItem.prevItem = null;
        newItem.row = row;
        newItem.column = column;
        newItem.itemIndex = itemIndex;
        return newItem;
    }
    
    private getCeilItemRowColumnAtGivenAbsPos(ax: number, ay: number): RowColumnPair {
        ax = Math.abs(ax);
        ay = Math.abs(ay);
        
        let row = Math.ceil((ay - this.mstartPadding.Y) / this.mitemSizeWithPadding.Y) - 1;
        let column = Math.ceil((ax - this.mstartPadding.X) / this.mitemSizeWithPadding.X) - 1;
        
        row = Math.max(0, Math.min(row, this.mrowCount - 1));
        column = Math.max(0, Math.min(column, this.mcolumnCount - 1));
        
        return new RowColumnPair(row, column);
    }
    
    public update(): void {
        if (!this.mlistViewInited) return;
        this.resetListView();
        if(!this.mListViewSizeInited) return;

        this.updateGridViewContent();
        this.clearAllTmpRecycledItem();
    }
    
    private createItemGroup(groupIndex: number): GridItemGroup {
        const ret = this.getOneItemGroupObj();
        ret.groupIndex = groupIndex;
        return ret;
    }
    
    private getContainerMovedDistance(): Vector2D {
        const pos = this.scrollView.GetScrollOffset();
        const validPos = this.getContainerVaildPos(this.isVertical?0:pos, this.isVertical?pos:0);
        return new Vector2D(Math.abs(validPos.X), Math.abs(validPos.Y));
    }
    
    private getContainerVaildPos(curX: number, curY: number): Vector2D {
        const maxCanMoveX = Math.max(this.mContainerSize.WidthOverride - this.viewPortWidth, 0);
        const maxCanMoveY = Math.max(this.mContainerSize.HeightOverride - this.viewPortHeight, 0);
        
        let newX = curX;
        let newY = curY;
        
        switch (this.marrangeType) {
            case GridItemArrangeType.TopLeftToBottomRight:
                newX = Math.max(Math.min(newX, 0), -maxCanMoveX);
                newY = Math.min(Math.max(newY, 0), maxCanMoveY);
                break;
            case GridItemArrangeType.BottomLeftToTopRight:
                newX = Math.max(Math.min(newX, 0), -maxCanMoveX);
                newY = Math.max(Math.min(newY, 0), -maxCanMoveY);
                break;
            case GridItemArrangeType.BottomRightToTopLeft:
                newX = Math.min(Math.max(newX, 0), maxCanMoveX);
                newY = Math.max(Math.min(newY, 0), -maxCanMoveY);
                break;
            case GridItemArrangeType.TopRightToBottomLeft:
                newX = Math.min(Math.max(newX, 0), maxCanMoveX);
                newY = Math.min(Math.max(newY, 0), maxCanMoveY);
                break;
        }
        
        return new Vector2D(newX, newY);
    }
    
    private updateCurFrameItemRangeData(): void {
        const distVector2 = this.getContainerMovedDistance();
       
        if (this.mneedCheckContentPosLeftCount <= 0 && 
            distVector2.Equals(this.mcurFrameItemRangeData.mCheckedPosition)) {
            return;
        }
        
        if (this.mneedCheckContentPosLeftCount > 0) {
            this.mneedCheckContentPosLeftCount--;
        }
        
        let distX = distVector2.X - this.mitemRecycleDistance.X;
        let distY = distVector2.Y - this.mitemRecycleDistance.Y;
        
        distX = Math.max(distX, 0);
        distY = Math.max(distY, 0);
        
        let val = this.getCeilItemRowColumnAtGivenAbsPos(distX, distY);
        this.mcurFrameItemRangeData.mMinColumn = val.mColumn;
        this.mcurFrameItemRangeData.mMinRow = val.mRow;
        
        distX = distVector2.X + this.mitemRecycleDistance.X + this.viewPortWidth;
        distY = distVector2.Y + this.mitemRecycleDistance.Y + this.viewPortHeight;
        
        val = this.getCeilItemRowColumnAtGivenAbsPos(distX, distY);
        this.mcurFrameItemRangeData.mMaxColumn = val.mColumn;
        this.mcurFrameItemRangeData.mMaxRow = val.mRow;
        this.mcurFrameItemRangeData.mCheckedPosition = distVector2;
        // Log.error(this.mcurFrameItemRangeData.mMaxColumn+"-"+this.mcurFrameItemRangeData.mMaxRow);
    }
    
    private updateRowItemGroupForRecycleAndNew(group: GridItemGroup): void {
        const minColumn = this.mcurFrameItemRangeData.mMinColumn;
        const maxColumn = this.mcurFrameItemRangeData.mMaxColumn;
        // if(minColumn!=0 && maxColumn!=0)
        // Log.error(minColumn+" "+ maxColumn);
        const row = group.groupIndex;
        
        // 回收超出范围的项
        while (group.first && group.first.column < minColumn) {
            this.recycleItemTmp(group.removeFirst());
        }
        
        while (group.last && 
              (group.last.column > maxColumn || group.last.itemIndex >= this.itemTotalCount)) {
            this.recycleItemTmp(group.removeLast());
        }
        
        // 添加新项
        if (!group.first) {
            const item = this.getNewItemByRowColumn(row, minColumn);
            if (item) {
                const itemPos = this.getItemPos(item.row, item.column,item.canvasPanelSlot.GetPosition());
                item.canvasPanelSlot.SetPosition(itemPos);
                group.addFirst(item);
            }
        }
        
        // 在左侧添加缺失的项
        while (group.first && group.first.column > minColumn) {
            const newColumn = group.first.column - 1;
            const item = this.getNewItemByRowColumn(row, newColumn);
            if (!item) break;
            const itemPos = this.getItemPos(item.row, item.column,item.canvasPanelSlot.GetPosition());
                item.canvasPanelSlot.SetPosition(itemPos);
            group.addFirst(item);
        }
        
        // 在右侧添加缺失的项
        while (group.last && group.last.column < maxColumn) {
            const newColumn = group.last.column + 1;
            const item = this.getNewItemByRowColumn(row, newColumn);
            if (!item) break;
            const itemPos = this.getItemPos(item.row, item.column,item.canvasPanelSlot.GetPosition());
            item.canvasPanelSlot.SetPosition(itemPos);
            group.addLast(item);
        }
    }
    
    private updateColumnItemGroupForRecycleAndNew(group: GridItemGroup): void {
        const minRow = this.mcurFrameItemRangeData.mMinRow;
        const maxRow = this.mcurFrameItemRangeData.mMaxRow;
        const column = group.groupIndex;
        
        // 回收超出范围的项
        while (group.first && group.first.row < minRow) {
            this.recycleItemTmp(group.removeFirst());
        }
        
        while (group.last && 
              (group.last.row > maxRow || group.last.itemIndex >= this.itemTotalCount)) {
            this.recycleItemTmp(group.removeLast());
        }
        
        // 添加新项
        if (!group.first) {
            const item = this.getNewItemByRowColumn(minRow, column);
            if (item) {
                const itemPos = this.getItemPos(item.row, item.column,item.canvasPanelSlot.GetPosition());
                item.canvasPanelSlot.SetPosition(itemPos);
                group.addFirst(item);
            }
        }
        
        // 在上方添加缺失的项
        while (group.first && group.first.row > minRow) {
            const newRow = group.first.row - 1;
            const item = this.getNewItemByRowColumn(newRow, column);
            if (!item) break;
            const itemPos = this.getItemPos(item.row, item.column,item.canvasPanelSlot.GetPosition());
            item.canvasPanelSlot.SetPosition(itemPos);
            group.addFirst(item);
        }
        
        // 在下方添加缺失的项
        while (group.last && group.last.row < maxRow) {
            const newRow = group.last.row + 1;
            const item = this.getNewItemByRowColumn(newRow, column);
            if (!item) break;
            const itemPos = this.getItemPos(item.row, item.column,item.canvasPanelSlot.GetPosition());
            item.canvasPanelSlot.SetPosition(itemPos);
            group.addLast(item);
        }
    }
    
    
  
    private updateFromSettingParam(param: LoopGridViewSettingParam): void {
        if (!param) return;
        
        if (param.mItemSize) {
            this.mitemSize.Set(param.mItemSize[0],param.mItemSize[1]);
        }
        
        if (param.mItemPadding) {
            this.mitemPadding.Set(param.mItemPadding[0],param.mItemPadding[1]);
        }
        
        if (param.mPadding) {
            this.mpadding = param.mPadding;
        }
        
        this.mfixedRowOrColumnCount = param.mFixedRowOrColumnCount;
    }
    
   
    
    private getShownGroup(groupIndex: number): GridItemGroup | null {
        if (groupIndex < 0) return null;
        if (this.mitemGroupList.length === 0) return null;
        
        const firstIndex = this.mitemGroupList[0].groupIndex;
        const lastIndex = this.mitemGroupList[this.mitemGroupList.length - 1].groupIndex;
        
        if (groupIndex < firstIndex || groupIndex > lastIndex) {
            return null;
        }
        
        return this.mitemGroupList[groupIndex - firstIndex];
    }
    
   
    private getOneItemGroupObj(): GridItemGroup {
        if (this.mitemGroupObjPool.length > 0) {
            return this.mitemGroupObjPool.pop();
        }
        return new GridItemGroup();
    }
    
    private recycleOneItemGroupObj(obj: GridItemGroup): void {
        this.mitemGroupObjPool.push(obj);
    }
    
}