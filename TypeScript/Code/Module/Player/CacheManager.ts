import { IManager } from "../../../Mono/Core/Manager/IManager"
import { JsonHelper } from "../../../Mono/Helper/JsonHelper";
import * as UE from 'ue'

export class CacheManager implements IManager {

    private static _instance: CacheManager;

    public static get instance(): CacheManager {
        return CacheManager._instance;
    }

    private cacheObj: Map<string, any>

    public init() {
        this.cacheObj = new Map<string, any>();
        UE.QiongQiPlayerPrefs.LoadSaveGame();
        CacheManager._instance = this;
    }
    public destroy() {
        CacheManager._instance = null;
    }

    public getString(key: string, defaultValue: string = null): string
    {
        return UE.QiongQiPlayerPrefs.GetString(key,defaultValue);
    }
    
    public getInt(key: string, defaultValue: number = 0): number
    {
        return UE.QiongQiPlayerPrefs.GetInt(key,defaultValue);
    }
    
    public getValue<T extends object>(type: new (...args:any[]) => T,key: string): T
    {
        let data:any = this.cacheObj.get(key);
        if (!!data)
        {
            return data as T;
        }
        var jStr = UE.QiongQiPlayerPrefs.GetString(key, null);
        if (jStr == null) return null;
        var res = JsonHelper.fromJson<T>(type,jStr);
        this.cacheObj[key] = res;
        return res;
    }
    
    public setString(key: string, value: string)
    {
        UE.QiongQiPlayerPrefs.SetString(key, value);
    }
    
    public setInt(key: string, value: number)
    {
        UE.QiongQiPlayerPrefs.SetInt(key, value);
    }
    
    public setValue<T extends object>(key: string, value: T)
    {
        this.cacheObj[key] = value;
        var jStr = JsonHelper.toJson(value);
        UE.QiongQiPlayerPrefs.SetString(key, jStr);
    }

    public deleteKey(key: string)
    {
        if (this.cacheObj.has(key))
        {
            this.cacheObj.delete(key);
        }
        UE.QiongQiPlayerPrefs.Delete(key);
    }
}