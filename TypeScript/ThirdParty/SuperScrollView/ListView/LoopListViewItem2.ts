import { CanvasPanelSlot, Widget } from "ue";
import { ListItemArrangeType } from "../../../ThirdParty/SuperScrollView/Common/CommonDefine";
import { LoopListView2 } from "./LoopListView2";

export class LoopListViewItem2
{
    // 成员变量
    private mItemIndex: number = -1;
    private mItemId: number = -1;
    private mParentListView: LoopListView2 = null;
    private mIsInitHandlerCalled: boolean = false;
    private mItemPrefabName: string = "";

    private mPadding: number = 0;

    private mItemCreatedCheckFrameCount: number = 0;
    private mStartPosOffset: number = 0;

    private mWidget: Widget;
    private mCanvasPanelSlot: CanvasPanelSlot;
    
    // 用户自定义数据
    private mUserObjectData: any = null;

    // 公共属性访问器
    get userObjectData(): any { return this.mUserObjectData; }
    set userObjectData(value: any) { this.mUserObjectData = value; }

    get startPosOffset(): number { return this.mStartPosOffset; }
    set startPosOffset(value: number) { this.mStartPosOffset = value; }

    get itemCreatedCheckFrameCount(): number { return this.mItemCreatedCheckFrameCount; }
    set itemCreatedCheckFrameCount(value: number) { this.mItemCreatedCheckFrameCount = value; }

    get padding(): number { return this.mPadding; }
    set padding(value: number) { this.mPadding = value; }


    get itemPrefabName(): string { return this.mItemPrefabName; }
    set itemPrefabName(value: string) { this.mItemPrefabName = value; }

    get itemIndex(): number { return this.mItemIndex; }
    set itemIndex(value: number) { this.mItemIndex = value; }

    get itemId(): number { return this.mItemId; }
    set itemId(value: number) { this.mItemId = value; }

    get isInitHandlerCalled(): boolean { return this.mIsInitHandlerCalled; }
    set isInitHandlerCalled(value: boolean) { this.mIsInitHandlerCalled = value; }

    get parentListView(): LoopListView2 { return this.mParentListView; }
    set parentListView(value: LoopListView2) { this.mParentListView = value; }

    // 获取节点尺寸信息
    get canvasPanelSlot(): CanvasPanelSlot { return this.mCanvasPanelSlot; }
    set canvasPanelSlot(value: CanvasPanelSlot) { this.mCanvasPanelSlot = value; }

    get widget(): Widget { return this.mWidget; }
    set widget(value: Widget) { this.mWidget = value; }

    // 位置计算属性
    get topY(): number {
        if (!this.parentListView) return 0;
        const pos = this.canvasPanelSlot.GetPosition();
        
        if (this.parentListView.arrangeType === ListItemArrangeType.TopToBottom) {
            return pos.Y;
        } 
        // else if (this.parentListView.arrangeType === ListItemArrangeType.BottomToTop) {
        //     return pos.Y + this.canvasPanelSlot.GetSize().Y;
        // }
        return 0;
    }

    get bottomY(): number {
        if (!this.parentListView) return 0;
        const pos = this.canvasPanelSlot.GetPosition();
        
        if (this.parentListView.arrangeType === ListItemArrangeType.TopToBottom) {
            return pos.Y - this.canvasPanelSlot.GetSize().Y;
        } 
        // else if (this.parentListView.arrangeType === ListItemArrangeType.BottomToTop) {
        //     return pos.Y;
        // }
        return 0;
    }

    get leftX(): number {
        if (!this.parentListView) return 0;
        const pos = this.canvasPanelSlot.GetPosition();
        
        if (this.parentListView.arrangeType === ListItemArrangeType.LeftToRight) {
            return pos.X;
        } 
        // else if (this.parentListView.arrangeType === ListItemArrangeType.RightToLeft) {
        //     return pos.X - this.canvasPanelSlot.GetSize().X;
        // }
        return 0;
    }

    get rightX(): number {
        if (!this.parentListView) return 0;
        const pos = this.canvasPanelSlot.GetPosition();
        
        if (this.parentListView.arrangeType === ListItemArrangeType.LeftToRight) {
            return pos.X + this.canvasPanelSlot.GetSize().X;
        } 
        // else if (this.parentListView.arrangeType === ListItemArrangeType.RightToLeft) {
        //     return pos.X;
        // }
        return 0;
    }

    get itemSize(): number {
        if (!this.parentListView) return 0;
        return this.parentListView.isVertList ? 
            this.canvasPanelSlot.GetSize().Y : 
            this.canvasPanelSlot.GetSize().X;
    }

    get itemSizeWithPadding(): number {
        return this.itemSize + this.mPadding;
    }

    /**
     * 获取Content下相对坐标
     * @param corners 
     * @returns 
     */
    public getLocalCorners(corners: number[]): void {
        if (!this.canvasPanelSlot) return;
        const alignment = this.canvasPanelSlot.GetAlignment();

        const worldPos = this.canvasPanelSlot.GetPosition();
        const width = this.canvasPanelSlot.GetSize().X;
        const height = this.canvasPanelSlot.GetSize().Y;
        
        corners[0] = worldPos.Y + height * (1-alignment.Y); // 下
        corners[1] = worldPos.Y - height * alignment.Y; // 上
        corners[2] = worldPos.X - width * alignment.X, // 左
        corners[3] = worldPos.X + width * (1-alignment.X); // 右

    }

    /**
     * 获取Scrollbox下相对坐标
     * @param corners 
     * @returns 
     */
    public getViewCorners(corners: number[]): void {
        if (!this.canvasPanelSlot) return;
        const alignment = this.canvasPanelSlot.GetAlignment();

        const worldPos = this.canvasPanelSlot.GetPosition();
        const width = this.canvasPanelSlot.GetSize().X;
        const height = this.canvasPanelSlot.GetSize().Y;
        corners[0] = worldPos.Y + height * (1-alignment.Y); // 下
        corners[1] = worldPos.Y - height * alignment.Y; // 上
        corners[2] = worldPos.X - width * alignment.X, // 左
        corners[3] = worldPos.X + width * (1-alignment.X); // 右
        
        const offset = this.parentListView.scrollRect.GetScrollOffset();
        if(this.parentListView.isVertList){
            corners[0] += this.parentListView.arrangeType == ListItemArrangeType.TopToBottom?-offset:offset;
            corners[1] += this.parentListView.arrangeType == ListItemArrangeType.TopToBottom?-offset:offset;
        }else{
            corners[2] += this.parentListView.arrangeType == ListItemArrangeType.LeftToRight?-offset:offset;
            corners[3] += this.parentListView.arrangeType == ListItemArrangeType.LeftToRight?-offset:offset;
        }
    }
}