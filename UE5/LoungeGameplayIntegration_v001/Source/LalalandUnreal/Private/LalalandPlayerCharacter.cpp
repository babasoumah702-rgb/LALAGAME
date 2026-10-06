#include "LalalandPlayerCharacter.h"
#include "LalalandPlayerArmsComponent.h"
#include "LalalandSpatial.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "LalalandServiceSubsystem.h"
#include "MixologyGameMode.h"
#include "InputCoreTypes.h"
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
    PlayerArms=CreateDefaultSubobject<ULalalandPlayerArmsComponent>(TEXT("PlayerArms"));
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
    if(const auto* Service=GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>())
        for(const auto& Actor:Service->GetState().characters) if(Actor.id==TEXT("USER"))
        {
            const FVector SeatCamera=Actor.posture==TEXT("sit")?FVector(-25,0,18):FVector(0,0,68);
            FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(FirstPersonCamera->GetRelativeLocation(),SeatCamera,DeltaSeconds,5.f));break;
        }
    if (!bAutomatedRoofClimb) FollowRequestedRoute(DeltaSeconds);
    if (bAutomatedRoofClimb && !bAutomatedRoofClimbFinished)
    {
        AutomatedRoofClimbElapsed += DeltaSeconds;
        const FVector Position = GetActorLocation();
        const FVector Route[] = {
            FVector(400.f, -420.f, 92.f),
            // Follow the west side of the staircase to its south entrance.
            // Crossing directly to X=745 here would hit the side of a 96 cm step.
            FVector(400.f, -420.f, 92.f),
            FVector(400.f, LalalandSpatial::StairBottom(), 92.f),
            FVector(745.f, LalalandSpatial::StairBottom(), 92.f)
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
        if (Position.Y >= 265.f && Position.Z >= LalalandSpatial::Roof()+80.f)
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
        if (Position.Y >= 265.f && Position.Z >= LalalandSpatial::Roof()+80.f)
        {
            bStairAuditFinished = true;
            UE_LOG(LogTemp, Log, TEXT("LALALAND_STAIR_AUDIT success=1 elapsed=%.2f location=%s"), StairAuditElapsed, *Position.ToCompactString());
            FPlatformMisc::RequestExit(false);
        }
        else if (StairAuditElapsed >= 18.f)
        {
            bStairAuditFinished = true;
            UE_LOG(LogTemp, Error, TEXT("LALALAND_STAIR_AUDIT success=0 elapsed=%.2f location=%s"), StairAuditElapsed, *Position.ToCompactString());
            FPlatformMisc::RequestExit(false);
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
    PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ALalalandPlayerCharacter::InteractAtBar);
    PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ALalalandPlayerCharacter::ConfirmAtBar);
    PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &ALalalandPlayerCharacter::ToggleBarView);
}

void ALalalandPlayerCharacter::InteractAtBar()
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto HUD=Cast<AMixologyHUD>(PC->GetHUD()))HUD->InteractAtBar();
}

void ALalalandPlayerCharacter::ConfirmAtBar()
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))
        if(H->Active&&!H->EntryMenu){UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_KEY Enter"));H->Confirm();}
}

void ALalalandPlayerCharacter::ToggleBarView()
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))
        if(H->Active&&!H->EntryMenu&&!H->Making){H->BeautyMode=!H->BeautyMode;UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_KEY V"));}
}

void ALalalandPlayerCharacter::MoveForward(float Value)
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))if(H->Active)return;
    const auto* Service=GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
    if(!Service || Service->GetState().paused || Service->GetState().sessionId.IsEmpty() || Service->GetState().intro.phase==TEXT("elevator"))return;
    if (!FMath::IsNearlyZero(Value))
    {
        CancelRequestedRoute();FVector Direction=GetActorForwardVector()*Value;
        const auto& Night=Service->GetState().firstNight;
        const bool Turn=Night.openingBall==TEXT("practice") || (Night.phase==TEXT("game_round") && Night.gameChoice==TEXT("join") && Night.participants.Num()>0 && Night.participants[Night.turn%Night.participants.Num()]==TEXT("USER"));
        if(Turn && GetActorLocation().X>=-320 && FMath::Abs(GetActorLocation().Y+150)<75 && Direction.X>0)Direction.X=0;
        AddMovementInput(Direction,1.f);
    }
}

void ALalalandPlayerCharacter::MoveRight(float Value)
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))if(H->Active)return;
    const auto* Service=GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
    if(!Service || Service->GetState().paused || Service->GetState().sessionId.IsEmpty() || Service->GetState().intro.phase==TEXT("elevator"))return;
    if (!FMath::IsNearlyZero(Value))
    {
        CancelRequestedRoute();FVector Direction=GetActorRightVector()*Value;
        const auto& Night=Service->GetState().firstNight;
        const bool Turn=Night.openingBall==TEXT("practice") || (Night.phase==TEXT("game_round") && Night.gameChoice==TEXT("join") && Night.participants.Num()>0 && Night.participants[Night.turn%Night.participants.Num()]==TEXT("USER"));
        if(Turn && GetActorLocation().X>=-320 && FMath::Abs(GetActorLocation().Y+150)<75 && Direction.X>0)Direction.X=0;
        AddMovementInput(Direction,1.f);
    }
}

void ALalalandPlayerCharacter::CancelRequestedRoute()
{
    GetCharacterMovement()->MaxWalkSpeed = 320.f;
    if (GetWorld()->GetTimeSeconds() < ManualMoveUntil) return;
    ManualMoveUntil = GetWorld()->GetTimeSeconds() + .5f;
    if (ULalalandServiceSubsystem* Service = GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>())
        for (const FLalalandActorDto& Actor : Service->GetState().characters)
            if (Actor.id == TEXT("USER") && (Actor.route.Num() || !Actor.conversationTarget.IsEmpty() || Actor.posture==TEXT("sit")))
            { FLalalandCommandDto Command; Command.type = TEXT("cancel_move"); Service->SendCommand(Command); break; }
}

void ALalalandPlayerCharacter::FollowRequestedRoute(float DeltaSeconds)
{
    ULalalandServiceSubsystem* Service = GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
    if (!Service || Service->GetState().paused || GetWorld()->GetTimeSeconds() < ManualMoveUntil) return;
    for (const FLalalandActorDto& Actor : Service->GetState().characters)
    {
        if (Actor.id != TEXT("USER") || !Actor.route.Num()) continue;
        const FLalalandPointDto& Point = Actor.route[0];
        const FVector Delta = FVector(Point.x * 100, Point.z * 100, GetActorLocation().Z) - GetActorLocation();
        GetCharacterMovement()->MaxWalkSpeed = 135.f;
        if (Delta.Size2D() > 12.f) AddMovementInput(Delta.GetSafeNormal2D(), 1.f);
        if (!bLooking && Controller) Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), FRotator(-4.f,Delta.Rotation().Yaw,0),DeltaSeconds,2.f));
        return;
    }
}

void ALalalandPlayerCharacter::Turn(float Value)
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))if(H->Active)return;
    if (bLooking) AddControllerYawInput(Value);
}

void ALalalandPlayerCharacter::LookUp(float Value)
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))if(H->Active)return;
    if (bLooking) AddControllerPitchInput(Value);
}

void ALalalandPlayerCharacter::BeginLook()
{
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))if(H->Active)return;
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
    if(auto PC=Cast<APlayerController>(GetController()))if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))
        if(H->Active){UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_KEY Escape"));H->ReturnToBar();return;}
    // The station owns Escape while crafting; do not issue a conflicting pause command.
    if(const auto* Service=GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>())
        if(Service->GetState().firstNight.craft.open)return;
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
    if (P.Z > LalalandSpatial::Roof()+40.f && P.Y >= 230.f) Command.area = TEXT("rooftop");
    else if (P.X >= 680.f && P.Y >= LalalandSpatial::StairBottom() && P.Y < 260.f) Command.area = TEXT("stairs");
    else if (P.Y < -518.f) Command.area = TEXT("corridor");
    else Command.area = TEXT("bar");
    Command.x = P.X / 100.0;
    const float FloorZ = P.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Command.y = FloorZ / 100.0;
    Command.z = P.Y / 100.0;
    Command.yaw = 90.f - GetActorRotation().Yaw;
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
