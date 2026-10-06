#include "LalalandGlassProp.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LalalandServiceSubsystem.h"
#include "MixologyCatalog.h"
#include "MixologyPresentation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

bool ALalalandGlassProp::BuildNativeMix(const FString& DrinkId,float Fill)
{
    static FMixCatalog Catalog;static bool Loaded=false;
    if(!Loaded){FString Error;Loaded=Catalog.Load(Error);}
    const auto Recipe=Catalog.Recipes.FindByPredicate([&](const auto& R){return R.Id==DrinkId;});
    if(!Recipe)return false;
    auto Glass=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Mixology/Visual_v005/SM_Tumbler.SM_Tumbler"));
    auto Body=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Mixology/Visual_v005/SM_LiquidBody.SM_LiquidBody"));
    auto Surface=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Mixology/Visual_v005/SM_Meniscus.SM_Meniscus"));
    if(!Glass||!Body||!Surface)return false;
    auto Part=[&](const TCHAR* Name,UStaticMesh* Mesh,const TCHAR* Material,int Sort){
        auto C=NewObject<UStaticMeshComponent>(this,Name);AddInstanceComponent(C);C->SetupAttachment(SceneRoot);C->SetStaticMesh(Mesh);
        C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Material));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetTranslucentSortPriority(Sort);C->RegisterComponent();return C;
    };
    Part(TEXT("NativeGlass"),Glass,TEXT("/Game/Mixology/Visual_v005/M_Glass_v005.M_Glass_v005"),2)->SetRelativeScale3D(FVector(.62f));
    auto L=Part(TEXT("NativeLiquid"),Body,TEXT("/Game/Mixology/Visual_v005/M_LiquidBody.M_LiquidBody"),0);
    NativeSurface=Part(TEXT("NativeMeniscus"),Surface,TEXT("/Game/Mixology/Visual_v005/M_LiquidSurface.M_LiquidSurface"),1);
    NativeHeight=MixologyPresentation::LiquidHeight(MixologyPresentation::Volume(Catalog,Recipe->Amounts));
    for(auto C:{L,NativeSurface.Get()}){
        auto M=C->CreateDynamicMaterialInstance(0);M->SetVectorParameterValue(TEXT("LiquidColor"),MixologyPresentation::LiquidColor(Catalog,Recipe->Amounts));
        M->SetScalarParameterValue(TEXT("LiquidOpacity"),MixologyPresentation::LiquidOpacity(Catalog,Recipe->Amounts));
    }
    L->SetRelativeLocation(FVector(0,0,1.426f));L->SetRelativeScale3D(FVector(.62f,.62f,NativeHeight/100.f));
    NativeSurface->SetRelativeLocation(FVector(0,0,1.426f+NativeHeight));NativeSurface->SetRelativeScale3D(FVector(.62f,.62f,1));
    LiquidParts.Add(L);LiquidScales.Add(L->GetRelativeScale3D());LiquidLocations.Add(L->GetRelativeLocation());
    bWasConsumed=Fill<.3f;if(bWasConsumed)VisibleFill=TargetFill=.18f/.7f;
    return true;
}

ALalalandGlassProp::ALalalandGlassProp()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;
}

void ALalalandGlassProp::Build(ELalalandGlassKind Kind, const FLinearColor& Liquid, float Fill)
{
    LalalandKit::Glass(this, SceneRoot, TEXT("Drink"), FVector::ZeroVector, Kind, Liquid, 1.f, Fill);
    bWasConsumed=Fill<.3f;
    TArray<UStaticMeshComponent*> Parts;GetComponents<UStaticMeshComponent>(Parts);
    for(auto* Part:Parts)if(Part->GetName().Contains(TEXT("_Liquid")))
    { LiquidParts.Add(Part);LiquidScales.Add(Part->GetRelativeScale3D());LiquidLocations.Add(Part->GetRelativeLocation()); }
}

void ALalalandGlassProp::SetConsumed(bool bConsumed)
{
    if(bConsumed&&!bWasConsumed){TargetFill=.18f/.7f;}
    bWasConsumed=bConsumed;
}

void ALalalandGlassProp::BeginSip(const FString& ActionId)
{
    if(ActionId.IsEmpty()||ActionId==SipActionId||bWasConsumed)return;
    SipActionId=ActionId;DrinkSeconds=1.4f;
}

void ALalalandGlassProp::CancelSip()
{
    if(!bWasConsumed)DrinkSeconds=0.f;
}

void ALalalandGlassProp::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* Service=GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
    if(Service&&Service->GetState().paused)return;
    DrinkSeconds=FMath::Max(0.f,DrinkSeconds-DeltaSeconds);
    VisibleFill=FMath::FInterpConstantTo(VisibleFill,TargetFill,DeltaSeconds,.6f);
    for(int32 Index=0;Index<LiquidParts.Num();++Index)
    {
        FVector Scale=LiquidScales[Index];Scale.Z*=VisibleFill;LiquidParts[Index]->SetRelativeScale3D(Scale);
        FVector Position=LiquidLocations[Index];if(!NativeSurface)Position.Z-=50.f*LiquidScales[Index].Z*(1.f-VisibleFill);LiquidParts[Index]->SetRelativeLocation(Position);
    }
    if(NativeSurface)NativeSurface->SetRelativeLocation(FVector(0,0,1.426f+NativeHeight*VisibleFill));
}
