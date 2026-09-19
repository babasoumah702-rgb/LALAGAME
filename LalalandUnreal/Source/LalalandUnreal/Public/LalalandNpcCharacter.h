#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LalalandDtos.h"
#include "LalalandNpcCharacter.generated.h"

class UTextRenderComponent;
class UAnimSequence;

UCLASS()
class LALALANDUNREAL_API ALalalandNpcCharacter final : public ACharacter
{
    GENERATED_BODY()

public:
    ALalalandNpcCharacter();
    virtual void Tick(float DeltaSeconds) override;
    void InitializeActor(const FString& InActorId, const FLinearColor& Color);
    void ApplyState(const FLalalandActorDto& Dto, const TMap<FString, ALalalandNpcCharacter*>& Cast);
    void ShowDialogue(const FString& Text, float Seconds = 7.f);
    FString GetActorId() const { return ActorId; }

private:
    void LoadCharacterMesh();
    void CacheAnimations();
    void PlaySemanticAnimation(const FString& Semantic, bool bLooping);
    UAnimSequence* FindAnimation(const TArray<FString>& Tokens) const;
    FVector ServerToWorld(double X, double Y, double Z) const;

    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PlaceholderBody;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> NameLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> BubbleText;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> AnimationAssets;
    FString ActorId;
    FString CurrentAnimation;
    FString DesiredPosture;
    FVector DesiredLocation;
    FRotator DesiredRotation;
    float BubbleRemaining = 0;
    float GestureRemaining = 0;
    float IdleElapsed = 0;
    float NextIdleGesture = 12.f;
    double LastGestureAt = -1;
    bool bReceivedInitialState = false;
};
