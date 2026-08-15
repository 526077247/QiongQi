import { ETTask } from "../../../ThirdParty/ETTask/ETTask";
import { Log } from "../Log/Log";
import * as string from "../../Helper/StringHelper"
import { JsonHelper } from "../../Helper/JsonHelper";
import { StringBuilder } from "../../Core/Object/StringBuilder";
import { UeHttpHelper, NewMap, BuiltinString, TMap, $Delegate, Texture2D } from "ue";

const DEFAULT_TIMEOUT: number = 10000; // 默认超时时间

export class HttpManager
{
    private static _instance: HttpManager = new HttpManager();
    public static get instance(): HttpManager {
        return HttpManager._instance;
    }

    private convertParamToStr(param: Record<string, string> ): string
    {
        if (param == null) return "";
        let builder: StringBuilder = new StringBuilder();
        let flag = 0;
        for (const key in param) {
            if (Object.prototype.hasOwnProperty.call(param, key)) {
                const element = param[key];
                if (flag == 0)
                {
                    builder.append(key + "=" + element);
                    flag = 1;
                }
                else
                {
                    builder.append("&" + key + "=" + element);
                }
            }
        }
        return builder.toString();
    }

    private convertHeaders(headers: Record<string, string>): TMap<string, string>
    {
        let headerMap: TMap<string, string> = NewMap(BuiltinString, BuiltinString);
        if (headers == null) return headerMap;
        for (const key in headers) {
            if (Object.prototype.hasOwnProperty.call(headers, key)) {
                headerMap.Set(key, headers[key]);
            }
        }
        return headerMap;
    }

    /**
     * 获取网络图片（本地缓存优先，缓存命中直接返回本地文件解码的纹理）
     * @param url 图片地址
     * @param local 是否优先读取本地缓存
     */
    public async httpGetImageOnline(url: string, local: boolean = true): Promise<Texture2D>
    {
        if (string.isNullOrWhiteSpace(url)) return null;
        const filePath = this.localFile(url);
        //本地是否存在图片
        if (local)
        {
            const texture = await this.loadLocalImage(filePath);
            if (texture != null) return texture;
        }
        return await this.downloadImage(url, filePath);
    }

    /**
     * 计算图片本地缓存文件路径
     * @param url 图片地址
     * @param dir 子目录
     * @param extend 扩展名
     */
    public localFile(url: string, dir: string = "img", extend: string = ".png"): string
    {
        const md5URLString: string = string.getHashString(url.trim());
        return `${UeHttpHelper.GetInstance().GetImageCacheDir()}/${dir}/${md5URLString}${extend}`;
    }

    private loadLocalImage(filePath: string): ETTask<Texture2D>
    {
        const response = ETTask.create<Texture2D>();
        let onResult = ((bSuccess: boolean, statusCode: number, texture: Texture2D) => {
            response.setResult(bSuccess ? texture : null);
        }) as unknown as $Delegate<(bSuccess: boolean, StatusCode: number, Texture: Texture2D) => void>;
        UeHttpHelper.GetInstance().LoadImageFromLocalFile(filePath, onResult);
        return response;
    }

    private downloadImage(url: string, cachePath: string): ETTask<Texture2D>
    {
        const response = ETTask.create<Texture2D>();
        let onResult = ((bSuccess: boolean, statusCode: number, texture: Texture2D) => {
            response.setResult(bSuccess ? texture : null);
        }) as unknown as $Delegate<(bSuccess: boolean, StatusCode: number, Texture: Texture2D) => void>;
        UeHttpHelper.GetInstance().HttpGetImage(url, this.convertHeaders(null), 10, cachePath, onResult);
        return response;
    }

    public async httpGetResult<T>(type: new (...args:any[]) => T, url: string, headers: Record<string,string>, param: Record<string,string>, timeout:number = DEFAULT_TIMEOUT):Promise<T>{
        let strParam = this.convertParamToStr(param);
        if (!string.isNullOrEmpty(strParam))
            url += "?" + strParam;
        let response = ETTask.create<string>();
        let onResult = ((bSuccess: boolean, statusCode: number, text: string) => {
            response.setResult(bSuccess ? text : "");
        }) as unknown as $Delegate<(bSuccess: boolean, StatusCode: number, ResponseText: string) => void>;
        UeHttpHelper.GetInstance().HttpGet(url, this.convertHeaders(headers), timeout / 1000, onResult);
        var text = await response;
        if(text!=null){
            try{
                return JsonHelper.fromJson<T>(type, text)
            }
            catch{
                Log.error("json.encode error:\n" + text);
                return null;
            }
        }else{
            Log.info(string.format("url {0} get fail.",url));
            return null;
        }
    }

    public async httpPostResult<T>(type: new (...args:any[]) => T, url: string, headers: Record<string,string>, param: Record<string,any>, timeout:number = DEFAULT_TIMEOUT):Promise<T>{
        let response = ETTask.create<string>();
        let onResult = ((bSuccess: boolean, statusCode: number, text: string) => {
            response.setResult(bSuccess ? text : "");
        }) as unknown as $Delegate<(bSuccess: boolean, StatusCode: number, ResponseText: string) => void>;
        UeHttpHelper.GetInstance().HttpPost(url, this.convertHeaders(headers), JSON.stringify(param), timeout / 1000, onResult);
        var text = await response;
        if(text!=null){
            try{
                return JsonHelper.fromJson<T>(type, text)
            }
            catch{
                Log.error("json.encode error:\n" + text);
                return null;
            }
        }else{
            Log.info(string.format("url {0} get fail.",url));
            return null;
        }
    }
}