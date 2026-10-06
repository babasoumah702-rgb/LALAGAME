#pragma once
#include "CoreMinimal.h"

// All coordinates are viewport widget units, not physical pixels.
namespace LalalandDialogueLayout
{
inline bool Place(const FVector2D& Anchor, const FVector2D& Size, const FVector2D& Viewport,
                  const TArray<FBox2D>& Occupied, FVector2D& Out)
{
    if (Size.X <= 0 || Size.Y <= 0 || Size.X + 16 > Viewport.X) return false;
    // Prefer close to the speaker, then progressively higher rows. Never place
    // a bubble below its head just to find room over the face/body.
    for (int32 Row = 0; Row < 5; ++Row)
        for (const float Side : {0.f, -1.f, 1.f, -2.f, 2.f})
        {
            const FVector2D P(FMath::Clamp(Anchor.X - Size.X * .5 + Side * (Size.X + 12), 8., Viewport.X - Size.X - 8.),
                              Anchor.Y - Size.Y - Row * (Size.Y + 12));
            if (P.Y < 8 || P.Y + Size.Y > Viewport.Y - 8) continue;
            if (FMath::Abs(P.X + Size.X * .5 - Anchor.X) > Size.X * 1.6) continue;
            const FBox2D Box(P - FVector2D(6,6), P + Size + FVector2D(6,6));
            bool bOverlaps = false;
            for (const FBox2D& Other : Occupied) if (Box.Intersect(Other)) { bOverlaps = true; break; }
            if (!bOverlaps) { Out = P; return true; }
        }
    return false; // Prefer briefly hiding overflow to unreadable stacked text.
}
}
