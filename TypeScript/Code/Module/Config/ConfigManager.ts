import { IManager } from "../../../Mono/Core/Manager/IManager";
import { JsonHelper } from "../../../Mono/Helper/JsonHelper";
import * as string from "../../../Mono/Helper/StringHelper"

export class ConfigManager implements IManager{

    private static _instance: ConfigManager;

    public static get instance(): ConfigManager {
        return ConfigManager._instance;
    }

    public init() {
        ConfigManager._instance = this;
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

}