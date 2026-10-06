#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Character.h"
#include "LoungeCharacterIntegrationGameMode.generated.h"

// Offline inspection pawn only: deliberately not the production player/arms.
UCLASS()
class ALoungeInspectionPawn : public ACharacter
{
    GENERATED_BODY()
public:
    ALoungeInspectionPawn();
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
    UPROPERTY() TObjectPtr<class UCameraComponent> Camera;
    void Forward(float Value);
    void Right(float Value);
    void Turn(float Value);
    void Look(float Value);
    void Exit();
    bool Modal() const;
    void Interact();
    void ConfirmMix();
    void ViewMix();
    void Sip();
    void ResetDemo();
};

UCLASS()
class ALoungeCharacterIntegrationGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ALoungeCharacterIntegrationGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TArray<TObjectPtr<class ALalalandNpcCharacter>> Characters;
    TMap<FString, ALalalandNpcCharacter*> Cast;
    TMap<FString, FVector> Anchors;
    TMap<FString, FVector> Goals;
    TMap<FString, float> GoalYaw;
    TArray<TSharedPtr<class FJsonValue>> Checks;
    TArray<TSharedPtr<class FJsonValue>> Captures;
    bool Automated=false;
    bool Ready=false;
    bool Finished=false;
    int32 CaptureIndex=0;
    int32 MotionPhase=0;
    double StartedAt=0;
    void ApplyGoal(ALalalandNpcCharacter* Character, const FVector& Ground, float Yaw);
    void SetMotionPhase(int32 Phase);
    void CheckMotionPhase(int32 Phase);
    void SetView(int32 View);
    void Finish();
};
