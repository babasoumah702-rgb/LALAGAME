#include "LalalandStage.h"
#include "LalalandNpcCharacter.h"
#include "LalalandSpatial.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/RectLight.h"
#include "Engine/TextRenderActor.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "TimerManager.h"

void ALalalandStage::BuildFutureEntrance()
{
    const FLinearColor Stone(.075f,.068f,.09f), Gold(.37f,.27f,.15f), Violet(.09f,.025f,.13f);
    auto Surface=[this](const FString& N,FVector P,FVector S,FLinearColor Tint,const TCHAR* Name,bool Block=false)
    {
        auto* A=AddBox(N,P,S,Tint,Block);
        FString Path=FString::Printf(TEXT("/Game/Environment/Future448/%s.%s"),Name,Name);
        if(auto* Base=LoadObject<UMaterialInterface>(nullptr,*Path))
        {
            auto* M=UMaterialInstanceDynamic::Create(Base,A);M->SetVectorParameterValue(TEXT("Tint"),Tint);
            M->SetScalarParameterValue(TEXT("Roughness"),.26f);M->SetScalarParameterValue(TEXT("Plane"),N.Contains(TEXT("Floor"))?0:1);
            A->GetStaticMeshComponent()->SetMaterial(0,M);
        }
        return A;
    };
    // Partition the visible floor; the east route already belongs to the stair corridor.
    Surface(TEXT("EntryFloorSouth_v010"),{260,-882.5f,-10},{11.2f,4.35f,.2f},Stone,TEXT("M_ElevatorStone_v008"));
    Surface(TEXT("EntryFloorNorthWest_v010"),{-100,-582.5f,-10},{4,1.65f,.2f},Stone,TEXT("M_ElevatorStone_v008"));
    auto* C=NewObject<UBoxComponent>(this,TEXT("EntryFloorCollider_v009"));
    C->SetupAttachment(RootComponent);C->SetRelativeLocation({260,-800,-10});C->SetBoxExtent({560,300,10});
    C->SetCollisionProfileName(TEXT("BlockAll"));AddInstanceComponent(C);C->RegisterComponent();ArchitectureColliders.Add(C);
    Surface(TEXT("EntryWestWall_v009"),{-306,-800,175},{.12f,6,3.5f},Stone,TEXT("M_FutureMetal"),true);
    Surface(TEXT("EntrySouthWall_v009"),{460,-1106,175},{7.2f,.12f,3.5f},Stone,TEXT("M_FutureMetal"),true);
    Surface(TEXT("EntryEastWall_v009"),{826,-920,175},{.12f,3.6f,3.5f},Stone,TEXT("M_FutureMetal"),true);
    Surface(TEXT("EntryCeiling_v009"),{260,-800,356},{11.2f,6,.12f},Stone,TEXT("M_FutureMetal"));
    // Solid screen hides the bar on arrival. The 250cm east-side route is not a blocked decorative gap.
    Surface(TEXT("EntryScreen_v009"),{-80,-655,165},{4.6f,.18f,3.3f},Violet,TEXT("M_ElevatorAmethyst_v008"),true);
    for(float X:{-302.f,142.f})Surface(FString::Printf(TEXT("EntryScreenTrim_%d_v009"),int(X)),{X,-666,165},{.025f,.025f,3.3f},Gold,TEXT("M_ElevatorMetal_v008"));
    for(float Z:{8.f,322.f})Surface(FString::Printf(TEXT("EntryScreenTrimZ_%d_v009"),int(Z)),{-80,-666,Z},{4.45f,.025f,.025f},Gold,TEXT("M_ElevatorMetal_v008"));
    auto* Logo=GetWorld()->SpawnActor<ATextRenderActor>(FVector(-80,-667,193),FRotator(0,-90,0));
    Logo->Tags.Add(TEXT("EntryLogo_v009"));auto* T=Logo->GetTextRender();T->SetText(FText::FromString(TEXT("LaLa-Land")));
    T->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);T->SetWorldSize(33);T->SetTextRenderColor(FColor(222,188,130));
    auto* Sub=GetWorld()->SpawnActor<ATextRenderActor>(FVector(-80,-667,160),FRotator(0,-90,0));
    Sub->GetTextRender()->SetText(FText::FromString(TEXT("A NIGHT OF POSSIBILITIES")));Sub->GetTextRender()->SetWorldSize(7);
    Sub->GetTextRender()->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);Sub->GetTextRender()->SetTextRenderColor(FColor(180,161,129));
    for(float X:{-264.f,400.f})
    {
        auto* Strip=AddBox(FString::Printf(TEXT("EntryLightStrip_%d_v009"),int(X)),{X,-830,337},{.025f,4.8f,.025f},{.5f,.29f,.13f},false);
        Strip->GetStaticMeshComponent()->SetCastShadow(false);
    }
    for(int32 I=0;I<5;++I)
        Surface(FString::Printf(TEXT("EntryCeilingBaffle_%d_v009"),I),{260,-1040.f+I*100,341},{10.4f,.035f,.12f},Gold,TEXT("M_ElevatorMetal_v008"));
    auto Light=[this](FVector P,float Power,FLinearColor Tint)
    {
        auto* L=GetWorld()->SpawnActor<ARectLight>(P,FRotator(-90,0,0));auto* LC=L->RectLightComponent.Get();
        LC->SetMobility(EComponentMobility::Movable);LC->SetIntensityUnits(ELightUnits::Lumens);LC->SetIntensity(Power);
        LC->SetLightColor(Tint);LC->SetSourceWidth(250);LC->SetSourceHeight(180);LC->SetAttenuationRadius(650);LC->SetSpecularScale(.25f);
    };
    Light({-80,-930,330},80,{1,.83f,.64f});Light({400,-760,330},75,{.77f,.71f,1});
    // Lower perimeter soffits frame a higher central volume without lowering the main aisle.
    Surface(TEXT("FutureSoffitWest_v009"),{-1910,300,392},{3.8f,16,.16f},Stone,TEXT("M_FutureMetal"));
    Surface(TEXT("FutureSoffitNorth_v009"),{-700,970,392},{28,2.6f,.16f},Stone,TEXT("M_FutureMetal"));
    Surface(TEXT("FutureEastUpperWall_v009"),{690,410,495},{.16f,13.8f,1.1f},Stone,TEXT("M_FutureMetal"));
    for(int32 I=0;I<9;++I)
    {
        const float X=-1560.f+I*170;
        Surface(FString::Printf(TEXT("FutureCeilingFin_%d_v009"),I),{X,300,543},{.025f,10,.14f},Gold,TEXT("M_ElevatorMetal_v008"));
    }
    UE_LOG(LogTemp,Display,TEXT("SPATIAL_V009_ENTRY corridor=11.2x6m elevator_shift_y=-600 screen=460cm main_door=300cm"));
}

void ALalalandStage::StartSpatialReview()
{
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
    for(int32 I=0;I<ElevatorDoors.Num();++I){ElevatorDoors[I]->SetActorLocation(FVector(I==0?-400:200,-1096,175));ElevatorDoors[I]->SetActorEnableCollision(false);}
    auto* Camera=GetWorld()->SpawnActor<ACameraActor>();PC->SetViewTarget(Camera);Camera->GetCameraComponent()->SetFieldOfView(90);
    const bool Furniture=FParse::Param(FCommandLine::Get(),TEXT("LalalandFurniturePreview"));
    const bool Atmosphere=Furniture||FParse::Param(FCommandLine::Get(),TEXT("LalalandAtmospherePreview"));
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/(Furniture?TEXT("FurnitureReview_v011"):Atmosphere?TEXT("AtmosphereReview_v010"):TEXT("SpatialReview_v009")));
    auto Hidden=MakeShared<TArray<TWeakObjectPtr<AStaticMeshActor>>>();
    FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);
    auto View=[Camera](FVector P,FVector Target,float FOV){Camera->SetActorLocation(P);Camera->SetActorRotation((Target-P).Rotation());Camera->GetCameraComponent()->SetFieldOfView(FOV);};
    View({-100,-1170,190},{-100,-1350,345},90);
    auto At=[this](float Seconds,TFunction<void()> Callback){FTimerHandle H;GetWorldTimerManager().SetTimer(H,MoveTemp(Callback),Seconds,false);};
    At(6,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("01_ElevatorChandelier.png"),false,false);});
    At(7,[View](){View({-100,-1070,168},{-80,-655,180},85);});
    At(11,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("02_ArrivalScreen.png"),false,false);});
    At(12,[View](){View({400,-800,168},{-700,300,245},85);});
    At(16,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("03_TurnIntoHall.png"),false,false);});
    At(17,[View](){View({200,-290,180},{-900,450,430},95);});
    At(21,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("04_RaisedHall.png"),false,false);});
    At(22,[View](){View({650,380,768},{-400,700,665},90);});
    At(26,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("05_Rooftop.png"),false,false);});
    At(27,[this,PC,Camera,View,Hidden,Atmosphere,Furniture]()
    {
        FCollisionQueryParams Q;if(PC->GetPawn())Q.AddIgnoredActor(PC->GetPawn());bool Pass=true;
        const FVector Route[]={{-100,-1465,98},{-100,-850,98},{400,-850,98},{400,-560,98},{400,-400,98},{400,-730,98},{745,-730,98}};
        for(int32 I=1;I<UE_ARRAY_COUNT(Route);++I)
        {
            FHitResult H;bool Block=GetWorld()->SweepSingleByChannel(H,Route[I-1],Route[I],FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(38,92),Q);Pass&=!Block;
            UE_LOG(LogTemp,Display,TEXT("SPATIAL_V009_ROUTE segment=%d clear=%d blocker=%s"),I,!Block,H.GetActor()?*H.GetActor()->GetName():TEXT("none"));
        }
        for(const FVector& P:Route)
        {
            FHitResult H;bool Found=GetWorld()->LineTraceSingleByChannel(H,P+FVector(0,0,40),P-FVector(0,0,150),ECC_Visibility,Q);
            Pass&=Found&&FMath::Abs(H.ImpactPoint.Z)<.3f;
            UE_LOG(LogTemp,Display,TEXT("SPATIAL_V009_FLOOR p=%s found=%d z=%.2f"),*P.ToCompactString(),Found,H.ImpactPoint.Z);
        }
        FHitResult Sight;bool Screen=GetWorld()->LineTraceSingleByChannel(Sight,{-100,-1070,168},{-220,450,168},ECC_Visibility,Q);
        Pass&=Screen;UE_LOG(LogTemp,Display,TEXT("SPATIAL_V009_AUDIT pass=%d entrance_sight_blocked=%d hardware_input_verified=0"),Pass,Screen);
        if(Furniture)
        {
            // Furniture clearance and a currently standing NPC are distinct results.
            // Never remove NPC collision from gameplay to make this test pass.
            FCollisionQueryParams StaticQ=Q;
            for(TActorIterator<ALalalandNpcCharacter> It(GetWorld());It;++It)StaticQ.AddIgnoredActor(*It);
            const FVector From[]={{-620,235,98},{-300,245,98},{-140,245,98},{-480,670,98},{60,680,98}};
            const FVector To[]={{120,235,98},{-300,350,98},{-140,350,98},{0,670,98},{60,950,98}};
            bool Clear=true;
            for(int32 I=0;I<UE_ARRAY_COUNT(From);++I)
            {
                FHitResult H,Occupied;const bool Block=GetWorld()->SweepSingleByChannel(H,From[I],To[I],FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(38,92),StaticQ);Clear&=!Block;
                const bool HasOccupant=GetWorld()->SweepSingleByChannel(Occupied,From[I],To[I],FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(38,92),Q);
                UE_LOG(LogTemp,Display,TEXT("FURNITURE_V011_ROUTE segment=%d static_clear=%d blocker=%s occupied_now=%d occupied_by=%s"),I,!Block,H.GetActor()?*H.GetActor()->GetName():TEXT("none"),HasOccupant,Occupied.GetActor()?*Occupied.GetActor()->GetName():TEXT("none"));
            }
            UE_LOG(LogTemp,Display,TEXT("FURNITURE_V011_AUDIT pass=%d capsule=38x92 seating_animation_verified=0"),Clear&&Pass);
        }
        for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
            for(FName Tag:It->Tags)
            {
                FString N=Tag.ToString();if(N==TEXT("Ceiling")||N==TEXT("UpperRooftop")||N==TEXT("ElevatorCeiling")||N.StartsWith(TEXT("EntryCeiling"))||N.StartsWith(TEXT("FutureSoffit"))||N.StartsWith(TEXT("FutureCeilingFin"))||N.StartsWith(TEXT("FutureCanopy"))||(N.StartsWith(TEXT("Roof"))&&!N.StartsWith(TEXT("RoofStair"))))
                {if(!It->IsHidden()){Hidden->Add(*It);It->SetActorHiddenInGame(true);}}
            }
        View({-600,-220,3000},{-600,-220,0},90);Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);Camera->GetCameraComponent()->SetOrthoWidth(3900);
        if(Atmosphere){Camera->SetActorLocation({-625,-305,3000});Camera->SetActorRotation(FRotator(-90,-90,0));Camera->GetCameraComponent()->SetOrthoWidth(5200);}
    });
    At(31,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("06_Plan.png"),false,false);});
    if(Atmosphere)
    {
        At(32,[Camera,View,Hidden](){for(auto W:*Hidden)if(W.IsValid())W->SetActorHiddenInGame(false);Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);View({-1400,480,168},{-1800,610,150},85);});
        At(36,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("07_BoothMaterials.png"),false,false);});
        At(37,[View](){View({-600,300,190},{-220,830,228},85);});
        At(41,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("08_BackBar.png"),false,false);});
        if(Furniture)
        {
            At(42,[View](){View({-380,95,165},{-220,430,110},65);});
            At(46,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("09_BarStools.png"),false,false);});
            At(47,[View](){View({-220,625,185},{-220,830,220},100);});
            At(51,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("10_CabinetDetails.png"),false,false);});
            At(53,[](){FPlatformMisc::RequestExit(false);});
        }
        else At(43,[](){FPlatformMisc::RequestExit(false);});
    }
    else At(33,[](){FPlatformMisc::RequestExit(false);});
}

void ALalalandStage::StartPlayerGripReview()
{
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/PLAYER_v009b/SK_PlayerArms_v009.SK_PlayerArms_v009"));
    auto* Anim=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/PLAYER_v009b/AN_PlayerHold_v009.AN_PlayerHold_v009"));
    if(!Mesh||!Anim){UE_LOG(LogTemp,Error,TEXT("PLAYER_GRIP_V009 missing_asset=1"));FPlatformMisc::RequestExit(false);return;}
    auto* Arms=GetWorld()->SpawnActor<ASkeletalMeshActor>(FVector(-700,-200,0),FRotator::ZeroRotator);
    auto* SC=Arms->GetSkeletalMeshComponent();SC->SetSkeletalMeshAsset(Mesh);SC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Arms->SetActorScale3D(FVector(1.67f));
    SC->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    SC->SetComponentTickEnabled(true);SC->PlayAnimation(Anim,true);SC->TickAnimation(0,false);SC->RefreshBoneTransforms();SC->SetCastShadow(false);
    auto* Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->SetActorLocation({-700,-200,160});Camera->SetActorRotation(FRotator(-35,90,0));
    Camera->GetCameraComponent()->SetFieldOfView(85);PC->SetViewTarget(Camera);
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("PlayerGripReview_v009"));FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Dir);
    FTimerHandle A,B,C,D;
    GetWorldTimerManager().SetTimer(A,[this,SC,Anim,Dir]()
    {
        SC->SetAnimationMode(EAnimationMode::AnimationSingleNode);SC->SetAnimation(Anim);SC->Play(true);
        SC->TickAnimation(1.f/30.f,false);SC->RefreshBoneTransforms();
        const FVector Hand=SC->GetBoneLocation(TEXT("hand_r"));
        auto* Cup=AddCylinder(TEXT("PlayerGripCup_v009"),Hand+FVector(0,0,3),{.06f,.06f,.12f},{.28f,.12f,.35f});Cup->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UE_LOG(LogTemp,Display,TEXT("PLAYER_GRIP_V009 right_hand=%s left_hand=%s test_only=1 production_enabled=0"),*Hand.ToCompactString(),*SC->GetBoneLocation(TEXT("hand_l")).ToCompactString());
    },3,false);
    GetWorldTimerManager().SetTimer(B,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("01_FirstPersonHold.png"),false,false);},7,false);
    GetWorldTimerManager().SetTimer(C,[Camera,SC]()
    {
        FVector Hand=SC->GetBoneLocation(TEXT("hand_r"));Camera->SetActorLocation(Hand+FVector(-36,30,20));Camera->SetActorRotation((Hand-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(55);
    },8,false);
    GetWorldTimerManager().SetTimer(D,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("02_HandDetail.png"),false,false);},12,false);
    FTimerHandle Exit;GetWorldTimerManager().SetTimer(Exit,[](){FPlatformMisc::RequestExit(false);},14,false);
}
