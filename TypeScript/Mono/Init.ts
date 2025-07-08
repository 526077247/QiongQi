import { Log } from "./Module/Log/Log";
import { ConsoleLog } from './Module/Log/ConsoleLog';
import { TimeInfo } from './Module/Timer/TimeInfo';
import { Entry } from "../Code/Entry";
import { ETTask } from "../ThirdParty/ETTask/ETTask";
import { ManagerProvider } from "./Core/Manager/ManagerProvider";
import { Define } from "./Define";
export class Init
{
    public static start() 
    {
        Log.logger = new ConsoleLog();
    
        Log.info("-------------------------QiongQi------------------------------");
        // 设置全局异常处理器
        ETTask.ExceptionHandler = (error) => {
            Log.error("Unhandled task exception:", error);
        };
        TimeInfo.instance.timeZone = TimeInfo.getUtcOffsetHours();
        Define.Game.NotifyUpdate.Add(Init.update);
        Entry.start();
    }

    public static update()
    {
        try
        {
            ManagerProvider.update();
            ManagerProvider.lateUpdate();
        }
        catch(e)
        {
            Log.error(e);
        }
    }
}