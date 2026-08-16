import * as UE from 'ue';
export class Define {

    public static Game: UE.QiongQiGameInstance
    public static DeltaTime: number

    private static readonly dWidth = 768;
    private static readonly dHeight = 1366;

    public static ScreenWidth: number;
    public static ScreenHeight: number;

    public static readonly DesignScreenWidth =
        Define.ScreenWidth > Define.ScreenHeight ? Math.max(Define.dWidth, Define.dHeight) : Math.min(Define.dWidth, Define.dHeight);
    public static readonly DesignScreenHeight =
        Define.ScreenWidth > Define.ScreenHeight ? Math.min(Define.dWidth, Define.dHeight) : Math.max(Define.dWidth, Define.dHeight);
    public static LogLevel = 1;

    public static Process = 1;

    public static readonly MinRepeatedTimerInterval: number = 100;

    public static readonly Debug = true;

    public static get Networked(): boolean
    {
        // ENetworkConnectionStatus：0=Unknown 1=Disabled 2=Local 3=Connected；仅 Disabled(1) 视为无网络
        const Status = Define.Game ? Define.Game.GetNetworkConnectionStatus() : 0;
        return Status !== 1;
    }

    public static readonly ForceUpdate = false;

    /** SH 服标记：由 UpdateIsSHProcess 依据 CDN 更新列表自动设置 */
    public static isSH: boolean = false;
}