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

    /**
     * 是否编辑器环境：由 C++ 层 QiongQiGameInstance::IsEditorEnvironment()（GIsEditor）获取，打包后为 false。
     * 用于仅在编辑器中生效的逻辑（如编辑器内调试输出、跳过正式更新流程等）。
     */
    public static get IsEditor(): boolean {
        return Define.Game ? Define.Game.IsEditorEnvironment() : false;
    }

    /**
     * 是否调试模式：编辑器恒为 true；打包版由打包面板固化的 IsDebugPackage 决定（Debug 包=true，Release 包=false）。
     * 用于调试逻辑（记忆服务器/切换服务器等）。
     */
    public static get Debug(): boolean {
        return Define.IsEditor || (Define.Game ? Define.Game.IsDebugPackage() : false);
    }

    public static get Networked(): boolean
    {
        // ENetworkConnectionStatus：0=Unknown 1=Disabled 2=Local 3=Connected；仅 Disabled(1) 视为无网络
        const Status = Define.Game ? Define.Game.GetNetworkConnectionStatus() : 0;
        return Status !== 1;
    }

    /**强制更新，不能跳过？ */
    public static readonly ForceUpdate = true;

    /** SH 服标记：由 UpdateIsSHProcess 依据 CDN 更新列表自动设置 */
    public static isSH: boolean = false;
}