#include "LalalandGameMode.h"
#include "GameFramework/PlayerController.h"
#include "LalalandPlayerCharacter.h"
#include "LalalandNpcCharacter.h"
#include "LalalandStage.h"
#include "LalalandRootWidget.h"
#include "LalalandServiceSubsystem.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "EngineUtils.h"

ALalalandGameMode::ALalalandGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ALalalandPlayerCharacter::StaticClass();
}

void ALalalandGameMode::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->SpawnActor<ALalalandStage>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            const bool bStairAudit = FParse::Param(FCommandLine::Get(), TEXT("LalalandStairAudit"));
            Pawn->SetActorLocation(bStairAudit ? FVector(745.f, -625.f, 92.f) : FVector(-100.f, -865.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
            Pawn->SetActorRotation(FRotator(0.f, 90.f, 0.f));
            PC->SetControlRotation(FRotator(-4.f, 90.f, 0.f));
        }
        PC->SetShowMouseCursor(true);
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        if (ULalalandRootWidget* UI = CreateWidget<ULalalandRootWidget>(PC, ULalalandRootWidget::StaticClass())) UI->AddToViewport(100);
    }
    const bool bCaptureElevator = FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureElevator"));
    const bool bCaptureInterior = FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureInterior"));
    const bool bCaptureRoof = FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureRoof"));
    const bool bCaptureBar = FParse::Param(FCommandLine::Get(), TEXT("LalalandCapture"));
    if (bCaptureInterior || bCaptureRoof)
    {
        FTimerHandle InteriorViewTimer;
        TWeakObjectPtr<ALalalandGameMode> WeakThis(this);
        GetWorldTimerManager().SetTimer(InteriorViewTimer, [WeakThis]()
        {
            if (!WeakThis.IsValid()) return;
            if (APlayerController* TestPC = WeakThis->GetWorld()->GetFirstPlayerController())
            {
                if (APawn* Pawn = TestPC->GetPawn())
                {
                    const bool bRoof = FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureRoof"));
                    Pawn->SetActorLocation(bRoof ? FVector(130.f, 575.f, 512.f) : FVector(-100.f, -410.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Pawn->SetActorRotation(FRotator(0.f, bRoof ? 145.f : 48.f, 0.f));
                    TestPC->SetControlRotation(FRotator(bRoof ? -8.f : -5.f, bRoof ? 145.f : 48.f, 0.f));
                }
            }
        }, 10.f, false);
    }
    if (bCaptureElevator || bCaptureBar || bCaptureInterior || bCaptureRoof)
    {
        FTimerHandle ScreenshotTimer;
        TWeakObjectPtr<ALalalandGameMode> WeakThis(this);
        GetWorldTimerManager().SetTimer(ScreenshotTimer, [WeakThis]()
        {
            if (!WeakThis.IsValid()) return;
            if (APlayerController* TestPC = WeakThis->GetWorld()->GetFirstPlayerController())
            {
                FVector ViewLocation;
                FRotator ViewRotation;
                TestPC->GetPlayerViewPoint(ViewLocation, ViewRotation);
                UE_LOG(LogTemp, Log, TEXT("LALALAND_VISUAL_CAPTURE location=%s rotation=%s"), *ViewLocation.ToCompactString(), *ViewRotation.ToCompactString());
                for (TActorIterator<ALalalandNpcCharacter> It(WeakThis->GetWorld()); It; ++It)
                {
                    UE_LOG(LogTemp, Log, TEXT("LALALAND_VISUAL_NPC name=%s location=%s hidden=%d meshVisible=%d bounds=%s"),
                        *It->GetName(), *It->GetActorLocation().ToCompactString(), It->IsHidden(), It->GetMesh()->IsVisible(), *It->GetMesh()->Bounds.BoxExtent.ToCompactString());
                }
                if (FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureUnlit"))) TestPC->ConsoleCommand(TEXT("viewmode unlit"));
            }
            const bool bElevator = FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureElevator"));
            const bool bInterior = FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureInterior"));
            const bool bRoof = FParse::Param(FCommandLine::Get(), TEXT("LalalandCaptureRoof"));
            FScreenshotRequest::RequestScreenshot(bElevator ? TEXT("LalalandElevator") : bRoof ? TEXT("LalalandRoof") : bInterior ? TEXT("LalalandInterior") : TEXT("LalalandVisual"), true, false);
        }, bCaptureElevator ? 3.f : 12.f, false);
    }
    bAutomatedPlaythrough = FParse::Param(FCommandLine::Get(), TEXT("LalalandFullPlaythrough"));
    bAutomatedRouteAudit = FParse::Param(FCommandLine::Get(), TEXT("LalalandRouteAudit"));
    if (bAutomatedPlaythrough)
    {
        PlaythroughReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("LalalandPlaythrough.txt"));
        RecordPlaythrough(TEXT("START client=Shipping route=offline-first-night-v0.2"), true);
    }
    else if (bAutomatedRouteAudit)
    {
        if (ALalalandPlayerCharacter* Player = Cast<ALalalandPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
        {
            Player->SetActorLocation(FVector(-160.f, -350.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
            Player->BeginAutomatedRoofClimb();
            UE_LOG(LogTemp, Log, TEXT("LALALAND_ROOF_ROUTE_AUDIT started=1"));
        }
    }
}

void ALalalandGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bAutomatedRouteAudit)
    {
        ALalalandPlayerCharacter* Player = Cast<ALalalandPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (Player && Player->IsAutomatedRoofClimbFinished())
        {
            UE_LOG(LogTemp, Log, TEXT("LALALAND_ROOF_ROUTE_AUDIT success=%d"), Player->DidAutomatedRoofClimbSucceed() ? 1 : 0);
            bAutomatedRouteAudit = false;
            FGenericPlatformMisc::RequestExit(false);
        }
    }
    if (bAutomatedPlaythrough && !bPlaythroughFinished) RunAutomatedPlaythrough(DeltaSeconds);
}

void ALalalandGameMode::RecordPlaythrough(const FString& Message, bool bOverwrite)
{
    const FString Line = FString::Printf(TEXT("[%06.2f] %s\r\n"), PlaythroughElapsed, *Message);
    FFileHelper::SaveStringToFile(Line, *PlaythroughReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(), bOverwrite ? 0 : FILEWRITE_Append);
}

void ALalalandGameMode::FailPlaythrough(const FString& Reason)
{
    if (bPlaythroughFinished) return;
    RecordPlaythrough(TEXT("FAIL ") + Reason);
    bPlaythroughFinished = true;
    FGenericPlatformMisc::RequestExit(false);
}

void ALalalandGameMode::FocusAutomatedCameraOnBounceTable()
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    APawn* Player = PC ? PC->GetPawn() : nullptr;
    if (!PC || !Player) return;
    // This point is outside the booth and table collision while retaining a
    // clear diagonal view down the full 6.4 m table.
    const FVector PlayerLocation(-160.f, -350.f, 92.f);
    const FVector CameraLocation = PlayerLocation + FVector(0.f, 0.f, 68.f);
    const FVector LookAt(85.f, -150.f, 112.f);
    const FRotator ViewRotation = (LookAt - CameraLocation).Rotation();
    Player->SetActorLocation(PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Player->SetActorRotation(FRotator(0.f, ViewRotation.Yaw, 0.f));
    PC->SetControlRotation(ViewRotation);
}

FString ALalalandGameMode::SendPlaythroughCommand(const FString& Type, const FString& Target, const FString& Intent,
    const FString& ObjectTarget, const FString& Text, double X, double Z)
{
    ULalalandServiceSubsystem* Service = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>() : nullptr;
    if (!Service) return FString();
    FLalalandCommandDto Command;
    Command.type = Type;
    Command.target = Target;
    Command.intent = Intent;
    Command.objectTarget = ObjectTarget;
    Command.text = Text;
    Command.x = X;
    Command.z = Z;
    const FString Id = Service->SendCommand(Command);
    if (!Id.IsEmpty()) RecordPlaythrough(FString::Printf(TEXT("COMMAND type=%s target=%s intent=%s object=%s"), *Type, *Target, *Intent, *ObjectTarget));
    return Id;
}

void ALalalandGameMode::RunAutomatedPlaythrough(float DeltaSeconds)
{
    PlaythroughElapsed += DeltaSeconds;
    if (PlaythroughElapsed > 180.f) { FailPlaythrough(TEXT("timeout after 180 seconds")); return; }
    const double Now = FPlatformTime::Seconds();
    if (Now < NextPlaythroughActionAt) return;
    ULalalandServiceSubsystem* Service = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>() : nullptr;
    if (!Service || !Service->IsReady()) return;
    const FLalalandStateDto& State = Service->GetState();
    const FLalalandFirstNightDto& Night = State.firstNight;
    if (!Night.phase.IsEmpty() && Night.phase != LastLoggedPhase)
    {
        LastLoggedPhase = Night.phase;
        RecordPlaythrough(TEXT("PHASE ") + Night.phase);
    }

    if (PlaythroughStep == 0)
    {
        if (!State.sessionId.IsEmpty()) { FailPlaythrough(TEXT("audit profile unexpectedly resumed an existing session")); return; }
        Service->OpenNewSession(TEXT("passerby"), TEXT("observe_only"), TEXT("natural"), false);
        RecordPlaythrough(TEXT("SESSION requested explicit offline rules"));
        PlaythroughStep = 1;
        NextPlaythroughActionAt = Now + 1.0;
        return;
    }
    if (PlaythroughStep == 1)
    {
        if (State.sessionId.IsEmpty() || Night.phase != TEXT("arrival")) return;
        FScreenshotRequest::RequestScreenshot(TEXT("PlaythroughArrival"), true, false);
        if (SendPlaythroughCommand(TEXT("opening_ball"), FString(), TEXT("return")).IsEmpty()) return;
        PlaythroughStep = 2;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 2)
    {
        if (Night.phase != TEXT("free_time")) return;
        ALalalandNpcCharacter* Target = nullptr;
        for (TActorIterator<ALalalandNpcCharacter> It(GetWorld()); It; ++It) if (It->GetActorId() == TEXT("B")) { Target = *It; break; }
        ALalalandPlayerCharacter* Player = Cast<ALalalandPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (!Target || !Player) { FailPlaythrough(TEXT("could not locate player or B")); return; }
        Player->SetActorLocation(Target->GetActorLocation() + FVector(80.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        Player->SetActorRotation((Target->GetActorLocation() - Player->GetActorLocation()).Rotation());
        RecordPlaythrough(TEXT("MOVE approached B within conversation distance"));
        PlaythroughStep = 3;
        NextPlaythroughActionAt = Now + 1.0;
        return;
    }
    if (PlaythroughStep == 3)
    {
        if (SendPlaythroughCommand(TEXT("talk"), TEXT("B"), FString(), FString(), TEXT("今晚先看看眼前的夜景，你呢？")).IsEmpty()) return;
        PlaythroughStep = 4;
        NextPlaythroughActionAt = Now + 2.0;
        return;
    }
    if (PlaythroughStep == 4)
    {
        if (SendPlaythroughCommand(TEXT("order_drink"), FString(), FString(), TEXT("terrace_breeze")).IsEmpty()) return;
        PlaythroughStep = 5;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 5)
    {
        bool bServed = false;
        for (const FLalalandDrinkPropDto& Drink : Night.drinks) if (Drink.owner == TEXT("USER") && Drink.id == TEXT("terrace_breeze") && Drink.status == TEXT("served")) bServed = true;
        if (!bServed) return;
        if (SendPlaythroughCommand(TEXT("consume_drink"), FString(), FString(), TEXT("terrace_breeze")).IsEmpty()) return;
        RecordPlaythrough(TEXT("CHECK drink ordered and consumed through normal command path"));
        PlaythroughStep = 6;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 6)
    {
        if (Night.phase != TEXT("game_choice")) return;
        FocusAutomatedCameraOnBounceTable();
        RecordPlaythrough(TEXT("CAMERA focused on bounce table and target cup"));
        FScreenshotRequest::RequestScreenshot(TEXT("PlaythroughGameChoice"), true, false);
        if (SendPlaythroughCommand(TEXT("bounce_choice"), FString(), TEXT("join")).IsEmpty()) return;
        PlaythroughStep = 7;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 7)
    {
        // Keep the automated first-person witness on the table for every real
        // Chaos throw. This does not run during ordinary player sessions.
        FocusAutomatedCameraOnBounceTable();
        if (Night.phase == TEXT("post_game"))
        {
            RecordPlaythrough(FString::Printf(TEXT("CHECK five-person Chaos game completed winner=%s last=%s"), *Night.winner, *Night.lastPlace));
            PlaythroughStep = 8;
            NextPlaythroughActionAt = Now + .5;
            return;
        }
        if (Night.phase != TEXT("game_round")) return;
        if (!Night.pendingThrow.id.IsEmpty())
        {
            if (Night.pendingThrow.id != LastPendingThrowId)
            {
                LastPendingThrowId = Night.pendingThrow.id;
                RecordPlaythrough(FString::Printf(TEXT("PHYSICS throw actor=%s round=%d"), *Night.pendingThrow.actor, Night.pendingThrow.round));
            }
            bThrowRequested = false;
            bAimSent = false;
            return;
        }
        if (!Night.participants.IsValidIndex(Night.turn % FMath::Max(1, Night.participants.Num()))) return;
        if (Night.participants[Night.turn % Night.participants.Num()] != TEXT("USER")) return;
        if (bThrowRequested) return;
        if (!bAimSent)
        {
            if (SendPlaythroughCommand(TEXT("set_throw_aim"), FString(), FString(), FString(), FString(), .67, 0).IsEmpty()) return;
            bAimSent = true;
            NextPlaythroughActionAt = Now + .5;
            return;
        }
        if (SendPlaythroughCommand(TEXT("throw_ball"), FString(), FString(), FString(), FString(), 0, .58).IsEmpty()) return;
        bThrowRequested = true;
        NextPlaythroughActionAt = Now + .5;
        return;
    }
    if (PlaythroughStep == 8)
    {
        if (Night.phase != TEXT("post_game")) return;
        if (Night.winner == TEXT("USER")) SendPlaythroughCommand(TEXT("post_game_choice"), FString(), TEXT("reward_voucher"));
        else if (Night.lastPlace == TEXT("USER")) SendPlaythroughCommand(TEXT("post_game_choice"), FString(), TEXT("penalty_pay"));
        else RecordPlaythrough(TEXT("CHECK no player post-game debt"));
        PlaythroughStep = 9;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 9)
    {
        if (Night.phase != TEXT("meteor_window")) return;
        RecordPlaythrough(TEXT("CHECK meteor window opened without dialogue gating"));
        PlaythroughStep = 10;
        NextPlaythroughActionAt = Now + .3;
        return;
    }
    if (PlaythroughStep == 10)
    {
        if (!Night.invitationResult.IsEmpty())
        {
            RecordPlaythrough(TEXT("CHECK rooftop invitation result=") + Night.invitationResult);
            PlaythroughStep = 11;
            NextPlaythroughActionAt = Now + .3;
            return;
        }
        if (bInvitationCommandSent) return;
        const bool bNpcInvited = !Night.inviteFrom.IsEmpty();
        const FString Id = bNpcInvited
            ? SendPlaythroughCommand(TEXT("npc_invite_reply"), FString(), TEXT("accept_friend"))
            : SendPlaythroughCommand(TEXT("invite_rooftop"), TEXT("B"));
        if (!Id.IsEmpty()) bInvitationCommandSent = true;
        return;
    }
    if (PlaythroughStep == 11)
    {
        if (SendPlaythroughCommand(TEXT("go_rooftop")).IsEmpty()) return;
        PlaythroughStep = 12;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 12)
    {
        ALalalandPlayerCharacter* Player = Cast<ALalalandPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (!Player) { FailPlaythrough(TEXT("player missing before stair route")); return; }
        Player->BeginAutomatedRoofClimb();
        bRoofClimbStarted = true;
        RecordPlaythrough(TEXT("MOVE began physical CharacterMovement stair climb"));
        PlaythroughStep = 13;
        return;
    }
    if (PlaythroughStep == 13)
    {
        ALalalandPlayerCharacter* Player = Cast<ALalalandPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (!Player || !Player->IsAutomatedRoofClimbFinished()) return;
        if (!Player->DidAutomatedRoofClimbSucceed()) { FailPlaythrough(TEXT("physical stair climb failed")); return; }
        // The route succeeds on the first roof-side frame. Submit that exact
        // witness position before waiting for the authoritative phase so the
        // audit does not depend on the 200 ms periodic reporter's timing.
        Player->ReportPositionNow();
        RecordPlaythrough(TEXT("CHECK physical stair climb succeeded"));
        PlaythroughStep = 14;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 14)
    {
        if (Night.phase != TEXT("rooftop")) return;
        FScreenshotRequest::RequestScreenshot(TEXT("PlaythroughRooftop"), true, false);
        RecordPlaythrough(TEXT("CHECK authoritative state reached rooftop"));
        PlaythroughStep = 15;
        NextPlaythroughActionAt = Now + 2.0;
        return;
    }
    if (PlaythroughStep == 15)
    {
        if (SendPlaythroughCommand(TEXT("end_first_night")).IsEmpty()) return;
        PlaythroughStep = 16;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 16 && Night.phase == TEXT("settled"))
    {
        RecordPlaythrough(FString::Printf(TEXT("SUCCESS ending=%s cash=%d keyActions=%d evaluations=%d"),
            *Night.ending, Night.availableCash, Night.keyActions.Num(), Night.evaluations.Num()));
        bPlaythroughFinished = true;
        FGenericPlatformMisc::RequestExit(false);
    }
}
