
import { Log } from "../../../Mono/Module/Log/Log";
import { TimerManager } from "../../../Mono/Module/Timer/TimerManager";
import { TimerType } from "../../../Mono/Module/Timer/TimerType";
// import { ReferenceCollector } from "../../../Mono/Module/UI/ReferenceCollector";
import * as string from "../../../Mono/Helper/StringHelper"
import { ESlateVisibility, PanelWidget, UserWidget, Widget } from "ue";
import { UIBaseContainer } from "./UIBaseContainer";

/**
 * 对应非PanelWidget
 */
export abstract class UIBaseComponent {

    public abstract getConstructor(): new () => UIBaseComponent;

    public parent: UIBaseContainer;
    protected widget : Widget;
    protected parentWidget : Widget;
    public path: string;
    private timerId: bigint = 0n;
    private _activeSelf: boolean = false;

    public get activeSelf(): boolean{
        return this._activeSelf;
    }

    public set activeSelf(val: boolean){
        this._activeSelf = val;
    }

    public setWidget(widget: Widget)
    {
        this.widget = widget;
    }

    public getWidget(): Widget
    {
        this.activatingWidget();
        return this.widget;
    }


    protected activatingWidget(): Widget
    {
        if (this.widget == null)
        {
            const pWidget = this.getParentWidget();
            if(string.isNullOrEmpty(this.path))
            {
                this.widget = pWidget;
                if (this.widget != null)
                {
                    return this.widget;
                }
            }

            if (!!pWidget)
            {
                // var rc = pTrans.getComponent<ReferenceCollector>(ReferenceCollector);
                // if (rc != null)
                // {
                //     this.node = rc.get(Node, this.path);
                // }

                if (this.widget == null)
                {
                    let pRoot = pWidget;
                    const pathvs = this.path.split('/');
                    for (let i = 0; i < pathvs.length; i++) 
                    {
                        const name = pathvs[i];
                        const pw = pRoot as PanelWidget;
                        if(!!pw.GetAllChildren)
                        {
                            const children = pw.GetAllChildren();
                            pRoot = null;
                            for (let index = 0; index < children.Num(); index++) {
                                const element: Widget = children.Get(index);
                                if(element.GetName() == name){
                                    pRoot = element;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            const uw = pRoot as UserWidget;
                            pRoot = null;
                            if(!!uw.WidgetTree)
                            {
                                pRoot = uw.WidgetTree.RootWidget;
                                if(name != "Root")
                                {
                                    i--;
                                }
                            }
                        }
                        if(pRoot == null)
                        {
                            break;
                        }
                    }
                    
                    this.widget = pRoot;
                    // if(EDITOR)
                    // {
                    //     if (this.node != null && !string.isNullOrEmpty(this.path) && rc != null)
                    //     {
                    //         rc.add(this.path, this.node);
                    //     }
                    // }
                }
            }
            
            if (this.widget == null)
            {
                Log.error(this.parent.constructor.name + "路径错误:" + this.path);
            }
        }

        return this.widget;
    }

    protected getParentWidget(): Widget
    {
        if (this.parentWidget == null)
        {
            var pui = this.parent;
            if (pui == null)
            {
                Log.error("ParentTransform is null Path:" + this.path);
            }
            else
            {
                pui.activatingWidget();
                this.parentWidget = pui.widget as Widget;
            }
        }

        return this.parentWidget;
    }

    public _afterOnEnable()
    {
        const thisAny = this as any;
        if (!!thisAny.update)
        {
            if(TimerManager.instance.remove(this.timerId)){
                this.timerId = 0n;
            }
            this.timerId = TimerManager.instance.newFrameTimer(TimerType.ComponentUpdate, thisAny.update, this);
        }
    }

    public _beforeOnDisable()
    {
        const thisAny = this as any;
        if (!!thisAny.update)
        {
            if(TimerManager.instance.remove(this.timerId))
            {
                this.timerId = 0n;
            }
        }
    }

    public beforeOnDestroy()
    {
        const thisAny = this as any;
        if (!!thisAny.update)
        {
            if(TimerManager.instance.remove(this.timerId))
            {
                this.timerId = 0n;
            }
        }
        
        if (this.parent != null && !!this.path)
            this.parent._innerRemoveComponent(this, this.path);
        else
            Log.info("Close window here, type name: " + this.constructor.name);
    }


    protected innerSetActive(active: boolean)
    {
        this._activeSelf = active;
        if (this.getWidget() != null && this.widget.Visibility != (this._activeSelf ? ESlateVisibility.Visible:ESlateVisibility.Collapsed))
            this.widget.SetVisibility(this._activeSelf ? ESlateVisibility.Visible:ESlateVisibility.Collapsed);
    }

    public setActive<P1 = void, P2 = void, P3 = void, P4 = void>(active: boolean, p1?: P1, p2?:P2, p3?: P3, p4?: P4)
    {
        const thisAny = this as any;
        if (active)
        {
            if(!!thisAny.onEnable) thisAny.onEnable(p1,p2,p3,p4);
            this._afterOnEnable();
            this.innerSetActive(active);
        }
        else
        {
            this.innerSetActive(active);
            this._beforeOnDisable();
            if(!!thisAny.onDisable) thisAny.onDisable();
        }
    }

}