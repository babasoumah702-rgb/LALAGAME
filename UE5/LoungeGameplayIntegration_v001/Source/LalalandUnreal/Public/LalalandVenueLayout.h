#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

// Shared with the Node service. Coordinates in the staged file are service metres.
// Do not silently fall back to the old venue if staging omitted this file.
namespace LalalandVenue
{
inline const TSharedPtr<FJsonObject>& Layout()
{
    static const TSharedPtr<FJsonObject> Data=[]()
    {
        FString Text; TSharedPtr<FJsonObject> Result;
        const FString Path=FPaths::ProjectContentDir()/TEXT("Server/scenarios/venue-layout-r1.json");
        const bool bLoaded=FFileHelper::LoadFileToString(Text,*Path)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Result) && Result.IsValid();
        if(!bLoaded)UE_LOG(LogTemp,Fatal,TEXT("Missing or invalid shared venue layout: %s"),*Path);
        if(Result->GetStringField(TEXT("version"))!=TEXT("future-lounge-448-v1"))UE_LOG(LogTemp,Fatal,TEXT("Unexpected venue layout version"));
        return Result;
    }();
    return Data;
}
inline FVector Point(const TSharedPtr<FJsonObject>& Object,float Height=0)
{
    return FVector(Object->GetNumberField(TEXT("x"))*100.f,Object->GetNumberField(TEXT("z"))*100.f,Height);
}
inline FVector Named(const TCHAR* Name,float Height=0){return Point(Layout()->GetObjectField(Name),Height);}
inline FVector Bar(const FVector& P){return P+Named(TEXT("barOffset"));}
inline FVector Bounce(const FVector& P){return P+Named(TEXT("bounceOffset"));}
inline FVector Counter(int32 Slot)
{
    const auto P=Layout()->GetObjectField(TEXT("counter"));
    return Point(P,117.f)+FVector((Slot%12)*P->GetNumberField(TEXT("spacing"))*100.f,-(Slot/12)*10.f,0);
}
}
