#include "LalalandStage.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "LalalandNpcCharacter.h"
#include "LalalandServiceSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

ALalalandStage::ALalalandStage()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    AmbientAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudio"));
    AmbientAudio->SetupAttachment(RootComponent);
    AmbientAudio->bAutoActivate = false;
    AmbientAudio->SetVolumeMultiplier(.2f);
}

void ALalalandStage::BeginPlay()
{
    Super::BeginPlay();
    BuildArchitecture();
    BuildLighting();
    SpawnCast();
    if (USoundWave* Lounge = LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/SW_Lounge.SW_Lounge")))
    {
        Lounge->bLooping = true;
        AmbientAudio->SetSound(Lounge);
        AmbientAudio->Play();
    }
    Service = GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
    if (Service)
    {
        Service->OnChanged.AddDynamic(this, &ALalalandStage::RefreshFromService);
        RefreshFromService();
    }
}

AStaticMeshActor* ALalalandStage::AddBox(const FString& Name, const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bCollision)
{
    AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
#if WITH_EDITOR
    Actor->SetActorLabel(Name);
#endif
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_Tint.M_Tint"));
    Actor->GetStaticMeshComponent()->SetStaticMesh(Cube);
    Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Actor->SetActorScale3D(Scale);
    Actor->GetStaticMeshComponent()->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (Base)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Actor);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Material->SetScalarParameterValue(TEXT("Roughness"), .7f);
        Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
    }
    return Actor;
}

void ALalalandStage::ApplyMaterial(AStaticMeshActor* Actor, const TCHAR* AssetPath)
{
    if (!Actor) return;
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, AssetPath))
    {
        Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
    }
}

AStaticMeshActor* ALalalandStage::AddCylinder(const FString& Name, const FVector& Location, const FVector& Scale, const FLinearColor& Color)
{
    AStaticMeshActor* Actor = AddBox(Name, Location, Scale, Color);
    Actor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    return Actor;
}

void ALalalandStage::BuildArchitecture()
{
    const FLinearColor Charcoal(.025f, .032f, .04f);
    const FLinearColor Brass(.42f, .19f, .055f);
    const FLinearColor Walnut(.21f, .075f, .028f);
    const FLinearColor MidnightGlass(.018f, .075f, .12f);
    // The authored service map uses metres. World X/Y below are the exact
    // service x/z values multiplied by 100, so navigation, perception and
    // visible furniture all describe the same room.
    AStaticMeshActor* Floor = AddBox(TEXT("BarFloor"), FVector(0, 0, -22), FVector(14, 10, .22f), FLinearColor(.055f, .045f, .038f));
    ApplyMaterial(Floor, TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
    // Runtime-spawned BasicShape collision is not guaranteed to be ready before
    // CharacterMovement's first floor test. A dedicated primitive keeps the
    // player and A-D at capsule height and is also deterministic in packaged builds.
    UBoxComponent* FloorCollider = NewObject<UBoxComponent>(this, TEXT("BarFloorCollider"));
    FloorCollider->SetupAttachment(RootComponent);
    FloorCollider->SetBoxExtent(FVector(1400.f, 1000.f, 22.f));
    FloorCollider->SetRelativeLocation(FVector(0.f, 0.f, -22.f));
    FloorCollider->SetCollisionProfileName(TEXT("BlockAll"));
    AddInstanceComponent(FloorCollider);
    FloorCollider->RegisterComponent();
    ArchitectureColliders.Add(FloorCollider);
    // Elevator car: centre (-100,-750), exit at Y=-500, player starts at
    // (-100,-865). This is now a real connected space instead of a prop on
    // the west wall of the bar.
    AStaticMeshActor* ElevatorFloor = AddBox(TEXT("ElevatorFloor"), FVector(-100, -750, -21), FVector(4, 5, .21f), FLinearColor(.08f, .075f, .07f));
    ApplyMaterial(ElevatorFloor, TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
    UBoxComponent* ElevatorFloorCollider = NewObject<UBoxComponent>(this, TEXT("ElevatorFloorCollider"));
    ElevatorFloorCollider->SetupAttachment(RootComponent);
    ElevatorFloorCollider->SetBoxExtent(FVector(200.f, 250.f, 21.f));
    ElevatorFloorCollider->SetRelativeLocation(FVector(-100.f, -750.f, -21.f));
    ElevatorFloorCollider->SetCollisionProfileName(TEXT("BlockAll"));
    AddInstanceComponent(ElevatorFloorCollider);
    ElevatorFloorCollider->RegisterComponent();
    ArchitectureColliders.Add(ElevatorFloorCollider);

    AddBox(TEXT("Ceiling"), FVector(0, 0, 455), FVector(14, 10, .12f), Charcoal, false);
    AddBox(TEXT("ElevatorCeiling"), FVector(-100, -750, 390), FVector(4, 5, .12f), Charcoal, false);
    AStaticMeshActor* NorthWall = AddBox(TEXT("NorthWall"), FVector(0, 490, 220), FVector(14, .12f, 2.25f), FLinearColor(.055f, .06f, .065f));
    ApplyMaterial(NorthWall, TEXT("/Game/Environment/Materials/M_Plaster.M_Plaster"));
    AddBox(TEXT("SouthWallLeft"), FVector(-500, -490, 220), FVector(4.f, .12f, 2.25f), Charcoal);
    AddBox(TEXT("SouthWallRight"), FVector(400, -490, 220), FVector(6.f, .12f, 2.25f), Charcoal);
    AddBox(TEXT("ElevatorWallLeft"), FVector(-300, -750, 195), FVector(.12f, 5.f, 1.95f), FLinearColor(.08f, .075f, .07f));
    AddBox(TEXT("ElevatorWallRight"), FVector(100, -750, 195), FVector(.12f, 5.f, 1.95f), FLinearColor(.08f, .075f, .07f));
    AddBox(TEXT("ElevatorBack"), FVector(-100, -995, 195), FVector(4.f, .12f, 1.95f), FLinearColor(.08f, .075f, .07f));
    AddBox(TEXT("ElevatorHeader"), FVector(-100, -500, 377), FVector(4.f, .12f, .42f), Brass);
    ElevatorDoors.Add(AddBox(TEXT("ElevatorDoorLeft"), FVector(-205, -496, 175), FVector(1.9f, .08f, 3.5f), FLinearColor(.12f, .14f, .16f)));
    ElevatorDoors.Add(AddBox(TEXT("ElevatorDoorRight"), FVector(5, -496, 175), FVector(1.9f, .08f, 3.5f), FLinearColor(.12f, .14f, .16f)));
    AddBox(TEXT("ElevatorLightPanel"), FVector(-100, -750, 382), FVector(2.1f, 2.4f, .025f), FLinearColor(.9f, .76f, .52f), false);
    AddBox(TEXT("ElevatorControlPanel"), FVector(88, -765, 155), FVector(.025f, .42f, .95f), FLinearColor(.16f, .18f, .2f), false);

    AStaticMeshActor* BarCounter = AddBox(TEXT("BarCounter"), FVector(-220, 55, 55), FVector(7.2f, .8f, 1.1f), Walnut);
    ApplyMaterial(BarCounter, TEXT("/Game/Environment/Materials/M_Wood.M_Wood"));
    AStaticMeshActor* BarTop = AddBox(TEXT("BarTop"), FVector(-220, 55, 113), FVector(7.5f, .95f, .08f), Brass);
    ApplyMaterial(BarTop, TEXT("/Game/Environment/Materials/M_Wood.M_Wood"));
    AStaticMeshActor* BackbarPicture = AddBox(TEXT("BackbarPicture"), FVector(-220, 475, 240), FVector(8.f, .025f, 2.05f), FLinearColor::White, false);
    ApplyMaterial(BackbarPicture, TEXT("/Game/Environment/Materials/M_BackbarWarm.M_BackbarWarm"));
    for (int32 Index = 0; Index < 11; ++Index)
    {
        const float X = -550.f + Index * 66.f;
        AddCylinder(FString::Printf(TEXT("Bottle_%02d"), Index), FVector(X, 410, 150 + (Index % 3) * 28), FVector(.07f, .07f, .25f + .05f * (Index % 2)),
            Index % 2 ? FLinearColor(.42f, .13f, .045f) : FLinearColor(.05f, .24f, .22f));
    }

    AStaticMeshActor* MainTable = AddCylinder(TEXT("MainTable"), FVector(165, -180, 70), FVector(1.6f, 1.6f, .12f), Walnut);
    ApplyMaterial(MainTable, TEXT("/Game/Environment/Materials/M_Wood.M_Wood"));
    AddCylinder(TEXT("MainTableBase"), FVector(165, -180, 34), FVector(.28f, .28f, .7f), Brass);
    const TArray<FVector> ChairPositions = {FVector(-25,-180,42), FVector(355,-180,42), FVector(165,-370,42), FVector(165,10,42)};
    for (int32 Index = 0; Index < ChairPositions.Num(); ++Index)
    {
        AStaticMeshActor* Chair = AddCylinder(FString::Printf(TEXT("Chair_%02d"), Index), ChairPositions[Index], FVector(.42f,.42f,.42f), FLinearColor(.11f,.055f,.035f));
        ApplyMaterial(Chair, TEXT("/Game/Environment/Materials/M_Leather.M_Leather"));
    }
    ThirdDrink = AddCylinder(TEXT("ThirdDrink"), FVector(220, -210, 105), FVector(.12f, .12f, .28f), FLinearColor(.55f, .18f, .08f));
    ThirdDrink->Tags.Add(TEXT("ThirdDrink"));
    ThirdDrink->SetActorHiddenInGame(true);
    ThirdDrink->SetActorEnableCollision(false);
    AddBox(TEXT("TerraceFrame"), FVector(690, -80, 220), FVector(.16f, 8.1f, 2.25f), Brass);
    AddBox(TEXT("TerraceGlass"), FVector(696, -80, 220), FVector(.05f, 7.8f, 2.05f), MidnightGlass, false);
    AddBox(TEXT("CityGlowA"), FVector(850, -330, 160), FVector(.45f, .6f, 1.6f), FLinearColor(.02f,.1f,.22f), false);
    AddBox(TEXT("CityGlowB"), FVector(900, 10, 115), FVector(.35f, .55f, 1.15f), FLinearColor(.12f,.035f,.18f), false);
    AddBox(TEXT("CityGlowC"), FVector(870, 330, 195), FVector(.5f, .42f, 1.95f), FLinearColor(.025f,.16f,.19f), false);
    AStaticMeshActor* BoothBack = AddBox(TEXT("BoothBack"), FVector(-490, -390, 105), FVector(2.8f, .36f, 1.05f), FLinearColor(.16f,.045f,.04f));
    AStaticMeshActor* BoothSeat = AddBox(TEXT("BoothSeat"), FVector(-490, -300, 45), FVector(2.8f, .8f, .42f), FLinearColor(.11f,.035f,.032f));
    ApplyMaterial(BoothBack, TEXT("/Game/Environment/Materials/M_Leather.M_Leather"));
    ApplyMaterial(BoothSeat, TEXT("/Game/Environment/Materials/M_Leather.M_Leather"));
    AddBox(TEXT("CorridorReserve"), FVector(645, -250, 135), FVector(1.f, 2.1f, 1.35f), Charcoal);
    AddBox(TEXT("WarmCeilingStrip"), FVector(-220, 70, 408), FVector(7.5f, .055f, .035f), FLinearColor(1.f,.24f,.045f), false);
    AddBox(TEXT("CoolWindowStrip"), FVector(675, -80, 410), FVector(.04f, 7.8f, .035f), FLinearColor(.05f,.28f,1.f), false);
}

void ALalalandStage::BuildLighting()
{
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(1.2f);
    ADirectionalLight* Moon = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 360), FRotator(-48.f, -28.f, 0));
    Moon->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Moon->GetLightComponent()->SetLightColor(FLinearColor(.22f, .34f, .68f));
    Moon->GetLightComponent()->SetIntensity(2.4f);
    const TArray<TPair<FVector, FLinearColor>> Lamps = {
        {FVector(-520, -260, 290), FLinearColor(.95f, .35f, .15f)},
        {FVector(150, -120, 330), FLinearColor(1.f, .32f, .12f)},
        {FVector(-220, 150, 310), FLinearColor(1.f, .2f, .055f)},
        {FVector(580, -120, 300), FLinearColor(.08f, .25f, 1.f)},
        {FVector(-500, -300, 270), FLinearColor(.55f, .12f, .08f)}
    };
    for (const auto& Lamp : Lamps)
    {
        APointLight* Light = GetWorld()->SpawnActor<APointLight>(Lamp.Key, FRotator::ZeroRotator);
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Light->GetLightComponent()->SetLightColor(Lamp.Value);
        Light->GetLightComponent()->SetIntensity(65000.f);
        CastChecked<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(1050.f);
    }
    APointLight* Fill = GetWorld()->SpawnActor<APointLight>(FVector(-100.f, -80.f, 390.f), FRotator::ZeroRotator);
    Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Fill->GetLightComponent()->SetLightColor(FLinearColor(.28f, .34f, .5f));
    Fill->GetLightComponent()->SetIntensity(42000.f);
    Fill->GetLightComponent()->SetCastShadows(false);
    CastChecked<UPointLightComponent>(Fill->GetLightComponent())->SetAttenuationRadius(2100.f);
    APointLight* ElevatorLight = GetWorld()->SpawnActor<APointLight>(FVector(-100.f, -780.f, 310.f), FRotator::ZeroRotator);
    ElevatorLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    ElevatorLight->GetLightComponent()->SetLightColor(FLinearColor(.95f, .73f, .48f));
    ElevatorLight->GetLightComponent()->SetIntensity(38000.f);
    ElevatorLight->GetLightComponent()->SetCastShadows(false);
    CastChecked<UPointLightComponent>(ElevatorLight->GetLightComponent())->SetAttenuationRadius(620.f);
    APostProcessVolume* Grade = GetWorld()->SpawnActor<APostProcessVolume>();
    Grade->bUnbound = true;
    Grade->Priority = 10.f;
    Grade->Settings.bOverride_AutoExposureMethod = true;
    Grade->Settings.AutoExposureMethod = AEM_Manual;
    Grade->Settings.bOverride_AutoExposureBias = true;
    Grade->Settings.AutoExposureBias = 3.f;
    Grade->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Grade->Settings.AutoExposureApplyPhysicalCameraExposure = false;
}

void ALalalandStage::SpawnCast()
{
    const TMap<FString, FLinearColor> Colors = {
        {TEXT("A"), FLinearColor(.16f, .32f, .45f)}, {TEXT("B"), FLinearColor(.62f, .18f, .12f)},
        {TEXT("C"), FLinearColor(.22f, .42f, .28f)}, {TEXT("D"), FLinearColor(.46f, .22f, .55f)},
        {TEXT("BARTENDER"), FLinearColor(.48f, .35f, .16f)}
    };
    const TMap<FString, FVector> Positions = {
        {TEXT("A"), FVector(-20, -260, 92)}, {TEXT("B"), FVector(-220, 0, 92)},
        {TEXT("C"), FVector(340, 120, 92)}, {TEXT("D"), FVector(-100, -425, 92)},
        {TEXT("BARTENDER"), FVector(-220, 110, 92)}
    };
    for (const auto& Pair : Positions)
    {
        ALalalandNpcCharacter* Npc = GetWorld()->SpawnActor<ALalalandNpcCharacter>(Pair.Value, FRotator::ZeroRotator);
        Npc->InitializeActor(Pair.Key, Colors.FindRef(Pair.Key));
        if (Pair.Key == TEXT("D"))
        {
            Npc->SetActorHiddenInGame(true);
            Npc->SetActorEnableCollision(false);
        }
        Cast.Add(Pair.Key, Npc);
    }
}

void ALalalandStage::RefreshFromService()
{
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    if (CurrentSessionId != Service->GetState().sessionId)
    {
        CurrentSessionId = Service->GetState().sessionId;
        DisplayedEvents.Empty();
        for (const FLalalandEventDto& Existing : Service->GetState().events) DisplayedEvents.Add(Existing.id);
    }
    if (ThirdDrink)
    {
        ThirdDrink->SetActorHiddenInGame(!Service->GetState().scene1.drinkPlaced);
        ThirdDrink->SetActorEnableCollision(Service->GetState().scene1.drinkPlaced);
    }
    const bool bInBar = Service->GetState().intro.phase != TEXT("elevator");
    if (bInBar != bBarEntranceOpened)
    {
        bBarEntranceOpened = bInBar;
        for (AStaticMeshActor* Door : ElevatorDoors)
        {
            if (!Door) continue;
            Door->SetActorHiddenInGame(bInBar);
            Door->SetActorEnableCollision(!bInBar);
        }
    }
    TMap<FString, ALalalandNpcCharacter*> RawCast;
    for (const auto& Pair : Cast) RawCast.Add(Pair.Key, Pair.Value.Get());
    for (const FLalalandActorDto& Actor : Service->GetState().characters)
    {
        if (TObjectPtr<ALalalandNpcCharacter>* Npc = Cast.Find(Actor.id)) (*Npc)->ApplyState(Actor, RawCast);
    }
    for (const FLalalandEventDto& Event : Service->GetState().events)
    {
        if (DisplayedEvents.Contains(Event.id) || Event.text.IsEmpty()) continue;
        DisplayedEvents.Add(Event.id);
        if (Event.objectTarget == TEXT("third_drink") && Event.type == TEXT("action"))
        {
            if (USoundBase* Cup = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Cup.SW_Cup")))
                UGameplayStatics::PlaySoundAtLocation(this, Cup, ThirdDrink ? ThirdDrink->GetActorLocation() : FVector(220, -210, 105));
        }
        if (Event.actor == TEXT("D") && Event.type == TEXT("action"))
        {
            if (USoundBase* Arrival = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Arrival.SW_Arrival")))
                UGameplayStatics::PlaySoundAtLocation(this, Arrival, FVector(-100, -425, 92));
        }
        if (TObjectPtr<ALalalandNpcCharacter>* Npc = Cast.Find(Event.actor)) (*Npc)->ShowDialogue(Event.text);
    }
}
