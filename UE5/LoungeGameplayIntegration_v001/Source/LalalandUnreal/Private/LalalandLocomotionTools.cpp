#include "LalalandLocomotionTools.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"
bool ULalalandLocomotionTools::FinalizeBlendSpace(UBlendSpace* Asset)
{
#if WITH_EDITOR
    if(!Asset || Asset->GetNumberOfBlendSamples()!=2)return false;
    for(int32 I=0;I<2;++I)
        if(!Asset->GetBlendSample(I).Animation || Asset->GetBlendSample(I).Animation->GetSkeleton()!=Asset->GetSkeleton())return false;
    Asset->ValidateSampleData();Asset->ResampleData();Asset->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
