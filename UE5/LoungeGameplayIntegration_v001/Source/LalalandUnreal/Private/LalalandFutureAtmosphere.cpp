#include "LalalandStage.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/RectLight.h"
#include "Materials/MaterialInstanceDynamic.h"

void ALalalandStage::BuildFutureAtmosphere()
{
    auto Finish=[](AStaticMeshActor* A,const TCHAR* Name,FLinearColor Tint,float Roughness)
    {
        const FString Path=FString::Printf(TEXT("/Game/Environment/Future448/Atmosphere_v010/%s.%s"),Name,Name);
        if(auto* Base=LoadObject<UMaterialInterface>(nullptr,*Path))
        {
            auto* M=UMaterialInstanceDynamic::Create(Base,A);M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Roughness"),Roughness);
            A->GetStaticMeshComponent()->SetMaterial(0,M);
        }
    };
    // Cladding is decorative only; the v009 wall still owns collision.
    for(int32 I=0;I<4;++I)
    {
        const float Y=-1025.f+I*140;
        auto* Panel=AddBox(FString::Printf(TEXT("AtmosphereEntryPanel_%d_v010"),I),{-298,Y,175},{.018f,1.36f,3.1f},{.075f,.06f,.057f},false);
        Finish(Panel,TEXT("M_EntryPanel_v010"),{.075f,.06f,.057f},.42f);
        auto* Trim=AddBox(FString::Printf(TEXT("AtmosphereEntrySeam_%d_v010"),I),{-296,Y+68,175},{.02f,.012f,3.1f},{.24f,.17f,.10f},false);
        Finish(Trim,TEXT("M_CanopyHousing_v010"),{.24f,.17f,.10f},.35f);
    }
    UStaticMesh* Leaf=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/Future448/Atmosphere_v010/SM_OliveLeaf_v010.SM_OliveLeaf_v010"));
    auto* Leaves=NewObject<UInstancedStaticMeshComponent>(this,TEXT("AtmosphereOliveLeaves_v010"));
    Leaves->SetupAttachment(RootComponent);AddInstanceComponent(Leaves);Leaves->SetStaticMesh(Leaf);
    Leaves->SetCollisionEnabled(ECollisionEnabled::NoCollision);Leaves->RegisterComponent();
    FRandomStream Random(1010);
    auto Stem=[this,Finish](const FString& Name,FVector A,FVector B,float Radius)
    {
        auto* S=AddCylinder(Name,(A+B)*.5f,{Radius*2/100,Radius*2/100,FVector::Distance(A,B)/100},{.085f,.064f,.037f});
        S->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        S->SetActorRotation(FRotationMatrix::MakeFromZ(B-A).Rotator());Finish(S,TEXT("M_EntryPanel_v010"),{.085f,.064f,.037f},.85f);
    };
    for(int32 Plant=0;Plant<2;++Plant)
    {
        // The 44cm diameter solid planter is wholly inside the existing inflated screen exclusion.
        const FVector P(Plant?115:-275,-670,0);
        auto* Base=AddCylinder(FString::Printf(TEXT("AtmospherePlanter_%d_v010"),Plant),P+FVector(0,0,25),{.44f,.44f,.5f},{.20f,.16f,.115f});
        Finish(Base,TEXT("M_CanopyHousing_v010"),{.20f,.16f,.115f},.32f);
        auto* Soil=AddCylinder(FString::Printf(TEXT("AtmosphereSoil_%d_v010"),Plant),P+FVector(0,0,50),{.39f,.39f,.012f},{.027f,.022f,.015f});
        Soil->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Finish(Soil,TEXT("M_EntryPanel_v010"),{.027f,.022f,.015f},.95f);
        Stem(FString::Printf(TEXT("AtmosphereStem_%d_v010"),Plant),P+FVector(0,0,50),P+FVector(2,2,195),2.5f);
        for(int32 Branch=0;Branch<10;++Branch)
        {
            const float Angle=Branch*2.39996f;const float Z=112.f+Branch*8.5f;
            const FVector A=P+FVector(0,0,Z),B=P+FVector(28*FMath::Cos(Angle),28*FMath::Sin(Angle),Z+24);
            Stem(FString::Printf(TEXT("AtmosphereBranch_%d_%d_v010"),Plant,Branch),A,B,.65f);
            for(int32 J=0;J<24;++J)
            {
                FVector Pos=FMath::Lerp(A,B,.3f+.7f*Random.FRand())+FVector(Random.FRandRange(-13,13),Random.FRandRange(-13,13),Random.FRandRange(-10,18));
                Leaves->AddInstance(FTransform(FRotator(Random.FRandRange(-65,65),Random.FRandRange(0,360),Random.FRandRange(-100,100)),Pos,FVector(Random.FRandRange(.65f,1.f))));
            }
        }
    }
    auto Fill=[this](FVector P,float Yaw,float Power,float Width,float Height,float Radius,FLinearColor Tint)
    {
        auto* L=GetWorld()->SpawnActor<ARectLight>(P,FRotator(0,Yaw,0));L->Tags.Add(TEXT("AtmosphereFill_v010"));
        auto* C=L->RectLightComponent.Get();C->SetMobility(EComponentMobility::Movable);C->SetIntensityUnits(ELightUnits::Lumens);
        C->SetIntensity(Power);C->SetSourceWidth(Width);C->SetSourceHeight(Height);C->SetAttenuationRadius(Radius);C->SetLightColor(Tint);
        C->SetCastShadows(false);C->SetSpecularScale(.2f);
    };
    Fill({-80,-905,245},90,25,380,220,380,{1,.88f,.74f});
    Fill({-220,765,235},90,45,400,220,350,{1,.83f,.64f});
    Fill({-1600,655,315},180,25,320,80,500,{1,.83f,.72f});
    UE_LOG(LogTemp,Display,TEXT("ATMOSPHERE_V010_BUILT local_meshes=3 rings=2 plants=2 leaf_instances=%d screen_zone_preserved=1 paid_api_requests=0"),Leaves->GetInstanceCount());
}
