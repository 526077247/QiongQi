import { UpdateTask } from "../UpdateTask";
import { UpdateRes } from "../UpdateRes";

/**
 * 热更流程基类。
 * process(task) 接收流程上下文（UpdateTask），返回 UpdateRes。
 * 多个 Process 由 UpdateTask.init(...) 按序串联，任一返回非 Over 立即短路。
 */
export abstract class UpdateProcess {
    public abstract process(task: UpdateTask): Promise<UpdateRes>;
}
