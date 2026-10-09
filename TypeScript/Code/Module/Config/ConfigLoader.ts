import * as UE from 'ue'
import { IConfigLoader } from "./IConfigLoader";
import { Log } from "../../../Mono/Module/Log/Log";

/**
 * 配置加载器：通过 C++ 辅助类 UE.QiongQiConfigLoader 读取 Content/AssetsPackage/Config 下所有 protobuf 二进制(.bytes)。
 * 编辑器直读 Content，打包后经 UE 虚拟文件系统穿透 pak 读取，行为一致。
 *
 * 注意：UE.QiongQiConfigLoader 的类型声明由 Puerts 自动生成（Typing/ue/ue.d.ts），
 * 新增/修改 UCLASS 后需重新生成声明文件。
 */
export class ConfigLoader implements IConfigLoader {

    public async loadAllAsync(callback: (name: string, data: ArrayBuffer) => void): Promise<void> {
        // puer-ts 反射下静态函数返回 TArray<FString> 会被转成 {}，故 C++ 侧改为返回 "|" 分隔的 FString
        const fileNames: string[] = UE.QiongQiConfigLoader.GetConfigFileNames().split('|');
        for (let i = 0; i < fileNames.length; i++) {
            const name = fileNames[i];
            if (name == null || name.length == 0) {
                continue;
            }
            const data = this.loadBytes(name);
            if (data != null) {
                callback(name, data);
            }
        }
    }

    public async loadSingleAsync(name: string): Promise<ArrayBuffer> {
        return this.loadBytes(name);
    }

    private loadBytes(name: string): ArrayBuffer {
        const content: ArrayBuffer = UE.QiongQiConfigLoader.LoadConfigBytes(name);
        if (content == null || content.byteLength == 0) {
            Log.error(`配置加载失败，文件不存在或内容为空：${name}.bytes`);
            return null;
        }
        return content;
    }
}
