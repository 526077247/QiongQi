import { Queue } from "../Core/Object/Queue"
import { ETTask } from "../../ThirdParty/ETTask/ETTask"
export class UELifeTimeHelper
{
    public static readonly updateFinishTask: Queue<ETTask> = new Queue<ETTask>();
    //等待这一帧所有update结束
    public static waitUpdateFinish(): ETTask
    {
        var task = ETTask.create(true);
        UELifeTimeHelper.updateFinishTask.enqueue(task);
        return task;
    }

    public static readonly lateUpdateFinishTask: Queue<ETTask> = new Queue<ETTask>();
    //等待这一帧所有lateupdate结束
    public static waitLateUpdateFinish(): ETTask
    {
        var task = ETTask.create(true);
        UELifeTimeHelper.lateUpdateFinishTask.enqueue(task);
        return task;
    }
}