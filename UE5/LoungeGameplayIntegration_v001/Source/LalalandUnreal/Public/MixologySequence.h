#pragma once
#include "CoreMinimal.h"

enum class EMixStage : uint8 { Mixing, Uncapping, Serving, Finishing, Ready };
struct FMixSequence {
 bool Shake=false, Stir=false;
 float Start=0;
 void Begin(const FString& Method,float Now);
 float Duration() const;
 EMixStage Stage(float Now) const;
 float StageProgress(float Now) const;
 float FillProgress(float Now) const;
 FString Label(float Now) const;
};
