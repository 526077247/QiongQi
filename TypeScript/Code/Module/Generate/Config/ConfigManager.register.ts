import { JsonHelper } from '../../../../Mono/Helper/JsonHelper';
import { SceneConfig, SceneConfigCategory } from './SceneConfig';
import * as SceneConfigCategoryData from '../Data/SceneConfigCategory.Data';
import { ServerConfig, ServerConfigCategory } from './ServerConfig';
import * as ServerConfigCategoryData from '../Data/ServerConfigCategory.Data';
export function register(loadOneInThread: Function){
	JsonHelper.registerClass(SceneConfig,'SceneConfig');
	JsonHelper.registerClass(SceneConfigCategory,'SceneConfigCategory');
	loadOneInThread(SceneConfigCategory,'SceneConfigCategory', SceneConfigCategoryData.SceneConfigCategoryData);
	JsonHelper.registerClass(ServerConfig,'ServerConfig');
	JsonHelper.registerClass(ServerConfigCategory,'ServerConfigCategory');
	loadOneInThread(ServerConfigCategory,'ServerConfigCategory', ServerConfigCategoryData.ServerConfigCategoryData);
}
