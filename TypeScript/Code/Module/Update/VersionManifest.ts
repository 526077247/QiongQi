/**
 * CDN 版本清单数据结构。
 * 注：CDN 更新设置（常量 + URL 构造）已合并至 UpdateTask（原 UpdateSetting）。
 * CDN 目录结构（与打包面板 CopyResultsToRelease 产出一致，资源直接展开到渠道平台根目录）：
 *   {routerListUrl}/{channel}_{platform}/{版本号}.json
 *   {routerListUrl}/{channel}_{platform}/xxx.pak
 */

/** CDN 版本清单中的单个文件条目 */
export class CdnFileInfo {
    /** CDN 上的文件名（原始 pak 文件名） */
    public name: string = "";
    /** 文件 MD5（用于完整性校验） */
    public md5: string = "";
    public size: number = 0;
}

/** 版本清单（{版本号}.json） */
export class CdnVersionManifest {
    public channel: string = "";
    public platform: string = "";
    public version: number = 0;
    public files: CdnFileInfo[] = [];
}
