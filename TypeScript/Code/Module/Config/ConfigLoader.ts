import * as UE from 'ue'
import { IConfigLoader } from "./IConfigLoader";
import { Log } from "../../../Mono/Module/Log/Log";

/**
 * 配置加载器：通过 C++ 辅助类 UE.QiongQiConfigLoader 读取 Content/AssetsPackage/Config 下所有 JSON。
 * 编辑器直读 Content，打包后经 UE 虚拟文件系统穿透 pak 读取，行为一致。
 *
 * 注意：UE.QiongQiConfigLoader 的类型声明由 Puerts 自动生成（Typing/ue/ue.d.ts），
 * 新增 UCLASS 后需重新生成声明文件，届时可移除下方 @ts-ignore。
 */
export class ConfigLoader implements IConfigLoader {

    public async loadAllAsync(callback: (name: string, data: any) => void): Promise<void> {
        // @ts-ignore  puer-ts 反射下静态函数返回 TArray<FString> 会被转成 {}，故 C++ 侧改为返回 "|" 分隔的 FString
        const fileNames: string[] = UE.QiongQiConfigLoader.GetConfigJsonFileNames().split('|');
        for (let i = 0; i < fileNames.length; i++) {
            const name = fileNames[i];
            const data = this.loadJson(name);
            if (data != null) {
                callback(name, data);
            }
        }
    }

    public async loadSingleAsync(name: string): Promise<any> {
        return this.loadJson(name);
    }

    private loadJson(name: string): any {
        // @ts-ignore
        const content: string = UE.QiongQiConfigLoader.LoadConfigJson(name);
        if (content == null || content.length == 0) {
            Log.error(`配置加载失败，文件不存在或内容为空：${name}.json`);
            return null;
        }
        return JSON.parse(content);
    }
}
