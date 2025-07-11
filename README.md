# QiongQi(穷奇)

基于[PuerTs](https://github.com/Tencent/puerts)

包含一个组件式UI框架

包含一个直接输出配置到ts代码的导Excel配置、读配置工具

我是Unity引擎开发者-> [TaoTie(饕餮)](https://github.com/526077247/TaoTie)

我是Cocos引擎开发者-> [TaoWu(梼杌)](https://github.com/526077247/TaoWu)

## 运行指南

1. 参考[官方文档](https://puerts.github.io/docs/puerts/unreal/install) 安装node、ts开发环境,下载虚拟机如v8_11.8.172,解压到QiongQi/Plugins/Puerts/ThirdParty
2. 右键QiongQi/QiongQi.uproject,选择生成vs project files
3. 进入项目目录下：QiongQi/Plugins/Puerts，并执行命令 node enable_puerts_module.js
4. 最后打开UE项目，切换到Content/AssetsPackage/Scenes/InitScene/Init场景运行