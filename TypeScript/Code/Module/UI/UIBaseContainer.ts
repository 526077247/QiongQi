import {UnOrderDoubleKeyDictionary} from "../../../Mono/Core/Object/UnOrderDoubleKeyDictionary"
import { Log } from "../../../Mono/Module/Log/Log";
import { PanelWidget, Widget } from "ue";
import { UIBaseComponent } from "./UIBaseComponent";
/**
 * 对应PanelWidget
 */
export abstract class UIBaseContainer extends UIBaseComponent {

    private components : UnOrderDoubleKeyDictionary<string, new (...args: any[]) => any, UIBaseComponent>; //[path]:[component_name:UIBaseComponent]
    private length: number = 0;

    public _afterOnEnable()
    {
        this.walk((component) =>
        {
            const componentAny = component as any;
            if (!!componentAny.onEnable) componentAny.onEnable();
            component.activeSelf = true;
            component._afterOnEnable();
        });
        super._afterOnEnable();
    }

    public _beforeOnDisable()
    {
        super._beforeOnDisable();
        this.walk((component) =>
        {
            component._beforeOnDisable();
            const componentAny = component as any;
            if (!!componentAny.onDisable) componentAny.onDisable();
        });
    }

    public beforeOnDestroy()
    {
        this._beforeOnDestroy();
        if (this.components != null)
        {
            var keys1 = [...this.components.keys()];
            for (let i = keys1.length - 1; i >= 0; i--)
            {
                const [res, map] = this.components.tryGetDic(keys1[i])
                if (res)
                {
                    var keys2 = [...map.keys()];
                    for (let j = keys2.length - 1; j >= 0; j--)
                    {
                        var component = map.get(keys2[j]);
                        component.beforeOnDestroy();
                        const componentAny = component as any;
                        // if (!!componentAny.onLanguageChange)
                        //     I18NManager.instance?.removeI18NEntity(componentAny);
                        if (!!componentAny.onDestroy) componentAny.onDestroy();
                    }
                }
            }
        }

        this.length--;
        if (this.length <= 0)
        {
            if (this.parent != null && !!this.path)
                this.parent._innerRemoveComponent(this, this.path);
            else
                Log.info("Close window here, type name: " + this.constructor.name);
        }
        else
            Log.error(this.getConstructor().name + "OnDestroy fail, length = "+ this.length);
    }


    /**
     * 遍历：注意，这里是无序的
     * @param callback 
     * @returns 
     */
    private walk(callback: (compent:UIBaseComponent) => void)
    {
        if (this.components == null) return;
        for (const [key, val] of this.components) 
        {
            if (!!val)
            {
                for (const [key2, val2] of val) {
                    callback(val2);
                }
            }
        }
    }

    /**
     * 记录Component
     * @param name 
     * @param componentClass 
     * @param component 
     * @returns 
     */
    private recordUIComponent(name: string, componentClass: new (...args: any[]) => any, component: UIBaseComponent)
    {
        if (this.components == null) this.components = new UnOrderDoubleKeyDictionary<string, new (...args: any[]) => any, UIBaseComponent>();
        if (this.components.containSubKey(name, componentClass))
        {
            Log.error("Already exist component_class : " + componentClass.name);
            return;
        }

        this.components.add(name, componentClass, component);
    }


    /**
     * 添加组件
     * @param type 类型
     * @param name 游戏物体名称
     * @returns 
     */
    public addComponentNotCreate<T extends UIBaseComponent>(type: new () => T, name: string): T
    {
        const componentInst: T = new type();
        componentInst.path = name;
        componentInst.parent = this;
        this.recordUIComponent(name, type, componentInst);
        this.length++;
        return componentInst;
    }

    /**
     * 添加组件
     * @param type 类型
     * @param name 游戏物体名称
     * @returns 
     */
    public addComponent<T extends UIBaseComponent, A = void, B = void, C = void>(type: new () => T, path: string = "", a?:A, b?:B, c?:C) : T
    {
        const componentInst: T = new type();
        componentInst.path = path;
        componentInst.parent = this;
        const componentAny = componentInst as any;
        if (!!componentAny.onCreate)
            componentAny.onCreate(a,b,c);
        // if (!!componentAny.onLanguageChange)
        //     I18NManager.instance?.registerI18NEntity(componentAny);
        this.recordUIComponent(path, type, componentInst);
        this.length++;
        return componentInst;
    }

    /**
     * 获取组件
     * @param type 
     * @param path 
     * @returns 
     */
    public getComponent<T extends UIBaseComponent>(type: (new () => T),path: string = ""): T
    {
        if (this.components == null) return null;
        const [res, component] = this.components.tryGetValue(path, type);
        if (res)
        {
            return component as T;
        }
        return null;
    }

    /**
     * 移除组件
     * @param type 
     * @param path 
     */
    public removeComponent<T extends UIBaseComponent>(type: new () => T,path: string = "")
    {
        var component = this.getComponent<T>(type, path);
        if (component != null)
        {
            const componentAny = component as any;
            component._beforeOnDisable();
            if(!!componentAny.onDisable) componentAny.onDisable();
            component.beforeOnDestroy();
            // if (!!componentAny.onLanguageChange)
            //     I18NManager.instance?.removeI18NEntity(componentAny);
            if(!!componentAny.onDestroy) componentAny.onDestroy();
            this.components.remove(path, type);
        }
    }

    /**
     * 移除所有组件
     * @param string 
     * @param path 
     */
    public removeAllComponent(path: string = ""){
        if (this.components == null) return;
        const [res, dic] = this.components.tryGetDic(path);
        if (res)
        {
            const list = [...dic.values()];
            for (const component of list) {
                if (component != null)
                {
                    const componentAny = component as any;
                    component._beforeOnDisable();
                    if(!!componentAny.onDisable) componentAny.onDisable();
                    component.beforeOnDestroy();
                    // if (!!componentAny.onLanguageChange)
                    //     I18NManager.instance?.removeI18NEntity(componentAny);
                    if(!!componentAny.onDestroy) componentAny.onDestroy();
                }
            }
        }
        this.components.remove(path);
    }

    public _innerRemoveComponent(component: UIBaseComponent, path: string)
    {
        if (component != null)
        {
            this.components.remove(path, component.getConstructor());
            this.length--;
        }
    }
}