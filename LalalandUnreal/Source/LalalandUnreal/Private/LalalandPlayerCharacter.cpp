#include "LalalandPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "LalalandServiceSubsystem.h"

ALalalandPlayerCharacter::ALalalandPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);
    GetCharacterMovement()->MaxWalkSpeed = 320.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1200.f;
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0, 0, 68.f));
    FirstPersonCamera->bUsePawnControlRotation = true;
    bUseControllerRotationYaw = true;
}

void ALalalandPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    PositionReportAccumulator += DeltaSeconds;
    if (PositionReportAccumulator >= 0.2f)
    {
        PositionReportAccumulator = 0;
        ReportPosition();
    }
}

void ALalalandPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ALalalandPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ALalalandPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ALalalandPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ALalalandPlayerCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("LookHold"), IE_Pressed, this, &ALalalandPlayerCharacter::BeginLook);
    PlayerInputComponent->BindAction(TEXT("LookHold"), IE_Released, this, &ALalalandPlayerCharacter::EndLook);
    PlayerInputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &ALalalandPlayerCharacter::TogglePauseMenu);
}

void ALalalandPlayerCharacter::MoveForward(float Value)
{
    if (!FMath::IsNearlyZero(Value)) AddMovementInput(GetActorForwardVector(), Value);
}

void ALalalandPlayerCharacter::MoveRight(float Value)
{
    if (!FMath::IsNearlyZero(Value)) AddMovementInput(GetActorRightVector(), Value);
}

void ALalalandPlayerCharacter::Turn(float Value)
{
    if (bLooking) AddControllerYawInput(Value);
}

void ALalalandPlayerCharacter::LookUp(float Value)
{
    if (bLooking) AddControllerPitchInput(Value);
}

void ALalalandPlayerCharacter::BeginLook()
{
    bLooking = true;
    if (APlayerController* PC = Cast<APlayerController>(Controller)) PC->SetShowMouseCursor(false);
}

void ALalalandPlayerCharacter::EndLook()
{
    bLooking = false;
    if (APlayerController* PC = Cast<APlayerController>(Controller)) PC->SetShowMouseCursor(true);
}

void ALalalandPlayerCharacter::TogglePauseMenu()
{
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (ULalalandServiceSubsystem* Service = Instance->GetSubsystem<ULalalandServiceSubsystem>())
        {
            FLalalandCommandDto Command;
            Command.type = TEXT("pause");
            Command.paused = !Service->GetState().paused;
            Service->SendCommand(Command);
        }
    }
}

void ALalalandPlayerCharacter::ReportPosition()
{
    if (!HasActorBegunPlay()) return;
    UGameInstance* Instance = GetGameInstance();
    ULalalandServiceSubsystem* Service = Instance ? Instance->GetSubsystem<ULalalandServiceSubsystem>() : nullptr;
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    const FVector P = GetActorLocation();
    FLalalandCommandDto Command;
    Command.type = TEXT("position");
    Command.actor = TEXT("USER");
    Command.area = TEXT("bar");
    Command.x = P.X / 100.0;
    Command.y = P.Z / 100.0;
    Command.z = P.Y / 100.0;
    Command.yaw = GetActorRotation().Yaw;
    Service->SendCommand(Command);
}
