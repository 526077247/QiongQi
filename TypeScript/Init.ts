import { Entry } from './Entry';
import * as UE from 'ue';
import { argv } from 'puerts';
import { Define } from "./Define"

Define.Game = argv.getByName("GameInstance") as UE.QiongQiGameInstance;
Entry.start();
