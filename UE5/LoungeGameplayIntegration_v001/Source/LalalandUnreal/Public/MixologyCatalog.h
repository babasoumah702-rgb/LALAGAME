#pragma once
#include "CoreMinimal.h"

struct FMixMaterial { FString Id, Name, Unit=TEXT("ml"); float Cost=0; bool Alcoholic=false; };
struct FMixRecipe { FString Id, Name, Kind, Method; TMap<FString,int32> Amounts; int32 CraftCost=0, OrderCost=-1; int32 Sweet=0,Sour=0,Bitter=0,Spirit=0; };
struct FMixCatalog {
 FString Language=TEXT("en");
 TArray<FMixMaterial> Materials;
 TArray<FMixRecipe> Recipes;
 bool Load(FString& Error,const FString& InLanguage=TEXT("en"));
 const FMixRecipe* Resolve(const TMap<FString,int32>& Amounts) const;
 int32 Quote(const TMap<FString,int32>& Amounts) const;
};
