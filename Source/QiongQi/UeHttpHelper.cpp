#include "UeHttpHelper.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

UUeHttpHelper* UUeHttpHelper::Instance = nullptr;

UUeHttpHelper* UUeHttpHelper::GetInstance()
{
    if (!Instance)
    {
        // 创建持久化实例
        Instance = NewObject<UUeHttpHelper>(GetTransientPackage(), UUeHttpHelper::StaticClass());
        Instance->AddToRoot(); // 防止垃圾回收
    }
    return Instance;
}

void UUeHttpHelper::HttpGet(const FString& Url, const TMap<FString, FString>& Headers, float TimeoutSeconds, FOnHttpResult OnResult)
{
    SendRequest(TEXT("GET"), Url, Headers, TEXT(""), TimeoutSeconds, MoveTemp(OnResult));
}

void UUeHttpHelper::HttpPost(const FString& Url, const TMap<FString, FString>& Headers, const FString& Body, float TimeoutSeconds, FOnHttpResult OnResult)
{
    SendRequest(TEXT("POST"), Url, Headers, Body, TimeoutSeconds, MoveTemp(OnResult));
}

void UUeHttpHelper::HttpGetImage(const FString& Url, const TMap<FString, FString>& Headers, float TimeoutSeconds, const FString& CachePath, FOnHttpImageResult OnResult)
{
    if (Url.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UeHttpHelper] Image url is empty."));
        OnResult.ExecuteIfBound(false, 0, nullptr);
        return;
    }

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetVerb(TEXT("GET"));
    Request->SetURL(Url);
    Request->SetTimeout(TimeoutSeconds);

    for (const auto& Pair : Headers)
    {
        if (!Pair.Key.IsEmpty())
        {
            Request->SetHeader(Pair.Key, Pair.Value);
        }
    }

    Request->OnProcessRequestComplete().BindLambda([OnResult, CachePath](FHttpRequestPtr RequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        if (!OnResult.IsBound())
        {
            return;
        }

        int32 StatusCode = 0;
        UTexture2D* Texture = nullptr;

        if (bConnectedSuccessfully && Response.IsValid())
        {
            StatusCode = Response->GetResponseCode();
            // 与 TS 侧既有判定语义一致：200-399 视为成功
            if (StatusCode >= 200 && StatusCode < 400)
            {
                const TArray<uint8>& Content = Response->GetContent();
                if (Content.Num() > 0)
                {
                    // 将图片字节解码为 Texture2D（失败返回 nullptr）
                    Texture = FImageUtils::ImportBufferAsTexture2D(Content);
                    // 本地缓存：将原始字节写入缓存文件（目录自动创建）
                    if (!CachePath.IsEmpty())
                    {
                        IFileManager::Get().MakeDirectory(*FPaths::GetPath(CachePath), true);
                        FFileHelper::SaveArrayToFile(Content, *CachePath);
                    }
                }
            }
        }

        OnResult.ExecuteIfBound(Texture != nullptr, StatusCode, Texture);
    });

    Request->ProcessRequest();
}

void UUeHttpHelper::LoadImageFromLocalFile(const FString& FilePath, FOnHttpImageResult OnResult)
{
    if (FilePath.IsEmpty() || !IFileManager::Get().FileExists(*FilePath))
    {
        OnResult.ExecuteIfBound(false, 0, nullptr);
        return;
    }

    TArray<uint8> Bytes;
    UTexture2D* Texture = nullptr;
    if (FFileHelper::LoadFileToArray(Bytes, *FilePath) && Bytes.Num() > 0)
    {
        Texture = FImageUtils::ImportBufferAsTexture2D(Bytes);
    }
    OnResult.ExecuteIfBound(Texture != nullptr, Texture != nullptr ? 200 : 0, Texture);
}

FString UUeHttpHelper::GetImageCacheDir()
{
    const FString CacheDir = FPaths::ProjectSavedDir() / TEXT("ImageCache");
    IFileManager::Get().MakeDirectory(*CacheDir, true);
    return CacheDir;
}

void UUeHttpHelper::SendRequest(const FString& Verb, const FString& Url, const TMap<FString, FString>& Headers, const FString& Body, float TimeoutSeconds, FOnHttpResult OnResult)
{
    if (Url.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UeHttpHelper] Request url is empty."));
        OnResult.ExecuteIfBound(false, 0, TEXT(""));
        return;
    }

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetVerb(Verb);
    Request->SetURL(Url);
    Request->SetTimeout(TimeoutSeconds);

    // 默认 Content-Type，用户传入同名 Header 时覆盖
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    for (const auto& Pair : Headers)
    {
        if (!Pair.Key.IsEmpty())
        {
            Request->SetHeader(Pair.Key, Pair.Value);
        }
    }

    if (!Body.IsEmpty())
    {
        Request->SetContentAsString(Body);
    }

    // 完成回调：FHttpModule 默认在 GameThread 触发，与 puerts JS 回调线程一致
    Request->OnProcessRequestComplete().BindLambda([OnResult](FHttpRequestPtr RequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
    {
        if (!OnResult.IsBound())
        {
            return;
        }

        int32 StatusCode = 0;
        FString ResponseText = TEXT("");
        bool bSuccess = false;

        if (bConnectedSuccessfully && Response.IsValid())
        {
            StatusCode = Response->GetResponseCode();
            ResponseText = Response->GetContentAsString();
            // 与 TS 侧既有判定语义一致：200-399 视为成功
            bSuccess = (StatusCode >= 200 && StatusCode < 400);
        }

        OnResult.ExecuteIfBound(bSuccess, StatusCode, ResponseText);
    });

    Request->ProcessRequest();
}
