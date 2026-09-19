#include "LalalandGameMode.h"
#include "GameFramework/PlayerController.h"
#include "LalalandPlayerCharacter.h"
#include "LalalandNpcCharacter.h"
#include "LalalandStage.h"
#include "LalalandRootWidget.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "EngineUtils.h"

ALalalandGameMode::ALalalandGameMode()
{
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
            Pawn->SetActorLocation(FVector(-100.f, -865.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
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
    const bool bCaptureBar = FParse::Param(FCommandLine::Get(), TEXT("LalalandCapture"));
    if (bCaptureInterior)
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
                    Pawn->SetActorLocation(FVector(-100.f, -410.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
                    Pawn->SetActorRotation(FRotator(0.f, 48.f, 0.f));
                    TestPC->SetControlRotation(FRotator(-5.f, 48.f, 0.f));
                }
            }
        }, 10.f, false);
    }
    if (bCaptureElevator || bCaptureBar || bCaptureInterior)
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
            FScreenshotRequest::RequestScreenshot(bElevator ? TEXT("LalalandElevator") : bInterior ? TEXT("LalalandInterior") : TEXT("LalalandVisual"), true, false);
        }, bCaptureElevator ? 3.f : 12.f, false);
    }
}
