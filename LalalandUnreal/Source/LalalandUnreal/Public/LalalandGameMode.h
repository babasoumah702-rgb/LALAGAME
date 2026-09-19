#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LalalandGameMode.generated.h"

UCLASS()
class LALALANDUNREAL_API ALalalandGameMode final : public AGameModeBase
{
    GENERATED_BODY()
public:
    ALalalandGameMode();
    virtual void BeginPlay() override;
};
