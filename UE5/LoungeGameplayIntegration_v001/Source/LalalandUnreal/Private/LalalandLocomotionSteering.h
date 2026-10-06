#pragma once
#include "CoreMinimal.h"

// Heading/arrival coordination, not foot locking or a turn-in-place animation.
namespace LalalandLocomotion
{
enum class EPhase : uint8 { Travel, BrakeForTurn, AlignForTravel, Arrive };
inline float WalkingPlayRate(float Speed) { return FMath::Clamp(Speed/135.f,.05f,1.4f); }
struct FSteering
{
    bool bTurnGate=false;
    EPhase Phase=EPhase::Arrive;
    float SpeedLimit=160.f;
    float FacingYaw=0.f;
    bool bMove=false;

    void Update(float Distance,float TargetYaw,float IdleYaw,float CurrentYaw,const FVector& Velocity,float Dt)
    {
        const float Speed=Velocity.Size2D();
        const float Error=FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw,TargetYaw));
        const bool HasTarget=Distance>10.f;
        bMove=false;SpeedLimit=0.f;
        if(!HasTarget)
        {
            bTurnGate=false;Phase=EPhase::Arrive;
            // Finish braking before rotating to a conversation/idle heading.
            const float Goal=Speed>12.f?Velocity.Rotation().Yaw:IdleYaw;
            FacingYaw=FMath::FixedTurn(CurrentYaw,Goal,180.f*FMath::Max(0.f,Dt));
            return;
        }
        if(Error>70.f)bTurnGate=true;
        if(bTurnGate)
        {
            if(Speed>12.f)
            {
                Phase=EPhase::BrakeForTurn;
                FacingYaw=FMath::FixedTurn(CurrentYaw,Velocity.Rotation().Yaw,180.f*FMath::Max(0.f,Dt));
                return;
            }
            Phase=EPhase::AlignForTravel;
            FacingYaw=FMath::FixedTurn(CurrentYaw,TargetYaw,180.f*FMath::Max(0.f,Dt));
            if(Error>15.f)return;
            bTurnGate=false;
        }
        Phase=EPhase::Travel;
        FacingYaw=FMath::FixedTurn(CurrentYaw,TargetYaw,180.f*FMath::Max(0.f,Dt));
        // Continuous small-corner slowdown and an 80 cm arrival envelope.
        const float Corner=FMath::Max(0.f,FMath::Cos(FMath::DegreesToRadians(Error)));
        const float Arrival=FMath::Clamp((Distance-10.f)/80.f,.12f,1.f);
        SpeedLimit=160.f*FMath::Min(Corner,Arrival);
        bMove=true;
    }
};
}
