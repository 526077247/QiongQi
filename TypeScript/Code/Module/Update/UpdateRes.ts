/**
 * 热更流程结果枚举。
 * Fail=-1 检查/下载失败；Over=0 正常结束（无更新/跳过/回退本地）；
 * Quit=1 用户退出；Restart=2 已下载新代码，需重启 JS 虚拟机。
 */
export enum UpdateRes {
    Fail = -1,
    Over = 0,
    Quit = 1,
    Restart = 2,
}
