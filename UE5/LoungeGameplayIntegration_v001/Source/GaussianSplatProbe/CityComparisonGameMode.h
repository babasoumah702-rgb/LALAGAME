#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CityComparisonGameMode.generated.h"

// Visual-only comparison. Does not replace the playable game or walking probe.
UCLASS()
class ACityComparisonGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACityComparisonGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void SetView(int32 Index);
    void Finish();
    double StartedAt=0;
    int32 ViewIndex=0;
    bool Automated=false;
    bool Terrace=false;
    bool InteriorMerge=false;
    bool Captured=false;
    TArray<TSharedPtr<class FJsonValue>> Captures;
};
