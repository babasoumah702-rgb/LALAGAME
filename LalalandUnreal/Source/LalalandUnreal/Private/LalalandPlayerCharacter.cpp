#include "LalalandPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "LalalandServiceSubsystem.h"
#include "Components/CapsuleComponent.h"

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

void ALalalandPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    bStairPhysicsAudit = FParse::Param(FCommandLine::Get(), TEXT("LalalandStairAudit"));
#endif
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
    UpdateIntoxication(DeltaSeconds);
    if (bAutomatedRoofClimb && !bAutomatedRoofClimbFinished)
    {
        AutomatedRoofClimbElapsed += DeltaSeconds;
        const FVector Position = GetActorLocation();
        const FVector Route[] = {
            FVector(500.f, -420.f, 92.f),
            // Follow the west side of the staircase to its south entrance.
            // Crossing directly to X=745 here would hit the side of a 96 cm step.
            FVector(630.f, -420.f, 92.f),
            FVector(630.f, -625.f, 92.f),
            FVector(745.f, -625.f, 92.f)
        };
        if (AutomatedRoofRouteStep < UE_ARRAY_COUNT(Route))
        {
            FVector Delta = Route[AutomatedRoofRouteStep] - Position;
            Delta.Z = 0.f;
            if (Delta.Size2D() < 38.f)
            {
                UE_LOG(LogTemp, Log, TEXT("LALALAND_ROOF_ROUTE waypoint=%d location=%s"), AutomatedRoofRouteStep, *Position.ToCompactString());
                ++AutomatedRoofRouteStep;
            }
            else
            {
                const FVector Direction = Delta.GetSafeNormal2D();
                SetActorRotation(Direction.Rotation());
                AddMovementInput(Direction, 1.f, true);
            }
        }
        else
        {
            SetActorRotation(FRotator(0.f, 90.f, 0.f));
            AddMovementInput(FVector::YAxisVector, 1.f, true);
        }
        if (Position.Y >= 265.f && Position.Z >= 500.f)
        {
            bAutomatedRoofClimbFinished = true;
            bAutomatedRoofClimbSucceeded = true;
            UE_LOG(LogTemp, Log, TEXT("LALALAND_ROOF_ROUTE success=1 elapsed=%.2f location=%s"), AutomatedRoofClimbElapsed, *Position.ToCompactString());
        }
        else if (AutomatedRoofClimbElapsed >= 30.f)
        {
            bAutomatedRoofClimbFinished = true;
            UE_LOG(LogTemp, Error, TEXT("LALALAND_ROOF_ROUTE success=0 elapsed=%.2f step=%d location=%s"), AutomatedRoofClimbElapsed, AutomatedRoofRouteStep, *Position.ToCompactString());
            if (AutomatedRoofRouteStep < UE_ARRAY_COUNT(Route))
            {
                FHitResult Hit;
                FCollisionQueryParams Params(SCENE_QUERY_STAT(LalalandRoofRoute), false, this);
                const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Position, Route[AutomatedRoofRouteStep], FQuat::Identity,
                    ECC_Pawn, FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Params);
                UE_LOG(LogTemp, Error, TEXT("LALALAND_ROOF_ROUTE blocker hit=%d actor=%s component=%s impact=%s"), bHit ? 1 : 0,
                    Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("none"),
                    Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("none"), *Hit.ImpactPoint.ToCompactString());
                if (Hit.GetActor()) UE_LOG(LogTemp, Error, TEXT("LALALAND_ROOF_ROUTE blockerTransform location=%s scale=%s bounds=%s"),
                    *Hit.GetActor()->GetActorLocation().ToCompactString(), *Hit.GetActor()->GetActorScale3D().ToCompactString(),
                    *Hit.GetActor()->GetComponentsBoundingBox(true).GetExtent().ToCompactString());
            }
        }
    }
#if !UE_BUILD_SHIPPING
    if (bStairPhysicsAudit && !bStairAuditFinished)
    {
        StairAuditElapsed += DeltaSeconds;
        AddMovementInput(FVector::YAxisVector, 1.f, true);
        const FVector Position = GetActorLocation();
        if (Position.Y >= 265.f && Position.Z >= 500.f)
        {
            bStairAuditFinished = true;
            UE_LOG(LogTemp, Log, TEXT("LALALAND_STAIR_AUDIT success=1 elapsed=%.2f location=%s"), StairAuditElapsed, *Position.ToCompactString());
        }
        else if (StairAuditElapsed >= 18.f)
        {
            bStairAuditFinished = true;
            UE_LOG(LogTemp, Error, TEXT("LALALAND_STAIR_AUDIT success=0 elapsed=%.2f location=%s"), StairAuditElapsed, *Position.ToCompactString());
        }
    }
#endif
}

void ALalalandPlayerCharacter::BeginAutomatedRoofClimb()
{
    // Start wherever the player currently is. The audit walks the same physical
    // bar -> corridor -> stair route that a player must use, so the server's
    // anti-teleport position validation remains active throughout.
    bAutomatedRoofClimb = true;
    bAutomatedRoofClimbFinished = false;
    bAutomatedRoofClimbSucceeded = false;
    AutomatedRoofClimbElapsed = 0.f;
    AutomatedRoofRouteStep = 0;
}

void ALalalandPlayerCharacter::ReportPositionNow()
{
    ReportPosition();
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
    if (P.Z > 380.f && P.Y >= 230.f) Command.area = TEXT("rooftop");
    else if (P.X >= 680.f && P.Y >= -585.f && P.Y < 260.f) Command.area = TEXT("stairs");
    else if (P.Y < -518.f) Command.area = TEXT("corridor");
    else Command.area = TEXT("bar");
    Command.x = P.X / 100.0;
    const float FloorZ = P.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Command.y = FloorZ / 100.0;
    Command.z = P.Y / 100.0;
    Command.yaw = GetActorRotation().Yaw;
    Service->SendCommand(Command);
}

void ALalalandPlayerCharacter::UpdateIntoxication(float DeltaSeconds)
{
    UGameInstance* Instance = GetGameInstance();
    ULalalandServiceSubsystem* Service = Instance ? Instance->GetSubsystem<ULalalandServiceSubsystem>() : nullptr;
    float Target = 0.f;
    if (Service)
    {
        const FString Stage = Service->GetState().firstNight.playerDrinkStage;
        const FString Fx = Service->GetState().firstNight.intoxFx;
        const float Scale = Fx == TEXT("off") ? 0.f : Fx == TEXT("low") ? .35f : 1.f;
        const float StageValue = Stage == TEXT("impaired") ? .42f : Stage == TEXT("light") ? .16f : 0.f;
        Target = StageValue * Scale;
    }
    IntoxicationAlpha = FMath::FInterpTo(IntoxicationAlpha, Target, DeltaSeconds, .55f);
    FirstPersonCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
    FirstPersonCamera->PostProcessSettings.SceneFringeIntensity = IntoxicationAlpha * 1.6f;
    FirstPersonCamera->PostProcessSettings.bOverride_VignetteIntensity = true;
    FirstPersonCamera->PostProcessSettings.VignetteIntensity = IntoxicationAlpha * .35f;
    FirstPersonCamera->SetFieldOfView(90.f + IntoxicationAlpha * 6.f);
}
