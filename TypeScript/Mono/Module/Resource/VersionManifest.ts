/**
 * 版本清单相关类型定义（与 TaoWu 项目对齐）
 */

/** 资源版本信息 */
export class Resver {
    /** 适用渠道列表 */
    public Channel: string[] = [];
    /** 生效的账号尾号列表 */
    public UpdateTailNumber: string[] = [];
    /** 是否强制更新（1 强制，0 不强制，-1 忽略） */
    public ForceUpdate: number = 0;
    /** 资源版本上限 */
    public MaxResVer: number = 0;

    public toString(): string {
        return `channel=[${this.Channel}], updateTailNumber=[${this.UpdateTailNumber}], forceUpdate=${this.ForceUpdate}, maxResVer=${this.MaxResVer}`;
    }
}

/** App 版本配置 */
export class AppConfig {
    /** App 下载地址 */
    public AppUrl: string = "";
    /** 各 App 版本对应的资源版本信息 { appVer: Resver } */
    public AppVer: Record<string, Resver> = {};
    /** 跳转渠道 */
    public JumpChannel: string = "";
}

/** CDN 更新列表配置（update_{platform}.list） */
export class UpdateListConfig {
    /** 各渠道的资源版本列表 { channel: { version: Resver } } */
    public ResList: Record<string, Record<string, Resver>> = {};
    /** 各渠道的 App 版本列表 { channel: AppConfig } */
    public AppList: Record<string, AppConfig> = {};
}
