import * as UE from 'ue';

import { Log } from "../Mono/Module/Log/Log"
import { ManagerProvider } from "../Mono/Core/Manager/ManagerProvider"
import { Messager } from "../Mono/Module/Messager/Messager"
import { TimerManager } from "../Mono/Module/Timer/TimerManager"
import { UIManager } from "./Module/UI/UIManager"
import { CoroutineLockManager } from "./Module/CoroutineLock/CoroutineLockManager"
import { SceneManager } from "./Module/Scene/SceneManager"
import { HomeScene } from "./Game/Scene/LoginScene"
import { I18NManager } from "./Module/I18N/I18NManager"
import { CacheManager } from "./Module/Player/CacheManager"
import { ConfigManager } from "./Module/Config/ConfigManager"
import { ImageLoaderManager } from "./Module/Resource/ImageLoaderManager"
import { UIUpdateView } from "./Game/UI/UIUpdate/UIUpdateView"
import { Define } from '../Mono/Define';
import { ServerConfigManager } from './Module/Update/ServerConfigManager';

export class Entry 
{  
    public static start()
    {
        Log.info("Entry.start");
        Entry.startAsync();
    }
    
    private static async startAsync() {
        try {
            ManagerProvider.registerManager(Messager);
            ManagerProvider.registerManager(CoroutineLockManager);
            ManagerProvider.registerManager(TimerManager);
            ManagerProvider.registerManager(CacheManager);

            const cm = ManagerProvider.registerManager(ConfigManager);

            ManagerProvider.registerManager(I18NManager);
            ManagerProvider.registerManager(UIManager);
            Log.info('IsEditor'+Define.IsEditor)
            if (!Define.IsEditor && (Define.Networked||Define.ForceUpdate)) {
                await cm.loadAsync();
                ManagerProvider.registerManager(ServerConfigManager);
                // === 阶段 B: 热更新检查 ===
                await UIManager.instance.openWindow<UIUpdateView, VoidFunction>(
                    UIUpdateView, UIUpdateView.PrefabPath, Entry.startGame,
                );
            } else {
                // 编辑器中直接进入游戏
                Entry.startGameAsync(false);
            }
        } catch (e) {
            Log.error(e);
        }
    }

    private static async startGame(){
        Entry.startGameAsync(true);
    }

    /**
     * 更新完成后, 注册剩余 Manager 并进入游戏
     */
    private static async startGameAsync(configInit: boolean)
    {
        if(!configInit) await ConfigManager.instance.loadAsync();
        ManagerProvider.registerManager(ImageLoaderManager);
        ManagerProvider.registerManager(SceneManager);
        await SceneManager.instance.switchScene(HomeScene)
    }
}  

