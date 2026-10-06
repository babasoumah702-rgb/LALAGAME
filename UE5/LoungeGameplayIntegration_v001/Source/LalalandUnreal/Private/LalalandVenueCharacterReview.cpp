// Actual venue geometry/lighting and NPC movement, driven by offline fixture DTOs.
// Not a backend or player-input acceptance test. No production changes here.
#include "LalalandCharacterReview.h"
#include "LalalandStage.h"
#include "LalalandNpcCharacter.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace1D.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
void Aim(ACameraActor* Camera,ALalalandNpcCharacter* Npc,bool Back=false)
{
    const FVector Head=Npc->GetMesh()->GetSocketLocation(TEXT("head"));
    const FVector Target=Head-FVector(0,0,58.f);
    Camera->SetActorLocation(Target+Npc->GetActorForwardVector()*(Back?-350.f:350.f)+FVector(0,0,25.f));
    Camera->SetActorRotation((Target-Camera->GetActorLocation()).Rotation());
}
void Target(ALalalandNpcCharacter* Npc,const FVector& Position,const TMap<FString,ALalalandNpcCharacter*>& Cast)
{
    FLalalandActorDto Dto;Dto.id=Npc->GetActorId();Dto.name=Dto.id;Dto.posture=TEXT("standing");
    Dto.x=Position.X/100.;Dto.z=Position.Y/100.;Dto.y=(Position.Z-92.f)/100.;Dto.yaw=90.;
    Npc->ApplyState(Dto,Cast);
}
FString Sample(ALalalandNpcCharacter* Npc,const FString& Phase,float T)
{
    auto* C=Npc->GetMesh();auto* Instance=C->GetSingleNodeInstance();
    const FVector Pelvis=C->GetSocketLocation(TEXT("pelvis")),Head=C->GetSocketLocation(TEXT("head"));
    const FVector Arm=C->GetSocketLocation(TEXT("lowerarm_r"))-C->GetSocketLocation(TEXT("upperarm_r"));
    const float Angle=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(-Arm.GetSafeNormal().Z,-1.f,1.f)));
    const float Offset=FVector::Dist2D(Pelvis,Npc->GetActorLocation());
    const auto& Floor=Npc->GetCharacterMovement()->CurrentFloor;
    const float Sole=FMath::Min(C->GetSocketLocation(TEXT("ball_l")).Z,C->GetSocketLocation(TEXT("ball_r")).Z)-Floor.HitResult.ImpactPoint.Z;
    const FString Asset=Instance&&Instance->GetCurrentAsset()?Instance->GetCurrentAsset()->GetName():TEXT("MISSING");
    FVector BlendInput(0),FilteredInput(0);if(Instance&&Cast<UBlendSpace1D>(Instance->GetCurrentAsset()))Instance->GetBlendSpaceState(BlendInput,FilteredInput);
    UE_LOG(LogTemp,Display,TEXT("VENUE_CHARACTER_V017 role=%s phase=%s t=%.2f speed=%.2f yaw=%.2f animation=%s pelvisOffset=%.2f headZ=%.2f soleZ=%.2f armAngle=%.2f"),*Npc->GetActorId(),*Phase,T,Npc->GetVelocity().Size2D(),Npc->GetActorRotation().Yaw,*Asset,Offset,Head.Z,Sole,Angle);
    const FVector L=Npc->GetActorLocation();
    return FString::Printf(TEXT("%s,%s,%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%s,%.3f,%.3f,%.3f,%.3f,%.6f,%.6f,%.6f\n"),*Npc->GetActorId(),*Phase,T,L.X,L.Y,L.Z,Npc->GetVelocity().Size2D(),Npc->GetActorRotation().Yaw,*Asset,Offset,Head.Z,Sole,Angle,BlendInput.X,FilteredInput.X,Npc->GetWorld()->GetTimeSeconds());
}
}

void StartLalalandVenueCharacterReview(UWorld* World)
{
    if(FParse::Param(FCommandLine::Get(),TEXT("LalalandSteeringReview"))){StartLalalandSteeringReview(World);return;}
    World->SpawnActor<ALalalandStage>();
    auto* PC=World->GetFirstPlayerController();
    if(PC){if(APawn* Pawn=PC->GetPawn()){Pawn->SetActorHiddenInGame(true);Pawn->SetActorEnableCollision(false);Pawn->SetActorLocation(FVector(20000,20000,1000));}if(PC->GetHUD())PC->GetHUD()->SetActorHiddenInGame(true);PC->SetShowMouseCursor(false);}
    auto* Camera=World->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(62.f);if(PC)PC->SetViewTarget(Camera);
    TMap<FString,ALalalandNpcCharacter*> Cast;
    for(TActorIterator<ALalalandNpcCharacter> It(World);It;++It){Cast.Add(It->GetActorId(),*It);It->SetActorTickEnabled(false);}
    const bool ReviewBlend=FParse::Param(FCommandLine::Get(),TEXT("LalalandLocomotionReview"));
    const FString Variant=FParse::Param(FCommandLine::Get(),TEXT("LalalandVenueAnchorCandidate"))?TEXT("AnchorCandidate"):FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacyBasicMotion"))?TEXT("BaselineWide"):TEXT("PromotedIsolated");
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/(ReviewBlend?TEXT("CharacterLocomotion_v018"):TEXT("CharacterVenue_v017"))/Variant);IFileManager::Get().MakeDirectory(*Dir,true);
    const auto CSV=MakeShared<FString>(TEXT("role,phase,time,x,y,z,speed,yaw,animation,pelvis_horizontal_offset,head_z,ball_height_above_floor,upperarm_angle,blend_input,blend_filtered,world_time\n"));
    for(int32 Index=0;Index<6;++Index)
    {
        const FString Role=Index/2==0?TEXT("B"):Index/2==1?TEXT("C"):TEXT("D");const bool Back=Index%2!=0;
        auto* Npc=Cast.FindRef(Role);check(Npc);
        FTimerHandle Setup,Shot;
        World->GetTimerManager().SetTimer(Setup,[Camera,Npc,Back,CSV]() {Aim(Camera,Npc,Back);*CSV+=Sample(Npc,Back?TEXT("rest_back"):TEXT("rest_front"),0);},2.f+Index*2.5f,false);
        World->GetTimerManager().SetTimer(Shot,[Dir,Role,Back](){FScreenshotRequest::RequestScreenshot(Dir/(Role+(Back?TEXT("_rest_back.png"):TEXT("_rest_front.png"))),false,false);},3.5f+Index*2.5f,false);
    }
    for(int32 R=0;R<3;++R)
    {
        const FString Role=R==0?TEXT("B"):R==1?TEXT("C"):TEXT("D");auto* Npc=Cast.FindRef(Role);
        const float Begin=18.f+R*9.f;
        const FVector Start(-500.f,-180.f,94.f),End(-500.f,60.f,94.f);
        FTimerHandle Setup,Move,Turn,Stop;
        World->GetTimerManager().SetTimer(Setup,[Npc,Cast,Start,Camera](){
            // Separate locomotion from fixture-created crowd congestion.
            // Other NPCs retain their original positions for the first six photos.
            int32 OtherIndex=0;
            for(const auto& Pair:Cast)if(Pair.Value!=Npc){Pair.Value->SetActorTickEnabled(false);Pair.Value->SetActorEnableCollision(false);Pair.Value->GetCharacterMovement()->StopMovementImmediately();Pair.Value->SetActorLocation(FVector(20000.f+OtherIndex++*300.f,20000.f,94.f));}
            Npc->SetActorEnableCollision(true);Npc->SetActorLocation(Start,false,nullptr,ETeleportType::TeleportPhysics);
            Target(Npc,Start,Cast);Npc->SetActorTickEnabled(true);Aim(Camera,Npc);
        },Begin,false);
        World->GetTimerManager().SetTimer(Move,[Npc,Cast,End](){Target(Npc,End,Cast);},Begin+1.f,false);
        World->GetTimerManager().SetTimer(Turn,[Npc,Cast,Start](){Target(Npc,Start,Cast);},Begin+3.5f,false);
        // Stop by a target at the reached location, through the real ApplyState path.
        World->GetTimerManager().SetTimer(Stop,[Npc,Cast](){Target(Npc,Npc->GetActorLocation(),Cast);},Begin+6.f,false);
        for(int32 I=0;I<33;++I)
        {
            const float Time=I*.25f;FTimerHandle Probe;
            const FString Phase=Time<1?TEXT("ready"):Time<3.5?TEXT("forward"):Time<6?TEXT("return"):TEXT("stop");
            World->GetTimerManager().SetTimer(Probe,[Npc,Camera,CSV,Phase,Time](){Aim(Camera,Npc);*CSV+=Sample(Npc,Phase,Time);},Begin+Time+.1f,false);
        }
        for(int32 I=0;I<3;++I)
        {
            FTimerHandle Shot;const FString Label=I==0?TEXT("walk"):I==1?TEXT("turn"):TEXT("stop");
            World->GetTimerManager().SetTimer(Shot,[Dir,Role,Label](){FScreenshotRequest::RequestScreenshot(Dir/(Role+TEXT("_")+Label+TEXT(".png")),false,false);},Begin+(I==0?2.f:I==1?3.8f:7.f),false);
        }
        FTimerHandle Freeze;World->GetTimerManager().SetTimer(Freeze,[Npc](){Npc->SetActorTickEnabled(false);Npc->GetCharacterMovement()->StopMovementImmediately();},Begin+8.5f,false);
        if(ReviewBlend)
        {
            for(int32 I=0;I<100;++I)
            {
                const float Time=I*.02f;FTimerHandle Probe;
                World->GetTimerManager().SetTimer(Probe,[Npc,CSV,Time](){*CSV+=Sample(Npc,TEXT("blend_start_dense"),Time);},Begin+.8f+Time,false);
            }
            for(int32 I=0;I<40;++I)
            {
                const float Time=I*.02f;FTimerHandle Probe;
                World->GetTimerManager().SetTimer(Probe,[Npc,CSV,Time](){*CSV+=Sample(Npc,TEXT("blend_stop_dense"),Time);},Begin+5.9f+Time,false);
            }
        }
    }
    if(ReviewBlend)for(int32 R=0;R<3;++R)
    {
        const FString Role=R==0?TEXT("B"):R==1?TEXT("C"):TEXT("D");auto* Npc=Cast.FindRef(Role);
        const FVector Place(-500.f,-180.f+R*80.f,94.f);FTimerHandle Quiet;
        World->GetTimerManager().SetTimer(Quiet,[Npc,Place,Cast](){Npc->SetActorEnableCollision(true);Npc->SetActorLocation(Place,false,nullptr,ETeleportType::TeleportPhysics);Target(Npc,Place,Cast);Npc->SetActorTickEnabled(true);},46.f,false);
        for(int32 I=0;I<22;++I){FTimerHandle Probe;World->GetTimerManager().SetTimer(Probe,[Npc,CSV,I](){*CSV+=Sample(Npc,TEXT("quiet_idle"),I);},46.5f+I,false);}
    }
    FTimerHandle Exit;World->GetTimerManager().SetTimer(Exit,[CSV,Dir](){FFileHelper::SaveStringToFile(*CSV,*(Dir/TEXT("movement.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);},ReviewBlend?69.f:46.f,false);
}
