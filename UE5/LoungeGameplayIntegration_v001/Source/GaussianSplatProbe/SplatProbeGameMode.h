#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "SplatProbeGameMode.generated.h"

class UCameraComponent;

UCLASS()
class ASplatProbePawn : public APawn
{
    GENERATED_BODY()
public:
    ASplatProbePawn();
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    void Forward(float Value);
    void Right(float Value);
    void Up(float Value);
    void Turn(float Value);
    void Look(float Value);
    void Exit();
};

UCLASS()
class ASplatProbeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASplatProbeGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    double StartedAt = 0;
    int32 Step = 0;
    bool AutoProbe = false;
    bool CompositeProbe = false;
    bool WindowPreview = false;
    bool CityPreview = false;
    bool InteriorPreview = false;
    UPROPERTY() TObjectPtr<AActor> OpaquePanel;
    UPROPERTY() TObjectPtr<AActor> GlassPanel;
    TArray<TSharedPtr<class FJsonValue>> CompositeCaptures;
    TArray<double> FrameDurations;
    void TickComposite(double Elapsed);
    void TickWindow(double Elapsed);
    void TickCity(double Elapsed);
    void TickInterior(double Elapsed);
    void ReportAndExit();
};
