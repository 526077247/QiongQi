# QiongQi(穷奇)

基于 UE5 + [PuerTs](https://github.com/Tencent/puerts) 的游戏框架，使用 TypeScript 编写全部游戏逻辑层。

## 功能概览

- **组件式 UI 框架** — 7 层级管理、窗口堆栈、组件生命周期（OnCreate/OnEnable/OnDisable/OnDestroy）、MsgBox 弹窗、路径化 Widget 查找
- **场景管理** — 异步场景切换、进度条管线、场景级 Manager 作用域、LoadingScene 子关卡流式加载
- **Excel 配置管线** — .NET 导出工具，Excel → JSON Data → 强类型 TypeScript 配置类，运行时按需加载
- **国际化 (I18N)** — 多语言切换、参数化文本、UI 自动注册多语言刷新
- **异步基础设施 (ETTask)** — Promise 风格的异步原语，支持取消令牌 (ETCancellationToken)，全链路 async/await
- **协程锁 (CoroutineLock)** — 异步互斥锁，超时自动释放，防止 UI/资源加载竞态
- **对象池 (ObjectPool)** — 泛型对象池，双重获取/回收检测，UIWindow/Timer/Lock 等广泛使用
- **高性能无限滚动列表** — 基于 SuperScrollView 的 LoopListView2 / LoopGridView，支持网格与列表两种模式
- **Timer 系统** — 单次定时器、可等待定时器、帧定时器、重复定时器，帧驱动
- **事件总线 (Messager)** — 基于 (groupId, messageId) 的类型化事件订阅/广播
- **资源加载 (ImageLoaderManager)** — LRU 缓存、引用计数、Sprite/SpriteAtlas 自动识别
- **HTTP 网络层** — 基于 UE FHttpModule 的异步请求/响应，类型化反序列化
- **玩家数据持久化 (CacheManager)** — 基于 UE SaveGame 的键值存储
- **热更新 (Hot Update & CDN)** — CDN 资源版本检查、断点续传下载、IoStore 三件套挂载、JS 虚拟机重启
- **打包工具 (QiongQiPackTools)** — Slate 打包面板，集成 UAT 整包构建、HotPatcher 补丁生成、IoStore、CDN 版本清单、Chunk 分配

## 目录结构

```
QiongQi/
├── TypeScript/                # TypeScript 游戏逻辑源码
│   ├── Start.ts              # JS 入口，获取 GameInstance 并启动
│   ├── Code/                  # 业务层
│   │   ├── Entry.ts           # 框架启动入口，注册所有 Manager
│   │   ├── Game/              # 场景与 UI 视图
│   │   │   ├── Scene/         #   LoginScene / MapScene
│   │   │   └── UI/            #   UIMain / UILoading / UIUpdate / UICommon
│   │   └── Module/            # 业务模块
│   │       ├── Config/        #   配置管理器
│   │       ├── Const/         #   常量定义 (LangType, CacheKeys, I18NKey)
│   │       ├── CoroutineLock/ #   协程锁
│   │       ├── Generate/      #   Excel 导出生成的代码 (自动)
│   │       ├── I18N/          #   国际化管理器
│   │       ├── Player/        #   玩家缓存持久化
│   │       ├── Resource/      #   图片/资源加载
│   │       ├── Scene/         #   场景管理器
│   │       ├── UI/            #   UI 框架核心
│   │       ├── UIComponent/   #   UE Widget 封装组件
│   │       └── Update/        #   热更新 (CDN 下载 / 版本检查 / 补丁挂载)
│   ├── Mono/                  # 框架底层
│   │   ├── Core/              #   Manager 注册体系 + 对象池 + 数据结构
│   │   ├── Module/            #   Timer / Messager / Log / Http / Update / I18N
│   │   ├── Helper/            #   Json / Random / String / UELifeTime 辅助类
│   │   ├── Define.ts          #   全局常量 (设计分辨率/DeltaTime/LogLevel/Debug)
│   │   └── Init.ts            #   框架初始化
│   └── ThirdParty/           # 第三方库
│       ├── ETTask/            #   异步原语 (ETTask<T> + ETCancellationToken)
│       └── SuperScrollView/   #   无限滚动列表/网格
├── Source/                    # C++ 游戏模块
│   └── QiongQi/               #   GameInstance / 下载 / HTTP / 资源 / 存档
├── Content/                   # UE 资源
│   └── AssetsPackage/         #   打包资源 (UI/Scenes/Config/...)
├── Excel/                     # Excel 配置表 + 导出工具脚本
├── Plugins/
│   ├── Puerts/                # PuerTs 插件 (V8 JS 引擎 + UE 绑定)
│   ├── HotPatcher/            # HotPatcher 插件 (IoStore 补丁打包)
│   ├── QiongQiEditor/         # 项目编辑器扩展 (打包面板)
│   │   └── Source/QiongQiPackTools/  # Slate 打包面板 + UAT/HotPatcher 集成
│   └── RuntimeFilesDownloader/ # 运行时文件下载插件
├── Tools/
│   ├── ExcelExport/           # .NET Excel 导出工具 (ExcelExport.dll)
│   └── FileServer/            # .NET 本地 CDN 文件服务器 (ASP.NET Core)
├── Config/                    # UE 配置文件
│   ├── DefaultGame.ini        #   打包设置 (IoStore/Chunk 规则/渠道版本)
│   ├── DefaultEngine.ini      #   引擎设置 (渲染/默认地图/GameInstance)
│   ├── DefaultPakFileRules.ini #  Pak 文件过滤规则
│   └── CDNInFirstPak.ini      #   全量进首包白名单目录
├── Bin/                       # .NET 导出工具 (ExcelExport.dll)
├── tsconfig.json              # TypeScript 编译配置
└── QiongQi.uproject           # UE5 工程文件
```

## 架构概览

### 启动流程

```
UE GameInstance (C++)
  └─ JS 入点 Start.ts → Init.start()
       ├─ 设置 Log / 异常处理器 / 时区
       └─ 绑定 Update 帧回调 → Entry.start()
            └─ 异步注册所有 Manager → 热更新检查 → switchScene(LoginScene)
```

### C++ 层核心类

| 类 | 职责 |
|---|---|
| `UQiongQiGameInstance` | 拥有 Puerts `FJsEnv`；暴露 `RestartJsEnv()`（热更后异步重建 JS 虚拟机）、网络状态、编辑器/调试包判断 |
| `UUeDownloadHelper` | CDN 下载桥：断点续传、MD5 校验、Pak 挂载（HotPatcher `UFlibPakHelper::MountPak`）、本地版本读写（`Saved/Paks/version.json`）、包内版本对齐 |
| `UeHttpHelper` | 异步 HTTP 请求（`FHttpModule`），替代浏览器 XHR |
| `ResourceManager` | `StreamableManager` 异步资源加载 + 缓存 |
| `QiongQiConfigLoader` | 从 `Content/AssetsPackage/Config` 加载 JSON 配置（支持 pak 内读取） |
| `QiongQiPlayerPrefs` / `QiongQiSaveGame` | 基于 `USaveGame` 的键值持久化（String/Float/Int/Bool） |

### Manager 体系

所有管理器实现 `IManager` 接口（`init()` / `destroy()`），通过 `ManagerProvider.registerManager()` 统一注册。实现 `IUpdate` / `ILateUpdate` 的管理器自动加入帧循环。

| Manager | 职责 |
|---|---|
| Messager | 类型化事件总线 (groupId, messageId) |
| CoroutineLockManager | 异步互斥锁，超时自动释放 |
| TimerManager | 单次/重复/帧/可等待定时器 |
| CacheManager | 玩家数据持久化 (UE SaveGame) |
| ConfigManager | Excel 配置运行时加载 |
| ImageLoaderManager | 图片 LRU 缓存 + 引用计数 |
| I18NManager | 多语言切换 + 文本查询 |
| UIManager | 窗口/弹窗/层级/组件管理 |
| SceneManager | 异步场景切换 + 进度管线 |
| ServerConfigManager | 服务器配置 / 渠道 / 版本管理 |

### UI 框架

**组件层级：**
```
UIBaseComponent          ← Widget 查找、显隐控制、生命周期、帧更新
  └─ UIBaseContainer     ← 子组件管理 (addComponent/getComponent/removeComponent)
       └─ UIBaseView      ← 窗口视图 (closeSelf/canBack/onInputKeyBack)
```

**7 层 UI 层级：**

| 层 | zOrder | 用途 |
|---|---|---|
| GameBackgroundLayer | 0 | 游戏背景 |
| BackgroundLayer | 1000 | 全屏 UI |
| GameLayer | 1800 | 游戏内 View |
| SceneLayer | 2000 | 场景级 UI |
| NormalLayer | 3000 | 常规多窗口 |
| TipLayer | 4000 | 提示/错误/网络弹窗 |
| TopLayer | 5000 | 场景加载遮罩 |

**窗口类型：** 单例窗口（`openWindow` / `closeWindow`）和多实例弹窗（`openBox` / `closeBox`），使用 CoroutineLock 防止加载竞态。

### 场景切换流程

```
switchScene → 开启 LoadingUI → onEnter() → 清理旧场景/旧窗口
→ 流式加载 LoadingScene 子关卡 → GC → 加载目标子关卡
→ onComplete() → onPrepare() → onSwitchSceneEnd() → 关闭 LoadingUI
```

## 热更新系统

### 架构

```
启动 → 挂载本地已下载 CDN pak (PakOrder=100+) → SyncLocalVersionFromPackage (包内版本对齐)
→ 创建 JsEnv → Entry.start() → 检查 CDN 版本清单 {version}.json
→ md5 对比 → 用户确认 → 下载 pak/utoc/ucas (断点续传+重试+md5校验)
→ 挂载 pak → SaveLocalVersion → RestartJsEnv (重建 JS 虚拟机加载新 Code)
```

### 版本清单格式

CDN 根目录下的 `{version}.json`，与历史版本清单合并保证全量：

```json
{
  "channel": "Default",
  "platform": "pc",
  "version": 1786890573,
  "files": [
    { "name": "1786890573_Windows_001_P.pak", "md5": "...", "size": 6064659 },
    { "name": "1786890573_Windows_001_P.utoc", "md5": "...", "size": 2921 },
    { "name": "1786890573_Windows_001_P.ucas", "md5": "...", "size": 5998560 }
  ]
}
```

### CDN 目录结构

```
CDN 根目录/{渠道}_{平台}/
  ├── {版本号}.json          # 版本清单
  ├── xxx.pak                # IoStore 三件套
  ├── xxx.utoc
  └── xxx.ucas
```

### 版本记录

- **包内版本**：打包面板写入 `DefaultGame.ini [QiongQi] ResourceVersion`，随包固化
- **本地版本**：`Saved/Paks/version.json`，启动时与包内版本取较大值写回
- **短路逻辑**：全量进首包模式（`FullInFirstPak=1`）下，本地版本 ≥ 最新版本时跳过下载

## 打包工具 (QiongQiPackTools)

Slate 打包面板集成在 `Plugins/QiongQiEditor/Source/QiongQiPackTools/`，提供：

- **整包构建**：UAT BuildCookRun 一键 Cook/Stage/Package
- **补丁生成**：HotPatcher 补丁，支持 IoStore（.pak/.utoc/.ucas 三件套）
- **Chunk 分配**：白名单目录进首包（pakchunk0），其余走 CDN（pakchunk100+）
- **CDN 版本清单**：自动扫描产物生成 `{版本号}.json`，合并历史清单保证全量
- **配置固化**：渠道/版本号/全量进首包/调试标志写入 `DefaultGame.ini` 随包固化
- **Release 交付**：自动复制 CDN 资源与整包到 `Release/` 目录

### 打包模式

| 模式 | 说明 |
|---|---|
| 打整包 + 全量进首包 | UAT 整包 + HotPatcher 全量补丁，CDN 不产出增量 |
| 打整包 + CDN 模式 | UAT 整包（Chunk 拆分）+ HotPatcher CDN 增量补丁 |
| 不打整包 | 仅 HotPatcher 增量补丁，需选择历史版本作为主版本 |

## Excel 配置管线

```
.xlsx 文件 → .NET 导出工具 (ExcelExport.dll) → Generate/Data/*.Data.ts (JSON)
                                            → Generate/Config/*.ts (强类型配置类)
                                            → 运行时 ConfigManager.loadOneConfig() 加载
```

| 脚本 | 功能 |
|---|---|
| `win_startExcelExport.bat` | 导出配置表 |
| `win_startI18NExport.bat` | 导出多语言文本 |
| `win_startAttrExport.bat` | 导出属性数据 |
| `win_startExportAll.bat` | 全量导出 |
| `策划校验表工具.bat` | 策划数据校验 |

## 配置文件

| 文件 | 用途 |
|---|---|
| `DefaultGame.ini` | 打包设置（IoStore/Chunk 规则）、`[QiongQi]` 渠道/版本/全量进首包/调试标志 |
| `DefaultEngine.ini` | 引擎设置（D3D12/SM6、默认地图 InitScene/Init、GameInstanceClass） |
| `DefaultPakFileRules.ini` | Pak 文件过滤规则（排除 Editor 内容） |
| `CDNInFirstPak.ini` | 全量进首包白名单目录列表 |

## 运行指南

### 0. 编译 UE 源码以支持自动图集（可选）

参考 [UE 源码获取流程](https://www.unrealengine.com/zh-CN/ue-on-github) 下载并修改编译 UE 源码。

**FPaper2DEditor** — 修改 `OnPropertyChanged`：

```cpp
void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedDelegateHandle);
    if (UPaperSpriteAtlas* Atlas = Cast<UPaperSpriteAtlas>(ObjectBeingModified))
    {
        FPaperAtlasGenerator::HandleAssetChangedEvent(Atlas);
    }
    else if (UPaperRuntimeSettings* Settings = Cast<UPaperRuntimeSettings>(ObjectBeingModified))
    {
        // Handle changes to experimental flags here
    }

    OnPropertyChangedDelegateHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FPaper2DEditor::OnPropertyChanged);
}
```

**FSlateAtlasedTextureResource** — 修改 `FindOrCreateAtlasedProxy`：

```cpp
FSlateShaderResourceProxy* FSlateAtlasedTextureResource::FindOrCreateAtlasedProxy(UObject* InAtlasedObject, const FSlateAtlasData& AtlasData)
{
    FSlateShaderResourceProxy* Proxy = ProxyMap.FindRef(InAtlasedObject);
    if ( Proxy == nullptr )
    {
        // when we use image-DrawAsBox with PaperSprite, we need to change its actual size as its actual dimension.
        FVector2D ActualSize(TextureObject->GetSurfaceWidth() * AtlasData.SizeUV.X, TextureObject->GetSurfaceHeight() * AtlasData.SizeUV.Y);

        Proxy = new FSlateShaderResourceProxy();
        Proxy->Resource = this;
        Proxy->ActualSize = ActualSize.IntPoint();
        Proxy->StartUV = FVector2f(AtlasData.StartUV);	// LWC_TODO: Precision loss
        Proxy->SizeUV = FVector2f(AtlasData.SizeUV);	// LWC_TODO: Precision loss

        ProxyMap.Add(InAtlasedObject, Proxy);
    }
#if WITH_EDITOR
    else
    {
        Proxy->Resource = this;
        Proxy->StartUV = FVector2f(AtlasData.StartUV);	// LWC_TODO: Precision loss
        Proxy->SizeUV = FVector2f(AtlasData.SizeUV);	// LWC_TODO: Precision loss
    }
#endif
    return Proxy;
}
```

然后打开 Project Setting，开启 atlas 实验性功能 **"Enable Sprite Atlas Groups"**。

### 1. 安装 Node.js 与 TypeScript 环境

参考 [PuerTs 官方文档](https://puerts.github.io/docs/puerts/unreal/install) 安装 Node.js 和 TypeScript 开发环境。

### 2. 下载 JS 虚拟机

下载 V8 引擎（如 `v8_11.8.172`），解压到 `QiongQi/Plugins/Puerts/ThirdParty`。

### 3. 生成 VS 工程并启用 PuerTs 模块

```bash
# 右键 QiongQi/QiongQi.uproject → Generate Visual Studio Project Files
cd QiongQi/Plugins/Puerts
node enable_puerts_module.js
```

### 4. 打开项目并运行

打开 UE 项目，切换到 `Content/AssetsPackage/Scenes/InitScene/Init` 场景，点击运行。

### 5. TypeScript 热编译

修改 TS 代码后，若 JS 未自动生成，在项目根目录执行：

```bash
tsc --watch
```

### 6. 本地 CDN 测试服务器

启动 `Tools/FileServer` 提供本地 CDN 文件服务，用于热更新下载测试。

## 相关项目

- Unity 引擎 → [TaoTie(饕餮)](https://github.com/526077247/TaoTie)
- Cocos 引擎 → [TaoWu(梼杌)](https://github.com/526077247/TaoWu)
- Godot 引擎 → [HunDun(混沌)](https://github.com/526077247/HunDun)
