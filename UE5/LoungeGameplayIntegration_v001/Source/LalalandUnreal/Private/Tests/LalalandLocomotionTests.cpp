#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandLocomotionBlendTest,"Lalaland.Animation.LocomotionBlend_v018",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLalalandLocomotionBlendTest::RunTest(const FString& Parameters)
{
    for(const FString Role:{TEXT("B"),TEXT("C"),TEXT("D")})
    {
        auto* Blend=LoadObject<UBlendSpace1D>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v018/Basic/BS_Locomotion_v018"),*Role));
        if(!TestNotNull(Role+TEXT(" blend asset"),Blend))continue;
        TestEqual(Role+TEXT(" sample count"),Blend->GetNumberOfBlendSamples(),2);
        for(int32 I=0;I<2;++I)
        {
            const auto& Sample=Blend->GetBlendSample(I);
            if(TestNotNull(Role+TEXT(" source clip"),Sample.Animation.Get()))
            {
                TestTrue(Role+TEXT(" compatible skeleton"),Sample.Animation->GetSkeleton()==Blend->GetSkeleton());
                TestTrue(Role+TEXT(" accepted v017 source"),Sample.Animation->GetPathName().Contains(TEXT("_v017/Basic/")));
            }
        }
        for(const float Alpha:{0.f,.25f,.5f,.75f,1.f})
        {
            TArray<FBlendSampleData> Data;int32 Cached=INDEX_NONE;
            if(!TestTrue(Role+TEXT(" serialized interpolation lookup"),Blend->GetSamplesFromBlendInput(FVector(Alpha,0,0),Data,Cached,false)))continue;
            float Sum=0,Walk=0;
            for(const auto& Sample:Data){Sum+=Sample.TotalWeight;if(Sample.SampleDataIndex==1)Walk+=Sample.TotalWeight;}
            TestTrue(Role+TEXT(" normalized weights"),FMath::IsNearlyEqual(Sum,1.f,1.e-4f));
            TestTrue(Role+TEXT(" expected walk weight"),FMath::IsNearlyEqual(Walk,Alpha,1.e-4f));
        }
    }
    return true;
}
#endif
