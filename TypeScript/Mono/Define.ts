import * as UE from 'ue';
export class Define {

    public static Game: UE.QiongQiGameInstance

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
}