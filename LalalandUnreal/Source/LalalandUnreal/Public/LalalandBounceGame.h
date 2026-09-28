#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LalalandBounceGame.generated.h"

class UBoxComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class ULalalandServiceSubsystem;

UCLASS()
class LALALANDUNREAL_API ALalalandBounceGame final : public AActor
{
    GENERATED_BODY()

public:
    ALalalandBounceGame();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UFUNCTION() void RefreshFromService();
    UFUNCTION() void HandleCommandAcknowledged(const FString& CommandId, const FString& Reason);
    UFUNCTION() void HandleCommandRejected(const FString& CommandId, const FString& Reason);
    UFUNCTION() void OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse, const FHitResult& Hit);
    UFUNCTION() void OnCupEntered(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    void LaunchPendingThrow();
    void LaunchPhysicsAuditThrow();
    void FinishPhysicsAudit(bool bEnteredCup);
    void ReportResult(bool bEnteredCup);
    void ApplyCupAssist();
    void Tint(UStaticMeshComponent* Component, const FLinearColor& Color, float Roughness = .65f);

    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> CupRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TableSurface;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> GameBall;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> BallGlow;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> OpeningBall;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> CupTrigger;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> CupWalls;
    UPROPERTY() TObjectPtr<ULalalandServiceSubsystem> Service;

    FString ActiveThrowId;
    FString ResultCommandId;
    float FlightSeconds = 0.f;
    float LastTableContactAt = -1.f;
    float ResultRetryRemaining = 0.f;
    int32 TableContacts = 0;
    bool bPendingCupResult = false;
    bool bCupEntered = false;
    bool bReported = false;
    bool bOpeningRolled = false;
    bool bCupAssisted = false;
    bool bLocalPhysicsAudit = false;
    bool bCapturedBallInFlight = false;
};
