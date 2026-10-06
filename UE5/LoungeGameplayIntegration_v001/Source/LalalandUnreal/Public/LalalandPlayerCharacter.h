#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LalalandPlayerCharacter.generated.h"

class UCameraComponent;
class ULalalandPlayerArmsComponent;

UCLASS()
class LALALANDUNREAL_API ALalalandPlayerCharacter final : public ACharacter
{
    GENERATED_BODY()

public:
    ALalalandPlayerCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void BeginAutomatedRoofClimb();
    void ReportPositionNow();
    bool IsAutomatedRoofClimbFinished() const { return bAutomatedRoofClimbFinished; }
    bool DidAutomatedRoofClimbSucceed() const { return bAutomatedRoofClimbSucceeded; }
    UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
    ULalalandPlayerArmsComponent* GetPlayerArmsComponent() const { return PlayerArms; }

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void BeginLook();
    void EndLook();
    void TogglePauseMenu();
    void InteractAtBar();
    void ConfirmAtBar();
    void ToggleBarView();
    void ReportPosition();
    void UpdateIntoxication(float DeltaSeconds);
    void FollowRequestedRoute(float DeltaSeconds);
    void CancelRequestedRoute();

    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FirstPersonCamera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<ULalalandPlayerArmsComponent> PlayerArms;
    bool bLooking = false;
    float PositionReportAccumulator = 0;
    float IntoxicationAlpha = 0.f;
    bool bStairPhysicsAudit = false;
    bool bStairAuditFinished = false;
    float StairAuditElapsed = 0.f;
    bool bAutomatedRoofClimb = false;
    bool bAutomatedRoofClimbFinished = false;
    bool bAutomatedRoofClimbSucceeded = false;
    float AutomatedRoofClimbElapsed = 0.f;
    int32 AutomatedRoofRouteStep = 0;
    float ManualMoveUntil = 0.f;
};
