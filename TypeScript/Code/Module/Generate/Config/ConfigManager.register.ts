import { JsonHelper } from '../../../../Mono/Helper/JsonHelper';
import { ConfigManager } from '../../Config/ConfigManager';
import { SceneConfig, SceneConfigCategory } from './SceneConfig';
// import * as SceneConfigCategoryData from '../Data/SceneConfigCategory.Data';
import { ServerConfig, ServerConfigCategory } from './ServerConfig';
// import * as ServerConfigCategoryData from '../Data/ServerConfigCategory.Data';
export function register(configBytes: Map<string, any>){
	JsonHelper.registerClass(SceneConfig,'SceneConfig');
	JsonHelper.registerClass(SceneConfigCategory,'SceneConfigCategory');
	ConfigManager.instance.loadOneInThread(SceneConfigCategory,'SceneConfigCategory', configBytes);
	// configBytes.set('SceneConfigCategory',SceneConfigCategoryData);
	JsonHelper.registerClass(ServerConfig,'ServerConfig');
	JsonHelper.registerClass(ServerConfigCategory,'ServerConfigCategory');
	ConfigManager.instance.loadOneInThread(ServerConfigCategory,'ServerConfigCategory', configBytes);
	// configBytes.set('ServerConfigCategory',ServerConfigCategoryData);
}
