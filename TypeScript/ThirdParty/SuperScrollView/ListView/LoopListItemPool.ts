import { CanvasPanel, Class, ESlateVisibility, PanelWidget, Vector2D, Widget, WidgetBlueprintLibrary } from "ue";
import { LoopListViewItem2 } from "./LoopListViewItem2";

export class ItemPool {
    private mPrefabObj: Class = null;
    private mPrefabName: string = "";
    private mInitCreateCount: number = 1;
    private mPadding: number = 0;
    private mStartPosOffset: number = 0;
    private mTmpPooledItemList: LoopListViewItem2[] = [];
    private mPooledItemList: LoopListViewItem2[] = [];
    private static mCurItemIdCount: number = 0;
    private mItemParent: CanvasPanel | null = null;
    public SizeX;
    public SizeY;

    public init(prefabObj: Class, mPrefabName: string, padding: number, startPosOffset: number, 
                createCount: number, parent: CanvasPanel): void {
        this.mPrefabObj = prefabObj;
        this.mPrefabName = mPrefabName;
        this.mInitCreateCount = createCount;
        this.mPadding = padding;
        this.mStartPosOffset = startPosOffset;
        this.mItemParent = parent;
        this.SizeX = null;
        this.SizeY = null;
        
        for (let i = 0; i < this.mInitCreateCount; ++i) {
            const item = this.createItem();
            this.recycleItemReal(item);
        }
    }

    public clearPool(): void {
        ItemPool.mCurItemIdCount = 0;
        this.destroyAllItem();
    }

    public cleanUp(beforeDestroy?: (node: Widget) => void): void {
        const count = this.mPooledItemList.length;
        for (let i = 0; i < count; ++i) {
            const item = this.mPooledItemList[i];
            if (beforeDestroy) beforeDestroy(item.widget);
            item.widget.RemoveFromParent();
        }
        this.mPooledItemList = [];
    }

    public getItem(index?: number): LoopListViewItem2 | null {
        if (!this.mPrefabObj) return null;
        
        ItemPool.mCurItemIdCount++;
        let item: LoopListViewItem2 | null = null;

        // 尝试从临时池中查找
        if (this.mTmpPooledItemList.length > 0) {
            if (index !== undefined) {
                for (let i = 0; i < this.mTmpPooledItemList.length; i++) {
                    if (this.mTmpPooledItemList[i].itemIndex === index) {
                        item = this.mTmpPooledItemList[i];
                        this.mTmpPooledItemList.splice(i, 1);
                        item.widget.SetVisibility(ESlateVisibility.Visible);
                        break;
                    }
                }
            }

            if (!item) {
                const lastIndex = this.mTmpPooledItemList.length - 1;
                item = this.mTmpPooledItemList[lastIndex];
                this.mTmpPooledItemList.splice(lastIndex, 1);
                item.widget.SetVisibility(ESlateVisibility.Visible);
            }
        } 
        // 从主池中获取
        else {
            if (this.mPooledItemList.length === 0) {
                item = this.createItem();
            } else {
                const lastIndex = this.mPooledItemList.length - 1;
                item = this.mPooledItemList[lastIndex];
                this.mPooledItemList.splice(lastIndex, 1);
                item.widget.SetVisibility(ESlateVisibility.Visible);
            }
        }

        if (item) {
            item.padding = this.mPadding;
            item.itemId = ItemPool.mCurItemIdCount;
        }
        return item;
    }

    public destroyAllItem(): void {
        this.clearTmpRecycledItem();
        const count = this.mPooledItemList.length;
        for (let i = 0; i < count; ++i) {
            this.mPooledItemList[i].widget.RemoveFromParent();
        }
        this.mPooledItemList = [];
    }

    private createItem(): LoopListViewItem2 {
        if (!this.mPrefabObj || !this.mItemParent) {
            throw new Error("ItemPool not initialized properly");
        }
        const newNode = WidgetBlueprintLibrary.Create(this.mItemParent.GetWorld(), this.mPrefabObj, null);
        const canvasSlots = this.mItemParent.AddChildToCanvas(newNode);
        canvasSlots.SetPosition(Vector2D.ZeroVector);
        newNode.SetRenderScale(Vector2D.ZeroVector);
        newNode.SetRenderTransformAngle(0);
        

        const itemComp = new LoopListViewItem2();
        if (!itemComp) {
            throw new Error("LoopListViewItem2 component missing on prefab");
        }
        
        itemComp.itemPrefabName = this.mPrefabName;
        itemComp.startPosOffset = this.mStartPosOffset;
        itemComp.canvasPanelSlot = canvasSlots;
        itemComp.widget = newNode;
        newNode.SetVisibility(ESlateVisibility.Visible);
        
        return itemComp;
    }

    private recycleItemReal(item: LoopListViewItem2): void {
        item.widget.SetVisibility(ESlateVisibility.Collapsed);
        this.mPooledItemList.push(item);
    }

    public recycleItem(item: LoopListViewItem2): void {
        this.mTmpPooledItemList.push(item);
    }

    public clearTmpRecycledItem(): void {
        const count = this.mTmpPooledItemList.length;
        if (count === 0) return;

        for (let i = 0; i < count; ++i) {
            this.recycleItemReal(this.mTmpPooledItemList[i]);
        }
        this.mTmpPooledItemList = [];
    }
}