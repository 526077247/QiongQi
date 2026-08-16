import { IManager } from "../../../Mono/Core/Manager/IManager";
import { JsonHelper } from "../../../Mono/Helper/JsonHelper";
import { Log } from "../../../Mono/Module/Log/Log";
import * as string from "../../../Mono/Helper/StringHelper"
import { IConfigLoader } from "./IConfigLoader";
import { ConfigLoader } from "./ConfigLoader";

export class ConfigManager implements IManager{

    private static _instance: ConfigManager;

    public static get instance(): ConfigManager {
        return ConfigManager._instance;
    }

    /** 全量原始 JSON 数据缓存（name -> JSON.parse 后的原始对象，未反序列化） */
    private rawConfigBytes: Map<string, any> = new Map<string, any>();

    /** 反序列化后的强类型缓存（type -> category 实例） */
    private configCache: Map<any, object> = new Map<any, object>();

    private loader: IConfigLoader;

    public init() {
        ConfigManager._instance = this;
        this.loader = new ConfigLoader();
    }

    public destroy() {
        ConfigManager._instance = null;
        this.rawConfigBytes.clear();
        this.configCache.clear();
        this.loader = null;
    }

    /**
     * 全量加载 AssetsPackage/Config 下所有配置 JSON 到内存（只 JSON.parse 不反序列化）
     */
    public async loadAsync(): Promise<void> {
        await this.loader.loadAllAsync((name, data) => {
            this.rawConfigBytes.set(name, data);
        });
    }

    /**
     * 惰性反序列化：首次访问某张表时反序列化并缓存，二次访问直接返回缓存
     * @param type 配置类（如 ServerConfigCategory）
     * @param name 配置文件名（默认取 type.name，如 "ServerConfigCategory"）
     */
    public get<T>(type: new (...args:any[]) => T, name: string = ""): T {
        if (string.isNullOrEmpty(name))
            name = type.name;

        if (this.configCache.has(type)) {
            return this.configCache.get(type) as unknown as T;
        }

        const rawData = this.rawConfigBytes.get(name);
        if (rawData == null) {
            Log.error(`配置加载失败，请确认已导出并已调用 loadAsync：${name}`);
            return null;
        }

        const category = JsonHelper.deserialize(type, rawData);
        category.endInit();
        this.configCache.set(type, category);
        return category as T;
    }

    /**
     * 单表按需加载（默认不缓存，供 I18N 多语言同类型不同语言名使用）
     * @param type 配置类
     * @param name 配置文件名（不含扩展名，默认取 type.name）
     * @param cache 为 true 时走 get() 的缓存逻辑
     */
    public async loadOneConfig<T>(type: new (...args:any[]) => T, name: string = "", cache: boolean = false): Promise<T> {
        if (string.isNullOrEmpty(name))
            name = type.name;

        if (cache) {
            return this.get(type, name);
        }

        const rawData = await this.loader.loadSingleAsync(name);
        if (rawData == null) {
            Log.error(`配置加载失败，请确认已导出：${name}`);
            return null;
        }

        const category = JsonHelper.deserialize(type, rawData);
        category.endInit();
        return category as T;
    }

}
