import { IManager } from "../../../Mono/Core/Manager/IManager";
import { LruCache } from "../../../Mono/Core/Object/LruCache";
// import { ResourceManager } from "./ResourceManager";
import * as string from "../../../Mono/Helper/StringHelper"
import { Log } from "../../../Mono/Module/Log/Log";
import { PaperSprite, ResourceManager, Texture2D } from "ue";
import { HttpManager } from "../../../Mono/Module/Http/HttpManager";
import { TimerManager } from "../../../Mono/Module/Timer/TimerManager";
class SpriteValue
{
    public asset: PaperSprite;
    public texture: Texture2D;
    public refCount: number;
}

enum SpriteType
{
    Sprite = 0,
    SpriteAtlas = 1
}
const ATLAS_KEY: string = "/Atlas/";

/**
 * 图片加载系统，仅支持加载Sprite类型的图片或网络图片
 * Texture类型的通过ResourcesManager自己加载管理
 */
export class ImageLoaderManager implements IManager{
    private static _instance: ImageLoaderManager;

    public static get instance(): ImageLoaderManager{
        return ImageLoaderManager._instance;
    }

    private cacheSingleSprite: LruCache<string, SpriteValue>;
    private cacheOnlineImage: Map<string, SpriteValue> ;
    private pendingLoads: Map<string, Promise<SpriteValue>> = new Map();
    public init(){
        ImageLoaderManager._instance = this;
        this.cacheSingleSprite = new LruCache<string, SpriteValue>();
        this.cacheSingleSprite.setCheckCanPopCallback(( key: string,  value: SpriteValue) => { return value.refCount == 0; });
        this.cacheSingleSprite.setPopCallback((key, value) =>
        {
            ResourceManager.GetInstance().ReleaseResource(value.asset);
            value.asset = null;
            value.texture = null;
            value.refCount = 0;
        });
        this.cacheOnlineImage = new Map<string, SpriteValue>();
    }

    public destroy() {
        this.pendingLoads.clear();
        ImageLoaderManager._instance = null;
    }

    /**
     * 同步加载已缓存的图片（image 和button已经封装 外部使用时候 谨慎使用）
     * @param imagePath 
     */
    public loadSpriteSync(imagePath: string): PaperSprite {
        return this.loadSingleImageSyncInternal(imagePath)?.asset;
    }

    /**
     * 异步加载图片（image 和button已经封装 外部使用时候 谨慎使用）
     * @param imagePath 
     */
    public async loadSpriteAsync(imagePath: string): Promise<PaperSprite>{
        const res = this.loadSpriteSync(imagePath);
        if(res != null) return res;
        const assetType = this.getSpriteLoadInfoByPath(imagePath);
        const sv = await this.loadSingleImageAsyncInternal(imagePath, assetType);
        return sv?.asset;
    }

    /**
     * 异步加载图片 （image 和button已经封装 外部使用时候 谨慎使用）
     * @param imagePath 
     * @returns 
     */
    public async loadTextureAsync(imagePath: string): Promise<Texture2D>
    {
        const assetType = this.getSpriteLoadInfoByPath(imagePath);
        const sv = await this.loadSingleImageAsyncInternal(imagePath, assetType);
        if(sv?.texture == null){
            Log.error("不能加载图集中的图片");
        }
        return sv?.texture;
    }

    /**
     * 释放图片
     * @param imagePath 
     * @returns 
     */
    public releaseImage(imagePath: string)
    {
        if (string.isNullOrEmpty(imagePath)) return;
        const value = this.cacheSingleSprite.onlyGet(imagePath);
        if (!!value && value.refCount > 0)
        {
            value.refCount--;
        }
    }

    public cleanup()
    {
        Log.info("ImageLoaderManager Cleanup");
        this.cacheSingleSprite.cleanUp();
    }

    public clear()
    {
        for (const [key,value] of this.cacheSingleSprite) {
            ResourceManager.GetInstance()?.ReleaseResource(value.asset);
            value.asset = null;
            value.texture = null;
            value.refCount = 0;
        }
        
        this.cacheSingleSprite.clear();
        Log.info("ImageLoaderManager Clear");
    }

    private loadSingleImageSyncInternal(assetAddress: string): SpriteValue
    {
        const cacheCls = this.cacheSingleSprite;
        const valueC = cacheCls.get(assetAddress);
        if (!!valueC)
        {
            if (valueC.asset == null)
            {
                cacheCls.remove(assetAddress);
            }
            else
            {
                valueC.refCount = valueC.refCount + 1;
                return valueC;
            }
        }
        return null;
    }

    private async loadSingleImageAsyncInternal(assetAddress: string, type: SpriteType): Promise<SpriteValue>
    {
        const res = this.loadSingleImageSyncInternal(assetAddress);
        if(res != null) return res;

        const pending = this.pendingLoads.get(assetAddress);
        if (pending)
        {
            const result = await pending;
            if (result)
            {
                result.refCount++;
            }
            return result;
        }

        const loadTask = this.doLoadSingleImage(assetAddress, type);
        this.pendingLoads.set(assetAddress, loadTask);
        try
        {
            const result = await loadTask;
            return result;
        }
        catch (ex)
        {
            Log.error(ex);
        }
        finally
        {
            this.pendingLoads.delete(assetAddress);
        }
        return null;
    }

    private async doLoadSingleImage(assetAddress: string, type: SpriteType): Promise<SpriteValue>
    {
        const cacheCls = this.cacheSingleSprite;
        ResourceManager.GetInstance().LoadResourceAsync(assetAddress);
        let suc:boolean = false;
        while(true)
        {
            await TimerManager.instance.waitAsync(1);
            const state = ResourceManager.GetInstance().GetLoadingState(assetAddress)
            if(state != 1)
            {
                suc = state == 2;
                break;
            }
        }
        if(!suc)
        {
            Log.error("图片精灵不存在！请检查图片设置！\n" + assetAddress);
            return null;
        }
        const asset: PaperSprite = ResourceManager.GetInstance().GetLoadedResource(assetAddress) as PaperSprite;
        if (asset != null)
        {
            let value = cacheCls.get(assetAddress);
            if (!!value)
            {
                value.refCount++;
                return value;
            }
            value = new SpriteValue();
            value.asset = asset;
            if(type == SpriteType.Sprite){
                value.texture = asset.BakedSourceTexture as Texture2D;
            }
            value.refCount = 1
            cacheCls.set(assetAddress, value);
            return value;
        }
        else
        {
            Log.error("图片精灵不存在！请检查图片设置！\n" + assetAddress);
        }
        return null;
    }


    private getSpriteLoadInfoByPath(imagePath: string): SpriteType
    {
        var index = imagePath.indexOf(ATLAS_KEY);
        return index < 0?SpriteType.Sprite:  SpriteType.SpriteAtlas;
    }

    /**
     * 获取网络图片纹理
     * @param url 图片地址
     * @param tryCount 失败重试次数
     */
    public async getOnlineTexture(url: string, tryCount: number = 3): Promise<Texture2D>
    {
        if (string.isNullOrWhiteSpace(url)) return null;

        // 同步缓存命中
        let data = this.cacheOnlineImage.get(url);
        if (!!data)
        {
            data.refCount++;
            return data.texture;
        }

        // 检查 pending：同一 URL 并发请求复用同一加载任务
        const pending = this.pendingLoads.get(url);
        if (pending)
        {
            const result = await pending;
            if (result)
            {
                result.refCount++;
            }
            return result?.texture;
        }

        // 创建加载任务
        const loadTask = this.doLoadOnlineTexture(url, tryCount);
        this.pendingLoads.set(url, loadTask);
        try
        {
            const result = await loadTask;
            return result?.texture;
        }
        catch (ex)
        {
            Log.error(ex);
        }
        finally
        {
            this.pendingLoads.delete(url);
        }
        return null;
    }

    private async doLoadOnlineTexture(url: string, tryCount: number): Promise<SpriteValue>
    {
        let texture = await HttpManager.instance.httpGetImageOnline(url, true);
        if (texture == null)
        {
            for (let i = 0; i < tryCount; i++)
            {
                texture = await HttpManager.instance.httpGetImageOnline(url, false);
                if (texture != null) break;
            }
        }
        if (texture == null)
        {
            Log.error("网络无资源 " + url);
            return null;
        }
        const data = new SpriteValue();
        data.texture = texture;
        data.refCount = 1;
        this.cacheOnlineImage.set(url, data);
        return data;
    }

    /**
     * 释放网络图片
     * @param url 图片地址
     * @param clear 引用计数归零时是否立即释放
     */
    public releaseOnlineImage(url: string, clear: boolean = true)
    {
        const data = this.cacheOnlineImage.get(url);
        if (data == null) return;
        data.refCount--;
        if (clear && data.refCount <= 0)
        {
            this.destroyOnlineImage(url);
        }

        // 缓存数量超限时，清理引用计数为 0 的项
        if (this.cacheOnlineImage.size > 10)
        {
            const temp: string[] = [];
            for (const [key, val] of this.cacheOnlineImage)
            {
                if (val.refCount == 0)
                {
                    temp[temp.length] = key;
                }
            }
            for (let index = 0; index < temp.length; index++)
            {
                this.destroyOnlineImage(temp[index]);
            }
        }
    }

    private destroyOnlineImage(url: string)
    {
        const data = this.cacheOnlineImage.get(url);
        if (data == null) return;
        data.texture = null;
        data.asset = null;
        data.refCount = 0;
        this.cacheOnlineImage.delete(url);
    }

}