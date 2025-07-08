import { JsonHelper } from '../../../../Mono/Helper/JsonHelper';
import { ConfigManager } from '../../Config/ConfigManager';
import { SceneConfig, SceneConfigCategory } from './SceneConfig';
import * as SceneConfigCategoryData from '../Data/SceneConfigCategory.Data';
import { ServerConfig, ServerConfigCategory } from './ServerConfig';
import * as ServerConfigCategoryData from '../Data/ServerConfigCategory.Data';
export function register(){
	JsonHelper.registerClass(SceneConfig,'SceneConfig');
	JsonHelper.registerClass(SceneConfigCategory,'SceneConfigCategory');
	ConfigManager.instance.loadOneInThread(SceneConfigCategory,'SceneConfigCategory', SceneConfigCategoryData.SceneConfigCategoryData);
	JsonHelper.registerClass(ServerConfig,'ServerConfig');
	JsonHelper.registerClass(ServerConfigCategory,'ServerConfigCategory');
	ConfigManager.instance.loadOneInThread(ServerConfigCategory,'ServerConfigCategory', ServerConfigCategoryData.ServerConfigCategoryData);
}
