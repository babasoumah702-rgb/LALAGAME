#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LalalandLocomotionSteering.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandSteeringTest,"Lalaland.Animation.Steering_v019",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLalalandSteeringTest::RunTest(const FString& Parameters)
{
    using namespace LalalandLocomotion;
    FSteering S;
    S.Update(200,180,0,0,FVector(160,0,0),1.f/60);
    TestTrue(TEXT("reversal brakes before sideways travel"),!S.bMove&&S.bTurnGate&&S.Phase==EPhase::BrakeForTurn);
    TestTrue(TEXT("braking faces current travel direction"),FMath::Abs(S.FacingYaw)<.001f);
    S.Update(200,180,0,0,FVector::ZeroVector,1.f/60);
    TestTrue(TEXT("stationary alignment gates translation"),!S.bMove&&S.Phase==EPhase::AlignForTravel);
    TestTrue(TEXT("yaw rate bounded"),FMath::Abs(FMath::FindDeltaAngleDegrees(0.f,S.FacingYaw))<=3.001f);
    S.Update(200,180,0,170,FVector::ZeroVector,1.f/60);
    TestTrue(TEXT("alignment hysteresis releases"),S.bMove&&!S.bTurnGate);
    S.Update(200,45,0,0,FVector(160,0,0),1.f/60);
    TestTrue(TEXT("small corner reduces cruise speed"),S.bMove&&S.SpeedLimit>100.f&&S.SpeedLimit<120.f);
    S.Update(20,0,90,0,FVector(160,0,0),1.f/60);
    TestTrue(TEXT("arrival slows before target radius"),S.bMove&&S.SpeedLimit<25.f);
    S.Update(5,0,90,0,FVector(100,0,0),1.f/60);
    TestTrue(TEXT("arrival removes movement input and preserves braking heading"),!S.bMove&&S.Phase==EPhase::Arrive&&FMath::Abs(S.FacingYaw)<.001f);
    S.Update(200,-179,0,179,FVector(0,0,0),1.f/60);
    TestTrue(TEXT("wraparound is short turn"),S.bMove&&!S.bTurnGate);
    S.Update(200,180,0,0,FVector::ZeroVector,0);
    TestTrue(TEXT("zero delta does not rotate"),FMath::Abs(S.FacingYaw)<.001f);
    TestTrue(TEXT("slow arrival no longer hits old 0.35 rate floor"),FMath::IsNearlyEqual(WalkingPlayRate(19.2f),19.2f/135.f));
    TestTrue(TEXT("play rate is finite at standstill"),WalkingPlayRate(0.f)>.0f);
    return true;
}
#endif
