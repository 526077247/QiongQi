import { IManager } from "../../../Mono/Core/Manager/IManager";
import { JsonHelper } from "../../../Mono/Helper/JsonHelper";
import * as string from "../../../Mono/Helper/StringHelper"
import { register } from "../Generate/Config/ConfigManager.register"
import { Log } from "../../../Mono/Module/Log/Log";

export class ConfigManager implements IManager{

    private static _instance: ConfigManager;

    public static get instance(): ConfigManager {
        return ConfigManager._instance;
    }

    public init() {
        ConfigManager._instance = this;
        register();
    }

    public destroy() {
        ConfigManager._instance = null;
    }

    public async loadOneConfig<T>(type: new (...args:any[]) => T, name: string = "")
    {
        if (string.isNullOrEmpty(name))
            name = type.name;
        // @ts-ignore
        const jObj = require(`../Generate/Data/${name}.Data`);
        const category = JsonHelper.deserialize(type, jObj);
        category.endInit()

        return category as T;
    }

    public loadOneInThread<T>(type: new (...args:any[]) => T, name: string = "", jObj: any){
        if (string.isNullOrEmpty(name))
            name = type.name;
        const category = JsonHelper.deserialize(type, jObj);
        category.endInit();
    }
}