#pragma once
#include "CoreMinimal.h"
#include "LoungeCharacterIntegrationGameMode.h"
#include "MixologyGameMode.h"
#include "LoungeRecordingGameMode.generated.h"

UCLASS()
class ALoungeRecordingHUD : public AMixologyHUD
{
    GENERATED_BODY()
public:
    ALoungeRecordingHUD();
};

// Separate, explicitly offline vertical slice for local capture.
UCLASS()
class ALoungeRecordingGameMode : public ALoungeCharacterIntegrationGameMode
{
    GENERATED_BODY()
public:
    ALoungeRecordingGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    bool Auto=false, Failed=false;
    int32 Phase=0;
    float At=0;
    FVector MoveStart;
    void FinishRecordingTest(bool Passed);
};
