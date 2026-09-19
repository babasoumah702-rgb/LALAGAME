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
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void BeginLook();
    void EndLook();
    void TogglePauseMenu();
    void ReportPosition();

    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FirstPersonCamera;
    bool bLooking = false;
    float PositionReportAccumulator = 0;
};
