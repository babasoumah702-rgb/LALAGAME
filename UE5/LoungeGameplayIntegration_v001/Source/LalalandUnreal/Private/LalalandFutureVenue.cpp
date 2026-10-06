#include "LalalandStage.h"
#include "LalalandVenueLayout.h"
#include "LalalandSpatial.h"
#include "LalalandKit.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PointLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"

namespace
{
const FLinearColor Dark(.035f,.039f,.05f), Bronze(.29f,.17f,.085f), Velvet(.13f,.038f,.095f);
void Finish(AStaticMeshActor* Actor,const TCHAR* Name,const FLinearColor& Tint,float Roughness=.4f)
{
    if(!Actor)return;
    const FString AssetName=FPaths::GetCleanFilename(Name);
    const FString Path=FString::Printf(TEXT("/Game/Environment/Future448/%s.%s"),Name,*AssetName);
    if(UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,*Path))
    {
        auto* M=UMaterialInstanceDynamic::Create(Base,Actor);
        M->SetVectorParameterValue(TEXT("Tint"),Tint);
        M->SetScalarParameterValue(TEXT("Roughness"),Roughness);
        Actor->GetStaticMeshComponent()->SetMaterial(0,M);
    }
    else UE_LOG(LogTemp,Warning,TEXT("FUTURE_FINISH_MISSING path=%s"),*Path);
}
}

void ALalalandStage::BuildFutureArchitecture()
{
    auto Collider=[this](const TCHAR* Name,FVector P,FVector Extent)
    {
        auto* C=NewObject<UBoxComponent>(this,FName(Name));
        C->SetupAttachment(RootComponent);C->SetRelativeLocation(P);C->SetBoxExtent(Extent);
        C->SetCollisionProfileName(TEXT("BlockAll"));C->CanCharacterStepUpOn=ECB_No;
        AddInstanceComponent(C);C->RegisterComponent();ArchitectureColliders.Add(C);
    };
    auto Stone=[this](const FString& Name,FVector P,FVector S,bool Collision=true)
    {
        auto* A=AddBox(Name,P,S,Dark,Collision);Finish(A,TEXT("M_FutureStone"),Dark,.38f);return A;
    };
    auto* MainFloor=Stone(TEXT("BarFloor"),{-700,300,-11},{28,16,.22f},false);
    Finish(MainFloor,TEXT("Atmosphere_v010/M_LoungeStone_v010"),{.045f,.043f,.054f},.22f);
    Collider(TEXT("BarFloorCollider"),{-700,300,-11},{1400,800,11});
    // Do not overlap the car floor: coplanar surfaces caused flickering and mixed finishes.
    Stone(TEXT("StairAccessCorridorEast"),{475,-582.5f,-10},{7.5f,1.65f,.2f},false);
    Collider(TEXT("StairAccessCorridorCollider"),{300,-582.5f,-10},{550,82.5f,10});
    BuildFutureElevator();
    BuildFutureEntrance();
    const float HallTop=LalalandSpatial::ClearHeight();
    AddBox(TEXT("Ceiling"),{-700,300,HallTop+6},{28,16,.12f},Dark,false);
    AddBox(TEXT("FutureSouthWallLeft_v009"),{-925,-490,HallTop*.5f},{23.5f,.12f,HallTop/100},Dark);
    AddBox(TEXT("FutureSouthWallRight_v009"),{612.5f,-490,HallTop*.5f},{1.25f,.12f,HallTop/100},Dark);

    // Glass is decorative; primitive boundary colliders remain deterministic.
    Collider(TEXT("WestWindowCollider"),{-2100,300,HallTop*.5f},{6,800,HallTop*.5f});
    Collider(TEXT("NorthWindowCollider"),{-700,1100,HallTop*.5f},{1400,6,HallTop*.5f});
    AddBox(TEXT("WestWindowLower"),{-2100,300,32},{.12f,16,.64f},Dark,false);
    AddBox(TEXT("NorthWindowLower"),{-700,1100,32},{28,.12f,.64f},Dark,false);
    auto Window=[this](const FString& Name,FVector P,FVector S)
    {
        auto* A=AddBox(Name,P,S,{.08f,.14f,.19f},false);
        Finish(A,TEXT("M_FutureWindow"),{.08f,.14f,.19f},.12f);
        A->GetStaticMeshComponent()->SetCastShadow(false);
    };
    Window(TEXT("FutureWestGlass"),{-2095,300,HallTop*.5f},{.015f,16,(HallTop-64)/100});
    Window(TEXT("FutureNorthGlass"),{-700,1095,HallTop*.5f},{28,.015f,(HallTop-64)/100});
    for(int32 I=0;I<=8;++I)
        AddBox(FString::Printf(TEXT("WestMullion_%d"),I),{-2090,-500.f+I*200,HallTop*.5f},{.06f,.08f,HallTop/100},Bronze,false);
    for(int32 I=0;I<=14;++I)
        AddBox(FString::Printf(TEXT("NorthMullion_%d"),I),{-2100.f+I*200,1090,HallTop*.5f},{.08f,.06f,HallTop/100},Bronze,false);
    AddBox(TEXT("WestWindowLightStrip"),{-2090,300,365},{.03f,16,.025f},{.15f,.3f,.6f},false);
    AddBox(TEXT("NorthWindowLightStrip"),{-700,1090,365},{28,.03f,.025f},{.35f,.16f,.5f},false);

    // Keep the two existing stair approaches, with an extended northern frame.
    AddBox(TEXT("TerraceFrameFarSouth"),{690,-1020,175},{.16f,1.6f,3.5f},Bronze,false);
    AddBox(TEXT("TerraceFrameSouthPost"),{690,-900,175},{.16f,.3f,3.5f},Bronze,false);
    AddBox(TEXT("TerraceFrameNorth"),{690,410,220},{.16f,13.8f,4.4f},Bronze,false);
    Collider(TEXT("TerraceFrameFarSouthCollider"),{690,-1020,175},{16,80,175});
    Collider(TEXT("TerraceFrameSouthPostCollider"),{690,-900,175},{16,15,175});
    Collider(TEXT("TerraceFrameNorthCollider"),{690,410,220},{16,690,225});
    Window(TEXT("FutureEastGlass"),{695,410,220},{.015f,13.8f,3.1f});
    for(int32 I=0;I<7;++I)
        AddBox(FString::Printf(TEXT("EastMullion_%d"),I),{692,-200.f+I*200,220},{.06f,.08f,4.4f},Bronze,false);
    AddBox(TEXT("EastWindowLightStrip"),{680,410,365},{.03f,13.8f,.025f},{.12f,.24f,.6f},false);

    auto* Front=Stone(TEXT("FutureBarCounter"),{-220,450,55},{8.6f,.95f,1.1f});
    Finish(Front,TEXT("M_FutureMetal"),Bronze,.48f);
    auto* FrontTop=Stone(TEXT("FutureBarTop"),{-220,450,113},{8.6f,.95f,.08f});
    // Imported artwork is visual only. Existing primitive colliders and the
    // 117cm serving surface stay aligned to the service's 8.6 x .95m footprint.
    UStaticMesh* BarMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/Future448/BarCounter/SM_FutureBarCounter_v002.SM_FutureBarCounter_v002"));
    if(BarMesh)
    {
        const FBox Bounds=BarMesh->GetBoundingBox();
        auto* Bar=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-220,450,0),FRotator::ZeroRotator);
        Bar->Tags.Add(TEXT("FutureAuthoredBarCounter"));
        auto* C=Bar->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);
        C->SetStaticMesh(BarMesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Bar->SetActorLocation(FVector(-220,450,0)-FVector(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Min.Z));
        Front->SetActorHiddenInGame(true);FrontTop->SetActorHiddenInGame(true);
        UE_LOG(LogTemp,Display,TEXT("FUTURE_BAR_PLACED size_cm=%.2fx%.2fx%.2f visual_only=1 serving_z=117"),Bounds.GetSize().X,Bounds.GetSize().Y,Bounds.GetSize().Z);
    }
    for(float X:{-605.f,165.f})
    {
        Stone(FString::Printf(TEXT("FutureReturn_%d"),int(X)),{X,645,55},{.9f,3.6f,1.1f});
        Stone(FString::Printf(TEXT("FutureReturnTop_%d"),int(X)),{X,645,113},{.96f,3.6f,.08f});
    }
    auto* OldStrip=AddBox(TEXT("BarFrontLightStrip"),{-220,408,28},{8.3f,.025f,.025f},{.6f,.38f,.16f},false);
    auto* OldRail=AddBox(TEXT("FutureBarFootRail"),{-220,388,24},{8.3f,.04f,.04f},Bronze,false);
    if(BarMesh)
    {
        OldStrip->SetActorHiddenInGame(true);OldRail->SetActorHiddenInGame(true);
        AddBox(TEXT("FutureBarVioletUnderlight"),{-220,450,1},{7.8f,.025f,.018f},{.24f,.07f,.5f},false);
    }
    UStaticMesh* CabinetMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/Future448/Furniture_v011/SM_Backbar_v011.SM_Backbar_v011"));
    if(CabinetMesh)
    {
        const FBox B=CabinetMesh->GetBoundingBox();
        // FBX forward-axis conversion puts the prepared front at local +Y in UE.
        const FRotator R(0,180,0);
        auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-220,830,0)-R.RotateVector(FVector(B.GetCenter().X,B.GetCenter().Y,B.Min.Z)),R);
        A->Tags.Add(TEXT("FutureAuthoredBackbar_v011"));
        auto* C=A->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(CabinetMesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const auto F=LalalandVenue::Layout()->GetObjectField(TEXT("furniture"))->GetObjectField(TEXT("backbarFootprint"));
        Collider(TEXT("FutureBackbarCollider_v011"),{-220,830,170},{float(F->GetNumberField(TEXT("w"))*50),float(F->GetNumberField(TEXT("h"))*50),170});
        UE_LOG(LogTemp,Display,TEXT("FURNITURE_V011_BACKBAR size_cm=%s front=-Y shelf_z=128.52,191.72,252.12"),*B.GetSize().ToCompactString());
    }
    else
    {
    UE_LOG(LogTemp,Warning,TEXT("FURNITURE_V011_BACKBAR missing=1 placeholder=1"));
    auto* Cabinet=Stone(TEXT("FutureBackbarCabinet"),{-220,830,60},{4.6f,.6f,1.2f});
    Finish(Cabinet,TEXT("M_FutureMetal"),{.09f,.075f,.065f},.32f);
    auto* ShelfBackdrop=AddBox(TEXT("FutureShelfBackdrop"),{-220,865,228},{4.6f,.06f,2.2f},Dark,false);
    Finish(ShelfBackdrop,TEXT("M_FutureMetal"),{.09f,.06f,.12f},.27f);
    for(int32 Row=0;Row<4;++Row)
    {
        auto* Shelf=AddBox(FString::Printf(TEXT("FutureShelf_%d"),Row),{-220,830,124.f+Row*52},{4.6f,.68f,.04f},Bronze,false);
        Finish(Shelf,TEXT("M_FutureMetal"),Bronze,.3f);
        AddBox(FString::Printf(TEXT("FutureShelfLight_%d"),Row),{-220,799,122.f+Row*52},{4.5f,.015f,.018f},{.7f,.45f,.2f},false);
    }
    for(float X:{-450.f,10.f})AddBox(FString::Printf(TEXT("FutureShelfVioletLight_%d"),int(X)),{X,798,218},{.018f,.025f,1.95f},{.2f,.08f,.42f},false);
    for(float X:{-453.f,13.f})
        AddBox(FString::Printf(TEXT("FutureShelfColumn_%d"),int(X)),{X,830,204},{.04f,.68f,1.6f},Bronze,false);
    }

    // Faceted placeholders share exactly the service's rotated seat footprints.
    const auto G=LalalandVenue::Layout()->GetObjectField(TEXT("boothGeometry"));
    UStaticMesh* SofaMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/Future448/Sofa/SM_CurvedVelvetSofa_v001Scaled.SM_CurvedVelvetSofa_v001Scaled"));
    int32 BoothIndex=0;
    for(const auto& V:LalalandVenue::Layout()->GetArrayField(TEXT("booths")))
    {
        const auto O=V->AsObject();const FVector P=LalalandVenue::Point(O);
        const int32 Count=G->GetIntegerField(TEXT("segments"));
        for(int32 I=0;I<Count;++I)
        {
            const float Degree=180.f-G->GetNumberField(TEXT("sweepDegrees"))*.5f+I*G->GetNumberField(TEXT("sweepDegrees"))/(Count-1)+O->GetNumberField(TEXT("yaw"));
            const FVector Radial(FMath::Cos(FMath::DegreesToRadians(Degree)),FMath::Sin(FMath::DegreesToRadians(Degree)),0);
            const FVector Q=P+Radial*G->GetNumberField(TEXT("radius"))*100;
            const FString Prefix=FString::Printf(TEXT("FutureBooth_%d_%d"),BoothIndex,I);
            auto* Base=AddBox(Prefix+TEXT("Base"),Q+FVector(0,0,14),{.5f,.7f,.28f},Bronze);
            auto* Seat=AddBox(Prefix+TEXT("Seat"),Q+FVector(0,0,37),{.5f,.7f,.14f},Velvet,false);
            auto* Back=AddBox(Prefix+TEXT("Back"),Q+Radial*42+FVector(0,0,65),{.5f,.14f,.42f},Velvet);
            for(auto* A:{Base,Seat,Back})A->SetActorRotation(FRotator(0,Degree+90,0));
            Finish(Base,TEXT("M_FutureMetal"),Bronze,.5f);
            Finish(Seat,TEXT("M_FutureVelvet"),Velvet,.85f);Finish(Back,TEXT("M_FutureVelvet"),Velvet,.85f);
            // Pilot collision proxies remain visible only when the authored mesh is absent.
            if(SofaMesh)for(auto* A:{Base,Seat,Back})A->SetActorHiddenInGame(true);
        }
        if(SofaMesh)
        {
            const FBox Bounds=SofaMesh->GetBoundingBox();
            const float UniformScale=270.f/Bounds.GetSize().X;
            const FRotator Rotation(0,-90+O->GetNumberField(TEXT("yaw")),0);
            auto* Sofa=GetWorld()->SpawnActor<AStaticMeshActor>(P+FVector(-105,0,0),Rotation);
            Sofa->Tags.Add(FName(*FString::Printf(TEXT("FutureRealSofa_%d"),BoothIndex)));
            auto* C=Sofa->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);
            C->SetStaticMesh(SofaMesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Sofa->SetActorScale3D(FVector(UniformScale));
            const FVector SofaPivotCorrection=Rotation.RotateVector(FVector(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Min.Z)*UniformScale);
            Sofa->SetActorLocation(P+FVector(-105,0,0)-SofaPivotCorrection);
            UE_LOG(LogTemp,Display,TEXT("FUTURE_SOFA_PLACED index=%d width=270cm depth=%.2fcm height=%.2fcm yaw=%.2f"),BoothIndex,Bounds.GetSize().Y*UniformScale,Bounds.GetSize().Z*UniformScale,Rotation.Yaw);
        }
        auto* Table=AddCylinder(FString::Printf(TEXT("FutureBoothTable_%d"),BoothIndex),P+FVector(20,0,51),{.9f,.9f,.08f},Dark);
        Finish(Table,TEXT("M_FutureStone"),Dark,.32f);
        AddCylinder(FString::Printf(TEXT("FutureBoothPedestal_%d"),BoothIndex),P+FVector(20,0,25),{.18f,.18f,.5f},Bronze);
        ++BoothIndex;
    }
    const FVector Lounge=LalalandVenue::Named(TEXT("loungeTable"));
    auto* Table=AddCylinder(TEXT("FutureWindowTable"),Lounge+FVector(0,0,51),{.9f,.9f,.08f},Dark);
    Finish(Table,TEXT("M_FutureStone"),Dark,.32f);
    AddCylinder(TEXT("FutureWindowPedestal"),Lounge+FVector(0,0,25),{.18f,.18f,.5f},Bronze);

    const int32 StairCount=LalalandSpatial::Architecture()->GetIntegerField(TEXT("stairCount"));
    AddBox(TEXT("StairLandingBottom"),{745,LalalandSpatial::StairBottom(),-10},{1.4f,.8f,.2f},Dark);
    for(int32 I=0;I<StairCount;++I)
    {
        const float Top=LalalandSpatial::Roof()*(I+1)/StairCount;
        const float Y=(LalalandSpatial::Architecture()->GetNumberField(TEXT("stairFirstZ"))+I*LalalandSpatial::Architecture()->GetNumberField(TEXT("stairRun")))*100;
        Stone(FString::Printf(TEXT("RoofStair_%02d"),I),{745,Y,Top*.5f},{1.4f,.49f,Top/100});
        AddBox(FString::Printf(TEXT("StairLight_%02d"),I),{678,Y,Top+2},{.018f,.26f,.018f},{.1f,.25f,.55f},false);
    }
    const float Roof=LalalandSpatial::Roof();
    Stone(TEXT("UpperRooftop"),{-647.5f,675,Roof-20},{29.05f,8.5f,.4f},false);
    Collider(TEXT("UpperRooftopCollider"),{-647.5f,675,Roof-20},{1452.5f,425,20});
    AddBox(TEXT("RoofRailNorth"),{-647.5f,1095,Roof+55},{29.05f,.06f,1.1f},Dark);
    AddBox(TEXT("RoofRailWest"),{-2095,675,Roof+55},{.06f,8.5f,1.1f},Dark);
    AddBox(TEXT("RoofRailEast"),{820,675,Roof+55},{.06f,8.5f,1.1f},Dark);
    AddBox(TEXT("RoofBench"),{-185,650,Roof+38},{2.1f,.62f,.28f},Velvet);
    AddBox(TEXT("RoofBenchBack"),{-185,705,Roof+85},{2.1f,.16f,.68f},Velvet);
    AddCylinder(TEXT("RoofLowTable"),{245,590,Roof+30},{.72f,.72f,.16f},Dark);
    auto* Sky=AddBox(TEXT("VenueSky"),FVector::ZeroVector,FVector(300),{.018f,.028f,.065f},false);
    Sky->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Sky->GetStaticMeshComponent()->SetCastShadow(false);
}

void ALalalandStage::BuildFutureDetails()
{
    const bool AuthoredCabinet=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/Future448/Furniture_v011/SM_Backbar_v011.SM_Backbar_v011"))!=nullptr;
    if(AuthoredCabinet)
    {
        // Measured horizontal shelf surfaces, not heights from the old four-row placeholder.
        const float Tops[]={128.52f,191.72f,252.12f};
        const float Xs[]={-180,-150,-120,-60,-30,0,70,100,130,170};
        int32 Row=0;for(float Z:Tops)
        {
            int32 I=0;for(float X:Xs)
            {
                LalalandKit::Bottle(this,RootComponent,FString::Printf(TEXT("FutureBottle_v011_%d_%d"),Row,I),{-220+X,830,Z+.15f},
                    I%3==0?FLinearColor(.14f,.065f,.028f):FLinearColor(.035f,.09f+.02f*(I%3),.08f),{.65f,.57f,.38f},1.1f);
                ++I;
            }
            ++Row;
        }
    }
    else for(int32 Row=0;Row<4;++Row)for(int32 I=0;I<12;++I)
        LalalandKit::Bottle(this,RootComponent,FString::Printf(TEXT("FutureBottle_%d_%d"),Row,I),
            {-420.f+I*36,820,126.f+Row*52},{.04f+.02f*(I%3),.12f,.1f},{.65f,.57f,.38f},.85f);
    USceneComponent* BarRoot=NewObject<USceneComponent>(this,TEXT("FutureBarProps"));
    BarRoot->SetupAttachment(RootComponent);BarRoot->SetRelativeLocation(LalalandVenue::Named(TEXT("barOffset")));
    AddInstanceComponent(BarRoot);BarRoot->RegisterComponent();
    // The native station owns the mixing bottles and shaker, without duplicates.
    LalalandKit::IceBucket(this,BarRoot,TEXT("FutureBarIce"),{-160,42,117});
    LalalandKit::Citrus(this,BarRoot,TEXT("FutureBarLime"),{-245,36,117},{.42f,.72f,.18f});
    LalalandKit::Coupe(this,BarRoot,TEXT("FutureCoupe"),{-100,25,117},{.8f,.42f,.16f});
    LalalandKit::Highball(this,BarRoot,TEXT("FutureHighball"),{0,24,117},{.28f,.72f,.42f});
    int32 I=0;
    UStaticMesh* StoolMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/Future448/Furniture_v011/SM_BarStool_v011.SM_BarStool_v011"));
    for(const auto& V:LalalandVenue::Layout()->GetArrayField(TEXT("stools")))
    {
        const FVector P=LalalandVenue::Point(V->AsObject());
        if(StoolMesh)
        {
            const FBox B=StoolMesh->GetBoundingBox();const FRotator R=FRotator::ZeroRotator;
            auto* A=GetWorld()->SpawnActor<AStaticMeshActor>(P-R.RotateVector(FVector(B.GetCenter().X,B.GetCenter().Y,B.Min.Z)),R);
            A->Tags.Add(FName(*FString::Printf(TEXT("FutureAuthoredStool_%d_v011"),I)));
            auto* C=A->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(StoolMesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            const auto F=LalalandVenue::Layout()->GetObjectField(TEXT("furniture"))->GetObjectField(TEXT("stoolFootprint"));
            auto* Proxy=NewObject<UBoxComponent>(this,FName(*FString::Printf(TEXT("FutureStoolCollider_%d_v011"),I)));
            Proxy->SetupAttachment(RootComponent);Proxy->SetRelativeLocation(P+FVector(0,0,B.GetSize().Z*.5f));
            Proxy->SetBoxExtent({float(F->GetNumberField(TEXT("w"))*50),float(F->GetNumberField(TEXT("h"))*50),B.GetSize().Z*.5f});
            Proxy->SetCollisionProfileName(TEXT("BlockAll"));Proxy->CanCharacterStepUpOn=ECB_No;AddInstanceComponent(Proxy);Proxy->RegisterComponent();ArchitectureColliders.Add(Proxy);
            UE_LOG(LogTemp,Display,TEXT("FURNITURE_V011_STOOL index=%d size_cm=%s seat=80 front=+Y visual_only=1"),I,*B.GetSize().ToCompactString());
        }
        else LalalandKit::BarStool(this,RootComponent,FString::Printf(TEXT("FutureStool_%d"),I),P);
        ++I;
    }
    for(const auto& V:LalalandVenue::Layout()->GetArrayField(TEXT("booths")))
    {
        const FVector P=LalalandVenue::Point(V->AsObject(),55);
        LalalandKit::WineGlass(this,RootComponent,FString::Printf(TEXT("FutureBoothWine_%d"),I),P+FVector(10,-15,0),{.55f,.12f,.22f},.9f);
        LalalandKit::Coupe(this,RootComponent,FString::Printf(TEXT("FutureBoothCoupe_%d"),I++),P+FVector(30,15,0),{.8f,.5f,.18f},.85f);
    }
    // Continuous local geometry: bronze housing and a recessed downward diffuser.
    for(int32 Arc=0;Arc<2;++Arc)
    {
        const TCHAR* Name=Arc?TEXT("SM_CanopyInner_v010"):TEXT("SM_CanopyOuter_v010");
        const FString Path=FString::Printf(TEXT("/Game/Environment/Future448/Atmosphere_v010/%s.%s"),Name,Name);
        if(auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path))
        {
            auto* Ring=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-700,280,LalalandSpatial::ClearHeight()-17-(Arc?20:0)),FRotator::ZeroRotator);
            Ring->Tags.Add(FName(*FString::Printf(TEXT("FutureCanopyRing_%d_v010"),Arc)));
            auto* C=Ring->GetStaticMeshComponent();C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            UE_LOG(LogTemp,Display,TEXT("ATMOSPHERE_V010_CANOPY arc=%d bounds=%s"),Arc,*C->Bounds.BoxExtent.ToCompactString());
            continue;
        }
        UE_LOG(LogTemp,Warning,TEXT("ATMOSPHERE_V010_CANOPY missing=%s using_box_fallback=1"),Name);
        for(int32 J=0;J<32;++J)
        {
        const float T=J*2*PI/32,Next=(J+1)*2*PI/32;
        const float RX=Arc?680:970,RY=Arc?390:580;
        const float Z=LalalandSpatial::ClearHeight()-17-(Arc?20:0);
        const FVector A(-700+RX*FMath::Cos(T),280+RY*FMath::Sin(T),Z);
        const FVector B(-700+RX*FMath::Cos(Next),280+RY*FMath::Sin(Next),Z);
        auto* Strip=AddBox(FString::Printf(TEXT("FutureCanopyStrip_%d_%d"),Arc,J),(A+B)*.5f,{FVector::Distance(A,B)/100,.1f,.035f},
            J%8<4?FLinearColor(.16f,.25f,.5f):FLinearColor(.34f,.16f,.4f),false);
        Strip->SetActorRotation(FRotator(0,FMath::RadiansToDegrees(FMath::Atan2(B.Y-A.Y,B.X-A.X)),0));
        Strip->GetStaticMeshComponent()->SetCastShadow(false);
        }
    }
    auto* Windows=NewObject<UInstancedStaticMeshComponent>(this,TEXT("FutureCityWindows"));
    AddInstanceComponent(Windows);Windows->SetupAttachment(RootComponent);
    Windows->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Windows->SetCollisionEnabled(ECollisionEnabled::NoCollision);Windows->SetCastShadow(false);Windows->RegisterComponent();
    if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/Materials/M_VenueGlow.M_VenueGlow")))
    {
        auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.3f,.38f,.6f));
        M->SetScalarParameterValue(TEXT("Intensity"),.8f);Windows->SetMaterial(0,M);
    }
    for(int32 Tower=0;Tower<28;++Tower)
    {
        const float Angle=Tower*360.f/28;
        const FVector R(FMath::Cos(FMath::DegreesToRadians(Angle)),FMath::Sin(FMath::DegreesToRadians(Angle)),0),T(-R.Y,R.X,0);
        const FVector C=FVector(-700,300,0)+R*(4200+(Tower%3)*270);
        const float H=760+(Tower%5)*165;
        auto* Building=AddBox(FString::Printf(TEXT("RoofCity_%02d"),Tower),C+FVector(0,0,H*.5f),{2.3f,1.8f,H/100},{.025f,.035f,.065f},false);
        Building->SetActorRotation(FRotator(0,Angle,0));
        for(int32 Row=0;Row<int(H/75);++Row)for(int32 Col=0;Col<3;++Col)if((Row+Col+Tower)%4)
            Windows->AddInstance(FTransform(FRotator(0,Angle,0),C-R*117+T*(-48+Col*48)+FVector(0,0,55+Row*75),FVector(.025f,.12f,.16f)));
    }
    UE_LOG(LogTemp,Display,TEXT("FUTURE_LOUNGE_V009_BUILT hall=28x16m clear_height_cm=550 roof_cm=600 external_corridor=1"));
}

void ALalalandStage::BuildFutureLighting()
{
    auto Point=[this](FVector P,FLinearColor Color,float Intensity,float Radius,bool Shadow=false)
    {
        auto* L=GetWorld()->SpawnActor<APointLight>(P,FRotator::ZeroRotator);
        auto* C=CastChecked<UPointLightComponent>(L->GetLightComponent());
        C->SetMobility(EComponentMobility::Movable);C->SetLightColor(Color);C->SetIntensity(Intensity);C->SetAttenuationRadius(Radius);C->SetCastShadows(Shadow);
    };
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetIntensity(.8f);
    auto* Moon=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,360),FRotator(-48,-28,0));
    Moon->GetLightComponent()->SetMobility(EComponentMobility::Movable);Moon->GetLightComponent()->SetLightColor({.3f,.38f,.6f});Moon->GetLightComponent()->SetIntensity(1.6f);
    Point({-220,600,315},{1,.72f,.46f},22000,1100,true);
    Point({-220,110,210},{.92f,.89f,.84f},10500,900);
    for(float X:{-480.f,-220.f,40.f})Point({X,401,5},{.35f,.1f,.75f},450,160);
    Point({-1780,820,300},{1,.62f,.43f},14000,650);
    Point({-1780,470,300},{1,.62f,.43f},14000,650);
    Point({-1670,495,155},{1,.72f,.57f},7500,450);
    Point({-1670,830,155},{1,.72f,.57f},7500,450);
    Point({-1000,-120,330},{1,.7f,.52f},25000,1000,true);
    Point({430,720,310},{.5f,.6f,1},20000,800);
    Point({-850,310,330},{.68f,.69f,.84f},20000,1500);
    // The elevator owns its local lights; keep them separate from the lounge.
    Point({-700,780,870},{.5f,.6f,.85f},36000,2300);
    Point({-170,630,840},{1,.65f,.4f},28000,1000);
    auto* Grade=GetWorld()->SpawnActor<APostProcessVolume>();Grade->bUnbound=true;Grade->Priority=10;
    auto& S=Grade->Settings;S.bOverride_AutoExposureMethod=true;S.AutoExposureMethod=AEM_Manual;
    S.bOverride_AutoExposureBias=true;S.AutoExposureBias=1.f;
    S.bOverride_AutoExposureApplyPhysicalCameraExposure=true;S.AutoExposureApplyPhysicalCameraExposure=false;
}
