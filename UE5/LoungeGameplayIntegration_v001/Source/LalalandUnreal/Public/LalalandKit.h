#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;

enum class ELalalandGlassKind : uint8
{
    Wine,
    Highball,
    Coupe,
    Pilsner
};

namespace LalalandKit
{
    UStaticMeshComponent* Shape(AActor* Owner, USceneComponent* Parent, const FName& Name,
        const TCHAR* MeshPath, const FVector& Location, const FVector& Scale,
        const FLinearColor& Color, float Roughness = .55f, bool bCollision = false,
        const FRotator& Rotation = FRotator::ZeroRotator);

    void WineGlass(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
        const FLinearColor& Liquid, float Size = 1.f, bool bInverted = false, float Fill = .62f);
    void Highball(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
        const FLinearColor& Liquid, float Size = 1.f, float Fill = .7f);
    void Coupe(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
        const FLinearColor& Liquid, float Size = 1.f, float Fill = .55f);
    void Pilsner(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
        const FLinearColor& Liquid, float Size = 1.f, float Fill = .72f);
    void Glass(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
        ELalalandGlassKind Kind, const FLinearColor& Liquid, float Size = 1.f, float Fill = .65f);

    void Bottle(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location,
        const FLinearColor& Glass, const FLinearColor& Label, float Size = 1.f);
    void BarStool(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location);
    void Shaker(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location);
    void IceBucket(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location);
    void Coaster(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location);
    void Citrus(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location, const FLinearColor& Color);
    void NapkinStack(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location);
    void Pendant(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location);
    void TargetCup(AActor* Owner, USceneComponent* Parent, const FString& Prefix, const FVector& Location, float Size = 1.f);

    ELalalandGlassKind KindFromDrink(const FString& DrinkId);
    FLinearColor LiquidFromDrink(const FString& DrinkId);
}
