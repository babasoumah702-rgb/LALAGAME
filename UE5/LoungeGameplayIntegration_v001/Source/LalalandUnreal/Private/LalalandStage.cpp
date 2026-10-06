#include "LalalandStage.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "LalalandNpcCharacter.h"
#include "LalalandBounceGame.h"
#include "LalalandGlassProp.h"
#include "LalalandKit.h"
#include "LalalandServiceSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "LalalandVenueLayout.h"
#include "LalalandSpatial.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "HAL/PlatformFileManager.h"
#include "Kismet/KismetSystemLibrary.h"

ALalalandStage::ALalalandStage()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    AmbientAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudio"));
    AmbientAudio->SetupAttachment(RootComponent);
    AmbientAudio->bAutoActivate = false;
    AmbientAudio->SetVolumeMultiplier(.2f);
}

void ALalalandStage::BeginPlay()
{
    Super::BeginPlay();
    BuildFutureArchitecture();
    BuildFutureDetails();
    BuildFutureLighting();
    BuildFutureAtmosphere();
    BounceGame = GetWorld()->SpawnActor<ALalalandBounceGame>(LalalandVenue::Named(TEXT("bounceOffset")), FRotator::ZeroRotator);
    SpawnCast();
    ArrivalSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Arrival.SW_Arrival"));
    CupSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Cup.SW_Cup"));
    DoorSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Door.SW_Door"));
    ElevatorSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Elevator.SW_Elevator"));
    LoungeSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Lounge.SW_Lounge"));
    PhoneSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_Phone.SW_Phone"));
    if (USoundWave* Lounge = ::Cast<USoundWave>(LoungeSound.Get()))
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
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandPlayerGripPreview"))) StartPlayerGripReview();
    else if(FParse::Param(FCommandLine::Get(),TEXT("LalalandSpatialPreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandAtmospherePreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandFurniturePreview"))) StartSpatialReview();
    else if(FParse::Param(FCommandLine::Get(),TEXT("LalalandElevatorPreview"))) StartElevatorReview();
    else if(FParse::Param(FCommandLine::Get(),TEXT("LalalandLayoutPreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandFeedbackPreview"))) StartLayoutPreview();
}

AStaticMeshActor* ALalalandStage::AddBox(const FString& Name, const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bCollision)
{
    const bool bMovedBar=Name==TEXT("BarCounter")||Name==TEXT("BarTop")||Name==TEXT("BarFootRail")||Name==TEXT("WarmCeilingStrip");
    AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(bMovedBar?LalalandVenue::Bar(Location):Location, FRotator::ZeroRotator);
    Actor->Tags.Add(FName(*Name));
#if WITH_EDITOR
    Actor->SetActorLabel(Name);
#endif
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    const bool bGlow = Name.Contains(TEXT("Light")) || Name.Contains(TEXT("Strip")) || Name.StartsWith(TEXT("RoofStar")) || Name.StartsWith(TEXT("Meteor")) || Name == TEXT("VenueSky");
    const bool bMetal = Name.StartsWith(TEXT("ElevatorWall")) || Name == TEXT("ElevatorBack") || Name.StartsWith(TEXT("ElevatorDoor"));
    const TCHAR* MaterialPath = Name == TEXT("VenueSky") ? TEXT("/Game/Environment/Materials/M_VenueSky.M_VenueSky") : bGlow ? TEXT("/Game/Environment/Materials/M_VenueGlow.M_VenueGlow") : bMetal ? TEXT("/Game/Environment/Materials/M_VenueMetal.M_VenueMetal") : TEXT("/Game/Environment/Materials/M_Tint.M_Tint");
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
    Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Actor->GetStaticMeshComponent()->SetStaticMesh(Cube);
    FVector ActualScale=Scale;
    // The old shell used half-height cubes centred at 220cm, leaving blue
    // gaps below/above every wall. Only the room shell changes, not portals.
    if(Name==TEXT("NorthWall")||Name==TEXT("WestWall")||Name.StartsWith(TEXT("SouthWall")))ActualScale.Z=4.4f;
    Actor->SetActorScale3D(ActualScale);
    Actor->GetStaticMeshComponent()->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (Base)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Actor);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Material->SetScalarParameterValue(TEXT("Roughness"), bMetal ? .28f : .7f);
        Material->SetScalarParameterValue(TEXT("Intensity"), Name == TEXT("VenueSky") ? .65f : 2.5f);
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
    AStaticMeshActor* Floor = AddBox(TEXT("BarFloor"), FVector(-150, 0, -22), FVector(17, 10, .22f), FLinearColor(.055f, .045f, .038f));
    ApplyMaterial(Floor, TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
    // Runtime-spawned BasicShape collision is not guaranteed to be ready before
    // CharacterMovement's first floor test. A dedicated primitive keeps the
    // player and A-D at capsule height and is also deterministic in packaged builds.
    UBoxComponent* FloorCollider = NewObject<UBoxComponent>(this, TEXT("BarFloorCollider"));
    FloorCollider->SetupAttachment(RootComponent);
    FloorCollider->SetBoxExtent(FVector(850.f, 500.f, 22.f));
    FloorCollider->SetRelativeLocation(FVector(-150.f, 0.f, -22.f));
    FloorCollider->SetCollisionProfileName(TEXT("BlockAll"));
    AddInstanceComponent(FloorCollider);
    FloorCollider->RegisterComponent();
    ArchitectureColliders.Add(FloorCollider);
    // The stair route goes around the south wall through a real corridor.
    // It formerly depended on an oversized, invisible bar-floor collider.
    AStaticMeshActor* CorridorFloor=AddBox(TEXT("SouthCorridorFloor"),FVector(300.f,-575.f,-10.f),FVector(11.f,1.8f,.2f),FLinearColor(.065f,.075f,.095f),false);
    ApplyMaterial(CorridorFloor,TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
    UBoxComponent* CorridorCollider=NewObject<UBoxComponent>(this,TEXT("SouthCorridorFloorCollider"));
    CorridorCollider->SetupAttachment(RootComponent);
    CorridorCollider->SetBoxExtent(FVector(550.f,90.f,10.f));
    CorridorCollider->SetRelativeLocation(FVector(300.f,-575.f,-10.f));
    CorridorCollider->SetCollisionProfileName(TEXT("BlockAll"));
    AddInstanceComponent(CorridorCollider);CorridorCollider->RegisterComponent();ArchitectureColliders.Add(CorridorCollider);
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

    // The bar ceiling is also the structural underside of the upper terrace.
    // Keeping it below the rooftop slab avoids the old 29 cm visual cavity
    // that placed the camera above a ceiling while standing on the roof.
    // Lounge wall rhythm breaks up the enlarged blank wall, outside the aisle.
    for (int32 Panel=0;Panel<8;++Panel)
        AddBox(FString::Printf(TEXT("WestAcousticSlat_%02d"),Panel),FVector(-1012.f,-300.f+Panel*82.f,190.f),
            FVector(.035f,.14f,2.7f),Walnut,false);
    AddBox(TEXT("LoungeLightStrip"),FVector(-1006.f,30.f,327.f),FVector(.025f,7.f,.035f),FLinearColor(.8f,.62f,.4f),false);
    AddBox(TEXT("Ceiling"), FVector(-150, 0, 390), FVector(17, 10, .12f), Charcoal, false);
    AddBox(TEXT("ElevatorCeiling"), FVector(-100, -750, 390), FVector(4, 5, .12f), Charcoal, false);
    AStaticMeshActor* NorthWall = AddBox(TEXT("NorthWall"), FVector(-150, 490, 220), FVector(17, .12f, 2.25f), FLinearColor(.055f, .06f, .065f));
    AddBox(TEXT("WestWall"), FVector(-1020,0,220), FVector(.12f,10.f,2.25f), Charcoal);
    ApplyMaterial(NorthWall, TEXT("/Game/Environment/Materials/M_Plaster.M_Plaster"));
    // Two real openings: the elevator at X -300..100 and the lit stair at X 665..825.
    AddBox(TEXT("SouthWallLeft"), FVector(-1025, -490, 220), FVector(9.f, .12f, 2.25f), Charcoal);
    AddBox(TEXT("SouthWallMiddle"), FVector(382.5f, -490, 220), FVector(2.825f, .12f, 2.25f), Charcoal);
    AddBox(TEXT("SouthWallRight"), FVector(1112.5f, -490, 220), FVector(2.875f, .12f, 2.25f), Charcoal);
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
    AStaticMeshActor* LowShelf=AddBox(TEXT("BackbarShelfLow"), FVector(-220, 428, 128), FVector(7.6f, .28f, .05f), Brass);
    LowShelf->GetStaticMeshComponent()->CanCharacterStepUpOn=ECB_No;
    // The navigation footprint reserves the cabinet beneath this overhang.
    // Represent that same solid volume in physics, rather than a thin ledge
    // which CharacterMovement can try to step under and trap a capsule.
    UBoxComponent* BackbarCabinet=NewObject<UBoxComponent>(this,TEXT("BackbarCabinetCollider"));
    BackbarCabinet->SetupAttachment(RootComponent);BackbarCabinet->SetBoxExtent(FVector(380.f,14.f,64.f));
    BackbarCabinet->SetRelativeLocation(FVector(-220.f,428.f,64.f));BackbarCabinet->SetCollisionProfileName(TEXT("BlockAll"));
    BackbarCabinet->CanCharacterStepUpOn=ECB_No;AddInstanceComponent(BackbarCabinet);BackbarCabinet->RegisterComponent();ArchitectureColliders.Add(BackbarCabinet);
    AddBox(TEXT("BackbarShelfHigh"), FVector(-220, 428, 208), FVector(7.6f, .28f, .05f), Brass);
    AddBox(TEXT("GlassRack"), FVector(-220, 438, 292), FVector(5.8f, .08f, .04f), FLinearColor(.12f, .12f, .14f));

    // The v0.2 handoff uses a long bounce table built by ALalalandBounceGame.
    // Keep only two small lounge tables here so the old round Scene-1 table cannot overlap it.
    for (int32 Index = 1; Index < 2; ++Index)
    {
        const FVector P = Index == 0 ? FVector(-790,-285,58) : FVector(585,285,58);
        AStaticMeshActor* LoungeTable = AddCylinder(FString::Printf(TEXT("LoungeTable_%02d"), Index), P, FVector(.62f,.62f,.1f), Walnut);
        ApplyMaterial(LoungeTable, TEXT("/Game/Environment/Materials/M_Wood.M_Wood"));
    }
    // Leave a real 2.4 m doorway in the terrace frame aligned with the south
    // stair opening. The previous single colliding beam formed an invisible
    // floor-to-ceiling wall and made the authored rooftop route impossible.
    // A second opening at the south end provides the front approach to the
    // staircase. The pieces remain visible while explicit primitive colliders
    // below define the two solid spans.
    AddBox(TEXT("TerraceFrameFarSouth"), FVector(690, -795, 220), FVector(.16f, 1.9f, 2.25f), Brass, false);
    AddBox(TEXT("TerraceFrameSouthPost"), FVector(690, -535, 220), FVector(.16f, .3f, 2.25f), Brass, false);
    AddBox(TEXT("TerraceFrameNorth"), FVector(690, 225, 220), FVector(.16f, 10.1f, 2.25f), Brass, false);
    const TArray<FName> FrameNames = {TEXT("TerraceFrameFarSouthCollider"), TEXT("TerraceFrameSouthPostCollider"), TEXT("TerraceFrameNorthCollider")};
    const TArray<FVector> FrameLocations = {FVector(690.f, -795.f, 220.f), FVector(690.f, -535.f, 220.f), FVector(690.f, 225.f, 220.f)};
    const TArray<FVector> FrameExtents = {FVector(16.f, 95.f, 225.f), FVector(16.f, 15.f, 225.f), FVector(16.f, 505.f, 225.f)};
    for (int32 FrameIndex = 0; FrameIndex < FrameNames.Num(); ++FrameIndex)
    {
        UBoxComponent* FrameCollider = NewObject<UBoxComponent>(this, FrameNames[FrameIndex]);
        FrameCollider->SetupAttachment(RootComponent);
        FrameCollider->SetBoxExtent(FrameExtents[FrameIndex]);
        FrameCollider->SetRelativeLocation(FrameLocations[FrameIndex]);
        FrameCollider->SetCollisionProfileName(TEXT("BlockAll"));
        AddInstanceComponent(FrameCollider);
        FrameCollider->RegisterComponent();
        ArchitectureColliders.Add(FrameCollider);
    }
    AddBox(TEXT("TerraceGlass"), FVector(696, -80, 220), FVector(.05f, 7.8f, 2.05f), MidnightGlass, false);
    for (int32 Mullion = 0; Mullion < 5; ++Mullion)
    {
        AddBox(FString::Printf(TEXT("TerraceMullion_%02d"), Mullion), FVector(692.f, -380.f + Mullion * 155.f, 220.f), FVector(.08f, .06f, 2.05f), Brass, false);
    }
    AddBox(TEXT("TerraceSill"), FVector(692, -80, 118), FVector(.18f, 7.9f, .08f), Brass, false);
    AddBox(TEXT("PilotWindowLowerWall"),FVector(690,-80,58),FVector(.12f,7.8f,1.16f),Charcoal);
    AddBox(TEXT("CityGlowA"), FVector(850, -330, 160), FVector(.45f, .6f, 1.6f), FLinearColor(.02f,.1f,.22f), false);
    AddBox(TEXT("CityGlowB"), FVector(900, 10, 115), FVector(.35f, .55f, 1.15f), FLinearColor(.12f,.035f,.18f), false);
    AddBox(TEXT("CityGlowC"), FVector(870, 330, 195), FVector(.5f, .42f, 1.95f), FLinearColor(.025f,.16f,.19f), false);
    int32 BoothIndex=0;
    for(const auto& Value:LalalandVenue::Layout()->GetArrayField(TEXT("booths")))
    {
        const FVector P=LalalandVenue::Point(Value->AsObject());
        for(int32 Side:{-1,1})
        {
            AStaticMeshActor* Seat=AddBox(FString::Printf(TEXT("PilotBooth_%d_Seat_%d"),BoothIndex,Side),P+FVector(0,Side*85.f,45.f),FVector(1.8f,.7f,.42f),FLinearColor(.11f,.035f,.032f));
            AStaticMeshActor* Back=AddBox(FString::Printf(TEXT("PilotBooth_%d_Back_%d"),BoothIndex,Side),P+FVector(0,Side*120.f,86.f),FVector(1.8f,.16f,.8f),FLinearColor(.16f,.045f,.04f));
            ApplyMaterial(Seat,TEXT("/Game/Environment/Materials/M_Leather.M_Leather"));
            ApplyMaterial(Back,TEXT("/Game/Environment/Materials/M_Leather.M_Leather"));
        }
        ApplyMaterial(AddBox(FString::Printf(TEXT("PilotBooth_%d_Table"),BoothIndex),P+FVector(0,0,62.f),FVector(1.2f,.65f,.1f),Walnut),TEXT("/Game/Environment/Materials/M_Wood.M_Wood"));
        ++BoothIndex;
    }
    // No CorridorReserve block in the public circulation aisle.
    AddBox(TEXT("WarmCeilingStrip"), FVector(-220, 70, 408), FVector(7.5f, .055f, .035f), FLinearColor(1.f,.24f,.045f), false);
    AddBox(TEXT("CoolWindowStrip"), FVector(675, -80, 410), FVector(.04f, 7.8f, .035f), FLinearColor(.05f,.28f,1.f), false);

    // Continuous physical route matching NightNavigator: corridor -> stairs -> upper rooftop.
    AddBox(TEXT("StairLandingBottom"), FVector(745,-560,8), FVector(1.4f,.7f,.08f), Charcoal);
    for (int32 Step = 0; Step < 13; ++Step)
    {
        const float Top = 32.f * (Step + 1);
        AStaticMeshActor* Stair = AddBox(FString::Printf(TEXT("RoofStair_%02d"), Step),
            FVector(745.f, -535.f + Step * 64.f, Top * .5f), FVector(1.4f,.68f,Top / 100.f), FLinearColor(.07f,.075f,.09f));
        ApplyMaterial(Stair, TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
        AddBox(FString::Printf(TEXT("StairLight_%02d"), Step), FVector(630.f,-535.f+Step*64.f,Top+2.f), FVector(.18f,.26f,.018f), FLinearColor(.08f,.3f,1.f), false);
    }
    AStaticMeshActor* RoofFloor = AddBox(TEXT("UpperRooftop"), FVector(130,575,400), FVector(13.5f,6.5f,.4f), FLinearColor(.065f,.075f,.095f));
    ApplyMaterial(RoofFloor, TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
    UBoxComponent* RoofFloorCollider = NewObject<UBoxComponent>(this, TEXT("UpperRooftopCollider"));
    RoofFloorCollider->SetupAttachment(RootComponent); RoofFloorCollider->SetBoxExtent(FVector(675.f,325.f,20.f));
    RoofFloorCollider->SetRelativeLocation(FVector(130.f,575.f,400.f)); RoofFloorCollider->SetCollisionProfileName(TEXT("BlockAll"));
    AddInstanceComponent(RoofFloorCollider); RoofFloorCollider->RegisterComponent(); ArchitectureColliders.Add(RoofFloorCollider);
    AddBox(TEXT("RoofRailNorth"), FVector(130,895,475), FVector(13.5f,.06f,.75f), MidnightGlass);
    AddBox(TEXT("RoofRailWest"), FVector(-540,575,475), FVector(.06f,6.5f,.75f), MidnightGlass);
    AddBox(TEXT("RoofRailEast"), FVector(800,575,475), FVector(.06f,6.5f,.75f), MidnightGlass);
    // Two-sided unlit enclosing sky: no ceiling plane or collision above the terrace.
    AStaticMeshActor* VenueSky = AddBox(TEXT("VenueSky"), FVector::ZeroVector, FVector(300.f), FLinearColor(.035f,.065f,.12f), false);
    VenueSky->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    VenueSky->GetStaticMeshComponent()->SetCastShadow(false);
    for (int32 Lamp = 0; Lamp < 7; ++Lamp)
        AddBox(FString::Printf(TEXT("RoofStringLight_%02d"), Lamp), FVector(-430.f+Lamp*185.f,760.f,590.f), FVector(.035f,.035f,.035f), FLinearColor(1.f,.55f,.15f), false);
    AStaticMeshActor* RoofBench = AddBox(TEXT("RoofBench"), FVector(-185,650,458), FVector(2.1f,.62f,.28f), FLinearColor(.12f,.035f,.055f));
    ApplyMaterial(RoofBench, TEXT("/Game/Environment/Materials/M_Leather.M_Leather"));
    AddBox(TEXT("RoofBenchBack"), FVector(-185,705,505), FVector(2.1f,.16f,.68f), FLinearColor(.09f,.025f,.05f));
    AStaticMeshActor* RoofRug = AddBox(TEXT("RoofRug"), FVector(245,590,423), FVector(2.5f,1.55f,.025f), FLinearColor(.08f,.12f,.22f), false);
    // Cloth tint stays distinct from the wooden terrace decking.
    (void)RoofRug;
    AStaticMeshActor* RoofTable = AddCylinder(TEXT("RoofLowTable"), FVector(245,590,450), FVector(.72f,.72f,.16f), FLinearColor(.28f,.09f,.035f));
    ApplyMaterial(RoofTable, TEXT("/Game/Environment/Materials/M_Wood.M_Wood"));
    LalalandKit::WineGlass(this, RootComponent, TEXT("RoofWineA"), FVector(220.f,575.f,462.f), FLinearColor(.62f,.78f,.28f), .9f);
    LalalandKit::Coupe(this, RootComponent, TEXT("RoofWineB"), FVector(275.f,605.f,462.f), FLinearColor(.88f,.52f,.12f), .9f);
    for (int32 Star = 0; Star < 30; ++Star)
    {
        const float X = -620.f + static_cast<float>((Star * 173) % 1450);
        const float Y = 180.f + static_cast<float>((Star * 97) % 850);
        const float Z = 735.f + static_cast<float>((Star * 47) % 210);
        AddBox(FString::Printf(TEXT("RoofStar_%02d"), Star), FVector(X,Y,Z), FVector(.012f,.012f,.012f),
            Star % 4 == 0 ? FLinearColor(.3f,.55f,1.f) : FLinearColor(1.f,.72f,.34f), false);
    }
    for (int32 Planter = 0; Planter < 4; ++Planter)
    {
        const float X = -400.f + Planter * 265.f;
        AddBox(FString::Printf(TEXT("RoofPlanter_%02d"), Planter), FVector(X,820,452), FVector(.65f,.35f,.32f), FLinearColor(.12f,.075f,.04f));
        for (int32 Leaf = 0; Leaf < 7; ++Leaf)
        {
            AStaticMeshActor* Foliage = AddBox(FString::Printf(TEXT("RoofLeaf_%02d_%02d"), Planter, Leaf),
                FVector(X-44.f+Leaf*14.f, 820.f+(Leaf%3-1)*17.f, 485.f+(Leaf%3)*10.f),
                FVector(.3f,.045f,.25f), FLinearColor(.055f,.19f+.012f*Leaf,.09f), false);
            Foliage->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            Foliage->SetActorRotation(FRotator(0.f,Leaf*31.f,Leaf%2 ? 28.f : -28.f));
        }
        for (int32 Flower = 0; Flower < 5; ++Flower)
        {
            AStaticMeshActor* Bloom = AddBox(FString::Printf(TEXT("RoofFlower_%02d_%02d"), Planter, Flower),
                FVector(X-30.f+Flower*15.f,799.f,490.f+(Flower%2)*8.f), FVector(.08f,.08f,.055f),
                FLinearColor(.78f,.73f,.62f), false);
            Bloom->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        }
    }
    // Distant skyline remains outside all playable/navigation geometry. Instance windows
    // in one component rather than spawning a light actor for every apartment.
    UInstancedStaticMeshComponent* Windows = NewObject<UInstancedStaticMeshComponent>(this, TEXT("CityWindows"));
    AddInstanceComponent(Windows);
    Windows->SetupAttachment(RootComponent);
    Windows->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Windows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Windows->SetCastShadow(false);
    Windows->RegisterComponent();
    if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/Materials/M_VenueGlow.M_VenueGlow")))
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Glow,this);
        Material->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.55f,.64f,.8f));
        Material->SetScalarParameterValue(TEXT("Intensity"),1.3f);
        Windows->SetMaterial(0,Material);
    }
    for (int32 Tower = 0; Tower < 28; ++Tower)
    {
        const float Angle = Tower * 360.f / 28.f;
        const FVector Radial(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle)), 0.f);
        const FVector Tangent(-Radial.Y,Radial.X,0.f);
        const FVector Center = FVector(130.f,575.f,0.f) + Radial * (2200.f+(Tower%3)*230.f);
        const float Height = 760.f+(Tower%5)*165.f;
        AStaticMeshActor* Building = AddBox(FString::Printf(TEXT("RoofCity_%02d"),Tower), Center+FVector(0,0,Height*.5f),
            FVector(2.3f,1.8f,Height/100.f), FLinearColor(.035f,.055f,.09f),false);
        Building->SetActorRotation(FRotator(0.f,Angle,0.f));
        for (int32 Row=0;Row<static_cast<int32>(Height/75.f);++Row)
            for (int32 Column=0;Column<3;++Column)
                if ((Row+Column+Tower)%4 != 0)
                    Windows->AddInstance(FTransform(FRotator(0.f,Angle,0.f),
                        Center-Radial*117.f+Tangent*(-48.f+Column*48.f)+FVector(0,0,55.f+Row*75.f),FVector(.025f,.12f,.16f)));
    }
}

void ALalalandStage::BuildBarDetails()
{
    USceneComponent* Root = RootComponent;
    const TArray<FLinearColor> BottleGlass = {
        FLinearColor(.28f, .05f, .04f), FLinearColor(.04f, .18f, .14f), FLinearColor(.08f, .07f, .22f),
        FLinearColor(.32f, .16f, .04f), FLinearColor(.05f, .12f, .08f), FLinearColor(.22f, .04f, .08f)
    };
    const TArray<FLinearColor> Labels = {
        FLinearColor(.82f, .72f, .48f), FLinearColor(.9f, .9f, .88f), FLinearColor(.55f, .12f, .1f),
        FLinearColor(.12f, .18f, .28f), FLinearColor(.75f, .55f, .2f), FLinearColor(.2f, .32f, .18f)
    };
    for (int32 Index = 0; Index < 10; ++Index)
    {
        const float X = -520.f + Index * 62.f;
        LalalandKit::Bottle(this, Root, FString::Printf(TEXT("ShelfLow_%02d"), Index), FVector(X, 418.f, 132.f),
            BottleGlass[Index % BottleGlass.Num()], Labels[Index % Labels.Num()], .9f + .08f * (Index % 3));
    }
    for (int32 Index = 0; Index < 9; ++Index)
    {
        const float X = -490.f + Index * 68.f;
        LalalandKit::Bottle(this, Root, FString::Printf(TEXT("ShelfHigh_%02d"), Index), FVector(X, 418.f, 212.f),
            BottleGlass[(Index + 2) % BottleGlass.Num()], Labels[(Index + 1) % Labels.Num()], 1.f);
    }
    for (int32 Index = 0; Index < 8; ++Index)
    {
        LalalandKit::WineGlass(this, Root, FString::Printf(TEXT("Hang_%02d"), Index),
            FVector(-430.f + Index * 58.f, 438.f, 278.f), FLinearColor(.7f, .78f, .42f), .85f, true, 0.f);
    }

    const float BarTopZ = 117.f;
    USceneComponent* BarRoot=NewObject<USceneComponent>(this,TEXT("PilotBarDetails"));
    BarRoot->SetupAttachment(RootComponent);BarRoot->SetRelativeLocation(LalalandVenue::Named(TEXT("barOffset")));
    AddInstanceComponent(BarRoot);BarRoot->RegisterComponent();Root=BarRoot;
    LalalandKit::Highball(this, Root, TEXT("BarMint"), FVector(-420.f, 18.f, BarTopZ), FLinearColor(.28f, .72f, .42f));
    LalalandKit::WineGlass(this, Root, TEXT("BarWineA"), FVector(-360.f, 8.f, BarTopZ), FLinearColor(.62f, .78f, .28f));
    LalalandKit::Coupe(this, Root, TEXT("BarCoupe"), FVector(-300.f, 22.f, BarTopZ), FLinearColor(.38f, .16f, .05f));
    LalalandKit::Pilsner(this, Root, TEXT("BarBeer"), FVector(-90.f, 12.f, BarTopZ), FLinearColor(.72f, .52f, .16f));
    LalalandKit::Highball(this, Root, TEXT("BarOolong"), FVector(-30.f, 24.f, BarTopZ), FLinearColor(.55f, .42f, .16f), 1.f, .5f);
    LalalandKit::Shaker(this, Root, TEXT("BarShaker"), FVector(-180.f, 42.f, BarTopZ));
    LalalandKit::IceBucket(this, Root, TEXT("BarIce"), FVector(-140.f, 48.f, BarTopZ));
    LalalandKit::Citrus(this, Root, TEXT("BarLime"), FVector(-248.f, 36.f, BarTopZ), FLinearColor(.42f, .72f, .18f));
    LalalandKit::Citrus(this, Root, TEXT("BarOrange"), FVector(-232.f, 28.f, BarTopZ), FLinearColor(.86f, .42f, .08f));
    LalalandKit::NapkinStack(this, Root, TEXT("BarNapkins"), FVector(-70.f, 40.f, BarTopZ));
    LalalandKit::Coaster(this, Root, TEXT("BarCoasterA"), FVector(-420.f, 18.f, BarTopZ));
    LalalandKit::Coaster(this, Root, TEXT("BarCoasterB"), FVector(-360.f, 8.f, BarTopZ));
    AddBox(TEXT("BarFootRail"), FVector(-220.f, 8.f, 22.f), FVector(7.1f, .06f, .05f), FLinearColor(.42f, .19f, .055f));

    const TArray<float> StoolXs = { -330.f, -190.f, -50.f };
    for (int32 Index = 0; Index < StoolXs.Num(); ++Index)
    {
        LalalandKit::BarStool(this, Root, FString::Printf(TEXT("Stool_%02d"), Index), FVector(StoolXs[Index], -65.f, 0.f));
    }

    LalalandKit::Pendant(this, Root, TEXT("PendantA"), FVector(-400.f, 40.f, 360.f));
    LalalandKit::Pendant(this, Root, TEXT("PendantB"), FVector(-220.f, 40.f, 360.f));
    LalalandKit::Pendant(this, Root, TEXT("PendantC"), FVector(-40.f, 40.f, 360.f));
    const TArray<FVector> PendantLights = { FVector(-400.f, 40.f, 338.f), FVector(-220.f, 40.f, 338.f), FVector(-40.f, 40.f, 338.f) };
    for (const FVector& Lamp : PendantLights)
    {
        APointLight* Light = GetWorld()->SpawnActor<APointLight>(LalalandVenue::Bar(Lamp), FRotator::ZeroRotator);
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Light->GetLightComponent()->SetLightColor(FLinearColor(1.f, .55f, .22f));
        Light->GetLightComponent()->SetIntensity(6500.f);
        Light->GetLightComponent()->SetCastShadows(false);
        CastChecked<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(420.f);
    }

    // Native MixologyStation now owns the bar-top tools and craft performance.
    Root=RootComponent;
    const FVector BoothP=LalalandVenue::Point(LalalandVenue::Layout()->GetArrayField(TEXT("booths"))[1]->AsObject(),68.f);
    LalalandKit::WineGlass(this, Root, TEXT("BoothWine"), BoothP+FVector(-15,0,0), FLinearColor(.55f, .12f, .22f), .9f);
    LalalandKit::Highball(this, Root, TEXT("BoothHighball"), BoothP+FVector(15,0,0), FLinearColor(.28f, .72f, .42f), .85f);
    LalalandKit::Coaster(this, Root, TEXT("BoothCoaster"), BoothP+FVector(-15,0,0));
    LalalandKit::WineGlass(this, Root, TEXT("WindowWine"), FVector(585.f, 285.f, 68.f), FLinearColor(.62f, .78f, .28f), .9f);
    LalalandKit::Coupe(this, Root, TEXT("WindowCoupe"), FVector(560.f, 268.f, 68.f), FLinearColor(.38f, .16f, .05f), .85f);
    LalalandKit::WineGlass(this, Root, TEXT("RoofWineA"), FVector(-200.f, 640.f, 474.f), FLinearColor(.72f, .52f, .16f), .95f);
    LalalandKit::WineGlass(this, Root, TEXT("RoofWineB"), FVector(-155.f, 655.f, 474.f), FLinearColor(.55f, .42f, .16f), .9f, false, .4f);
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
        {FVector(-520, -260, 290), FLinearColor(.95f, .62f, .4f)},
        {FVector(150, -120, 330), FLinearColor(1.f, .65f, .4f)},
        {FVector(-220, 150, 310), FLinearColor(1.f, .65f, .38f)},
        {FVector(580, -120, 300), FLinearColor(.08f, .25f, 1.f)},
        {FVector(-800, -300, 270), FLinearColor(.85f, .5f, .35f)}
    };
    for (const auto& Lamp : Lamps)
    {
        APointLight* Light = GetWorld()->SpawnActor<APointLight>(Lamp.Key, FRotator::ZeroRotator);
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Light->GetLightComponent()->SetLightColor(Lamp.Value);
        Light->GetLightComponent()->SetIntensity(18000.f);
        CastChecked<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(1050.f);
    }
    APointLight* Fill = GetWorld()->SpawnActor<APointLight>(FVector(-100.f, -80.f, 320.f), FRotator::ZeroRotator);
    Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Fill->GetLightComponent()->SetLightColor(FLinearColor(.62f, .68f, .8f));
    Fill->GetLightComponent()->SetIntensity(28000.f);
    Fill->GetLightComponent()->SetCastShadows(false);
    CastChecked<UPointLightComponent>(Fill->GetLightComponent())->SetAttenuationRadius(2100.f);
    APointLight* ElevatorLight = GetWorld()->SpawnActor<APointLight>(FVector(-100.f, -780.f, 310.f), FRotator::ZeroRotator);
    ElevatorLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    ElevatorLight->GetLightComponent()->SetLightColor(FLinearColor(.95f, .73f, .48f));
    ElevatorLight->GetLightComponent()->SetIntensity(11000.f);
    ElevatorLight->GetLightComponent()->SetCastShadows(false);
    CastChecked<UPointLightComponent>(ElevatorLight->GetLightComponent())->SetAttenuationRadius(620.f);
    const TArray<TPair<FVector,FLinearColor>> RoofLamps = {
        {FVector(-240.f,650.f,650.f),FLinearColor(1.f,.65f,.38f)},
        {FVector(360.f,600.f,630.f),FLinearColor(.38f,.55f,.9f)},
        {FVector(90.f,820.f,610.f),FLinearColor(1.f,.55f,.2f)}
    };
    for(const auto& Lamp:RoofLamps)
    {
        APointLight* Light=GetWorld()->SpawnActor<APointLight>(Lamp.Key,FRotator::ZeroRotator);
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);Light->GetLightComponent()->SetLightColor(Lamp.Value);
        Light->GetLightComponent()->SetIntensity(34000.f);Light->GetLightComponent()->SetCastShadows(false);
        CastChecked<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(1450.f);
    }
    APointLight* RoofFill = GetWorld()->SpawnActor<APointLight>(FVector(130.f,575.f,780.f), FRotator::ZeroRotator);
    RoofFill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    RoofFill->GetLightComponent()->SetLightColor(FLinearColor(.5f,.62f,.85f));
    RoofFill->GetLightComponent()->SetIntensity(34000.f);
    RoofFill->GetLightComponent()->SetCastShadows(false);
    CastChecked<UPointLightComponent>(RoofFill->GetLightComponent())->SetAttenuationRadius(1900.f);
    APostProcessVolume* Grade = GetWorld()->SpawnActor<APostProcessVolume>();
    Grade->bUnbound = true;
    Grade->Priority = 10.f;
    Grade->Settings.bOverride_AutoExposureMethod = true;
    Grade->Settings.AutoExposureMethod = AEM_Manual;
    Grade->Settings.bOverride_AutoExposureBias = true;
    Grade->Settings.AutoExposureBias = 1.35f;
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
        {TEXT("A"), FVector(-400, -300, 92)}, {TEXT("B"), FVector(-300, 300, 92)},
        {TEXT("C"), FVector(300, -300, 92)}, {TEXT("D"), FVector(400, 300, 92)},
        {TEXT("BARTENDER"), LalalandVenue::Named(TEXT("bartender"),92.f)}
    };
    for (const auto& Pair : Positions)
    {
        FVector Position=Pair.Value;
        if(Pair.Key!=TEXT("BARTENDER"))Position=LalalandVenue::Point(LalalandVenue::Layout()->GetObjectField(TEXT("rest"))->GetObjectField(Pair.Key),92.f);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        ALalalandNpcCharacter* Npc = GetWorld()->SpawnActor<ALalalandNpcCharacter>(Position, FRotator::ZeroRotator, SpawnParameters);
        if (!Npc)
        {
            UE_LOG(LogTemp, Error, TEXT("LALALAND_CAST_SPAWN_FAILED actor=%s position=%s"), *Pair.Key, *Position.ToString());
            continue;
        }
        Npc->InitializeActor(Pair.Key, Colors.FindRef(Pair.Key));
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
        SubmittedSipEffects.Empty();
        SubmittedDeliveryEffects.Empty();
        LastAudioPhase.Empty();
        LastSongChoice.Empty();
        for (const FLalalandEventDto& Existing : Service->GetState().events) DisplayedEvents.Add(Existing.id);
    }
    ApplyEntranceDoors();
    UpdateAudio();
    TMap<FString, ALalalandNpcCharacter*> RawCast;
    for (const auto& Pair : Cast) RawCast.Add(Pair.Key, Pair.Value.Get());
    for (const FLalalandActorDto& Actor : Service->GetState().characters)
    {
        if (TObjectPtr<ALalalandNpcCharacter>* Npc = Cast.Find(Actor.id)) (*Npc)->ApplyState(Actor, RawCast);
    }
    for (const FLalalandEventDto& Event : Service->GetState().events)
    {
        if (DisplayedEvents.Contains(Event.id)) continue;
        DisplayedEvents.Add(Event.id);
        PlayEventAudio(Event);
        if (Event.text.IsEmpty()) continue;
        if (Event.intent == TEXT("bounce_launch"))
            if (TObjectPtr<ALalalandNpcCharacter>* Npc = Cast.Find(Event.actor)) (*Npc)->TriggerGesture(TEXT("throw"));
        if (Event.type == TEXT("speech") && Event.privacy != TEXT("private") && Event.actor != TEXT("USER"))
            if (TObjectPtr<ALalalandNpcCharacter>* Npc = Cast.Find(Event.actor)) (*Npc)->ShowDialogue(Event.text);
    }
    UpdateDrinkProps();
}

void ALalalandStage::ApplyEntranceDoors()
{
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandLayoutPreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandElevatorPreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandSpatialPreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandAtmospherePreview"))||FParse::Param(FCommandLine::Get(),TEXT("LalalandFurniturePreview")))
    {
        for(int32 I=0;I<ElevatorDoors.Num();++I){ElevatorDoors[I]->SetActorLocation(FVector(I==0?-400.f:200.f,-496,175)+LalalandSpatial::ElevatorOffset());ElevatorDoors[I]->SetActorEnableCollision(false);}
        return;
    }
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    const auto& Intro=Service->GetState().intro;
    const bool bInBar=Intro.phase!=TEXT("elevator");
    const float Open= bInBar ? 1.f : FMath::Clamp((Intro.progress-5.4f)/1.6f,0.f,1.f);
    if (bDoorStateInitialized && !bBarEntranceOpened && bInBar && DoorSound) UGameplayStatics::PlaySound2D(this,DoorSound,.72f);
    bDoorStateInitialized=true; bBarEntranceOpened=bInBar;
    for(int32 Index=0;Index<ElevatorDoors.Num();++Index)
    {
        AStaticMeshActor* Door=ElevatorDoors[Index];if(!Door)continue;
        Door->SetActorHiddenInGame(false);
        Door->SetActorLocation(FVector((Index==0?-196.f:-4.f)+(Index==0?-196.f:196.f)*Open,-496,175)+LalalandSpatial::ElevatorOffset());
        Door->SetActorEnableCollision(Open<.98f);
    }
}

void ALalalandStage::StartLayoutPreview()
{
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandFeedbackPreview")))
    {
        auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
        auto* Camera=GetWorld()->SpawnActor<ACameraActor>();PC->SetViewTarget(Camera);
        Camera->SetActorLocation(FVector(-100,-930,165));Camera->SetActorRotation(FRotator(-22,90,0));
        const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("FeedbackReview_v006"));
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);
        FTimerHandle A,B,C,D,E,F;
        GetWorldTimerManager().SetTimer(A,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("07_Elevator.png"),false,false);},3.f,false);
        GetWorldTimerManager().SetTimer(B,[Camera](){Camera->SetActorLocation(FVector(-220,310,170));Camera->SetActorRotation(FRotator(-8,90,0));},4.f,false);
        GetWorldTimerManager().SetTimer(C,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("08_Bartender.png"),false,false);},6.f,false);
        GetWorldTimerManager().SetTimer(D,[Camera](){Camera->SetActorLocation(FVector(-950,-350,168));Camera->SetActorRotation((FVector(-1050,450,80)-Camera->GetActorLocation()).Rotation());},7.f,false);
        GetWorldTimerManager().SetTimer(E,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("09_LoungeFloor.png"),false,false);},9.f,false);
        GetWorldTimerManager().SetTimer(F,[](){FPlatformMisc::RequestExit(false);},11.f,false);return;
    }
    // Offline layout captures only: no new session, no AI calls, no acceptance claim.
    APlayerController* PC=GetWorld()->GetFirstPlayerController();
    if(!PC)return;
    ACameraActor* Camera=GetWorld()->SpawnActor<ACameraActor>();
    Camera->SetActorLocation(FVector(-100,-445,168));
    Camera->SetActorRotation((FVector(-220,550,160)-Camera->GetActorLocation()).Rotation());
    PC->SetViewTarget(Camera);
    const FString Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("LayoutReview"));
    FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Directory);
    FTimerHandle First,Overhead,Second,Craft,Third,Lounge,Fourth,Sofa,Fifth,BarFront,Sixth,BarBack,Seventh,Exit;
    GetWorldTimerManager().SetTimer(First,[Directory](){FScreenshotRequest::RequestScreenshot(Directory/TEXT("01-arrival.png"),false,false);},5.f,false);
    GetWorldTimerManager().SetTimer(Overhead,[this,Camera]()
    {
        for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
        {
            bool bRoof=It->ActorHasTag(TEXT("UpperRooftop"));
            for(FName Tag:It->Tags)bRoof|=Tag.ToString().StartsWith(TEXT("Roof"))&&!Tag.ToString().StartsWith(TEXT("RoofStair"));
            bool bCanopy=false;for(FName Tag:It->Tags)bCanopy|=Tag.ToString().StartsWith(TEXT("FutureCanopy"));
            if(bRoof||bCanopy||It->ActorHasTag(TEXT("Ceiling"))||It->ActorHasTag(TEXT("ElevatorCeiling")))It->SetActorHiddenInGame(true);
        }
        for(UStaticMeshComponent* Part:TInlineComponentArray<UStaticMeshComponent*>(this))
            if(Part->GetComponentLocation().Z>420)Part->SetVisibility(false);
        Camera->SetActorLocation(FVector(-700,120,2700));Camera->SetActorRotation(FRotator(-90,-90,0));
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        Camera->GetCameraComponent()->SetOrthoWidth(3700.f);
    },9.f,false);
    GetWorldTimerManager().SetTimer(Second,[Directory](){FScreenshotRequest::RequestScreenshot(Directory/TEXT("02-overview.png"),false,false);},13.f,false);
    GetWorldTimerManager().SetTimer(Craft,[this,Camera]()
    {
        for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
        {
            bool bRoof=It->ActorHasTag(TEXT("UpperRooftop"));
            for(FName Tag:It->Tags)bRoof|=Tag.ToString().StartsWith(TEXT("Roof"))&&!Tag.ToString().StartsWith(TEXT("RoofStair"));
            bool bCanopy=false;for(FName Tag:It->Tags)bCanopy|=Tag.ToString().StartsWith(TEXT("FutureCanopy"));
            if(bRoof||bCanopy||It->ActorHasTag(TEXT("Ceiling"))||It->ActorHasTag(TEXT("ElevatorCeiling")))It->SetActorHiddenInGame(false);
        }
        for(UStaticMeshComponent* Part:TInlineComponentArray<UStaticMeshComponent*>(this))Part->SetVisibility(true);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
        Camera->SetActorLocation(FVector(-545,330,174));
        Camera->SetActorRotation((FVector(-550,455,135)-Camera->GetActorLocation()).Rotation());
    },17.f,false);
    GetWorldTimerManager().SetTimer(Third,[Directory](){FScreenshotRequest::RequestScreenshot(Directory/TEXT("03-craft-placeholder.png"),false,false);},21.f,false);
    GetWorldTimerManager().SetTimer(Lounge,[Camera]()
    {
        Camera->SetActorLocation(FVector(280,-270,180));
        Camera->SetActorRotation((FVector(-1380,660,155)-Camera->GetActorLocation()).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(78);
    },25.f,false);
    GetWorldTimerManager().SetTimer(Fourth,[Directory](){FScreenshotRequest::RequestScreenshot(Directory/TEXT("04-lounge.png"),false,false);},29.f,false);
    GetWorldTimerManager().SetTimer(Sofa,[Camera]()
    {
        Camera->SetActorLocation(FVector(-1500,550,140));
        Camera->SetActorRotation((FVector(-1900,495,45)-Camera->GetActorLocation()).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(65);
    },33.f,false);
    GetWorldTimerManager().SetTimer(Fifth,[Directory](){FScreenshotRequest::RequestScreenshot(Directory/TEXT("05-sofa.png"),false,false);},37.f,false);
    GetWorldTimerManager().SetTimer(BarFront,[Camera]()
    {
        Camera->SetActorLocation(FVector(-220,-160,150));
        Camera->SetActorRotation((FVector(-220,450,80)-Camera->GetActorLocation()).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(78);
    },41.f,false);
    GetWorldTimerManager().SetTimer(Sixth,[Directory](){FScreenshotRequest::RequestScreenshot(Directory/TEXT("06-bar-front.png"),false,false);},45.f,false);
    GetWorldTimerManager().SetTimer(BarBack,[Camera]()
    {
        Camera->SetActorLocation(FVector(-750,750,190));
        Camera->SetActorRotation((FVector(-220,450,80)-Camera->GetActorLocation()).Rotation());
    },49.f,false);
    GetWorldTimerManager().SetTimer(Seventh,[Directory](){FScreenshotRequest::RequestScreenshot(Directory/TEXT("07-bar-back.png"),false,false);},53.f,false);
    GetWorldTimerManager().SetTimer(Exit,[this](){UKismetSystemLibrary::QuitGame(this,GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false);},57.f,false);
}

void ALalalandStage::UpdateAudio()
{
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    const FLalalandStateDto& State = Service->GetState();
    FString Phase = State.intro.phase==TEXT("elevator") ? TEXT("elevator") : State.firstNight.phase;
    if (Phase.IsEmpty()) Phase = State.intro.phase;
    if (Phase != LastAudioPhase)
    {
        LastAudioPhase = Phase;
        if (Phase == TEXT("elevator") && ElevatorSound) UGameplayStatics::PlaySound2D(this, ElevatorSound, .65f);
        else if ((Phase == TEXT("arrival") || Phase == TEXT("free_time")) && ArrivalSound) UGameplayStatics::PlaySound2D(this, ArrivalSound, .52f);
    }

    float Volume = .20f;
    float Pitch = 1.f;
    if (Phase == TEXT("elevator")) { Volume = .12f; Pitch = .96f; }
    else if (Phase == TEXT("game_call") || Phase == TEXT("game_choice")) { Volume = .21f; Pitch = 1.02f; }
    else if (Phase == TEXT("game_round")) { Volume = .26f; Pitch = 1.07f; }
    else if (Phase == TEXT("post_game"))
    {
        Volume = .20f;
        if (State.firstNight.songChoice == TEXT("night_view")) Pitch = .94f;
        else if (State.firstNight.songChoice == TEXT("table_beat")) Pitch = 1.10f;
        else if (State.firstNight.songChoice == TEXT("quiet_corner")) Pitch = .86f;
    }
    else if (Phase == TEXT("meteor_window") || Phase == TEXT("rooftop")) { Volume = .09f; Pitch = .92f; }
    else if (Phase == TEXT("settled")) { Volume = .10f; Pitch = .90f; }
    AmbientAudio->AdjustVolume(.8f, Volume);
    AmbientAudio->SetPitchMultiplier(Pitch);

    if (State.firstNight.songChoice != LastSongChoice)
    {
        LastSongChoice = State.firstNight.songChoice;
        if (!LastSongChoice.IsEmpty() && ArrivalSound) UGameplayStatics::PlaySound2D(this, ArrivalSound, .34f, Pitch);
    }
}

void ALalalandStage::PlayEventAudio(const FLalalandEventDto& Event)
{
    const FString& Intent = Event.intent;
    if (Intent == TEXT("make_drink") || Intent == TEXT("drink_served") || Intent == TEXT("drink_accepted")
        || Intent == TEXT("drink_consumed") || Intent == TEXT("bounce_hit") || Intent == TEXT("bounce_miss"))
    {
        if (CupSound) UGameplayStatics::PlaySound2D(this, CupSound, .68f);
    }
    else if (Intent == TEXT("game_call") || Intent == TEXT("game_result") || Intent == TEXT("meteor_window"))
    {
        if (ArrivalSound) UGameplayStatics::PlaySound2D(this, ArrivalSound, .54f);
    }
    else if (Intent == TEXT("rooftop_route") || Intent == TEXT("rooftop_arrival"))
    {
        if (DoorSound) UGameplayStatics::PlaySound2D(this, DoorSound, .66f);
    }
    else if (Intent.Contains(TEXT("phone")) || Event.objectTarget.Contains(TEXT("phone")))
    {
        if (PhoneSound) UGameplayStatics::PlaySound2D(this, PhoneSound, .62f);
    }
}

void ALalalandStage::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ApplyEntranceDoors();
    UpdateMeteors(DeltaSeconds);
    UpdateDrinkProps();
    PositionReportRemaining -= DeltaSeconds;
    if (PositionReportRemaining <= 0 && Service && !Service->GetState().sessionId.IsEmpty() && !Service->GetState().paused)
    {
        PositionReportRemaining = .2f;
        FLalalandCommandDto Command; Command.type = TEXT("positions");
        for (const auto& Pair : Cast)
        {
            const FVector P = Pair.Value->GetActorLocation();
            FLalalandPositionDto Item;
            Item.actor = Pair.Key;
            Item.x = P.X / 100.; Item.z = P.Y / 100.;
            Item.y = (P.Z - Pair.Value->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) / 100.;
            Item.yaw = 90.f - Pair.Value->GetActorRotation().Yaw;
            Item.area = P.Z > 380 && P.Y >= 230 ? TEXT("rooftop") : P.X >= 680 && P.Y >= -585 && P.Y < 260 ? TEXT("stairs") : P.Y < -518 ? TEXT("corridor") : TEXT("bar");
            Command.items.Add(Item);
        }
        Service->SendCommand(Command);
    }
}

void ALalalandStage::UpdateMeteors(float DeltaSeconds)
{
    if (!Service) return;
    const FString Phase = Service->GetState().firstNight.phase;
    const bool bShow = Phase == TEXT("meteor_window") || Phase == TEXT("rooftop");
    if (!bShow)
    {
        for (AStaticMeshActor* Meteor : Meteors) if (Meteor) Meteor->SetActorHiddenInGame(true);
        return;
    }
    MeteorSpawnRemaining -= DeltaSeconds;
    if (MeteorSpawnRemaining <= 0.f && Meteors.Num() < 10)
    {
        MeteorSpawnRemaining = .35f + FMath::FRand() * .55f;
        const FVector Start(FMath::FRandRange(-420.f, 520.f), FMath::FRandRange(240.f, 980.f), FMath::FRandRange(720.f, 980.f));
        AStaticMeshActor* Meteor = AddBox(FString::Printf(TEXT("Meteor_%02d"), Meteors.Num()), Start, FVector(.42f, .018f, .018f), FLinearColor(1.f, .86f, .55f), false);
        Meteor->SetActorRotation(FRotator(-18.f, FMath::FRandRange(110.f, 160.f), 0.f));
        Meteors.Add(Meteor);
    }
    for (AStaticMeshActor* Meteor : Meteors)
    {
        if (!Meteor) continue;
        Meteor->SetActorHiddenInGame(false);
        Meteor->AddActorWorldOffset(Meteor->GetActorForwardVector() * 420.f * DeltaSeconds);
        if (Meteor->GetActorLocation().Z < 430.f || Meteor->GetActorLocation().X > 1100.f)
        {
            Meteor->SetActorLocation(FVector(FMath::FRandRange(-420.f, 520.f), FMath::FRandRange(240.f, 980.f), FMath::FRandRange(720.f, 980.f)));
        }
    }
}

void ALalalandStage::UpdateDrinkProps()
{
    if (!Service) return;
    TSet<FString> Live;
    int32 Slot = 0;
    for (const FLalalandDrinkPropDto& Drink : Service->GetState().firstNight.drinks)
    {
        if (Drink.status == TEXT("refused")||Drink.status==TEXT("failed")) continue;
        const FString Instance = Drink.instanceId.IsEmpty() ? FString::FromInt(Slot) : Drink.instanceId;
        const FString Key = Instance;
        Live.Add(Key);
        // Keep drinks on the counter unless a real drinking clip is active.
        // Idle clips can put the hands behind the spine: permanent attachment
        // makes a glass intersect the body and implies an unplayed action.
        const FVector RestLocation=LalalandVenue::Counter(Slot);
        ++Slot;
        ALalalandGlassProp* Glass = ::Cast<ALalalandGlassProp>(DrinkProps.FindRef(Key));
        if (!Glass)
        {
            Glass = GetWorld()->SpawnActor<ALalalandGlassProp>(RestLocation, FRotator::ZeroRotator);
            const float Fill = Drink.status == TEXT("consumed") ? .18f : .7f;
            if(!Glass){
                FLalalandCommandDto Failure;Failure.type=TEXT("drink_prop_result");Failure.intent=TEXT("failed");Failure.objectTarget=Instance;Failure.requestId=Instance;
                Service->SendCommand(Failure);continue;
            }
            if(!Glass->BuildNativeMix(Drink.id,Fill))Glass->Build(LalalandKit::KindFromDrink(Drink.id), LalalandKit::LiquidFromDrink(Drink.id), Fill);
            DrinkProps.Add(Key, Glass);
        }
        if(!Drink.propConfirmed&&!SubmittedDeliveryEffects.Contains(TEXT("prop:")+Instance)){
            FLalalandCommandDto Ready;Ready.type=TEXT("drink_prop_result");Ready.intent=TEXT("ready");Ready.objectTarget=Instance;Ready.requestId=Instance;
            if(!Service->SendCommand(Ready).IsEmpty())SubmittedDeliveryEffects.Add(TEXT("prop:")+Instance);
        }
        Glass->SetConsumed(Drink.status==TEXT("consumed"));
        if(!Drink.sipActionId.IsEmpty())Glass->BeginSip(Drink.sipActionId);
        else if(Drink.status!=TEXT("consumed"))Glass->CancelSip();
        bool bLastPlayerGlass=Drink.owner==TEXT("USER")&&Drink.status==TEXT("served");
        for(const auto& Later:Service->GetState().firstNight.drinks)
            if(&Later>&Drink && Later.owner==TEXT("USER") && Later.status==TEXT("served"))bLastPlayerGlass=false;
        const bool bHoldingBall=Service->GetState().firstNight.openingBall==TEXT("held")||Service->GetState().firstNight.openingBall==TEXT("practice");
        if(!Drink.deliveryActionId.IsEmpty())
        {
            // Consent and starting an animation are not receipt. Only submit
            // this cup's effect when it actually reaches the mapped hand.
            ALalalandNpcCharacter* Receiver=Cast.FindRef(Drink.deliveryTarget);
            APlayerController* PC=GetWorld()->GetFirstPlayerController();
            const bool bReachable=Receiver && PC && PC->GetPawn() && !Receiver->GetHandAnchor().IsNone()
                && FVector::Dist(PC->GetPawn()->GetActorLocation(),Receiver->GetActorLocation())<150.f
                && PC->LineOfSightTo(Receiver) && Receiver->GetVelocity().SizeSquared()<100.f;
            if(bReachable)
            {
                const FVector Hand=Receiver->GetMesh()->GetSocketLocation(Receiver->GetHandAnchor());
                Glass->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                if(!Service->GetState().paused)
                    Glass->SetActorLocation(FMath::VInterpConstantTo(Glass->GetActorLocation(),Hand,GetWorld()->GetDeltaSeconds(),100.f));
                Glass->SetActorRotation(FRotator::ZeroRotator);
                Glass->SetActorScale3D(FVector::OneVector);
                if(!Service->GetState().paused && FVector::Dist(Glass->GetActorLocation(),Hand)<6.f
                    && Service->GetState().elapsed-Drink.deliveryStartedAt>=.6
                    && !SubmittedDeliveryEffects.Contains(Drink.deliveryActionId))
                {
                    FLalalandCommandDto Effect;
                    Effect.id=Drink.deliveryActionId+TEXT("-handoff-effect");Effect.type=TEXT("gift_drink");
                    Effect.intent=TEXT("effect");Effect.objectTarget=Drink.deliveryOfferId;Effect.requestId=Drink.deliveryActionId;
                    if(!Service->SendCommand(Effect).IsEmpty())SubmittedDeliveryEffects.Add(Drink.deliveryActionId);
                }
                continue;
            }
            // Movement, a missing hand mapping or a blocked reach must not
            // transfer ownership. Server timeout leaves the cup with USER.
        }
        if(Drink.owner==TEXT("USER") && !bHoldingBall && (bLastPlayerGlass||Glass->IsDrinking()))
        {
            if(APlayerController* PC=GetWorld()->GetFirstPlayerController())
            {
                Glass->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                const float Sip=Glass->IsDrinking()?FMath::Sin(Glass->GetSipProgress()*PI):0.f;
                const FRotator Rotation=PC->PlayerCameraManager->GetCameraRotation();
                const float HeldHeight=Glass->IsNativeMix()?-28.f:-45.f;
                Glass->SetActorLocation(PC->PlayerCameraManager->GetCameraLocation()+Rotation.RotateVector(FVector(42-18*Sip,24-12*Sip,HeldHeight+20*Sip)));
                Glass->SetActorRotation(Rotation+FRotator(-20*Sip,0,0));
                // The cup has reached the first-person mouth position. This
                // effect acknowledges the specific requested action and cup;
                // receipt/elapsed time alone never consumes a drink.
                if(!Service->GetState().paused&&!Drink.sipActionId.IsEmpty()&&Service->GetState().elapsed-Drink.sipStartedAt>=.6&&Glass->GetSipProgress()>=.5f&&!SubmittedSipEffects.Contains(Drink.sipActionId))
                {
                    FLalalandCommandDto Effect;
                    Effect.id=Drink.sipActionId+TEXT("-sip-effect");Effect.type=TEXT("drink_effect");
                    Effect.requestId=Drink.sipActionId;Effect.objectTarget=Drink.instanceId;Effect.intent=TEXT("sip");
                    if(!Service->SendCommand(Effect).IsEmpty())SubmittedSipEffects.Add(Drink.sipActionId);
                }
                continue;
            }
        }
        ALalalandNpcCharacter* Npc = Cast.FindRef(Drink.owner);
        const bool bNpcAction=Npc&&!Drink.sipActionId.IsEmpty();
        const bool bHandCup=Npc&&(Drink.cupPlacement==TEXT("hand") || (bNpcAction&&Drink.sipPhase!=TEXT("approach")&&Drink.sipPhase!=TEXT("queued")));
        if (bHandCup && Npc->HasDrinkGrip() && !Npc->GetHandAnchor().IsNone())
        {
            Glass->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            const FVector Hand=Npc->GetMesh()->GetSocketLocation(Npc->GetHandAnchor());
            FVector Goal=Hand;
            const bool bLower=bNpcAction&&Drink.sipPhase==TEXT("lower");
            const float Progress=Npc->GetCupAnimationProgress();
            // Follow the actual grip, then settle the cup onto the counter.
            // Never teleport a newly received cup back to its slot.
            if(bLower&&Progress>.45f&&FVector::Dist2D(Npc->GetActorLocation(),RestLocation)<160.f)
                Goal=FMath::Lerp(Hand,RestLocation,FMath::Clamp((Progress-.45f)/.5f,0.f,1.f));
            if(!Service->GetState().paused)
                Glass->SetActorLocation(FMath::VInterpConstantTo(Glass->GetActorLocation(),Goal,GetWorld()->GetDeltaSeconds(),130.f));
            Glass->SetActorRotation(FRotator::ZeroRotator);Glass->SetActorScale3D(FVector::OneVector);
            const FString EffectKey=Drink.sipActionId+(bLower?TEXT("-finish"):TEXT("-sip"));
            if(!Service->GetState().paused&&bNpcAction&&Npc->HasCupAction(Drink.sipPhase)
                &&Service->GetState().elapsed-Drink.sipStartedAt>=.6&&!SubmittedSipEffects.Contains(EffectKey))
            {
                const auto Kind=LalalandKit::KindFromDrink(Drink.id);
                const float RimHeight=Kind==ELalalandGlassKind::Coupe?12.f:Kind==ELalalandGlassKind::Wine?17.f:Kind==ELalalandGlassKind::Pilsner?19.f:16.f;
                const bool bContact=bLower ? Progress>.90f&&FVector::Dist(Glass->GetActorLocation(),RestLocation)<5.f
                    : Drink.sipPhase==TEXT("sip")&&Progress>.35f&&Progress<.65f
                      &&FVector::Dist(Glass->GetActorLocation()+FVector(0,0,RimHeight),Npc->GetMouthLocation())<8.f;
                if(bLower&&Progress>.85f&&!SubmittedSipEffects.Contains(EffectKey+TEXT("-diagnostic")))
                {
                    SubmittedSipEffects.Add(EffectKey+TEXT("-diagnostic"));
                    UE_LOG(LogTemp,Log,TEXT("LALALAND_CUP_LOWER actor=%s progress=%.3f cup=%s rest=%s body=%s distance=%.2f"),
                        *Drink.owner,Progress,*Glass->GetActorLocation().ToCompactString(),*RestLocation.ToCompactString(),*Npc->GetActorLocation().ToCompactString(),FVector::Dist(Glass->GetActorLocation(),RestLocation));
                }
                if(bContact)
                {
                    FLalalandCommandDto Effect;Effect.id=EffectKey+TEXT("-effect");Effect.type=TEXT("drink_effect");Effect.actor=Drink.owner;
                    Effect.requestId=Drink.sipActionId;Effect.objectTarget=Drink.instanceId;Effect.intent=bLower?TEXT("finish"):TEXT("sip");
                    if(!Service->SendCommand(Effect).IsEmpty())SubmittedSipEffects.Add(EffectKey);
                }
            }
        }
        else if(!bHandCup)
        {
            Glass->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            if(!Service->GetState().paused)Glass->SetActorLocation(FMath::VInterpConstantTo(Glass->GetActorLocation(),RestLocation,GetWorld()->GetDeltaSeconds(),130.f));
            Glass->SetActorRotation(FRotator::ZeroRotator);Glass->SetActorScale3D(FVector::OneVector);
        }
        // Missing animation/hand mapping retains ownership and consumption
        // state. The server cancels the action; it never fabricates a sip.

    }
    TArray<FString> Stale;
    for (const auto& Pair : DrinkProps) if (!Live.Contains(Pair.Key)) Stale.Add(Pair.Key);
    for (const FString& Key : Stale)
    {
        if (AActor* Actor = DrinkProps.FindRef(Key)) Actor->Destroy();
        DrinkProps.Remove(Key);
    }
}
