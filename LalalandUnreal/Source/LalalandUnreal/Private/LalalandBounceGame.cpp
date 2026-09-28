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
        OpeningBall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        OpeningBall->SetSimulatePhysics(true);
        OpeningBall->SetWorldLocation(FVector(120.f, -410.f, 10.f), false, nullptr, ETeleportType::TeleportPhysics);
        OpeningBall->SetPhysicsLinearVelocity(FVector(-95.f, -55.f, 0.f));
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
    if (CommandId == ResultCommandId) ResultCommandId.Empty();
}

void ALalalandBounceGame::HandleCommandRejected(const FString& CommandId, const FString&)
{
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
    GameBall->SetSimulatePhysics(false);
    GameBall->SetVisibility(true); BallGlow->SetVisibility(true); GameBall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GameBall->SetSimulatePhysics(true);
    GameBall->SetWorldLocation(FVector(-245.f, -150.f, 148.f), false, nullptr, ETeleportType::TeleportPhysics);
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
    GameBall->SetWorldLocation(FVector(-245.f, -150.f, 148.f), false, nullptr, ETeleportType::TeleportPhysics);
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
