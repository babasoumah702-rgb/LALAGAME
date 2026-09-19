#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LalalandStage.generated.h"

class ALalalandNpcCharacter;
class AStaticMeshActor;
class UAudioComponent;
class UBoxComponent;
class ULalalandServiceSubsystem;
class UMaterialInterface;

UCLASS()
class LALALANDUNREAL_API ALalalandStage final : public AActor
{
    GENERATED_BODY()

public:
    ALalalandStage();
    virtual void BeginPlay() override;

private:
    UFUNCTION() void RefreshFromService();
    AStaticMeshActor* AddBox(const FString& Name, const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bCollision = true);
    AStaticMeshActor* AddCylinder(const FString& Name, const FVector& Location, const FVector& Scale, const FLinearColor& Color);
    void ApplyMaterial(AStaticMeshActor* Actor, const TCHAR* AssetPath);
    void BuildArchitecture();
    void BuildLighting();
    void SpawnCast();

    UPROPERTY() TObjectPtr<ULalalandServiceSubsystem> Service;
    UPROPERTY() TMap<FString, TObjectPtr<ALalalandNpcCharacter>> Cast;
    UPROPERTY() TObjectPtr<AStaticMeshActor> ThirdDrink;
    UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> ElevatorDoors;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> AmbientAudio;
    UPROPERTY() TArray<TObjectPtr<UBoxComponent>> ArchitectureColliders;
    TSet<FString> DisplayedEvents;
    FString CurrentSessionId;
    bool bBarEntranceOpened = false;
};
