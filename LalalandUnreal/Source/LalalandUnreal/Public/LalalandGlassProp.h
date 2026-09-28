#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LalalandKit.h"
#include "LalalandGlassProp.generated.h"

UCLASS()
class LALALANDUNREAL_API ALalalandGlassProp final : public AActor
{
    GENERATED_BODY()

public:
    ALalalandGlassProp();
    void Build(ELalalandGlassKind Kind, const FLinearColor& Liquid, float Fill = .68f);

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
};
