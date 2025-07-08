
#include "QiongQiPlayerPrefs.h"
#include "Kismet/GameplayStatics.h"

UQiongQiSaveGame* UQiongQiPlayerPrefs::data = nullptr;

UQiongQiSaveGame* UQiongQiPlayerPrefs::LoadSaveGame()
{
    data = Cast<UQiongQiSaveGame>(UGameplayStatics::LoadGameFromSlot(UQiongQiSaveGame::SlotName, 0));
    if (data == nullptr)
    {
        data = Cast<UQiongQiSaveGame>(UGameplayStatics::CreateSaveGameObject(UQiongQiSaveGame::StaticClass()));
    }
    return data;
}

void UQiongQiPlayerPrefs::SaveData()
{
    if (data == nullptr) return;
    UGameplayStatics::SaveGameToSlot(data, UQiongQiSaveGame::SlotName, 0);
}

void UQiongQiPlayerPrefs::SetString(const FString& Key, const FString& Value)
{
    if (data == nullptr) return;
    data->StringData.Add(Key, Value);
}

FString UQiongQiPlayerPrefs::GetString(const FString& Key, const FString& DefaultValue)
{
    if (data == nullptr) return DefaultValue;
    if (const FString* Value = data->StringData.Find(Key))
    {
        return *Value;
    }
    return DefaultValue;
}

void UQiongQiPlayerPrefs::SetFloat(const FString& Key,float Value)
{
    if (data == nullptr) return;
    data->FloatData.Add(Key, Value);
}

float UQiongQiPlayerPrefs::GetFloat(const FString& Key, float DefaultValue)
{
    if (data == nullptr) return DefaultValue;
    if (const float* Value = data->FloatData.Find(Key))
    {
        return *Value;
    }
    return DefaultValue;
}

void UQiongQiPlayerPrefs::SetInt(const FString& Key, int32 Value)
{
    if (data == nullptr) return;
    data->IntData.Add(Key, Value);
}

int32 UQiongQiPlayerPrefs::GetInt(const FString& Key, int32 DefaultValue)
{
    if (data == nullptr) return DefaultValue;
    if (const int32* Value = data->IntData.Find(Key))
    {
        return *Value;
    }
    return DefaultValue;
}


void UQiongQiPlayerPrefs::SetBool(const FString& Key, bool Value)
{
    if (data == nullptr) return;
    data->BoolData.Add(Key, Value);
}

bool UQiongQiPlayerPrefs::GetBool(const FString& Key, bool DefaultValue)
{
    if (data == nullptr) return DefaultValue;
    if (const bool* Value = data->BoolData.Find(Key))
    {
        return *Value;
    }
    return DefaultValue;
}

void UQiongQiPlayerPrefs::Delete(const FString& Key)
{
    if (data == nullptr) return;
    data->StringData.Remove(Key);
    data->FloatData.Remove(Key);
    data->IntData.Remove(Key);
    data->BoolData.Remove(Key);
}
