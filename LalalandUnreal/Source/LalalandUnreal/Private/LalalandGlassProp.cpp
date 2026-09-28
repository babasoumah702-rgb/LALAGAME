#include "LalalandGlassProp.h"
#include "Components/SceneComponent.h"

ALalalandGlassProp::ALalalandGlassProp()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;
}

void ALalalandGlassProp::Build(ELalalandGlassKind Kind, const FLinearColor& Liquid, float Fill)
{
    LalalandKit::Glass(this, SceneRoot, TEXT("Drink"), FVector::ZeroVector, Kind, Liquid, 1.f, Fill);
}
