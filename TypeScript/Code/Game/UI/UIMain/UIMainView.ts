import { CanvasPanel, ScrollBox, SizeBox, UeBridgeHelper, WidgetLayoutLibrary } from "ue";
import { Define } from "../../../../Mono/Define";
import { Log } from "../../../../Mono/Module/Log/Log";
import { IUpdate } from "../../../../Mono/Module/Update/IUpdate";
import { LoopGridView, LoopGridViewSettingParam } from "../../../../ThirdParty/SuperScrollView/GridView/LoopGridView";
import { LoopGridViewItem } from "../../../../ThirdParty/SuperScrollView/GridView/LoopGridViewItem";
import { LoopListView2 } from "../../../../ThirdParty/SuperScrollView/ListView/LoopListView2";
import { LoopListViewItem2 } from "../../../../ThirdParty/SuperScrollView/ListView/LoopListViewItem2";
import { IOnCreate } from "../../../Module/UI/IOnCreate";
import { IOnEnable } from "../../../Module/UI/IOnEnable";
// import { IOnWidthPaddingChange } from "../../../Module/UI/IOnWidthPaddingChange";
import { UIBaseView } from "../../../Module/UI/UIBaseView";
import { UIEmptyView } from "../../../Module/UIComponent/UIEmptyView";
import { UIImage } from "../../../Module/UIComponent/UIImage";
import { UILoopGridView } from "../../../Module/UIComponent/UILoopGridView";
import { UILoopListView2 } from "../../../Module/UIComponent/UILoopListView2";
import { UIText } from "../../../Module/UIComponent/UIText";
import { MenuPara, UIMenu } from "../UICommon/UIMenu";
import { CellItem } from "./CellItem";
import { DateItem } from "./DateItem";


export class UIMainView extends UIBaseView implements IOnCreate, IOnEnable, IUpdate{

    public static readonly PrefabPath:string = "/Game/AssetsPackage/UI/UIMain/Prefabs/UIMainView.UIMainView_C";

    public getConstructor()
    {
        return UIMainView;
    }

    get isOnWidthPaddingChange(){
        return true;
    }

    public image: UIImage;
	public text: UIText;
    public menu: UIMenu

    public loopGridView: UILoopGridView;
    public loopListView2: UILoopListView2;
    public welcome: UIEmptyView;

    public curId: number;

    public firstDay: Date;
	public totalDay: number;

    private config: Map<number,string> =new Map<number,string>([
        [1, "欢迎"],
        [2, "网格列表"],
        [3, "无限循环滚动列表"],
    ]);

    public onCreate()
    {
        this.image = this.addComponent<UIImage>(UIImage,"Image");
		this.text = this.addComponent<UIText>(UIText,"Text");
		this.menu = this.addComponent<UIMenu,string>(UIMenu,"UIMenu","UIMenuItem");
        this.loopGridView = this.addComponent<UILoopGridView>(UILoopGridView,"ScrollList/LoopGrid");
        this.loopGridView.initGridView(0,this.onGetGridItemByIndex.bind(this), 7);
        this.loopGridView.addItemPrefabConfData("GridSizeBox/GridContent/EmptyItem");
        this.loopGridView.addItemPrefabConfData("GridSizeBox/GridContent/CellItem");
       
        this.loopListView2 = this.addComponent<UILoopListView2>(UILoopListView2,"ScrollList/LoopList");
        this.loopListView2.initListView(this.onGetListItemByIndex.bind(this));
        this.loopListView2.addItemPrefabConfData("SizeBox/Content/DateItem");
        this.welcome = this.addComponent<UIEmptyView>(UIEmptyView,"ScrollList/Welcome");

        //模拟读配置
        const paras: MenuPara[] = [];
        for (const [id, name] of this.config) {
            const menuPara = new MenuPara();
            menuPara.id = id;
            menuPara.name = name;
            paras[paras.length] = menuPara;
        }
        
        this.menu.setData(paras, this.onMenuIndexChanged.bind(this));
    }

    public update() 
    {
        // var box = this.loopListView2.getWidget() as ScrollBox;
        // var content = ((box.GetChildAt(0) as SizeBox).GetChildAt(0) as CanvasPanel)
        // for (let index = 0; index < content.GetChildrenCount(); index++) {
        //     const element = content.GetChildAt(index);
        //     Log.info(element.GetName() + " " + WidgetLayoutLibrary.SlotAsCanvasSlot(element).GetPosition().X+" " + WidgetLayoutLibrary.SlotAsCanvasSlot(element).GetPosition().Y)
           
        // }
        // Log.info(Define.DeltaTime)
    }


    public onEnable()
    {
        this.menu.setActiveIndex(0,true);
    }

    public onMenuIndexChanged(para: MenuPara){
        this.curId = para.id;
		this.refreshItemSpaceShow();
    }

    public onGetGridItemByIndex(gridView: LoopGridView, index: number, row: number, column: number): LoopGridViewItem
    {
        if (index < 0 || index >= this.totalDay) return null;
        let item:LoopGridViewItem;
        if (index < this.firstDay.getDay())
        {
            item = gridView.newListViewItem("EmptyItem");
            if (!item.isInitHandlerCalled)
            {
                item.isInitHandlerCalled = true;
            }
        }
        else
        {
            item = gridView.newListViewItem("CellItem");
            let cellItem: CellItem;
            if (!item.isInitHandlerCalled)
            {
                item.isInitHandlerCalled = true;
                cellItem = this.loopGridView.addItemViewComponent<CellItem>(CellItem,item);
            }
            else
            {
                cellItem = this.loopGridView.getUIItemView<CellItem>(CellItem,item);
            }
            var date = new Date(this.firstDay.getFullYear() , this.firstDay.getMonth(), index - this.firstDay.getDay() + 1); 
            cellItem.setData(date);
        }
        
        return item;
    }

    public onGetListItemByIndex(listView: LoopListView2, index: number): LoopListViewItem2
    {
        const item: LoopListViewItem2 = listView.newListViewItem("DateItem");
        let dateItem: DateItem;
        if (!item.isInitHandlerCalled)
        {
            item.isInitHandlerCalled = true;
            dateItem = this.loopListView2.addItemViewComponent<DateItem>(DateItem,item);
        }
        else
        {
            dateItem = this.loopListView2.getUIItemView<DateItem>(DateItem,item);
        }
        dateItem.setData(index);
        return item;
    }

    public refreshItemSpaceShow()
    {
        var conf = this.config.get(this.curId);
        this.text.setText(conf);
        switch (this.curId)
        {
            case 1:
                this.welcome.setActive(true);
                this.loopGridView.setActive(false);
                this.loopListView2.setActive(false);
                break;
            case 2:
                this.welcome.setActive(false);
                this.loopGridView.setActive(true);
                this.loopListView2.setActive(false);
                const dtNow: Date = new Date();     
                this.firstDay = new Date(dtNow.getFullYear(), dtNow.getMonth(), 1); 
                const days = this.getDaysInCurrentMonth(dtNow.getFullYear(), dtNow.getMonth());
                this.totalDay = days + this.firstDay.getDay();
                this.loopGridView.setListItemCount(this.totalDay);
                this.loopGridView.refreshAllShownItem();
                break;
            case 3:
                this.welcome.setActive(false);
                this.loopGridView.setActive(false);
                this.loopListView2.setActive(true);
                this.loopListView2.setListItemCount(-1);
                this.loopListView2.refreshAllShownItem();
                break;
            default:
                this.welcome.setActive(false);
                this.loopGridView.setActive(false);
                this.loopListView2.setActive(false);
                break;
        }
    }

    private getDaysInCurrentMonth(year:number,month:number) {
        
        // 创建下个月的第一天
        const nextMonth: any = new Date(year, month + 1, 1);
       
        // 下个月第一天减去一天，得到当前月份的最后一天
        const lastDayOfCurrentMonth = new Date(nextMonth - 1);
       
        // 获取日期部分，即当前月份的天数
        return lastDayOfCurrentMonth.getDate();
      }
      
}