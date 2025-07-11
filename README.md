# QiongQi(穷奇)

基于[PuerTs](https://github.com/Tencent/puerts)

包含一个组件式UI框架

包含一个直接输出配置到ts代码的导Excel配置、读配置工具

我是Unity引擎开发者-> [TaoTie(饕餮)](https://github.com/526077247/TaoTie)

我是Cocos引擎开发者-> [TaoWu(梼杌)](https://github.com/526077247/TaoWu)

## 运行指南

0. 参考[UE源码获取流程](https://www.unrealengine.com/zh-CN/ue-on-github)下载并修改编译UE源码以支持自动图集（如果需要的话）
* FPaper2DEditor //设置
```
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
* FSlateAtlasedTextureResource 
```
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
1. 参考[官方文档](https://puerts.github.io/docs/puerts/unreal/install) 安装node、ts开发环境,下载虚拟机如v8_11.8.172,解压到QiongQi/Plugins/Puerts/ThirdParty
2. 右键QiongQi/QiongQi.uproject,选择生成vs project files
3. 进入项目目录下：QiongQi/Plugins/Puerts，并执行命令 node enable_puerts_module.js
4. 最后打开UE项目，切换到Content/AssetsPackage/Scenes/InitScene/Init场景运行
5. 修改ts代码后，对应js没有自动生成？cmd进入QiongQi目录，执行命令：tsc --watch