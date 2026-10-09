/**
 * 配置加载器接口
 */
export interface IConfigLoader {
    /**
     * 全量加载 AssetsPackage/Config 下所有 protobuf 配置，逐条回调原始二进制数据
     * @param callback 每加载一个配置文件回调一次（name 不含扩展名，data 为文件二进制 ArrayBuffer）
     */
    loadAllAsync(callback: (name: string, data: ArrayBuffer) => void): Promise<void>;

    /**
     * 按文件名加载单个配置（I18N 多语言按需加载使用）
     * @param name 配置文件名（不含扩展名）
     */
    loadSingleAsync(name: string): Promise<ArrayBuffer>;
}
