// Fill out your copyright notice in the Description page of Project Settings.

#include "QiongQiConfigLoader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
	FString GetConfigDir()
	{
		return FPaths::ProjectContentDir() / TEXT("AssetsPackage/Config");
	}
}

FString UQiongQiConfigLoader::GetConfigJsonFileNames()
{
	TArray<FString> Result;
	const FString ConfigDir = GetConfigDir();
	IFileManager::Get().FindFiles(Result, *(ConfigDir / TEXT("*.json")), true, false);
	UE_LOG(LogTemp, Warning, TEXT("[ConfigLoader] FindFiles found %d files in %s"), Result.Num(), *ConfigDir);
	TArray<FString> Names;
	for (FString& Name : Result)
	{
		Names.Add(FPaths::GetBaseFilename(FPaths::GetCleanFilename(Name)));
	}
	return FString::Join(Names, TEXT("|"));
}

FString UQiongQiConfigLoader::LoadConfigJson(const FString& FileName)
{
	FString Content;
	const FString FilePath = GetConfigDir() / (FileName + TEXT(".json"));
	FFileHelper::LoadFileToString(Content, *FilePath);
	return Content;
}
