import { Init } from './Mono/Init'
import * as UE from 'ue';
import { $ref, argv } from 'puerts';
import { Define } from './Mono/Define';

Define.Game = argv.getByName("GameInstance") as UE.QiongQiGameInstance;
Init.start();