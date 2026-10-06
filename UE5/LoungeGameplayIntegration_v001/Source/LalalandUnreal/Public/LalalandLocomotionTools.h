#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LalalandLocomotionTools.generated.h"
class UBlendSpace;
UCLASS()
class LALALANDUNREAL_API ULalalandLocomotionTools final : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Editor generation only. Runtime must use serialized triangulation data.
    UFUNCTION(BlueprintCallable,Category="Lalaland|Validation")
    static bool FinalizeBlendSpace(UBlendSpace* Asset);
};
