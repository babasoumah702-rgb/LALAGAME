#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LalalandPlayerCharacter.generated.h"

class UCameraComponent;

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

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void BeginLook();
    void EndLook();
    void TogglePauseMenu();
    void ReportPosition();
    void UpdateIntoxication(float DeltaSeconds);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FirstPersonCamera;
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
};
