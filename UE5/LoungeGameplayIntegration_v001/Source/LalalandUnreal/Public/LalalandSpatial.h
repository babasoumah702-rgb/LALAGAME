#pragma once
#include "LalalandVenueLayout.h"

// World centimetres read from the same staged layout used by the Node authority.
namespace LalalandSpatial
{
inline const TSharedPtr<FJsonObject> Architecture(){return LalalandVenue::Layout()->GetObjectField(TEXT("architecture"));}
inline float Roof(){return Architecture()->GetNumberField(TEXT("roofHeight"))*100.f;}
inline float ClearHeight(){return Architecture()->GetNumberField(TEXT("hallClearHeight"))*100.f;}
inline FVector ElevatorOffset(){return FVector(0,Architecture()->GetNumberField(TEXT("elevatorOffsetZ"))*100.f,0);}
inline FVector Spawn(){return LalalandVenue::Point(Architecture()->GetObjectField(TEXT("elevatorSpawn")),92);}
inline float StairBottom(){return Architecture()->GetNumberField(TEXT("stairBottomZ"))*100.f;}
}
