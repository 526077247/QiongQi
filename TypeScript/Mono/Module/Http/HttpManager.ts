import { ETTask } from "../../../ThirdParty/ETTask/ETTask";
import { Log } from "../Log/Log";
export class HttpManager
{
    private static _instance: HttpManager = new HttpManager();
    public static get instance(): HttpManager {
        return HttpManager._instance;
    }

}