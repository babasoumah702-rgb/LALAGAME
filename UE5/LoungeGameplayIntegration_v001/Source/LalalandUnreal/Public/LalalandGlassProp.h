#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LalalandKit.h"
#include "LalalandGlassProp.generated.h"

class UStaticMeshComponent;

UCLASS()
class LALALANDUNREAL_API ALalalandGlassProp final : public AActor
{
    GENERATED_BODY()

public:
    ALalalandGlassProp();
    virtual void Tick(float DeltaSeconds) override;
    void SetConsumed(bool bConsumed);
    void BeginSip(const FString& ActionId);
    void CancelSip();
    bool IsDrinking() const { return DrinkSeconds>0.f; }
    float GetSipProgress() const { return 1.f-DrinkSeconds/1.4f; }
    void Build(ELalalandGlassKind Kind, const FLinearColor& Liquid, float Fill = .68f);
    bool BuildNativeMix(const FString& DrinkId, float Fill);
    bool IsNativeMix() const { return NativeSurface!=nullptr; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> LiquidParts;
    TArray<FVector> LiquidScales;
    TArray<FVector> LiquidLocations;
    float VisibleFill=1.f;
    float TargetFill=1.f;
    float DrinkSeconds=0.f;
    bool bWasConsumed=false;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> NativeSurface;
    float NativeHeight=0;
    FString SipActionId;
};
