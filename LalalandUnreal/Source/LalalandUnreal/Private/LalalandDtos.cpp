#include "LalalandDtos.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool ReadObject(const FString& Json, TSharedPtr<FJsonObject>& Out, FString& Error)
    {
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
        if (!FJsonSerializer::Deserialize(Reader, Out) || !Out.IsValid())
        {
            Error = TEXT("返回内容不是有效 JSON。");
            return false;
        }
        return true;
    }
}

bool FLalalandJson::ParseBootstrap(const FString& Json, FLalalandBootstrapDto& Out, FString& Error)
{
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Out, 0, 0))
    {
        Error = TEXT("入口配置格式不兼容。");
        return false;
    }
    return Out.version == 1;
}

bool FLalalandJson::ParseStateEnvelope(const FString& Json, FLalalandStateDto& Out, FString& Error)
{
    TSharedPtr<FJsonObject> Root;
    if (!ReadObject(Json, Root, Error)) return false;
    const TSharedPtr<FJsonObject>* StateObject = nullptr;
    if (!Root->TryGetObjectField(TEXT("state"), StateObject) || !StateObject || !StateObject->IsValid())
    {
        Error = Root->GetStringField(TEXT("error"));
        if (Error.IsEmpty()) Error = TEXT("本地服务没有返回游戏状态。");
        return false;
    }
    if (!FJsonObjectConverter::JsonObjectToUStruct(StateObject->ToSharedRef(), &Out, 0, 0))
    {
        Error = TEXT("游戏状态格式不兼容。");
        return false;
    }
    return Out.version == 1 && !Out.sessionId.IsEmpty();
}

FString FLalalandJson::WriteSession(const FLalalandSessionRequest& Request)
{
    FString Json;
    FJsonObjectConverter::UStructToJsonObjectString(Request, Json, 0, 0);
    return Json;
}

FString FLalalandJson::WriteCommand(const FLalalandCommandDto& Command)
{
    FString Json;
    FJsonObjectConverter::UStructToJsonObjectString(Command, Json, 0, 0);
    return Json;
}
