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
- **HTTP 网络层** — 基于 XMLHttpRequest 的 JSON 请求/响应，类型化反序列化
- **玩家数据持久化 (CacheManager)** — 基于 UE SaveGame 的键值存储
- **AssetImportPlugin** — UE 编辑器资源导入插件

## 目录结构

```
QiongQi/
├── TypeScript/                # TypeScript 游戏逻辑源码
│   ├── Start.ts              # JS 入口，获取 GameInstance 并启动
│   ├── Code/                  # 业务层
│   │   ├── Entry.ts           # 框架启动入口，注册所有 Manager
│   │   ├── Game/              # 场景与 UI 视图
│   │   │   ├── Scene/         #   LoginScene / MapScene
│   │   │   └── UI/            #   UIMain / UILoading / UICommon
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
│   │       └── UIComponent/   #   UE Widget 封装组件
│   ├── Mono/                  # 框架底层
│   │   ├── Core/              #   Manager 注册体系 + 对象池 + 数据结构
│   │   ├── Module/            #   Timer / Messager / Log / Http / Update / I18N
│   │   ├── Helper/             #   Json / Random / String / UELifeTime 辅助类
│   │   ├── Define.ts          #   全局常量 (设计分辨率/DeltaTime/LogLevel/Debug)
│   │   └── Init.ts            #   框架初始化
│   └── ThirdParty/           # 第三方库
│       ├── ETTask/            #   异步原语 (ETTask<T> + ETCancellationToken)
│       └── SuperScrollView/   #   无限滚动列表/网格
├── Content/                   # UE 资源
│   └── AssetsPackage/         #   打包资源 (UI/Scenes/...)
├── Excel/                     # Excel 配置表 + 导出工具脚本
├── Plugins/
│   ├── Puerts/                # PuerTs 插件 (V8 JS 引擎 + UE 绑定)
│   └── AssetImportPlugin/     # 资源导入插件
├── Config/                    # UE 配置文件
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
            └─ 异步注册所有 Manager → switchScene(LoginScene)
```

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

### Excel 配置管线

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

## 相关项目

- Unity 引擎 → [TaoTie(饕餮)](https://github.com/526077247/TaoTie)
- Cocos 引擎 → [TaoWu(梼杌)](https://github.com/526077247/TaoWu)
- Godot 引擎 → [HunDun(混沌)](https://github.com/526077247/HunDun)