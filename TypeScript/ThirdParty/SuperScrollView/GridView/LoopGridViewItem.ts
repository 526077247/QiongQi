
import { CanvasPanelSlot, Widget } from 'ue';
import { LoopGridView } from './LoopGridView';

export class LoopGridViewItem {
    // 项目索引 (0 到 itemTotalCount-1)
    private mItemIndex: number = -1;
    
    // 行索引
    private mRow: number = -1;
    
    // 列索引
    private mColumn: number = -1;
    
    // 项目唯一ID
    private mItemId: number = -1;
    
    // 父级网格视图
    private mParentGridView: LoopGridView | null = null;
    
    // 初始化处理程序是否已调用
    private mIsInitHandlerCalled: boolean = false;
    
    // 项目预制体名称
    private mItemPrefabName: string = '';
    

    private mWidget: Widget;
    private mCanvasPanelSlot: CanvasPanelSlot;
    
    // 用户自定义数据
    private mUserObjectData: any = null;
   
    // 相邻项目引用
    private mPrevItem: LoopGridViewItem | null = null;
    private mNextItem: LoopGridViewItem | null = null;

    // 用户自定义数据访问器
    public get userObjectData(): any {
        return this.mUserObjectData;
    }
    public set userObjectData(value: any) {
        this.mUserObjectData = value;
    }
    
    // 项目预制体名称访问器
    public get itemPrefabName(): string {
        return this.mItemPrefabName;
    }
    public set itemPrefabName(value: string) {
        this.mItemPrefabName = value;
    }

    // 行列索引访问器
    public get row(): number {
        return this.mRow;
    }
    public set row(value: number) {
        this.mRow = value;
    }
    
    public get column(): number {
        return this.mColumn;
    }
    public set column(value: number) {
        this.mColumn = value;
    }

    // 项目索引访问器
    public get itemIndex(): number {
        return this.mItemIndex;
    }
    public set itemIndex(value: number) {
        this.mItemIndex = value;
    }

    // 项目ID访问器
    public get itemId(): number {
        return this.mItemId;
    }
    public set itemId(value: number) {
        this.mItemId = value;
    }

    // 初始化处理程序状态访问器
    public get isInitHandlerCalled(): boolean {
        return this.mIsInitHandlerCalled;
    }
    public set isInitHandlerCalled(value: boolean) {
        this.mIsInitHandlerCalled = value;
    }

    // 父级网格视图访问器
    public get parentGridView(): LoopGridView | null {
        return this.mParentGridView;
    }
    public set parentGridView(value: LoopGridView | null) {
        this.mParentGridView = value;
    }

    // 相邻项目访问器
    public get prevItem(): LoopGridViewItem | null {
        return this.mPrevItem;
    }
    public set prevItem(value: LoopGridViewItem | null) {
        this.mPrevItem = value;
    }
    
    public get nextItem(): LoopGridViewItem | null {
        return this.mNextItem;
    }
    public set nextItem(value: LoopGridViewItem | null) {
        this.mNextItem = value;
    }

    // 获取节点尺寸信息
    get canvasPanelSlot(): CanvasPanelSlot { return this.mCanvasPanelSlot; }
    set canvasPanelSlot(value: CanvasPanelSlot) { this.mCanvasPanelSlot = value; }

    get widget(): Widget { return this.mWidget; }
    set widget(value: Widget) { this.mWidget = value; }
}