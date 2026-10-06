#pragma once
#include "MixologyCatalog.h"

// Presentation only: never changes recipe matching, money or character state.
namespace MixologyPresentation {
 int32 Volume(const FMixCatalog& Catalog, const TMap<FString,int32>& Amounts);
 FLinearColor LiquidColor(const FMixCatalog& Catalog, const TMap<FString,int32>& Amounts);
 // Art-directed translucency, not measured absorption or a physical fluid solver.
 float LiquidOpacity(const FMixCatalog& Catalog, const TMap<FString,int32>& Amounts);
 float LiquidHeight(int32 Millilitres);
 FString MethodLabel(const FString& Method);
 // Align the opening over the glass; deliberately stylized, not a fluid solver.
 FTransform PourTransform(const FVector& Home,const FVector& Mouth,const FVector& Scale,float LocalOpening,float Blend);
}
