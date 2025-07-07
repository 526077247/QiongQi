import { Entry } from './Code/Entry'
import * as UE from 'ue';
import { argv } from 'puerts';
import { Define } from './Mono/Define';
import { Log } from './Mono/Module/Log/Log';
import { ConsoleLog } from './Mono/Module/Log/ConsoleLog';
import { ETTask } from './ThirdParty/ETTask/ETTask';
import { TimeInfo } from './Mono/Module/Timer/TimeInfo';

Define.Game = argv.getByName("GameInstance") as UE.QiongQiGameInstance;
Log.logger = new ConsoleLog();
Log.info("-------------------------QiongQi------------------------------");
// 设置全局异常处理器
ETTask.ExceptionHandler = (error) => {
    Log.error("Unhandled task exception:", error);
};
TimeInfo.instance.timeZone = TimeInfo.getUtcOffsetHours();
Entry.start();
