#include "LalalandGameMode.h"
#include "GameFramework/PlayerController.h"
#include "LalalandCharacterReview.h"
#include "MixologyGameMode.h"
#include "LalalandPlayerCharacter.h"
#include "LalalandNpcCharacter.h"
#include "LalalandStage.h"
#include "LalalandSpatial.h"
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
#include "Engine/Light.h"
#include "Components/LightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

ALalalandGameMode::ALalalandGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ALalalandPlayerCharacter::StaticClass();
    HUDClass = AMixologyHUD::StaticClass();
}

void ALalalandGameMode::BeginPlay()
{
    Super::BeginPlay();
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandPlayerArmsReview")))
    {
        StartLalalandPlayerArmsReview(GetWorld());return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandBartenderReview")))
    {
        StartLalalandBartenderReview(GetWorld());
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandCupReview")))
    {
        StartLalalandCupReview(GetWorld());
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandVenueCharacterReview")))
    {
        StartLalalandVenueCharacterReview(GetWorld());
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandStretchReview")))
    {
        StartLalalandStretchReview(GetWorld());
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandReplacementReview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandShadingReview")))
    {
        StartLalalandReplacementReview(GetWorld());
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandCharacterReview")))
    {
        StartLalalandCharacterReview(GetWorld());
        return;
    }
    GetWorld()->SpawnActor<ALalalandStage>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            const bool bStairAudit = FParse::Param(FCommandLine::Get(), TEXT("LalalandStairAudit"));
            Pawn->SetActorLocation(bStairAudit ? FVector(745.f, LalalandSpatial::StairBottom(), 92.f) : LalalandSpatial::Spawn(), false, nullptr, ETeleportType::TeleportPhysics);
            Pawn->SetActorRotation(FRotator(0.f, 90.f, 0.f));
            PC->SetControlRotation(FRotator(-4.f, 90.f, 0.f));
        }
        PC->SetShowMouseCursor(true);
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        const bool Review=FParse::Param(FCommandLine::Get(),TEXT("LalalandSpatialPreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandPlayerGripPreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandAtmospherePreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandFurniturePreview"));
        if(!Review)if (ULalalandRootWidget* UI = CreateWidget<ULalalandRootWidget>(PC, ULalalandRootWidget::StaticClass())) UI->AddToViewport(100);
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
    bNormalDeclinePlaythrough = FParse::Param(FCommandLine::Get(), TEXT("LalalandNormalDeclinePlaythrough"));
    bGiftPlaythrough = FParse::Param(FCommandLine::Get(), TEXT("LalalandGiftPlaythrough")) || FParse::Param(FCommandLine::Get(),TEXT("LalalandNpcSipPlaythrough"));
    bNormalPlaythrough = bGiftPlaythrough || bNormalDeclinePlaythrough || FParse::Param(FCommandLine::Get(), TEXT("LalalandNormalPlaythrough"));
    bAutomatedPlaythrough = bNormalPlaythrough || FParse::Param(FCommandLine::Get(), TEXT("LalalandFullPlaythrough"));
    bAutomatedRouteAudit = FParse::Param(FCommandLine::Get(), TEXT("LalalandRouteAudit"));
    if (bAutomatedPlaythrough)
    {
        PlaythroughReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("LalalandPlaythrough.txt"));
        RecordPlaythrough(bNormalPlaythrough ? TEXT("START route=normal-speed offline no-server-bypasses") : TEXT("START route=accelerated-audit offline"), true);
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
#if !UE_BUILD_SHIPPING
    FString AuditMode;
    if(FParse::Value(FCommandLine::Get(),TEXT("LalalandMaterialAudit="),AuditMode))
    {
        static bool Requested=false,Prepared=false;
        static float AuditElapsed=0;
        static int32 AuditIndex=-1;
        auto* Service=GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
        if(!Service||!Service->IsReady())return;
        if(!Requested){Requested=true;Service->OpenNewSession(TEXT("passerby"),TEXT("observe_only"),TEXT("natural"),false);return;}
        if(Service->GetState().sessionId.IsEmpty()||Service->GetState().intro.phase==TEXT("elevator"))return;
        auto* PC=GetWorld()->GetFirstPlayerController();if(!PC||!PC->GetPawn())return;
        if(!Prepared)
        {
            Prepared=true;FLalalandCommandDto Pause;Pause.type=TEXT("pause");Pause.paused=true;Service->SendCommand(Pause);
            PC->GetPawn()->SetActorEnableCollision(false);
            if(AuditMode==TEXT("neutral")||AuditMode==TEXT("gray"))
                for(TActorIterator<ALight> It(GetWorld());It;++It)It->GetLightComponent()->SetLightColor(FLinearColor::White);
            if(AuditMode==TEXT("unlit"))PC->ConsoleCommand(TEXT("viewmode unlit"));
            if(AuditMode==TEXT("gray"))for(TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)
            {
                if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/Materials/M_Tint.M_Tint")))
                {
                    auto* Gray=UMaterialInstanceDynamic::Create(Base,*It);Gray->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.5f,.5f,.5f));
                    for(int32 SlotIndex=0;SlotIndex<It->GetMesh()->GetNumMaterials();++SlotIndex)It->GetMesh()->SetMaterial(SlotIndex,Gray);
                }
            }
            TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Widgets,ULalalandRootWidget::StaticClass(),true);
            for(auto* Widget:Widgets)Widget->RemoveFromParent();
        }
        AuditElapsed+=DeltaSeconds;
        const int32 NextIndex=FMath::FloorToInt(AuditElapsed/4.f);
        if(NextIndex>=4){FGenericPlatformMisc::RequestExit(false);return;}
        const FString Id=FString::Chr(TCHAR('A'+NextIndex));
        // Isolated material comparison fixture, not an ordinary gameplay screenshot.
        for(TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)It->SetActorHiddenInGame(It->GetActorId()!=Id);
        if(NextIndex!=AuditIndex)
        {
            AuditIndex=NextIndex;
            for(TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)if(It->GetActorId()==Id)
            {
                const FVector Eye=It->GetActorLocation()+It->GetActorForwardVector()*140.f+FVector(0,0,65);
                PC->GetPawn()->SetActorLocation(Eye-FVector(0,0,68));
                PC->SetControlRotation((It->GetActorLocation()+FVector(0,0,60)-Eye).Rotation());
                UE_LOG(LogTemp,Log,TEXT("F08_FIXTURE mode=%s actor=%s mesh=%s"),*AuditMode,*Id,*GetNameSafe(It->GetMesh()->GetSkeletalMeshAsset()));
            }
            FString AuditOutput=TEXT("D:/LalalandWork/Rectification-20261002/F08");
            FParse::Value(FCommandLine::Get(),TEXT("LalalandAuditOutput="),AuditOutput);
            const FString Capture=FPaths::Combine(AuditOutput,FString::Printf(TEXT("%s-%s.png"),*AuditMode,*Id));
            FTimerHandle CaptureTimer;GetWorldTimerManager().SetTimer(CaptureTimer,[Capture](){FScreenshotRequest::RequestScreenshot(Capture,false,false);},2.f,false);
        }
        return;
    }
    // Explicit visual fixture only: does not create dialogue/world events.
    if (FParse::Param(FCommandLine::Get(), TEXT("LalalandBubblePreview")) && PlaythroughElapsed > 22.f && PlaythroughElapsed < 65.f)
    {
        for (TActorIterator<ALalalandNpcCharacter> It(GetWorld()); It; ++It)
            if (It->GetActorId().Len() == 1) It->ShowDialogue(TEXT("【排版测试】") + It->GetActorId() + TEXT("：多人中文长句应该完整换行，避开人物脸部、其他气泡与底部操作区。"), 3.f);
        if (!bBubblePreviewCaptured && PlaythroughElapsed > 45.f)
        {
            bBubblePreviewCaptured = true;
            FScreenshotRequest::RequestScreenshot(TEXT("UIFixture-Multispeaker"), true, false);
        }
    }
#endif
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
    const FVector PlayerLocation = bNormalPlaythrough ? Player->GetActorLocation() : FVector(-160.f, -350.f, 92.f);
    const FVector CameraLocation = PlayerLocation + FVector(0.f, 0.f, 68.f);
    const FVector LookAt(85.f, -150.f, 112.f);
    const FRotator ViewRotation = (LookAt - CameraLocation).Rotation();
    if (!bNormalPlaythrough) Player->SetActorLocation(PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
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
    if (Type == TEXT("move_to")) Command.location = ObjectTarget;
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
    if (PlaythroughElapsed > (bGiftPlaythrough ? 120.f : bNormalPlaythrough ? 900.f : 180.f)) { FailPlaythrough(TEXT("timeout")); return; }
    if (auto* Controller=GetWorld()->GetFirstPlayerController())
        if(Controller->GetPawn()&&Controller->GetPawn()->GetActorLocation().Z < -120.f)
        { FailPlaythrough(TEXT("player fell below the physical floor"));return; }
    const double Now = FPlatformTime::Seconds();
    if (Now < NextPlaythroughActionAt) return;
    ULalalandServiceSubsystem* Service = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>() : nullptr;
    if (!Service || !Service->IsReady()) return;
    const FLalalandStateDto& State = Service->GetState();
    const FLalalandFirstNightDto& Night = State.firstNight;
    if (bNormalPlaythrough && Now >= NextMotionAuditAt && !State.sessionId.IsEmpty())
    {
        NextMotionAuditAt = Now + 15;
        for (const FLalalandActorDto& Dto : State.characters)
        {
            FVector Actual = FVector::ZeroVector;
            if (Dto.id == TEXT("USER") && GetWorld()->GetFirstPlayerController()->GetPawn()) Actual = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
            else for (TActorIterator<ALalalandNpcCharacter> It(GetWorld()); It; ++It) if (It->GetActorId() == Dto.id) { Actual = It->GetActorLocation(); break; }
            if (Dto.route.Num()) RecordPlaythrough(FString::Printf(TEXT("MOTION %s body=%s server=(%.2f,%.2f,%.2f) area=%s route=%d next=(%.2f,%.2f)"),
                *Dto.id, *Actual.ToCompactString(), Dto.x, Dto.z, Dto.y, *Dto.area, Dto.route.Num(), Dto.route[0].x, Dto.route[0].z));
        }
    }
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
        if (State.sessionId.IsEmpty() || State.intro.phase==TEXT("elevator") || Night.phase != TEXT("arrival")) return;
        FScreenshotRequest::RequestScreenshot(TEXT("PlaythroughArrival"), true, false);
        // Full-route automation does not claim to validate manual E pickup.
        if (SendPlaythroughCommand(TEXT("opening_ball"), FString(), TEXT("ignore")).IsEmpty()) return;
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
        if (bNormalPlaythrough)
        {
            SendPlaythroughCommand(TEXT("move_to"), FString(), FString(), TEXT("entrance"));
            PlaythroughStep = 18;
            return;
        }
        else Player->SetActorLocation(Target->GetActorLocation() + FVector(80.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        Player->SetActorRotation((Target->GetActorLocation() - Player->GetActorLocation()).Rotation());
        RecordPlaythrough(TEXT("MOVE approached B within conversation distance"));
        PlaythroughStep = 3;
        NextPlaythroughActionAt = Now + 1.0;
        return;
    }
    if (PlaythroughStep == 18)
    {
        const FLalalandActorDto* User = State.characters.FindByPredicate([](const FLalalandActorDto& A){return A.id == TEXT("USER");});
        if (!User || User->area != TEXT("bar")) return;
        SendPlaythroughCommand(TEXT("approach_target"), TEXT("B"));
        RecordPlaythrough(TEXT("MOVE walked out of elevator; approaching B along navigation"));
        PlaythroughStep = 3;
        return;
    }
    if (PlaythroughStep == 3)
    {
        if (bNormalPlaythrough)
        {
            const FLalalandActorDto* User = State.characters.FindByPredicate([](const FLalalandActorDto& A){return A.id == TEXT("USER");});
            const FLalalandActorDto* Target = State.characters.FindByPredicate([](const FLalalandActorDto& A){return A.id == TEXT("B");});
            if (!User || !Target || FMath::Square(User->x-Target->x)+FMath::Square(User->z-Target->z)>2.25) return;
            GetWorld()->GetFirstPlayerController()->SetControlRotation(FRotator(-4.f, FMath::RadiansToDegrees(FMath::Atan2(Target->z-User->z,Target->x-User->x)),0.f));
        }
        if (SendPlaythroughCommand(TEXT("talk"), TEXT("B"), FString(), FString(), TEXT("今晚先看看眼前的夜景，你呢？")).IsEmpty()) return;
        PlaythroughStep = 4;
        NextPlaythroughActionAt = Now + 2.0;
        return;
    }
    if (PlaythroughStep == 4)
    {
        if (SendPlaythroughCommand(TEXT("order_drink"), FString(), FString(), bGiftPlaythrough?TEXT("oolong_lemon_soda_no_syrup"):TEXT("gin_tonic_fixed")).IsEmpty()) return;
        PlaythroughStep = 5;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if (PlaythroughStep == 5)
    {
        if(bGiftPlaythrough)
        {
            for(const auto& Drink:Night.drinks)if(Drink.owner==TEXT("USER")&&Drink.status==TEXT("served")&&Drink.id==TEXT("oolong_lemon_soda_no_syrup"))
            {
                FLalalandCommandDto Offer;Offer.type=TEXT("order_drink");Offer.intent=TEXT("offer_owned");Offer.target=TEXT("B");
                Offer.objectTarget=Drink.id;Offer.requestId=Drink.instanceId;
                if(Service->SendCommand(Offer).IsEmpty())return;
                RecordPlaythrough(TEXT("CHECK owned cup proposed; no extra payment"));PlaythroughStep=19;return;
            }
            return;
        }
        bool bServed = false;
        for (const FLalalandDrinkPropDto& Drink : Night.drinks) if (Drink.owner == TEXT("USER") && Drink.id == TEXT("gin_tonic_fixed") && Drink.status == TEXT("served")) bServed = true;
        if (!bServed) return;
        if (SendPlaythroughCommand(TEXT("consume_drink"), FString(), FString(), TEXT("gin_tonic_fixed")).IsEmpty()) return;
        RecordPlaythrough(TEXT("CHECK drink ordered; sip action requested, awaiting physical effect"));
        PlaythroughStep = 6;
        NextPlaythroughActionAt = Now + .8;
        return;
    }
    if(PlaythroughStep==19)
    {
        for(const auto& Option:State.interaction.suggestions)if(Option.id.StartsWith(TEXT("gift_deliver_"))&&Option.enabled)
        {
            if(SendPlaythroughCommand(TEXT("gift_drink"),FString(),TEXT("deliver"),Option.id.Mid(13)).IsEmpty())return;
            RecordPlaythrough(TEXT("CHECK consent received; handoff requested, awaiting actual hand contact"));PlaythroughStep=20;return;
        }
        return;
    }
    if(PlaythroughStep==20)
    {
        for(const auto& Drink:Night.drinks)if(Drink.owner==TEXT("B")&&Drink.id==TEXT("oolong_lemon_soda_no_syrup")&&Drink.receivedFrom==TEXT("USER"))
        {
            if(Drink.status!=TEXT("accepted")||Night.availableCash!=13){FailPlaythrough(TEXT("gift state or payment mismatch"));return;}
            if(FParse::Param(FCommandLine::Get(),TEXT("LalalandNpcSipPlaythrough")))
            {
                RecordPlaythrough(TEXT("CHECK received gift; waiting for explicit NPC choice and physical sip"));PlaythroughStep=21;return;
            }
            RecordPlaythrough(TEXT("SUCCESS gift-only actual hand contact owner=B status=accepted cash=14; no automatic sip"));
            bPlaythroughFinished=true;FGenericPlatformMisc::RequestExit(false);return;
        }
        return;
    }
    if(PlaythroughStep==21)
    {
        for(const auto& Drink:Night.drinks)if(Drink.owner==TEXT("B")&&Drink.id==TEXT("oolong_lemon_soda_no_syrup")&&Drink.receivedFrom==TEXT("USER"))
        {
            ALalalandNpcCharacter* Npc=nullptr;
            for(TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)if(It->GetActorId()==TEXT("B")){Npc=*It;break;}
            if(Npc&&GetWorld()->GetFirstPlayerController())
            {
                const FVector Camera=GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
                GetWorld()->GetFirstPlayerController()->SetControlRotation((Npc->GetActorLocation()+FVector(0,0,45)-Camera).Rotation());
            }
            if(Drink.status==TEXT("consumed")&&Drink.cupPlacement==TEXT("counter")&&Drink.sipActionId.IsEmpty())
            {
                RecordPlaythrough(TEXT("SUCCESS explicit NPC choice, actual mouth contact, return route and counter contact; cash=14"));
                FScreenshotRequest::RequestScreenshot(TEXT("NpcCupReturned"),true,false);
                PlaythroughStep=22;NextPlaythroughActionAt=Now+1;return;
            }
        }
        return;
    }
    if(PlaythroughStep==22){bPlaythroughFinished=true;FGenericPlatformMisc::RequestExit(false);return;}
    if (PlaythroughStep == 6)
    {
        bool bConsumed=false;
        for(const auto& Drink:Night.drinks)if(Drink.owner==TEXT("USER")&&Drink.id==TEXT("gin_tonic_fixed")&&Drink.status==TEXT("consumed"))bConsumed=true;
        if(!bConsumed)return;
        if (Night.phase != TEXT("game_choice")) return;
        if (bNormalDeclinePlaythrough)
        {
            if(SendPlaythroughCommand(TEXT("bounce_choice"), FString(), TEXT("decline")).IsEmpty())return;
            RecordPlaythrough(TEXT("CHECK explicitly declined game; waiting for natural meteor transition"));
            PlaythroughStep=9;
            return;
        }
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
        if (bNormalPlaythrough)
        {
            const FLalalandActorDto* User = State.characters.FindByPredicate([](const FLalalandActorDto& A){return A.id == TEXT("USER");});
            if (!User) return;
            if (FMath::Square(User->x+3.2)+FMath::Square(User->z+1.5)>.5)
            {
                if (!User->route.Num()) SendPlaythroughCommand(TEXT("move_to"),FString(),FString(),TEXT("main_table"));
                NextPlaythroughActionAt=Now+1.f;
                return;
            }
        }
        if (bThrowRequested) return;
        if (!bAimSent)
        {
            // Aim compensates for the actual first-person hand's 22 cm right
            // offset; this is an ordinary aiming command, not a forced hit.
            if (SendPlaythroughCommand(TEXT("set_throw_aim"), FString(), FString(), FString(), FString(), .57, 0).IsEmpty()) return;
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
        if (Night.invitationResult == TEXT("friend_pending"))
        {
            SendPlaythroughCommand(TEXT("npc_invite_reply"), FString(), TEXT("accept_friend"));
            NextPlaythroughActionAt = Now + .8;
            return;
        }
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
        if (!bNormalPlaythrough) Player->BeginAutomatedRoofClimb();
        bRoofClimbStarted = true;
        RecordPlaythrough(TEXT("MOVE began physical CharacterMovement stair climb"));
        PlaythroughStep = 13;
        return;
    }
    if (PlaythroughStep == 13)
    {
        ALalalandPlayerCharacter* Player = Cast<ALalalandPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (!Player) return;
        if (bNormalPlaythrough) { if (Night.phase != TEXT("rooftop")) return; }
        else
        {
            if (!Player->IsAutomatedRoofClimbFinished()) return;
            if (!Player->DidAutomatedRoofClimbSucceed()) { FailPlaythrough(TEXT("physical stair climb failed")); return; }
        }
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
        if (bNormalPlaythrough && !Night.invited.IsEmpty() && (Night.invitationResult==TEXT("friend")||Night.invitationResult==TEXT("romance")))
        {
            const FLalalandActorDto* User=State.characters.FindByPredicate([](const FLalalandActorDto& A){return A.id==TEXT("USER");});
            bool bTogether=false;
            for(const FString& CompanionId:TArray<FString>{Night.invited})
            {
                const FLalalandActorDto* Companion=State.characters.FindByPredicate([&](const FLalalandActorDto& A){return A.id==CompanionId;});
                if(User&&Companion&&Companion->area==TEXT("rooftop")&&FMath::Square(User->x-Companion->x)+FMath::Square(User->z-Companion->z)<16.)bTogether=true;
            }
            if(!bTogether)return;
            RecordPlaythrough(TEXT("CHECK confirmed companion physically arrived within rooftop conversation range"));
        }
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
