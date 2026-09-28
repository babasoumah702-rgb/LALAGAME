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
#include "GameFramework/Pawn.h"
#include "LalalandNpcCharacter.h"
#include "LalalandBounceGame.h"
#include "LalalandGlassProp.h"
#include "LalalandKit.h"
#include "LalalandServiceSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

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
    BuildArchitecture();
    BuildBarDetails();
    BuildLighting();
    BounceGame = GetWorld()->SpawnActor<ALalalandBounceGame>(FVector::ZeroVector, FRotator::ZeroRotator);
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

    // The bar ceiling is also the structural underside of the upper terrace.
    // Keeping it below the rooftop slab avoids the old 29 cm visual cavity
    // that placed the camera above a ceiling while standing on the roof.
    AddBox(TEXT("Ceiling"), FVector(0, 0, 390), FVector(14, 10, .12f), Charcoal, false);
    AddBox(TEXT("ElevatorCeiling"), FVector(-100, -750, 390), FVector(4, 5, .12f), Charcoal, false);
    AStaticMeshActor* NorthWall = AddBox(TEXT("NorthWall"), FVector(0, 490, 220), FVector(14, .12f, 2.25f), FLinearColor(.055f, .06f, .065f));
    ApplyMaterial(NorthWall, TEXT("/Game/Environment/Materials/M_Plaster.M_Plaster"));
    // Two real openings: the elevator at X -300..100 and the lit stair at X 665..825.
    AddBox(TEXT("SouthWallLeft"), FVector(-850, -490, 220), FVector(5.5f, .12f, 2.25f), Charcoal);
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
    AddBox(TEXT("BackbarShelfLow"), FVector(-220, 428, 128), FVector(7.6f, .28f, .05f), Brass);
    AddBox(TEXT("BackbarShelfHigh"), FVector(-220, 428, 208), FVector(7.6f, .28f, .05f), Brass);
    AddBox(TEXT("GlassRack"), FVector(-220, 438, 292), FVector(5.8f, .08f, .04f), FLinearColor(.12f, .12f, .14f));

    // The v0.2 handoff uses a long bounce table built by ALalalandBounceGame.
    // Keep only two small lounge tables here so the old round Scene-1 table cannot overlap it.
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FVector P = Index == 0 ? FVector(-490,-285,58) : FVector(585,285,58);
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
    AStaticMeshActor* RoofFloor = AddBox(TEXT("UpperRooftop"), FVector(130,575,400), FVector(6.75f,3.25f,.4f), FLinearColor(.065f,.075f,.095f));
    ApplyMaterial(RoofFloor, TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
    UBoxComponent* RoofFloorCollider = NewObject<UBoxComponent>(this, TEXT("UpperRooftopCollider"));
    RoofFloorCollider->SetupAttachment(RootComponent); RoofFloorCollider->SetBoxExtent(FVector(675.f,325.f,20.f));
    RoofFloorCollider->SetRelativeLocation(FVector(130.f,575.f,400.f)); RoofFloorCollider->SetCollisionProfileName(TEXT("BlockAll"));
    AddInstanceComponent(RoofFloorCollider); RoofFloorCollider->RegisterComponent(); ArchitectureColliders.Add(RoofFloorCollider);
    AddBox(TEXT("RoofRailNorth"), FVector(130,895,475), FVector(6.75f,.06f,.75f), MidnightGlass);
    AddBox(TEXT("RoofRailWest"), FVector(-540,575,475), FVector(.06f,3.25f,.75f), MidnightGlass);
    AddBox(TEXT("RoofRailEast"), FVector(800,575,475), FVector(.06f,3.25f,.75f), MidnightGlass);
    AddBox(TEXT("RoofSkyCanopy"), FVector(130,575,990), FVector(15.f,10.f,.08f), FLinearColor(.008f,.018f,.07f), false);
    for (int32 Lamp = 0; Lamp < 7; ++Lamp)
        AddBox(FString::Printf(TEXT("RoofStringLight_%02d"), Lamp), FVector(-430.f+Lamp*185.f,760.f,590.f), FVector(.035f,.035f,.035f), FLinearColor(1.f,.55f,.15f), false);
    AStaticMeshActor* RoofBench = AddBox(TEXT("RoofBench"), FVector(-185,650,458), FVector(2.1f,.62f,.28f), FLinearColor(.12f,.035f,.055f));
    ApplyMaterial(RoofBench, TEXT("/Game/Environment/Materials/M_Leather.M_Leather"));
    AddBox(TEXT("RoofBenchBack"), FVector(-185,705,505), FVector(2.1f,.16f,.68f), FLinearColor(.09f,.025f,.05f));
    AStaticMeshActor* RoofRug = AddBox(TEXT("RoofRug"), FVector(245,590,423), FVector(2.5f,1.55f,.025f), FLinearColor(.08f,.12f,.22f), false);
    ApplyMaterial(RoofRug, TEXT("/Game/Environment/Materials/M_Floor.M_Floor"));
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
        AddBox(FString::Printf(TEXT("RoofPlant_%02d"), Planter), FVector(X,820,495), FVector(.5f,.28f,.18f), FLinearColor(.025f,.18f,.08f), false);
    }
    // Simple skyline masses give the upper terrace a readable horizon without importing a new asset pack.
    for (int32 Tower = 0; Tower < 8; ++Tower)
    {
        const float Y = 240.f + Tower * 105.f;
        const float Height = 170.f + (Tower % 3) * 85.f;
        AddBox(FString::Printf(TEXT("RoofCity_%02d"), Tower), FVector(1030.f,Y,Height*.5f), FVector(.8f,.42f,Height/200.f), FLinearColor(.012f,.025f,.055f), false);
        AddBox(FString::Printf(TEXT("RoofCityLight_%02d"), Tower), FVector(948.f,Y,Height*.62f), FVector(.02f,.22f,.035f), Tower%2?FLinearColor(1.f,.38f,.08f):FLinearColor(.05f,.25f,1.f), false);
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

    const TArray<float> StoolXs = { -480.f, -360.f, -240.f, -120.f };
    for (int32 Index = 0; Index < StoolXs.Num(); ++Index)
    {
        LalalandKit::BarStool(this, Root, FString::Printf(TEXT("Stool_%02d"), Index), FVector(StoolXs[Index], -38.f, 0.f));
    }

    LalalandKit::Pendant(this, Root, TEXT("PendantA"), FVector(-400.f, 40.f, 360.f));
    LalalandKit::Pendant(this, Root, TEXT("PendantB"), FVector(-220.f, 40.f, 360.f));
    LalalandKit::Pendant(this, Root, TEXT("PendantC"), FVector(-40.f, 40.f, 360.f));
    const TArray<FVector> PendantLights = { FVector(-400.f, 40.f, 338.f), FVector(-220.f, 40.f, 338.f), FVector(-40.f, 40.f, 338.f) };
    for (const FVector& Lamp : PendantLights)
    {
        APointLight* Light = GetWorld()->SpawnActor<APointLight>(Lamp, FRotator::ZeroRotator);
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Light->GetLightComponent()->SetLightColor(FLinearColor(1.f, .55f, .22f));
        Light->GetLightComponent()->SetIntensity(6500.f);
        Light->GetLightComponent()->SetCastShadows(false);
        CastChecked<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(420.f);
    }

    LalalandKit::WineGlass(this, Root, TEXT("BoothWine"), FVector(-490.f, -285.f, 68.f), FLinearColor(.55f, .12f, .22f), .9f);
    LalalandKit::Highball(this, Root, TEXT("BoothHighball"), FVector(-455.f, -270.f, 68.f), FLinearColor(.28f, .72f, .42f), .85f);
    LalalandKit::Coaster(this, Root, TEXT("BoothCoaster"), FVector(-490.f, -285.f, 68.f));
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
        Light->GetLightComponent()->SetIntensity(18000.f);
        CastChecked<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(1050.f);
    }
    APointLight* Fill = GetWorld()->SpawnActor<APointLight>(FVector(-100.f, -80.f, 390.f), FRotator::ZeroRotator);
    Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Fill->GetLightComponent()->SetLightColor(FLinearColor(.28f, .34f, .5f));
    Fill->GetLightComponent()->SetIntensity(7500.f);
    Fill->GetLightComponent()->SetCastShadows(false);
    CastChecked<UPointLightComponent>(Fill->GetLightComponent())->SetAttenuationRadius(2100.f);
    APointLight* ElevatorLight = GetWorld()->SpawnActor<APointLight>(FVector(-100.f, -780.f, 310.f), FRotator::ZeroRotator);
    ElevatorLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    ElevatorLight->GetLightComponent()->SetLightColor(FLinearColor(.95f, .73f, .48f));
    ElevatorLight->GetLightComponent()->SetIntensity(11000.f);
    ElevatorLight->GetLightComponent()->SetCastShadows(false);
    CastChecked<UPointLightComponent>(ElevatorLight->GetLightComponent())->SetAttenuationRadius(620.f);
    const TArray<TPair<FVector,FLinearColor>> RoofLamps = {
        {FVector(-240.f,650.f,650.f),FLinearColor(1.f,.28f,.08f)},
        {FVector(360.f,600.f,630.f),FLinearColor(.08f,.24f,1.f)},
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
    RoofFill->GetLightComponent()->SetLightColor(FLinearColor(.22f,.32f,.62f));
    RoofFill->GetLightComponent()->SetIntensity(24000.f);
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
        {TEXT("BARTENDER"), FVector(-220, 110, 92)}
    };
    for (const auto& Pair : Positions)
    {
        ALalalandNpcCharacter* Npc = GetWorld()->SpawnActor<ALalalandNpcCharacter>(Pair.Value, FRotator::ZeroRotator);
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
        if ((Event.type == TEXT("speech") || Event.type == TEXT("message")) && Event.actor != TEXT("USER"))
            if (TObjectPtr<ALalalandNpcCharacter>* Npc = Cast.Find(Event.actor)) (*Npc)->ShowDialogue(Event.text);
    }
    UpdateDrinkProps();
}

void ALalalandStage::ApplyEntranceDoors()
{
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    const bool bInBar = Service->GetState().intro.phase != TEXT("elevator")
        || !Service->GetState().firstNight.contentVersion.IsEmpty();
    if (bDoorStateInitialized && bInBar == bBarEntranceOpened) return;
    const bool bWasInitialized = bDoorStateInitialized;
    bDoorStateInitialized = true;
    bBarEntranceOpened = bInBar;
    for (AStaticMeshActor* Door : ElevatorDoors)
    {
        if (!Door) continue;
        Door->SetActorHiddenInGame(bInBar);
        Door->SetActorEnableCollision(!bInBar);
    }
    if (bWasInitialized && DoorSound) UGameplayStatics::PlaySound2D(this, DoorSound, .72f);
}

void ALalalandStage::UpdateAudio()
{
    if (!Service || Service->GetState().sessionId.IsEmpty()) return;
    const FLalalandStateDto& State = Service->GetState();
    FString Phase = State.firstNight.phase;
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
    for (const FLalalandDrinkPropDto& Drink : Service->GetState().firstNight.drinks)
    {
        if (Drink.status == TEXT("refused")) continue;
        const FString Key = Drink.owner + TEXT(":") + Drink.id;
        Live.Add(Key);
        FVector Location(-220.f, 20.f, 145.f);
        if (TObjectPtr<ALalalandNpcCharacter>* Npc = Cast.Find(Drink.owner))
        {
            Location = (*Npc)->GetActorLocation() + FVector(28.f, 12.f, 28.f);
        }
        else if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            Location = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 28.f + FVector(0, 0, 18.f);
        }
        if (TObjectPtr<AActor>* Existing = DrinkProps.Find(Key))
        {
            if (*Existing) (*Existing)->SetActorLocation(Location);
            continue;
        }
        ALalalandGlassProp* Glass = GetWorld()->SpawnActor<ALalalandGlassProp>(Location, FRotator::ZeroRotator);
        const float Fill = Drink.status == TEXT("consumed") ? .18f : .7f;
        Glass->Build(LalalandKit::KindFromDrink(Drink.id), LalalandKit::LiquidFromDrink(Drink.id), Fill);
        DrinkProps.Add(Key, Glass);
    }
    TArray<FString> Stale;
    for (const auto& Pair : DrinkProps) if (!Live.Contains(Pair.Key)) Stale.Add(Pair.Key);
    for (const FString& Key : Stale)
    {
        if (AActor* Actor = DrinkProps.FindRef(Key)) Actor->Destroy();
        DrinkProps.Remove(Key);
    }
}
