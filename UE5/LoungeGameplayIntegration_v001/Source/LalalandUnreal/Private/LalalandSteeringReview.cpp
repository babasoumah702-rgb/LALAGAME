// Offline fixture on the actual Stage/NPC path. No backend session or paid API.
#include "LalalandCharacterReview.h"
#include "LalalandStage.h"
#include "LalalandNpcCharacter.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"

void StartLalalandSteeringReview(UWorld* World)
{
    World->SpawnActor<ALalalandStage>();
    auto* PC=World->GetFirstPlayerController();
    if(PC){if(APawn* Pawn=PC->GetPawn()){Pawn->SetActorHiddenInGame(true);Pawn->SetActorEnableCollision(false);Pawn->SetActorLocation(FVector(20000,20000,1000));}if(PC->GetHUD())PC->GetHUD()->SetActorHiddenInGame(true);PC->SetShowMouseCursor(false);}
    auto* Camera=World->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(58.f);if(PC)PC->SetViewTarget(Camera);
    TMap<FString,ALalalandNpcCharacter*> Actors;
    for(TActorIterator<ALalalandNpcCharacter> It(World);It;++It){Actors.Add(It->GetActorId(),*It);It->SetActorTickEnabled(false);}
    const bool Legacy=FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacySteering"));
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("CharacterSteering_v019")/(Legacy?TEXT("Baseline"):TEXT("Candidate")));
    IFileManager::Get().MakeDirectory(*Dir,true);
    const auto CSV=MakeShared<FString>(TEXT("role,phase,time,world_time,x,y,z,speed,yaw,velocity_yaw,heading_error,distance,steering_phase,max_speed,animation,pelvis_offset\n"));
    const auto Events=MakeShared<FString>(TEXT("role,event,world_time,speed_before_command\n"));
    for(int32 R=0;R<3;++R)
    {
        const FString Role=R==0?TEXT("B"):R==1?TEXT("C"):TEXT("D");auto* Npc=Actors.FindRef(Role);check(Npc);
        const float Begin=2.f+R*12.f;
        const auto Goal=MakeShared<FVector>(FVector(-500,-180,94));
        const auto ReverseOrigin=MakeShared<FVector>(FVector::ZeroVector);
        const auto Phase=MakeShared<FString>(TEXT("ready"));
        const auto SetTarget=[Npc,Actors,Goal,Phase,Events,Role](const FVector& P,const FString& Event){*Goal=P;*Phase=Event;*Events+=FString::Printf(TEXT("%s,%s,%.6f,%.3f\n"),*Role,*Event,Npc->GetWorld()->GetTimeSeconds(),Npc->GetVelocity().Size2D());FLalalandActorDto D;D.id=Npc->GetActorId();D.name=D.id;D.posture=TEXT("standing");D.x=P.X/100.;D.z=P.Y/100.;D.y=(P.Z-92.f)/100.;D.yaw=90.;Npc->ApplyState(D,Actors);};
        FTimerHandle Setup,Forward,Reverse,Arrival,Freeze;
        World->GetTimerManager().SetTimer(Setup,[Npc,Actors,SetTarget](){
            int32 I=0;for(const auto& Pair:Actors){Pair.Value->SetActorTickEnabled(false);Pair.Value->SetActorEnableCollision(false);Pair.Value->GetCharacterMovement()->StopMovementImmediately();Pair.Value->SetActorLocation(FVector(20000+I++*300,20000,94));}
            Npc->SetActorEnableCollision(true);Npc->SetActorLocation(FVector(-500,-180,94),false,nullptr,ETeleportType::TeleportPhysics);Npc->SetActorRotation(FRotator(0,90,0));SetTarget(Npc->GetActorLocation(),TEXT("ready"));Npc->SetActorTickEnabled(true);
        },Begin,false);
        World->GetTimerManager().SetTimer(Forward,[SetTarget](){SetTarget(FVector(-500,60,94),TEXT("forward"));},Begin+1.f,false);
        World->GetTimerManager().SetTimer(Reverse,[SetTarget,Npc,ReverseOrigin](){*ReverseOrigin=Npc->GetActorLocation();SetTarget(FVector(-500,-180,94),TEXT("reverse"));},Begin+2.2f,false);
        World->GetTimerManager().SetTimer(Arrival,[SetTarget,Npc](){SetTarget(Npc->GetActorLocation()+FVector(0,80,0),TEXT("arrival"));},Begin+8.f,false);
        for(int32 I=0;I<551;++I)
        {
            const float T=I*.02f;FTimerHandle Probe;
            World->GetTimerManager().SetTimer(Probe,[Npc,Camera,CSV,Goal,Phase,ReverseOrigin,Role,T,SetTarget](){
                const FVector L=Npc->GetActorLocation(),V=Npc->GetVelocity();const float Yaw=Npc->GetActorRotation().Yaw;
                const float Error=V.Size2D()>1.f?FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,V.Rotation().Yaw)):0.f;
                if(*Phase==TEXT("reverse")&&FVector::Dist2D(L,*ReverseOrigin)>30.f&&FVector::Dist2D(L,*Goal)>20.f&&V.Size2D()>60.f&&Error<20.f)
                    SetTarget(L+V.GetSafeNormal2D().RotateAngleAxis(90.f,FVector::UpVector)*140.f,TEXT("corner"));
                // Trigger on actual progress, not a clock: the baseline travels
                // faster through a corner and can already be idle at a fixed time.
                const float Remaining=FVector::Dist2D(L,*Goal);
                if(*Phase==TEXT("corner")&&Remaining<100.f&&Remaining>20.f&&V.Size2D()>60.f)SetTarget(L,TEXT("stop"));
                auto* Instance=Npc->GetMesh()->GetSingleNodeInstance();const FString Asset=Instance&&Instance->GetCurrentAsset()?Instance->GetCurrentAsset()->GetName():TEXT("MISSING");
                const float Offset=FVector::Dist2D(L,Npc->GetMesh()->GetSocketLocation(TEXT("pelvis")));
                *CSV+=FString::Printf(TEXT("%s,%s,%.3f,%.6f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.3f,%s,%.3f\n"),*Role,**Phase,T,Npc->GetWorld()->GetTimeSeconds(),L.X,L.Y,L.Z,V.Size2D(),Yaw,V.Rotation().Yaw,Error,FVector::Dist2D(L,*Goal),Npc->GetLocomotionSteeringPhase(),Npc->GetCharacterMovement()->MaxWalkSpeed,*Asset,Offset);
                const FVector Look=Npc->GetMesh()->GetSocketLocation(TEXT("head"))-FVector(0,0,65);
                Camera->SetActorLocation(Look+FVector(290,-310,60));Camera->SetActorRotation((Look-Camera->GetActorLocation()).Rotation());
            },Begin+.01f+T,false);
        }
        for(int32 I=0;I<8;++I)
        {
            const float Times[]={2.15f,2.3f,2.6f,2.95f,3.3f,4.8f,5.2f,10.8f};FTimerHandle Shot;
            World->GetTimerManager().SetTimer(Shot,[Dir,Role,I](){FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("%s_%02d.png"),*Role,I),false,false);},Begin+Times[I],false);
        }
        World->GetTimerManager().SetTimer(Freeze,[Npc](){Npc->SetActorTickEnabled(false);Npc->GetCharacterMovement()->StopMovementImmediately();},Begin+11.5f,false);
    }
    FTimerHandle Exit;World->GetTimerManager().SetTimer(Exit,[CSV,Events,Dir](){FFileHelper::SaveStringToFile(*CSV,*(Dir/TEXT("movement.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FFileHelper::SaveStringToFile(*Events,*(Dir/TEXT("events.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);},38.f,false);
}
