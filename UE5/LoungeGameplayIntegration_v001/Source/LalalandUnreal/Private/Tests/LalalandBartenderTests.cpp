#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandBartenderAssetsTest,"Lalaland.Animation.Bartender_v023",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLalalandBartenderAssetsTest::RunTest(const FString& Parameters)
{
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/BARTENDER_v022/SK_Bartender_v022"));
    auto* Kiko=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/A_v021/SK_A_v021"));
    auto* KikoOriginal=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/A/SK_A"));
    TestNotNull(TEXT("Returned Kiko mesh"),Kiko);
    if(Kiko&&TestNotNull(TEXT("Kiko original mesh"),KikoOriginal))TestTrue(TEXT("Kiko original animation skeleton retained"),Kiko->GetSkeleton()==KikoOriginal->GetSkeleton());
    if(!TestNotNull(TEXT("Returned bartender mesh"),Mesh))return false;
    for(const FString Name:{TEXT("Basic/LL_Idle_v023"),TEXT("Basic/LL_Walk_v023"),TEXT("LL_HostCall_v023")})
    {
        auto* Clip=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/Characters/BARTENDER_v023/")+Name));
        if(TestNotNull(Name,Clip))
        {
            TestTrue(Name+TEXT(" own skeleton"),Clip->GetSkeleton()==Mesh->GetSkeleton());
            TestTrue(Name+TEXT(" nonempty"),Clip->GetPlayLength()>.1f);
            TestFalse(Name+TEXT(" no double root movement"),Clip->bEnableRootMotion);
        }
    }
    auto* Blend=LoadObject<UBlendSpace1D>(nullptr,TEXT("/Game/Characters/BARTENDER_v023/Basic/BS_Locomotion_v023"));
    if(!TestNotNull(TEXT("Bartender blend"),Blend))return false;
    TestEqual(TEXT("Idle walk samples"),Blend->GetNumberOfBlendSamples(),2);
    TestTrue(TEXT("Blend own skeleton"),Blend->GetSkeleton()==Mesh->GetSkeleton());
    for(const float Alpha:{0.f,.25f,.5f,.75f,1.f})
    {
        TArray<FBlendSampleData> Data;int32 Cache=INDEX_NONE;
        if(!TestTrue(TEXT("Blend serialized lookup"),Blend->GetSamplesFromBlendInput(FVector(Alpha,0,0),Data,Cache,false)))continue;
        float Sum=0,Walk=0;
        for(const auto& S:Data){Sum+=S.TotalWeight;if(S.SampleDataIndex==1)Walk+=S.TotalWeight;}
        TestTrue(TEXT("Normalized"),FMath::IsNearlyEqual(Sum,1.f,1.e-4f));
        TestTrue(TEXT("Walk weight"),FMath::IsNearlyEqual(Walk,Alpha,1.e-4f));
    }
    return true;
}
#endif
