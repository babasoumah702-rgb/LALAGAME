#include "LalalandKit.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    const TCHAR* CubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");
    const TCHAR* CylinderMesh = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    const TCHAR* SphereMesh = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    const TCHAR* ConeMesh = TEXT("/Engine/BasicShapes/Cone.Cone");

    const FLinearColor GlassTint(.78f, .88f, .92f);
    const FLinearColor Brass(.52f, .28f, .08f);
    const FLinearColor Chrome(.62f, .64f, .68f);
    const FLinearColor Walnut(.18f, .07f, .03f);
    const FLinearColor Leather(.12f, .035f, .03f);

    void Tint(UStaticMeshComponent* Component, const FLinearColor& Color, float Roughness)
    {
        UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_Tint.M_Tint"));
        if(Color.Equals(GlassTint,.001f))
            if(UMaterialInterface* Glass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/Materials/M_Glass.M_Glass")))Base=Glass;
        if (!Component || !Base) return;
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Component);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
        Component->SetMaterial(0, Material);
    }
}

UStaticMeshComponent* LalalandKit::Shape(AActor* Owner, USceneComponent* Parent, const FName& Name,
    const TCHAR* MeshPath, const FVector& Location, const FVector& Scale,
    const FLinearColor& Color, float Roughness, bool bCollision, const FRotator& Rotation)
{
    if (!Owner || !Parent) return nullptr;
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner, Name);
    Component->SetupAttachment(Parent);
    Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, MeshPath));
    Component->SetRelativeLocation(Location);
    Component->SetRelativeRotation(Rotation);
    Component->SetRelativeScale3D(Scale);
    Component->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (bCollision) Component->SetCollisionProfileName(TEXT("BlockAll"));
    Owner->AddInstanceComponent(Component);
    Component->RegisterComponent();
    Tint(Component, Color, Roughness);
    return Component;
}

void LalalandKit::WineGlass(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
    const FLinearColor& Liquid, float Size, bool bInverted, float Fill)
{
    const FVector S(Size);
    const FRotator Flip = bInverted ? FRotator(180.f, 0.f, 0.f) : FRotator::ZeroRotator;
    const FVector Origin = bInverted ? Location + FVector(0, 0, 16.f * Size) : Location;
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Foot"))), CylinderMesh, Origin + Flip.RotateVector(FVector(0, 0, .8f) * Size), FVector(.08f, .08f, .016f) * S, GlassTint, .08f, false, Flip);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Stem"))), CylinderMesh, Origin + Flip.RotateVector(FVector(0, 0, 6.2f) * Size), FVector(.011f, .011f, .1f) * S, GlassTint, .08f, false, Flip);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Bowl"))), ConeMesh, Origin + Flip.RotateVector(FVector(0, 0, 13.4f) * Size), FVector(.095f, .095f, .075f) * S, GlassTint, .06f, false, Flip + FRotator(180.f, 0.f, 0.f));
    if (!bInverted && Fill > .05f)
    {
        Shape(Owner, Parent, FName(*(Prefix + TEXT("_Liquid"))), ConeMesh,
            Origin + FVector(0, 0, (11.2f + Fill * 1.4f) * Size),
            FVector(.07f, .07f, .04f * Fill + .02f) * S, Liquid, .18f, false, FRotator(180.f, 0.f, 0.f));
    }
}

void LalalandKit::Highball(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
    const FLinearColor& Liquid, float Size, float Fill)
{
    const FVector S(Size);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Glass"))), CylinderMesh, Location + FVector(0, 0, 8.f * Size), FVector(.05f, .05f, .16f) * S, GlassTint, .07f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Base"))), CylinderMesh, Location + FVector(0, 0, .6f * Size), FVector(.055f, .055f, .012f) * S, GlassTint, .1f);
    if (Fill > .05f)
    {
        Shape(Owner, Parent, FName(*(Prefix + TEXT("_Liquid"))), CylinderMesh,
            Location + FVector(0, 0, 3.2f * Size + 5.5f * Fill * Size),
            FVector(.038f, .038f, .11f * Fill) * S, Liquid, .22f);
    }
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_IceA"))), CubeMesh, Location + FVector(1.2f, -.6f, 6.5f) * Size, FVector(.018f, .016f, .018f) * S, FLinearColor(.85f, .92f, .95f), .12f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_IceB"))), CubeMesh, Location + FVector(-.8f, .9f, 5.2f) * Size, FVector(.015f, .018f, .015f) * S, FLinearColor(.8f, .9f, .94f), .12f);
}

void LalalandKit::Coupe(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
    const FLinearColor& Liquid, float Size, float Fill)
{
    const FVector S(Size);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Foot"))), CylinderMesh, Location + FVector(0, 0, .7f * Size), FVector(.07f, .07f, .014f) * S, GlassTint, .08f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Stem"))), CylinderMesh, Location + FVector(0, 0, 4.6f * Size), FVector(.01f, .01f, .07f) * S, GlassTint, .08f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Bowl"))), SphereMesh, Location + FVector(0, 0, 9.2f * Size), FVector(.11f, .11f, .055f) * S, GlassTint, .06f);
    if (Fill > .05f)
    {
        Shape(Owner, Parent, FName(*(Prefix + TEXT("_Liquid"))), SphereMesh,
            Location + FVector(0, 0, 8.8f * Size), FVector(.085f, .085f, .032f * Fill + .02f) * S, Liquid, .2f);
    }
}

void LalalandKit::Pilsner(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
    const FLinearColor& Liquid, float Size, float Fill)
{
    const FVector S(Size);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Glass"))), ConeMesh, Location + FVector(0, 0, 9.5f * Size), FVector(.07f, .07f, .19f) * S, GlassTint, .07f, false, FRotator(180.f, 0.f, 0.f));
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Foot"))), CylinderMesh, Location + FVector(0, 0, .7f * Size), FVector(.06f, .06f, .014f) * S, GlassTint, .1f);
    if (Fill > .05f)
    {
        Shape(Owner, Parent, FName(*(Prefix + TEXT("_Liquid"))), ConeMesh,
            Location + FVector(0, 0, 8.2f * Size), FVector(.05f, .05f, .14f * Fill) * S, Liquid, .25f, false, FRotator(180.f, 0.f, 0.f));
        Shape(Owner, Parent, FName(*(Prefix + TEXT("_Head"))), CylinderMesh, Location + FVector(0, 0, (8.f + 10.f * Fill) * Size), FVector(.048f, .048f, .018f) * S, FLinearColor(.92f, .88f, .72f), .4f);
    }
}

void LalalandKit::Glass(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
    ELalalandGlassKind Kind, const FLinearColor& Liquid, float Size, float Fill)
{
    switch (Kind)
    {
    case ELalalandGlassKind::Wine: WineGlass(Owner, Parent, Prefix, Location, Liquid, Size, false, Fill); break;
    case ELalalandGlassKind::Coupe: Coupe(Owner, Parent, Prefix, Location, Liquid, Size, Fill); break;
    case ELalalandGlassKind::Pilsner: Pilsner(Owner, Parent, Prefix, Location, Liquid, Size, Fill); break;
    default: Highball(Owner, Parent, Prefix, Location, Liquid, Size, Fill); break;
    }
}

void LalalandKit::Bottle(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
    const FLinearColor& GlassColor, const FLinearColor& Label, float Size)
{
    const FVector S(Size);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Body"))), CylinderMesh, Location + FVector(0, 0, 9.f * Size), FVector(.045f, .045f, .18f) * S, GlassColor, .18f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Shoulder"))), ConeMesh, Location + FVector(0, 0, 19.2f * Size), FVector(.045f, .045f, .045f) * S, GlassColor, .18f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Neck"))), CylinderMesh, Location + FVector(0, 0, 23.4f * Size), FVector(.016f, .016f, .055f) * S, GlassColor, .16f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Foil"))), CylinderMesh, Location + FVector(0, 0, 26.6f * Size), FVector(.02f, .02f, .022f) * S, Brass, .28f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Label"))), CubeMesh, Location + FVector(2.4f, 0, 8.5f) * Size, FVector(.004f, .032f, .07f) * S, Label, .55f);
}

void LalalandKit::BarStool(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location)
{
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Base"))), CylinderMesh, Location + FVector(0, 0, 4.f), FVector(.22f, .22f, .04f), Chrome, .22f, true);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Post"))), CylinderMesh, Location + FVector(0, 0, 28.f), FVector(.035f, .035f, .48f), Chrome, .22f, true);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Footrest"))), CylinderMesh, Location + FVector(0, 0, 22.f), FVector(.18f, .18f, .018f), Brass, .3f, true);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Seat"))), CylinderMesh, Location + FVector(0, 0, 54.f), FVector(.2f, .2f, .045f), Leather, .48f, true);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Pad"))), CylinderMesh, Location + FVector(0, 0, 57.f), FVector(.18f, .18f, .02f), FLinearColor(.16f, .04f, .035f), .55f);
}

void LalalandKit::Shaker(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location)
{
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Body"))), CylinderMesh, Location + FVector(0, 0, 8.f), FVector(.045f, .045f, .16f), Chrome, .12f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Cap"))), ConeMesh, Location + FVector(0, 0, 17.5f), FVector(.045f, .045f, .05f), Chrome, .12f);
}

void LalalandKit::IceBucket(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location)
{
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Bucket"))), CylinderMesh, Location + FVector(0, 0, 7.f), FVector(.12f, .12f, .14f), Chrome, .18f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Ice"))), SphereMesh, Location + FVector(0, 0, 12.f), FVector(.09f, .09f, .05f), FLinearColor(.82f, .9f, .94f), .15f);
}

void LalalandKit::Coaster(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location)
{
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Disc"))), CylinderMesh, Location + FVector(0, 0, .4f), FVector(.07f, .07f, .008f), Walnut, .55f);
}

void LalalandKit::Citrus(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location, const FLinearColor& Color)
{
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Fruit"))), SphereMesh, Location + FVector(0, 0, 3.2f), FVector(.055f, .055f, .055f), Color, .42f);
}

void LalalandKit::NapkinStack(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location)
{
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Stack"))), CubeMesh, Location + FVector(0, 0, 1.6f), FVector(.1f, .1f, .032f), FLinearColor(.86f, .82f, .74f), .62f);
}

void LalalandKit::Pendant(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location)
{
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Cord"))), CylinderMesh, Location + FVector(0, 0, 24.f), FVector(.012f, .012f, .42f), FLinearColor(.08f, .08f, .09f), .7f, false);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Shade"))), ConeMesh, Location + FVector(0, 0, 2.f), FVector(.16f, .16f, .12f), Brass, .28f, false, FRotator(180.f, 0.f, 0.f));
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Bulb"))), SphereMesh, Location + FVector(0, 0, 1.f), FVector(.045f, .045f, .045f), FLinearColor(1.f, .72f, .38f), .08f, false);
}

void LalalandKit::TargetCup(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location, float Size)
{
    const FVector S(Size);
    const FLinearColor Red(.62f, .1f, .05f);
    // Four thin walls keep the target visibly open. The previous solid
    // cylinder hid the ball even though Chaos correctly reported an overlap.
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_WallXPos"))), CubeMesh, Location + FVector(8.f, 0, 7.f) * Size, FVector(.012f, .16f, .14f) * S, Red, .22f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_WallXNeg"))), CubeMesh, Location + FVector(-8.f, 0, 7.f) * Size, FVector(.012f, .16f, .14f) * S, Red, .22f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_WallYPos"))), CubeMesh, Location + FVector(0, 8.f, 7.f) * Size, FVector(.16f, .012f, .14f) * S, Red, .22f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_WallYNeg"))), CubeMesh, Location + FVector(0, -8.f, 7.f) * Size, FVector(.16f, .012f, .14f) * S, Red, .22f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Base"))), CylinderMesh, Location + FVector(0, 0, .9f * Size), FVector(.16f, .16f, .018f) * S, Brass, .25f);
    Shape(Owner, Parent, FName(*(Prefix + TEXT("_Liquid"))), CylinderMesh, Location + FVector(0, 0, 2.f * Size), FVector(.12f, .12f, .018f) * S, FLinearColor(.45f, .08f, .04f), .28f);
}

ELalalandGlassKind LalalandKit::KindFromDrink(const FString& DrinkId)
{
    if(DrinkId.StartsWith(TEXT("whiskey_sour"))||DrinkId==TEXT("cranberry_gin_sour"))return ELalalandGlassKind::Coupe;
    if (DrinkId == TEXT("old_city") || DrinkId == TEXT("second_bounce")) return ELalalandGlassKind::Coupe;
    if (DrinkId == TEXT("house_beer")) return ELalalandGlassKind::Pilsner;
    if (DrinkId == TEXT("terrace_breeze")) return ELalalandGlassKind::Wine;
    return ELalalandGlassKind::Highball;
}

FLinearColor LalalandKit::LiquidFromDrink(const FString& DrinkId)
{
    if (DrinkId == TEXT("terrace_breeze")) return FLinearColor(.62f, .78f, .28f);
    if (DrinkId == TEXT("mint_lime")) return FLinearColor(.28f, .72f, .42f);
    if (DrinkId == TEXT("meteor_tail")) return FLinearColor(.88f, .52f, .12f);
    if (DrinkId == TEXT("old_city")) return FLinearColor(.38f, .16f, .05f);
    if (DrinkId == TEXT("second_bounce")) return FLinearColor(.55f, .28f, .08f);
    if (DrinkId == TEXT("house_beer")) return FLinearColor(.72f, .52f, .16f);
    if (DrinkId == TEXT("oolong_pomelo_0")) return FLinearColor(.55f, .42f, .16f);
    if (DrinkId == TEXT("berry_ginger_0")) return FLinearColor(.55f, .12f, .22f);
    if (DrinkId == TEXT("lime_soda_0")) return FLinearColor(.55f, .82f, .48f);
    if(DrinkId.StartsWith(TEXT("oolong")))return FLinearColor(.55f,.42f,.16f);
    if(DrinkId.StartsWith(TEXT("cranberry")))return FLinearColor(.62f,.12f,.24f);
    if(DrinkId.Contains(TEXT("mojito")))return FLinearColor(.4f,.76f,.42f);
    if(DrinkId.StartsWith(TEXT("gin_")))return FLinearColor(.78f,.82f,.65f);
    if(DrinkId.StartsWith(TEXT("negroni")))return FLinearColor(.55f,.14f,.05f);
    return FLinearColor(.42f, .12f, .08f);
}
