import { CanvasPanel, Class, ESlateVisibility, NewObject, Vector2D, Widget, WidgetBlueprintLibrary } from 'ue';
import { LoopGridViewItem } from './LoopGridViewItem';

export class GridItemPool {
    private _prefab: Class = null;
    private _prefabName: string = "";
    private _initCreateCount: number = 1;
    private _tmpPooledItems: LoopGridViewItem[] = [];
    private _pooledItems: LoopGridViewItem[] = [];
    private static _curItemIdCount: number = 0;
    private _itemParent: CanvasPanel = null;

    constructor() {}

    public init(prefab: Class, createCount: number, parent: CanvasPanel): void {
        this._prefab = prefab;
        this._prefabName = prefab.GetName();
        this._initCreateCount = createCount;
        this._itemParent = parent;
        
        for (let i = 0; i < this._initCreateCount; ++i) {
            const viewItem = this.createItem();
            this.recycleItemReal(viewItem);
        }
    }

    public clearPool(): void {
        GridItemPool._curItemIdCount = 0;
        this.destroyAllItem();
    }

    public cleanUp(beforeDestroy?: (node: Widget) => void): void {
        this._pooledItems.forEach(item => {
            beforeDestroy?.(item.widget);
            item.widget.RemoveFromParent();
        });
        this._pooledItems.length = 0;
    }

    public getItem(index: number = null): LoopGridViewItem {
        GridItemPool._curItemIdCount++;
        let item: LoopGridViewItem = null;

        if (this._tmpPooledItems.length > 0) {
            if (index !== null) {
                const idx = this._tmpPooledItems.findIndex(i => i.itemIndex === index);
                if (idx !== -1) {
                    item = this._tmpPooledItems[idx];
                    this._tmpPooledItems.splice(idx, 1);
                    item.widget.SetVisibility(ESlateVisibility.Visible);
                }
            }

            if (!item) {
                item = this._tmpPooledItems.pop();
                item.widget.SetVisibility(ESlateVisibility.Visible);
            }
        } else {
            if (this._pooledItems.length === 0) {
                item = this.createItem();
            } else {
                item = this._pooledItems.pop();
                item.widget.SetVisibility(ESlateVisibility.Visible);
            }
        }

        item.itemId = GridItemPool._curItemIdCount;
        return item;
    }

    public destroyAllItem(): void {
        this.clearTmpRecycledItem();
        this._pooledItems.forEach(item => item.widget.RemoveFromParent());
        this._pooledItems.length = 0;
    }

    private createItem(): LoopGridViewItem {
        if (!this._prefab || !this._itemParent) {
            throw new Error("ItemPool not initialized properly");
        }
        let newNode: Widget = WidgetBlueprintLibrary.Create(this._itemParent.GetWorld(), this._prefab, null);
        if(newNode == null){
            newNode = NewObject(this._prefab, null) as Widget;
            if(newNode == null) throw new Error("ItemPool Create Fail " + this._prefab.GetName());
        }
        const canvasSlots = this._itemParent.AddChildToCanvas(newNode);
        
        canvasSlots.SetPosition(Vector2D.ZeroVector);
        newNode.SetRenderScale(Vector2D.ZeroVector);
        newNode.SetRenderTransformAngle(0);
        

        const itemComp = new LoopGridViewItem();
        if (!itemComp) {
            throw new Error("LoopListViewItem2 component missing on prefab");
        }
        
        itemComp.itemPrefabName = this._prefabName;
        itemComp.canvasPanelSlot = canvasSlots;
        itemComp.widget = newNode;
        newNode.SetVisibility(ESlateVisibility.Visible);
        
        return itemComp;
    }

    private recycleItemReal(item: LoopGridViewItem): void {
        item.widget.SetVisibility(ESlateVisibility.Collapsed);
        this._pooledItems.push(item);
    }

    public recycleItem(item: LoopGridViewItem): void {
        item.prevItem = null;
        item.nextItem = null;
        this._tmpPooledItems.push(item);
    }

    public clearTmpRecycledItem(): void {
        this._tmpPooledItems.forEach(item => this.recycleItemReal(item));
        this._tmpPooledItems.length = 0;
    }
}