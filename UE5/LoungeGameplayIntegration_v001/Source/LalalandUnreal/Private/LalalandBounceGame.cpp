#include "LalalandBounceGame.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LalalandDtos.h"
#include "LalalandKit.h"
#include "LalalandServiceSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "EngineUtils.h"
#include "LalalandNpcCharacter.h"
#include "LalalandRootWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "InputCoreTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "LalalandVenueLayout.h"

ALalalandBounceGame::ALalalandBounceGame()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;
    CupRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CupRoot"));
    CupRoot->SetupAttachment(SceneRoot);
    CupRoot->SetRelativeLocation(FVector(340.f, -150.f, 102.f));
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    TableSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BounceTable"));
    TableSurface->SetupAttachment(SceneRoot);
    TableSurface->SetStaticMesh(Cube);
    TableSurface->SetRelativeLocation(FVector(60.f, -150.f, 92.f));
    // Engine BasicShapes/Cube is 100 cm across, so a 6.4 X scale produces
    // the intended 6.4 m long table. The previous 3.2 scale left the target
    // cup physically beyond the far edge.
    TableSurface->SetRelativeScale3D(FVector(6.4f, 1.25f, .1f));
    TableSurface->SetCollisionProfileName(TEXT("BlockAll"));
    TableSurface->ComponentTags.Add(TEXT("BounceTable"));

    GameBall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GameBall"));
    GameBall->SetupAttachment(SceneRoot);
    GameBall->SetStaticMesh(Sphere);
    GameBall->SetRelativeScale3D(FVector(.10f));
    GameBall->SetCollisionProfileName(TEXT("PhysicsActor"));
    // A thrown ping-pong ball must not be deflected by the player/NPC capsules.
    // Characters stand around the table, but only the table, cup and authored
    // world geometry participate in the bounce result.
    GameBall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    GameBall->SetNotifyRigidBodyCollision(true);
    GameBall->SetSimulatePhysics(false);
    GameBall->SetVisibility(false);
    GameBall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GameBall->BodyInstance.bUseCCD = true;
    GameBall->BodyInstance.SetMassOverride(0.003f, true);
    GameBall->OnComponentHit.AddDynamic(this, &ALalalandBounceGame::OnBallHit);

    BallGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("BallGlow"));
    BallGlow->SetupAttachment(GameBall);
    BallGlow->SetRelativeLocation(FVector::ZeroVector);
    BallGlow->SetLightColor(FLinearColor(1.f, .22f, .035f));
    BallGlow->SetIntensity(2200.f);
    BallGlow->SetAttenuationRadius(150.f);
    BallGlow->SetCastShadows(false);
    BallGlow->SetVisibility(false);

    OpeningBall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OpeningBall"));
    OpeningBall->SetupAttachment(SceneRoot);
    OpeningBall->SetStaticMesh(Sphere);
    OpeningBall->SetRelativeScale3D(FVector(.08f));
    OpeningBall->SetCollisionProfileName(TEXT("PhysicsActor"));
    OpeningBall->SetSimulatePhysics(false);
    OpeningBall->SetVisibility(false);
    OpeningBall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OpeningBall->BodyInstance.bUseCCD = true;
    for (int32 Index=0; Index<8; ++Index)
    {
        UStaticMeshComponent* Dot=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("AimDot%d"),Index));
        Dot->SetupAttachment(SceneRoot); Dot->SetStaticMesh(Sphere); Dot->SetRelativeScale3D(FVector(.025f));
        Dot->SetCollisionEnabled(ECollisionEnabled::NoCollision); Dot->SetVisibility(false); AimDots.Add(Dot);
    }

    CupTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("CupInside"));
    CupTrigger->SetupAttachment(CupRoot);
    CupTrigger->SetRelativeLocation(FVector(0.f, 0.f, 8.f));
    CupTrigger->SetBoxExtent(FVector(6.f, 6.f, 7.f));
    CupTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CupTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    CupTrigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    CupTrigger->OnComponentBeginOverlap.AddDynamic(this, &ALalalandBounceGame::OnCupEntered);

    const TArray<FVector> WallLocations = {
        FVector(0.f, -8.f, 8.f), FVector(0.f, 8.f, 8.f),
        FVector(-8.f, 0.f, 8.f), FVector(8.f, 0.f, 8.f)
    };
    const TArray<FVector> WallScales = {
        FVector(.16f, .012f, .07f), FVector(.16f, .012f, .07f),
        FVector(.012f, .16f, .07f), FVector(.012f, .16f, .07f)
    };
    for (int32 Index = 0; Index < WallLocations.Num(); ++Index)
    {
        UStaticMeshComponent* Wall = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("CupWall%d"), Index));
        Wall->SetupAttachment(CupRoot); Wall->SetStaticMesh(Cube); Wall->SetRelativeLocation(WallLocations[Index]);
        Wall->SetRelativeScale3D(WallScales[Index]); Wall->SetCollisionProfileName(TEXT("BlockAll")); CupWalls.Add(Wall);
    }
}

void ALalalandBounceGame::BeginPlay()
{
    Super::BeginPlay();
    Tint(TableSurface, FLinearColor(.15f, .055f, .025f));
    Tint(GameBall, FLinearColor(.95f, .42f, .08f), .35f);
    Tint(OpeningBall, FLinearColor(.95f, .42f, .08f), .35f);
    UPhysicalMaterial* BallMaterial = NewObject<UPhysicalMaterial>(this);
    BallMaterial->Restitution = .56f; BallMaterial->Friction = .08f;
    BallMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
    BallMaterial->FrictionCombineMode = EFrictionCombineMode::Min;
    GameBall->SetPhysMaterialOverride(BallMaterial); OpeningBall->SetPhysMaterialOverride(BallMaterial);
    UPhysicalMaterial* TableMaterial = NewObject<UPhysicalMaterial>(this);
    TableMaterial->Restitution = .50f;
    TableMaterial->Friction = .12f;
    TableMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
    TableMaterial->FrictionCombineMode = EFrictionCombineMode::Min;
    TableSurface->SetPhysMaterialOverride(TableMaterial);
    for (UStaticMeshComponent* Wall : CupWalls)
    {
        Tint(Wall, FLinearColor(.55f, .06f, .035f), .4f);
        Wall->SetVisibility(false);
        Wall->SetHiddenInGame(true);
    }
    LalalandKit::TargetCup(this, CupRoot, TEXT("CupVisual"), FVector(0.f, 0.f, 0.f));
    LalalandKit::Shape(this, SceneRoot, TEXT("TableLegFL"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(-220.f, -200.f, 46.f), FVector(.08f, .08f, .9f), FLinearColor(.12f, .05f, .02f), .55f, true);
    LalalandKit::Shape(this, SceneRoot, TEXT("TableLegFR"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(340.f, -200.f, 46.f), FVector(.08f, .08f, .9f), FLinearColor(.12f, .05f, .02f), .55f, true);
    LalalandKit::Shape(this, SceneRoot, TEXT("TableLegBL"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(-220.f, -100.f, 46.f), FVector(.08f, .08f, .9f), FLinearColor(.12f, .05f, .02f), .55f, true);
    LalalandKit::Shape(this, SceneRoot, TEXT("TableLegBR"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(340.f, -100.f, 46.f), FVector(.08f, .08f, .9f), FLinearColor(.12f, .05f, .02f), .55f, true);
    // This is a visual trim above the actual Chaos table. Collision here would
    // intercept every legal bounce before it reaches BounceTable and leave the
    // authoritative result with zero table contacts.
    LalalandKit::Shape(this, SceneRoot, TEXT("TableRail"), TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(60.f, -150.f, 98.f), FVector(6.56f, 1.32f, .03f), FLinearColor(.42f, .19f, .055f), .3f, false);
    LalalandKit::WineGlass(this, SceneRoot, TEXT("TableWine"), FVector(-40.f, -95.f, 98.f), FLinearColor(.62f, .78f, .28f), .8f);
    LalalandKit::Highball(this, SceneRoot, TEXT("TableHighball"), FVector(20.f, -210.f, 98.f), FLinearColor(.28f, .72f, .42f), .75f);
    LalalandKit::Coaster(this, SceneRoot, TEXT("TableCoaster"), FVector(-40.f, -95.f, 98.f));
    Service = GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
    if (Service)
    {
        Service->OnChanged.AddDynamic(this, &ALalalandBounceGame::RefreshFromService);
        Service->OnCommandAcknowledged.AddDynamic(this, &ALalalandBounceGame::HandleCommandAcknowledged);
        Service->OnCommandRejected.AddDynamic(this, &ALalalandBounceGame::HandleCommandRejected);
        RefreshFromService();
    }
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("LalalandPhysicsAudit")))
    {
        FTimerHandle AuditTimer;
        GetWorldTimerManager().SetTimer(AuditTimer, this, &ALalalandBounceGame::LaunchPhysicsAuditThrow, 1.0f, false);
    }
#endif
}

void ALalalandBounceGame::Tint(UStaticMeshComponent* Component, const FLinearColor& Color, float Roughness)
{
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_Tint.M_Tint"));
    if (!Component || !Base) return;
    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Component);
    Material->SetVectorParameterValue(TEXT("Tint"), Color); Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    Component->SetMaterial(0, Material);
}

void ALalalandBounceGame::RefreshFromService()
{
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    const FLalalandFirstNightDto& Night = Service->GetState().firstNight;
    const bool bOpeningVisible = Night.openingBall == TEXT("rolling");
    OpeningBall->SetVisibility(bOpeningVisible);
    if (bOpeningVisible && !bOpeningRolled)
    {
        bOpeningRolled = true;
        OpeningBall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        OpeningBall->SetSimulatePhysics(false);
        OpeningBall->SetWorldLocation(FVector(40.f, -440.f, 5.f));
    }
    if (!bOpeningVisible)
    {
        OpeningBall->SetSimulatePhysics(false);
        OpeningBall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (Night.cupAssist) ApplyCupAssist();
    if (!Night.pendingThrow.id.IsEmpty() && Night.pendingThrow.id != ActiveThrowId) LaunchPendingThrow();
    if (Night.pendingThrow.id.IsEmpty() && !ActiveThrowId.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("LALALAND_BOUNCE_RESOLVED throw=%s contacts=%d cup=%d final=%s"),
            *ActiveThrowId, TableContacts, bPendingCupResult ? 1 : 0, *GameBall->GetComponentLocation().ToCompactString());
        ActiveThrowId.Empty(); ResultCommandId.Empty(); GameBall->SetSimulatePhysics(false); GameBall->SetVisibility(false); BallGlow->SetVisibility(false);
        GameBall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void ALalalandBounceGame::HandleCommandAcknowledged(const FString& CommandId, const FString&)
{
    if (CommandId == ThrowCommandId) ThrowCommandId.Empty();
    if (CommandId == OpeningCommandId) { OpeningCommandId.Empty(); HandoffSeconds=-1.f; }
    if (CommandId == ResultCommandId) ResultCommandId.Empty();
}

void ALalalandBounceGame::HandleCommandRejected(const FString& CommandId, const FString&)
{
    if (CommandId == ThrowCommandId) ThrowCommandId.Empty();
    if (CommandId == OpeningCommandId) { OpeningCommandId.Empty(); HandoffSeconds=-1.f; }
    if (CommandId != ResultCommandId) return;
    ResultCommandId.Empty();
    if (Service && Service->GetState().firstNight.pendingThrow.id == ActiveThrowId)
    {
        // Retry the exact same result with the same deterministic command id.
        // The backend command ledger makes this safe if the first response was
        // lost after the state mutation was already committed.
        bReported = false;
        ResultRetryRemaining = 0.75f;
    }
}

void ALalalandBounceGame::LaunchPendingThrow()
{
    const FLalalandPendingThrowDto& Throw = Service->GetState().firstNight.pendingThrow;
    ActiveThrowId = Throw.id; ResultCommandId.Empty(); FlightSeconds = 0.f; LastTableContactAt = -1.f; ResultRetryRemaining = 0.f;
    TableContacts = 0; bCupEntered = false; bPendingCupResult = false; bReported = false;
    if(Throw.actor!=TEXT("USER"))
    {
        NpcWindup=0;GameBall->SetSimulatePhysics(false);GameBall->SetCollisionEnabled(ECollisionEnabled::NoCollision);GameBall->SetVisibility(true);
        for(TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It) if(It->GetActorId()==Throw.actor) It->TriggerGesture(TEXT("throw"));
        GameBall->SetWorldLocation(ThrowOrigin(Throw.actor));return;
    }
    ReleasePendingThrow();
}

void ALalalandBounceGame::ReleasePendingThrow()
{
    const auto& Throw=Service->GetState().firstNight.pendingThrow;
    NpcWindup=-1.f;
    GameBall->SetSimulatePhysics(false);
    GameBall->SetVisibility(true); BallGlow->SetVisibility(true); GameBall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GameBall->SetSimulatePhysics(true);
    GameBall->SetWorldLocation(ThrowOrigin(Throw.actor), false, nullptr, ETeleportType::TeleportPhysics);
    const float SideBase = static_cast<float>((Throw.aim - .67) * 220.0);
    float Jitter = 0.f;
    if (Throw.actor == TEXT("USER"))
    {
        const FString Stage = Service->GetState().firstNight.playerDrinkStage;
        if (Stage == TEXT("impaired")) Jitter = 55.f;
        else if (Stage == TEXT("light")) Jitter = 22.f;
    }
    const float Side = SideBase + FMath::FRandRange(-Jitter, Jitter);
    const float Forward = 500.f + static_cast<float>(Throw.power) * 100.f;
    // Keep the ball above the near rim while descending into the trigger.
    // The values are calibrated against the real 6.4 m Chaos table.
    const float Up = 255.f + static_cast<float>(Throw.power) * 43.f;
    GameBall->SetPhysicsLinearVelocity(FVector(Forward, Side, Up));
    UE_LOG(LogTemp, Log, TEXT("LALALAND_BOUNCE_LAUNCH throw=%s actor=%s aim=%.3f power=%.3f velocity=%s"),
        *Throw.id, *Throw.actor, Throw.aim, Throw.power, *FVector(Forward, Side, Up).ToCompactString());
}

void ALalalandBounceGame::LaunchPhysicsAuditThrow()
{
    bLocalPhysicsAudit = true;
    ActiveThrowId = TEXT("local-physics-audit");
    FlightSeconds = 0.f; LastTableContactAt = -1.f; ResultRetryRemaining = 0.f; TableContacts = 0;
    bCupEntered = false; bPendingCupResult = false; bReported = false;
    GameBall->SetSimulatePhysics(false);
    GameBall->SetVisibility(true); BallGlow->SetVisibility(true);
    GameBall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GameBall->SetSimulatePhysics(true);
    GameBall->SetWorldLocation(LalalandVenue::Bounce(FVector(-245.f, -150.f, 148.f)), false, nullptr, ETeleportType::TeleportPhysics);
    GameBall->SetPhysicsLinearVelocity(FVector(558.f, 0.f, 280.f));
    UE_LOG(LogTemp, Log, TEXT("LALALAND_PHYSICS_AUDIT launch=1"));
}

void ALalalandBounceGame::FinishPhysicsAudit(bool bEnteredCup)
{
    bReported = true;
    bPendingCupResult = bEnteredCup;
    UE_LOG(LogTemp, Log, TEXT("LALALAND_PHYSICS_AUDIT success=%d contacts=%d flight=%.3f final=%s"),
        bEnteredCup ? 1 : 0, TableContacts, FlightSeconds, *GameBall->GetComponentLocation().ToCompactString());
    GameBall->SetSimulatePhysics(false);
    bLocalPhysicsAudit = false;
    ActiveThrowId.Empty();
}

void ALalalandBounceGame::ApplyCupAssist()
{
    if (bCupAssisted) return;
    bCupAssisted = true;
    CupRoot->SetRelativeLocation(FVector(280.f, -150.f, 102.f));
    CupRoot->SetRelativeScale3D(FVector(1.4f));
}

void ALalalandBounceGame::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Server time and body gestures already pause. Freeze Chaos as well,
    // preserving momentum so resume does not invent a new throw or result.
    if (Service && Service->GetState().paused)
    {
        if (!bPhysicsPaused)
        {
            bPhysicsPaused = true;
            bResumeGameBallPhysics = GameBall->IsSimulatingPhysics();
            bResumeOpeningBallPhysics = OpeningBall->IsSimulatingPhysics();
            PausedGameVelocity = GameBall->GetPhysicsLinearVelocity();
            PausedGameAngularVelocity = GameBall->GetPhysicsAngularVelocityInRadians();
            PausedOpeningVelocity = OpeningBall->GetPhysicsLinearVelocity();
            PausedOpeningAngularVelocity = OpeningBall->GetPhysicsAngularVelocityInRadians();
            if (bResumeGameBallPhysics) GameBall->SetSimulatePhysics(false);
            if (bResumeOpeningBallPhysics) OpeningBall->SetSimulatePhysics(false);
        }
        return;
    }
    if (bPhysicsPaused)
    {
        bPhysicsPaused = false;
        if (bResumeGameBallPhysics)
        {
            GameBall->SetSimulatePhysics(true);
            GameBall->SetPhysicsLinearVelocity(PausedGameVelocity);
            GameBall->SetPhysicsAngularVelocityInRadians(PausedGameAngularVelocity);
        }
        if (bResumeOpeningBallPhysics)
        {
            OpeningBall->SetSimulatePhysics(true);
            OpeningBall->SetPhysicsLinearVelocity(PausedOpeningVelocity);
            OpeningBall->SetPhysicsAngularVelocityInRadians(PausedOpeningAngularVelocity);
        }
    }
    UpdateOpeningBall(DeltaSeconds);
    UpdatePlayerThrow(DeltaSeconds);
    if(NpcWindup>=0 && Service)
    {
        if(Service->GetState().firstNight.pendingThrow.id!=ActiveThrowId){NpcWindup=-1.f;return;}
        GameBall->SetWorldLocation(ThrowOrigin(Service->GetState().firstNight.pendingThrow.actor));
        if(!Service->GetState().paused)NpcWindup+=DeltaSeconds;
        if(NpcWindup>=.65f){
            bool Ready=false;
            for(TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)if(It->GetActorId()==Service->GetState().firstNight.pendingThrow.actor)
                Ready=FVector::Dist2D(It->GetActorLocation(),LalalandVenue::Bounce(FVector(-320,-150,0)))<=65.f&&It->GetVelocity().Size2D()<10.f&&FVector::DotProduct(It->GetActorForwardVector(),FVector::ForwardVector)>.98f;
            if(Ready)ReleasePendingThrow();
        }
        return;
    }
    if (ActiveThrowId.IsEmpty() || bReported) return;
    ResultRetryRemaining = FMath::Max(0.f, ResultRetryRemaining - DeltaSeconds);
    if (ResultRetryRemaining > 0.f) return;
    FlightSeconds += DeltaSeconds;
    if (!bCapturedBallInFlight && FlightSeconds >= .32f && FParse::Param(FCommandLine::Get(), TEXT("LalalandFullPlaythrough")))
    {
        bCapturedBallInFlight = true;
        FScreenshotRequest::RequestScreenshot(TEXT("PlaythroughBallInFlight"), true, false);
    }
    if (bLocalPhysicsAudit)
    {
        if (bCupEntered) FinishPhysicsAudit(true);
        else if (FlightSeconds > 4.5f || GameBall->GetComponentLocation().Z < -30.f) FinishPhysicsAudit(false);
        return;
    }
    if (bCupEntered) ReportResult(true);
    else if (FlightSeconds > 4.5f || GameBall->GetComponentLocation().Z < -30.f) ReportResult(false);
}

bool ALalalandBounceGame::IsPlayerThrowReady() const
{
    if (!Service || Service->GetState().paused || !ThrowCommandId.IsEmpty()) return false;
    const auto& Night=Service->GetState().firstNight;
    const APlayerController* PC=GetWorld()->GetFirstPlayerController();
    const APawn* Player=PC?PC->GetPawn():nullptr;
    const bool bPractice=Night.openingBall==TEXT("practice") && (Night.phase==TEXT("arrival") || Night.phase==TEXT("free_time"));
    return Player && Night.pendingThrow.id.IsEmpty() && (bPractice || (Night.phase==TEXT("game_round") && Night.gameChoice==TEXT("join")
        && Night.participants.IsValidIndex(Night.turn % FMath::Max(1,Night.participants.Num()))
        && Night.participants[Night.turn % Night.participants.Num()]==TEXT("USER")))
        && FVector::Dist2D(Player->GetActorLocation(),LalalandVenue::Bounce(FVector(-320,-150,0)))<75.f;
}

FVector ALalalandBounceGame::ThrowOrigin(const FString& ActorId) const
{
    if (ActorId==TEXT("USER"))
    {
        if (const APlayerController* PC=GetWorld()->GetFirstPlayerController())
            return PC->PlayerCameraManager->GetCameraLocation()+PC->PlayerCameraManager->GetCameraRotation().RotateVector(FVector(65,22,-22));
    }
    for (TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)
        if (It->GetActorId()==ActorId)
        {
            if(!It->GetHandAnchor().IsNone())return It->GetMesh()->GetSocketLocation(It->GetHandAnchor());
            return It->GetActorLocation()+FVector(45,0,48);
        }
    return LalalandVenue::Bounce(FVector(-255,-150,140));
}

void ALalalandBounceGame::UpdatePlayerThrow(float DeltaSeconds)
{
    APlayerController* PC=GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    const bool bEDown=PC->IsInputKeyDown(EKeys::E);
    bool bInputAllowed=true;
    TArray<UUserWidget*> Widgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Widgets,ULalalandRootWidget::StaticClass(),true);
    for (UUserWidget* Widget:Widgets) bInputAllowed &= CastChecked<ULalalandRootWidget>(Widget)->AllowsWorldInteraction();
    const bool bReady=IsPlayerThrowReady();
    if (!bReady && bInputAllowed && bEDown && !bEWasDown && OpeningCommandId.IsEmpty() && Service)
    {
        if (Service->GetState().firstNight.openingBall==TEXT("held")) RequestOpeningReturn();
        else if (GetWorldPrompt()==TEXT("E · 拾起球"))
        {
            FLalalandCommandDto Command; Command.type=TEXT("opening_ball"); Command.intent=TEXT("pickup");
            OpeningCommandId=Service->SendCommand(Command);
        }
        else if(GetWorldPrompt()==TEXT("E · 在卡座坐下") || GetWorldPrompt()==TEXT("E · 起身"))
        {
            FLalalandCommandDto Command;Command.type=GetWorldPrompt()==TEXT("E · 起身")?TEXT("cancel_move"):TEXT("sit_view");OpeningCommandId=Service->SendCommand(Command);
        }
    }
    for (UStaticMeshComponent* Dot:AimDots) Dot->SetVisibility(bReady&&bInputAllowed);
    if (bReady&&!bWasThrowReady&&bInputAllowed) PC->SetControlRotation(FRotator(0,0,0));
    bWasThrowReady=bReady;
    if (!bReady || !bInputAllowed)
    {
        bCharging=false; ChargeSeconds=0; bEWasDown=bEDown;
        if (ActiveThrowId.IsEmpty()) { GameBall->SetVisibility(false); BallGlow->SetVisibility(false); }
        return;
    }
    float MouseX=0,MouseY=0; int32 Width=0,Height=0; PC->GetViewportSize(Width,Height);
    if (Width>0 && PC->GetMousePosition(MouseX,MouseY)) PlayerAim=FMath::Clamp(.67f+(MouseX/Width-.5f)*.6f,0.f,1.f);
    if (bEDown&&!bEWasDown) { bCharging=true; ChargeSeconds=0; }
    if (PC->IsInputKeyDown(EKeys::RightMouseButton)) { bCharging=false; ChargeSeconds=0; }
    if(PC->GetPawn() && PC->GetPawn()->GetVelocity().Size2D()>5.f){bCharging=false;ChargeSeconds=0;}
    if (bCharging&&bEDown) ChargeSeconds=FMath::Min(1.5f,ChargeSeconds+DeltaSeconds);
    const float Power=FMath::Clamp(GetCharge(),.15f,1.f);
    const FVector Origin=ThrowOrigin(TEXT("USER"));
    if (ActiveThrowId.IsEmpty())
    {
        GameBall->SetSimulatePhysics(false); GameBall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GameBall->SetWorldLocation(Origin); GameBall->SetVisibility(true);
    }
    const FVector Velocity(500+Power*100,(PlayerAim-.67f)*220,255+Power*43);
    for (int32 I=0; I<AimDots.Num(); ++I)
    {
        const float T=(I+1)*.04f;
        AimDots[I]->SetWorldLocation(Origin+Velocity*T+FVector(0,0,-490*T*T));
    }
    if (!bEDown&&bEWasDown&&bCharging)
    {
        FLalalandCommandDto Command; Command.type=TEXT("throw_ball"); Command.x=PlayerAim; Command.z=Power;
        ThrowCommandId=Service->SendCommand(Command); bCharging=false; ChargeSeconds=0;
        GameBall->SetVisibility(false);
    }
    bEWasDown=bEDown;
}

FVector ALalalandBounceGame::OpeningReceiver() const
{
    for (TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)
        if (It->GetActorId()==TEXT("C"))
        {
            for (FName Bone:{FName(TEXT("hand_r")),FName(TEXT("R_Hand"))})
                if (It->GetMesh()->GetBoneIndex(Bone)!=INDEX_NONE) return It->GetMesh()->GetBoneLocation(Bone);
            return It->GetActorLocation()+FVector(25,0,30);
        }
    return FVector::ZeroVector;
}

FString ALalalandBounceGame::GetWorldPrompt() const
{
    if (!Service || Service->GetState().sessionId.IsEmpty() || Service->GetState().paused) return FString();
    if (IsPlayerThrowReady()) return TEXT("鼠标瞄准 · 按住 E 蓄力 / 松开出手 · 右键取消");
    const APlayerController* PC=GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return FString();
    const FString State=Service->GetState().firstNight.openingBall;
    for(const auto& Actor:Service->GetState().characters)if(Actor.id==TEXT("USER")&&Actor.posture==TEXT("sit"))return TEXT("E · 起身");
    if(State!=TEXT("held") && Service->GetState().firstNight.phase!=TEXT("game_round"))
        for(const auto& Booth:LalalandVenue::Layout()->GetArrayField(TEXT("booths")))
        {
            FVector Entry=LalalandVenue::Named(TEXT("boothEntry"));Entry.Y=LalalandVenue::Point(Booth->AsObject()).Y;
            if(FVector::Dist2D(PC->GetPawn()->GetActorLocation(),Entry)<110.f)return TEXT("E · 在卡座坐下");
        }
    const FVector Target=State==TEXT("rolling")?OpeningBall->GetComponentLocation():OpeningReceiver();
    const FVector ToTarget=Target-PC->PlayerCameraManager->GetCameraLocation();
    if ((State==TEXT("rolling")||State==TEXT("held")) && FVector::Dist2D(PC->GetPawn()->GetActorLocation(),Target)<170.f
        && FVector::DotProduct(ToTarget.GetSafeNormal(),PC->PlayerCameraManager->GetCameraRotation().Vector())>.85f)
    {
        FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(BallInteraction),false,this); Params.AddIgnoredActor(PC->GetPawn());
        if (!GetWorld()->LineTraceSingleByChannel(Hit,PC->PlayerCameraManager->GetCameraLocation(),Target,ECC_Visibility,Params)
            || (State==TEXT("held") && Hit.GetActor() && Hit.GetActor()->IsA<ALalalandNpcCharacter>()))
            return State==TEXT("rolling")?TEXT("E · 拾起球"):TEXT("E · 递给万塞");
    }
    return State==TEXT("held")?TEXT("手中有球 · 走近万塞递回，或选择试投"):State==TEXT("practice")?TEXT("走到长桌发球端，准备一次不计分练习"):FString();
}

void ALalalandBounceGame::RequestOpeningReturn()
{
    if (!Service || !OpeningCommandId.IsEmpty() || HandoffSeconds>=0 || GetWorldPrompt()!=TEXT("E · 递给万塞")) return;
    HandoffStart=OpeningBall->GetComponentLocation(); HandoffSeconds=0;
}

void ALalalandBounceGame::UpdateOpeningBall(float DeltaSeconds)
{
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    const FString State=Service->GetState().firstNight.openingBall;
    const APlayerController* PC=GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    if (State==TEXT("held") || (State==TEXT("practice") && Service->GetState().firstNight.pendingThrow.id.IsEmpty() && !IsPlayerThrowReady()))
    {
        OpeningBall->SetVisibility(true); OpeningBall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FVector Location=PC->PlayerCameraManager->GetCameraLocation()+PC->PlayerCameraManager->GetCameraRotation().RotateVector(FVector(65,22,-22));
        if (HandoffSeconds>=0)
        {
            if (!Service->GetState().paused) HandoffSeconds+=DeltaSeconds;
            Location=FMath::Lerp(HandoffStart,OpeningReceiver(),FMath::Clamp(HandoffSeconds/.45f,0.f,1.f));
            if (HandoffSeconds>=.45f && OpeningCommandId.IsEmpty() && !Service->GetState().paused)
            {
                FLalalandCommandDto Command; Command.type=TEXT("opening_ball"); Command.intent=TEXT("return");
                OpeningCommandId=Service->SendCommand(Command);
            }
        }
        OpeningBall->SetWorldLocation(Location);
    }
    else if (State==TEXT("returned"))
    {
        if (!Service->GetState().paused) ReturnedSeconds+=DeltaSeconds;
        OpeningBall->SetWorldLocation(OpeningReceiver()); OpeningBall->SetVisibility(ReturnedSeconds<2.f);
    }
    else if (State!=TEXT("rolling")) OpeningBall->SetVisibility(false);
}

void ALalalandBounceGame::OnBallHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector, const FHitResult&)
{
    if (ActiveThrowId.IsEmpty()) return;
    if (bLocalPhysicsAudit || FParse::Param(FCommandLine::Get(), TEXT("LalalandFullPlaythrough"))) UE_LOG(LogTemp, Log, TEXT("LALALAND_BOUNCE_CONTACT actor=%s component=%s table=%d"),
        OtherActor ? *OtherActor->GetName() : TEXT("none"), OtherComponent ? *OtherComponent->GetName() : TEXT("none"), OtherComponent == TableSurface ? 1 : 0);
    if (OtherComponent == TableSurface && (LastTableContactAt < 0.f || FlightSeconds - LastTableContactAt > .08f)
        && FMath::Abs(GameBall->GetPhysicsLinearVelocity().Z) > 45.f)
    {
        ++TableContacts;
        LastTableContactAt = FlightSeconds;
    }
}

void ALalalandBounceGame::OnCupEntered(UPrimitiveComponent*, AActor*, UPrimitiveComponent* OtherComponent, int32, bool, const FHitResult&)
{
    if (OtherComponent == GameBall && TableContacts > 0) bCupEntered = true;
}

void ALalalandBounceGame::ReportResult(bool bEnteredCup)
{
    if (bReported || !Service) return;
    bPendingCupResult = bEnteredCup;
    FLalalandCommandDto Command;
    Command.id = TEXT("physics-") + ActiveThrowId.Replace(TEXT(":"), TEXT("-"));
    Command.type = TEXT("bounce_result"); Command.requestId = ActiveThrowId;
    Command.open = bEnteredCup; Command.x = TableContacts;
    ResultCommandId = Service->SendCommand(Command);
    bReported = !ResultCommandId.IsEmpty();
    if (!bReported) ResultRetryRemaining = .75f;
}
