#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "TerraceProbeGameMode.generated.h"

class UCameraComponent;

UCLASS()
class ATerraceProbeCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ATerraceProbeCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    void WalkForward(float Value);
    void WalkRight(float Value);
    void Turn(float Value);
    void Look(float Value);
    void Exit();
};

UCLASS()
class ATerraceProbeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ATerraceProbeGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    bool Auto = false;
    int32 Stage = 0;
    double Started = 0;
    FVector WalkStart = FVector::ZeroVector;
    FVector WalkEnd = FVector::ZeroVector;
    FVector RailEnd = FVector::ZeroVector;
    FVector SideStart = FVector::ZeroVector;
    FVector SideEnd = FVector::ZeroVector;
    TArray<TSharedPtr<class FJsonValue>> Captures;
    void Capture(const FString& Name);
    void Finish();
};
