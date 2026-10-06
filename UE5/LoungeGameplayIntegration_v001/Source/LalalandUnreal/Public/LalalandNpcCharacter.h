#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "LalalandDtos.h"
#include "LalalandNpcCharacter.generated.h"

class UTextRenderComponent;
class UAnimSequence;

UCLASS()
class LALALANDUNREAL_API ULalalandDialogueWidget final : public UUserWidget
{
    GENERATED_BODY()
};

UCLASS()
class LALALANDUNREAL_API ALalalandNpcCharacter final : public ACharacter
{
    GENERATED_BODY()

public:
    ALalalandNpcCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void InitializeActor(const FString& InActorId, const FLinearColor& Color);
    void ApplyState(const FLalalandActorDto& Dto, const TMap<FString, ALalalandNpcCharacter*>& Cast);
    void ShowDialogue(const FString& Text, float Seconds = 7.f);
    bool GetDialogueLayout(FVector& Anchor, FVector2D& Size) const;
    void ApplyDialogueLayout(const FVector2D& Offset, bool bShow);
    void TriggerGesture(const FString& Semantic) { PlaySemanticAnimation(Semantic, false); }
    FString GetActorId() const { return ActorId; }
    FName GetHandAnchor() const { return RightHandAnchor; }
    bool HasDrinkGrip() const { return bDrinkGripAnimation; }
    FVector GetMouthLocation() const;
    float GetCupAnimationProgress() const;
    bool HasCupAction(const FString& Phase) const;
    int32 GetLocomotionSteeringPhase() const { return LocomotionSteeringPhase; }

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
    UPROPERTY() TObjectPtr<class UWidgetComponent> DialogueWidget;
    UPROPERTY() TObjectPtr<class UTextBlock> DialogueLabel;
    FName HeadAnchor;
    FName RightHandAnchor;
    TArray<FString> DialoguePages;
    FString ConversationTarget;
    FString ActorId;
    FString CurrentAnimation;
    FString DesiredPosture;
    FVector DesiredLocation;
    FVector LastMovementLocation=FVector::ZeroVector;
    float StalledMovementSeconds=0.f;
    float AvoidanceRecoverySeconds=0.f;
    float MovementDiagnosticCooldown=0.f;
    FRotator DesiredRotation;
    float BubbleRemaining = 0;
    float GestureRemaining = 0;
    float IdleElapsed = 0;
    float BaseMeshFootZ = -92.f;
    float NextIdleGesture = 12.f;
    float LocomotionWalkWeight = 0.f;
    bool bLocomotionTurnGate = false;
    int32 LocomotionSteeringPhase = -1;
    bool UsesReviewedBasicMotion() const;
    double LastGestureAt = -1;
    bool bReceivedInitialState = false;
    bool bDrinkGripAnimation = false;
    bool bDialogueEligible = false;
};
