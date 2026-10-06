#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LalalandStage.generated.h"

class ALalalandNpcCharacter;
class ALalalandBounceGame;
class AStaticMeshActor;
class UAudioComponent;
class UBoxComponent;
class ULalalandServiceSubsystem;
class UMaterialInterface;
class USoundBase;
struct FLalalandEventDto;

UCLASS()
class LALALANDUNREAL_API ALalalandStage final : public AActor
{
    GENERATED_BODY()

public:
    ALalalandStage();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UFUNCTION() void RefreshFromService();
    void ApplyEntranceDoors();
    AStaticMeshActor* AddBox(const FString& Name, const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bCollision = true);
    AStaticMeshActor* AddCylinder(const FString& Name, const FVector& Location, const FVector& Scale, const FLinearColor& Color);
    void ApplyMaterial(AStaticMeshActor* Actor, const TCHAR* AssetPath);
    void BuildArchitecture();
    void BuildBarDetails();
    void BuildLighting();
    void BuildFutureArchitecture();
    void BuildFutureDetails();
    void BuildFutureLighting();
    void BuildFutureAtmosphere();
    void BuildFutureElevator();
    void BuildFutureEntrance();
    void StartSpatialReview();
    void StartPlayerGripReview();
    void StartElevatorReview();
    void StartLayoutPreview();
    void SpawnCast();
    void UpdateMeteors(float DeltaSeconds);
    void UpdateDrinkProps();
    void UpdateAudio();
    void PlayEventAudio(const FLalalandEventDto& Event);

    UPROPERTY() TObjectPtr<ULalalandServiceSubsystem> Service;
    UPROPERTY() TMap<FString, TObjectPtr<ALalalandNpcCharacter>> Cast;
    UPROPERTY() TObjectPtr<ALalalandBounceGame> BounceGame;
    UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> ElevatorDoors;
    UPROPERTY() TArray<TObjectPtr<AStaticMeshActor>> Meteors;
    UPROPERTY() TMap<FString, TObjectPtr<AActor>> DrinkProps;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> AmbientAudio;
    UPROPERTY() TObjectPtr<USoundBase> ArrivalSound;
    UPROPERTY() TObjectPtr<USoundBase> CupSound;
    UPROPERTY() TObjectPtr<USoundBase> DoorSound;
    UPROPERTY() TObjectPtr<USoundBase> ElevatorSound;
    UPROPERTY() TObjectPtr<USoundBase> LoungeSound;
    UPROPERTY() TObjectPtr<USoundBase> PhoneSound;
    UPROPERTY() TArray<TObjectPtr<UBoxComponent>> ArchitectureColliders;
    TSet<FString> DisplayedEvents;
    TSet<FString> SubmittedSipEffects;
    TSet<FString> SubmittedDeliveryEffects;
    FString CurrentSessionId;
    FString LastAudioPhase;
    FString LastSongChoice;
    bool bBarEntranceOpened = false;
    bool bDoorStateInitialized = false;
    float MeteorSpawnRemaining = 0.f;
    float PositionReportRemaining = 0.f;
};
