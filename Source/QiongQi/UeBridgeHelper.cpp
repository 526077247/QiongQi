
// Fill out your copyright notice in the Description page of Project Settings.


#include "UeBridgeHelper.h"

UUeBridgeHelper* UUeBridgeHelper::Instance = nullptr;

UUeBridgeHelper* UUeBridgeHelper::GetInstance()
{
    if (!Instance)
    {
        // 创建持久化实例
        Instance = NewObject<UUeBridgeHelper>(GetTransientPackage(), UUeBridgeHelper::StaticClass());
        Instance->AddToRoot(); // 防止垃圾回收
    }
    return Instance;
}

FVector2f UUeBridgeHelper::GetGeometryLocalSize(const FGeometry& Geometry)
{
    UE::Slate::FDeprecateVector2DResult res = Geometry.GetLocalSize();
    return res;
}

FVector2f UUeBridgeHelper::GetGeometryDrawSize(const FGeometry& Geometry)
{
    UE::Slate::FDeprecateVector2DResult res = Geometry.GetDrawSize();
    return res;
}