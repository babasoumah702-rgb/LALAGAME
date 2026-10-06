#include "LalalandStage.h"
#include "LalalandSpatial.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
// All values in cm. Existing footprint, floor Z and service coordinates stay unchanged.
constexpr float CabinTop=420.f;
const FLinearColor Champagne(.42f,.30f,.17f), WallBronze(.18f,.135f,.10f), Amethyst(.047f,.023f,.075f);
void ElevatorFinish(AStaticMeshActor* Actor,const TCHAR* Name,FLinearColor Tint,float Roughness,float Plane=0.f)
{
    const FString Path=FString::Printf(TEXT("/Game/Environment/Future448/%s.%s"),Name,Name);
    if(auto* Base=LoadObject<UMaterialInterface>(nullptr,*Path))
    {
        auto* M=UMaterialInstanceDynamic::Create(Base,Actor);
        M->SetVectorParameterValue(TEXT("Tint"),Tint);
        M->SetScalarParameterValue(TEXT("Roughness"),Roughness);
        M->SetScalarParameterValue(TEXT("Plane"),Plane);
        Actor->GetStaticMeshComponent()->SetMaterial(0,M);
    }
    else UE_LOG(LogTemp,Error,TEXT("ELEVATOR_V008_MATERIAL_MISSING %s"),*Path);
}
}

void ALalalandStage::BuildFutureElevator()
{
    auto Surface=[this](const FString& Name,FVector P,FVector S,const TCHAR* Material,FLinearColor Tint,float R,float Plane=0.f,bool Collision=false)
    {
        auto* A=AddBox(Name,P+LalalandSpatial::ElevatorOffset(),S,Tint,Collision);ElevatorFinish(A,Material,Tint,R,Plane);return A;
    };
    auto Metal=[&](const FString& Name,FVector P,FVector S,FLinearColor Tint=Champagne)
    {return Surface(Name,P,S,TEXT("M_ElevatorMetal_v008"),Tint,.3f);};
    auto Glow=[this](const FString& Name,FVector P,FVector S,FLinearColor Tint)
    {
        auto* A=AddBox(Name,P+LalalandSpatial::ElevatorOffset(),S,Tint,false);A->GetStaticMeshComponent()->SetCastShadow(false);
        if(auto* M=::Cast<UMaterialInstanceDynamic>(A->GetStaticMeshComponent()->GetMaterial(0)))M->SetScalarParameterValue(TEXT("Intensity"),1.25f);
        return A;
    };
    Surface(TEXT("ElevatorFloor"),{-100,-750,-10.5f},{4,5,.21f},TEXT("M_ElevatorStone_v008"),{.30f,.27f,.23f},.24f);
    auto* C=NewObject<UBoxComponent>(this,TEXT("ElevatorFloorCollider"));
    C->SetupAttachment(RootComponent);C->SetRelativeLocation(FVector(-100,-750,-21)+LalalandSpatial::ElevatorOffset());C->SetBoxExtent({200,250,21});
    C->SetCollisionProfileName(TEXT("BlockAll"));C->CanCharacterStepUpOn=ECB_No;
    AddInstanceComponent(C);C->RegisterComponent();ArchitectureColliders.Add(C);
    Metal(TEXT("ElevatorSill"),{-100,-500,.05f},{3.84f,.20f,.003f});
    // Inlay is visual only and flush: it must not stop a capsule or create a step.
    for(float X:{-272.f,72.f})Metal(FString::Printf(TEXT("ElevatorFloorBorder_%d"),int(X)),{X,-750,.045f},{.014f,4.54f,.003f});
    for(float Y:{-977.f,-523.f})Metal(FString::Printf(TEXT("ElevatorFloorBorderY_%d"),int(Y)),{-100,Y,.045f},{3.44f,.014f,.003f});

    Surface(TEXT("ElevatorWallLeft"),{-300,-750,213},{.12f,5,4.26f},TEXT("M_ElevatorMetal_v008"),WallBronze,.34f,2,true);
    Surface(TEXT("ElevatorWallRight"),{100,-750,213},{.12f,5,4.26f},TEXT("M_ElevatorMetal_v008"),WallBronze,.34f,2,true);
    Surface(TEXT("ElevatorBack"),{-100,-995,213},{4,.12f,4.26f},TEXT("M_ElevatorMetal_v008"),WallBronze,.34f,1,true);
    Surface(TEXT("ElevatorCeiling"),{-100,-750,426},{4,5,.12f},TEXT("M_ElevatorStone_v008"),{.15f,.135f,.115f},.58f);

    // Three framed jewel panels. Pattern is procedural, not a baked perspective image.
    auto Jewel=[&](const FString& Name,FVector P,FVector S,float Plane)
    {
        Metal(Name+TEXT("Frame"),P,S+FVector(.035f,.035f,.055f));
        FVector Shift=Plane==1?FVector(0,1.9f,0):FVector(P.X<0?1.9f:-1.9f,0,0);
        auto* A=Surface(Name,P+Shift,S,TEXT("M_ElevatorAmethyst_v008"),Amethyst,.23f,Plane);
        A->GetStaticMeshComponent()->SetCastShadow(false);
    };
    Jewel(TEXT("ElevatorBackJewel"),{-100,-987,220},{1.52f,.025f,3.36f},1);
    for(float X:{-292.f,92.f})Jewel(FString::Printf(TEXT("ElevatorSideJewel_%d"),int(X)),{X,-745,220},{.025f,1.30f,3.36f},2);
    // Fine vertical joints break the large metal sheets into intentional modules.
    for(float X:{-276.f,-181.f,-19.f,76.f})Metal(FString::Printf(TEXT("ElevatorBackJoint_%d"),int(X)),{X,-987,213},{.008f,.014f,3.86f});
    for(float X:{-291.f,91.f})for(float Y:{-972.f,-920.f,-820.f,-670.f,-570.f,-525.f})
        Metal(FString::Printf(TEXT("ElevatorSideJoint_%d_%d"),int(X),int(Y)),{X,Y,213},{.014f,.008f,3.86f});
    for(float X:{-290.f,90.f})
    {
        Metal(FString::Printf(TEXT("ElevatorSkirting_%d"),int(X)),{X,-750,8},{.035f,4.82f,.16f});
        Metal(FString::Printf(TEXT("ElevatorHandrail_%d"),int(X)),{X<0?-276.f:76.f,-763,104},{.027f,4.02f,.027f});
        for(float Y:{-951.f,-556.f})Metal(FString::Printf(TEXT("ElevatorRailBracket_%d_%d"),int(X),int(Y)),{X<0?-283.f:83.f,Y,104},{.16f,.022f,.022f});
        for(float Y:{-979.f,-525.f})Glow(FString::Printf(TEXT("ElevatorWallLight_%d_%d"),int(X),int(Y)),{X,Y,213},{.018f,.026f,3.68f},{.66f,.46f,.27f});
    }
    Metal(TEXT("ElevatorBackSkirting"),{-100,-985,8},{3.80f,.035f,.16f});
    Metal(TEXT("ElevatorBackHandrail"),{-100,-972,104},{3.50f,.027f,.027f});
    for(float X:{-258.f,58.f})Metal(FString::Printf(TEXT("ElevatorBackBracket_%d"),int(X)),{X,-979,104},{.022f,.16f,.022f});

    // Shallow coffer: 420cm structural clearance, 402cm beneath the perimeter fascia.
    for(float X:{-274.f,74.f})
    {
        Surface(FString::Printf(TEXT("ElevatorCoffer_%d"),int(X)),{X,-750,411},{.48f,4.86f,.18f},TEXT("M_ElevatorStone_v008"),{.12f,.105f,.085f},.58f);
        Metal(FString::Printf(TEXT("ElevatorCofferTrim_%d"),int(X)),{X<0?-249.f:49.f,-750,405},{.018f,4.30f,.018f});
        Glow(FString::Printf(TEXT("ElevatorCoveLight_%d"),int(X)),{X<0?-247.f:47.f,-750,413},{.026f,4.27f,.035f},{.62f,.44f,.27f});
    }
    for(float Y:{-963.f,-537.f})
    {
        Surface(FString::Printf(TEXT("ElevatorCofferY_%d"),int(Y)),{-100,Y,411},{3.02f,.52f,.18f},TEXT("M_ElevatorStone_v008"),{.12f,.105f,.085f},.58f);
        Glow(FString::Printf(TEXT("ElevatorCoveLightY_%d"),int(Y)),{-100,Y<-750?-935.f:-565.f,413},{2.94f,.026f,.035f},{.62f,.44f,.27f});
    }
    // Visible mounting plate only; do not fabricate the user's forthcoming chandelier.
    auto* Mount=AddCylinder(TEXT("ElevatorChandelierMount_v008"),FVector(-100,-750,419)+LalalandSpatial::ElevatorOffset(),{.18f,.18f,.016f},Champagne);
    Mount->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ElevatorFinish(Mount,TEXT("M_ElevatorMetal_v008"),Champagne,.3f);
    auto* Anchor=NewObject<USceneComponent>(this,TEXT("ElevatorChandelierAnchor_v008"));
    Anchor->SetupAttachment(RootComponent);Anchor->SetRelativeLocation(FVector(-100,-750,420)+LalalandSpatial::ElevatorOffset());
    Anchor->ComponentTags.Add(TEXT("ChandelierMount_100cmWidth_111cmDrop"));AddInstanceComponent(Anchor);Anchor->RegisterComponent();
    if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/Future448/Chandelier_v009/SM_Chandelier_v009.SM_Chandelier_v009")))
    {
        const FBox B=Mesh->GetBoundingBox();const float Scale=100.f/B.GetSize().X;
        auto* Lamp=GetWorld()->SpawnActor<AStaticMeshActor>();Lamp->Tags.Add(TEXT("ElevatorChandelier_v009"));
        auto* MC=Lamp->GetStaticMeshComponent();MC->SetMobility(EComponentMobility::Movable);MC->SetStaticMesh(Mesh);
        MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lamp->SetActorScale3D(FVector(Scale));
        Lamp->SetActorLocation(Anchor->GetComponentLocation()-FVector(B.GetCenter().X,B.GetCenter().Y,B.Max.Z)*Scale);
        Mount->SetActorHiddenInGame(true);
        UE_LOG(LogTemp,Display,TEXT("CHANDELIER_V009_PLACED width_cm=100 drop_cm=%.2f bottom_cm=%.2f"),B.GetSize().Z*Scale,420-B.GetSize().Z*Scale);
    }
    else UE_LOG(LogTemp,Warning,TEXT("CHANDELIER_V009_ASSET_PENDING"));

    // Door leaves meet at X=-100 with no old 20cm slit. Keep 350cm door height.
    Surface(TEXT("ElevatorHeader"),{-100,-500,391},{4,.12f,.82f},TEXT("M_ElevatorMetal_v008"),WallBronze,.34f,1,true);
    ElevatorDoors.Add(Surface(TEXT("ElevatorDoorLeft"),{-196,-496,175},{1.92f,.08f,3.5f},TEXT("M_ElevatorMetal_v008"),{.23f,.19f,.145f},.30f,1,true));
    ElevatorDoors.Add(Surface(TEXT("ElevatorDoorRight"),{-4,-496,175},{1.92f,.08f,3.5f},TEXT("M_ElevatorMetal_v008"),{.23f,.19f,.145f},.30f,1,true));
    for(float X:{-293.f,93.f})Metal(FString::Printf(TEXT("ElevatorDoorJamb_%d"),int(X)),{X,-508,175},{.04f,.035f,3.5f});
    Metal(TEXT("ElevatorControlPanel"),{87,-569,147},{.035f,.24f,.78f},{.31f,.24f,.16f});
    for(int32 I=0;I<3;++I)Glow(FString::Printf(TEXT("ElevatorButtonLight_%d"),I),{84.9f,-569,130.f+I*15},{.008f,.04f,.04f},{.32f,.24f,.13f});

    auto Light=[this](FVector P,FLinearColor Color,float Power,float Radius)
    {
        auto* L=GetWorld()->SpawnActor<APointLight>(P+LalalandSpatial::ElevatorOffset(),FRotator::ZeroRotator);
        L->Tags.Add(TEXT("ElevatorLocalLight_v008"));auto* LC=CastChecked<UPointLightComponent>(L->GetLightComponent());
        LC->SetMobility(EComponentMobility::Movable);LC->SetLightColor(Color);LC->SetIntensity(Power);
        LC->SetAttenuationRadius(Radius);LC->SetSourceRadius(3);LC->SetSpecularScale(0);LC->SetCastShadows(true);
    };
    auto Area=[this](FVector P,FRotator R,float W,float H,float Lumens)
    {
        auto* L=GetWorld()->SpawnActor<ARectLight>(P+LalalandSpatial::ElevatorOffset(),R);L->Tags.Add(TEXT("ElevatorLocalLight_v008"));
        auto* LC=L->RectLightComponent.Get();LC->SetMobility(EComponentMobility::Movable);
        LC->SetIntensityUnits(ELightUnits::Lumens);LC->SetIntensity(Lumens);LC->SetLightColor({1,.86f,.71f});
        LC->SetSourceWidth(W);LC->SetSourceHeight(H);LC->SetAttenuationRadius(650);LC->SetCastShadows(true);LC->SetSpecularScale(.22f);
    };
    // Calibrated to this project's manual non-physical exposure; do not import
    // real-room lumen figures without changing the shared exposure model first.
    Area({-100,-750,398},FRotator(-90,0,0),220,285,100);
    Area({-100,-595,290},FRotator(0,-90,0),220,210,30);
    // A small upward fill reveals the crystal rods below the metal ring.
    Area({-100,-750,300},FRotator(90,0,0),85,85,12);
    Light({-240,-940,265},{.72f,.54f,1},250,205);
    Light({40,-940,265},{.72f,.54f,1},250,205);
    UE_LOG(LogTemp,Display,TEXT("ELEVATOR_V009_BUILT footprint=400x500cm structural_clearance=420cm perimeter_clearance=402cm floor_z=0 chandelier_anchor=-100,-1350,420"));
}

void ALalalandStage::StartElevatorReview()
{
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    auto* Camera=GetWorld()->SpawnActor<ACameraActor>();PC->SetViewTarget(Camera);
    Camera->GetCameraComponent()->SetFieldOfView(96);
    Camera->SetActorLocation({-100,-445,190});Camera->SetActorRotation(FRotator(4,-90,0));
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("ElevatorReview_v008"));
    FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);
    FTimerHandle A,B,C,D,E,F,G;
    GetWorldTimerManager().SetTimer(A,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("01_Entrance.png"),false,false);},6.f,false);
    GetWorldTimerManager().SetTimer(B,[this,Camera]()
    {
        // Audit the open doorway at three lateral positions using a player-sized capsule.
        bool Clear=true;FCollisionQueryParams Q;
        if(APawn* Pawn=GetWorld()->GetFirstPlayerController()->GetPawn())Q.AddIgnoredActor(Pawn);
        for(float X:{-180.f,-100.f,-20.f})
        {
            FHitResult Hit;const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,{X,-720,98},{X,-420,98},FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(38,92),Q);
            Clear&=!Blocked;
            UE_LOG(LogTemp,Display,TEXT("ELEVATOR_V008_DOOR_SWEEP x=%.0f clear=%d blocking=%s"),X,!Blocked,Hit.GetActor()?*Hit.GetActor()->GetName():TEXT("none"));
        }
        for(float Y:{-900.f,-750.f,-550.f,-495.f,-450.f})
        {
            FHitResult Hit;const bool Found=GetWorld()->LineTraceSingleByChannel(Hit,{-100,Y,40},{-100,Y,-50},ECC_Visibility,Q);
            Clear&=Found&&FMath::Abs(Hit.ImpactPoint.Z)<.25f;
            UE_LOG(LogTemp,Display,TEXT("ELEVATOR_V008_FLOOR y=%.0f found=%d z=%.3f"),Y,Found,Hit.ImpactPoint.Z);
        }
        UE_LOG(LogTemp,Display,TEXT("ELEVATOR_V008_GEOMETRY_AUDIT pass=%d hardware_input_verified=0"),Clear);
        Camera->SetActorLocation({-70,-590,168});Camera->SetActorRotation(FRotator(-25,-103,0));
    },7.f,false);
    GetWorldTimerManager().SetTimer(C,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("02_StoneAndWalls.png"),false,false);},10.f,false);
    GetWorldTimerManager().SetTimer(D,[Camera](){Camera->SetActorLocation({-100,-670,190});Camera->SetActorRotation(FRotator(54,-90,0));Camera->GetCameraComponent()->SetFieldOfView(82);},11.f,false);
    GetWorldTimerManager().SetTimer(E,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("03_CeilingMount.png"),false,false);},14.f,false);
    GetWorldTimerManager().SetTimer(F,[Camera](){Camera->SetActorLocation({-100,-910,168});Camera->SetActorRotation(FRotator(-9,90,0));Camera->GetCameraComponent()->SetFieldOfView(90);},15.f,false);
    GetWorldTimerManager().SetTimer(G,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("04_ExitThreshold.png"),false,false);},18.f,false);
    FTimerHandle Exit;GetWorldTimerManager().SetTimer(Exit,[](){FPlatformMisc::RequestExit(false);},20.f,false);
}
