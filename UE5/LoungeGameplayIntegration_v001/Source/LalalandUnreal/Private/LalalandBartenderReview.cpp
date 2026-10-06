// Future-lounge, offline actor/geometry rehearsal; not authoritative story progression.
#include "LalalandCharacterReview.h"
#include "LalalandStage.h"
#include "LalalandNpcCharacter.h"
#include "LalalandVenueLayout.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "SkeletalRenderPublic.h"

namespace {
void ProbeSole(ALalalandNpcCharacter* Host,const TCHAR* Phase) {
    TArray<FFinalSkinVertex> Vertices;Host->GetMesh()->GetCPUSkinnedVertices(Vertices,0);
    float Lowest=MAX_flt;for(const auto& V:Vertices)Lowest=FMath::Min(Lowest,Host->GetMesh()->GetComponentTransform().TransformPosition(FVector(V.Position)).Z);
    UE_LOG(LogTemp,Display,TEXT("BARTENDER_V023_SOLE phase=%s measured_world_z=%.4f vertices=%d"),Phase,Lowest,Vertices.Num());
}
void HostTarget(ALalalandNpcCharacter* Npc,FVector Point,float Yaw,const TMap<FString,ALalalandNpcCharacter*>& Cast) {
    FLalalandActorDto D;D.id=Npc->GetActorId();D.name=TEXT("调酒师");D.posture=TEXT("standing");
    D.x=Point.X/100.;D.z=Point.Y/100.;D.y=(Point.Z-92)/100.;D.yaw=90-Yaw;
    Npc->ApplyState(D,Cast);
}
void HostCamera(ACameraActor* C,ALalalandNpcCharacter* Npc) {
    const FVector Focus=Npc->GetActorLocation()+FVector(0,0,-10);
    C->SetActorLocation(Focus+FVector(-260,-300,95));C->SetActorRotation((Focus-C->GetActorLocation()).Rotation());
}
struct FHostReview {
    int32 Step=0;float Entered=0;float ReachedAt=-1;bool Completed=false;bool WalkShot=false;
    FString CSV=TEXT("world_time,phase,x,y,z,speed,yaw,pelvis_offset,animation\n");
    TArray<FVector> Points;TArray<FString> Phases;
};
}

void StartLalalandBartenderReview(UWorld* World) {
    World->SpawnActor<ALalalandStage>();
    auto* PC=World->GetFirstPlayerController();
    if(PC){if(APawn* P=PC->GetPawn()){P->SetActorHiddenInGame(true);P->SetActorEnableCollision(false);P->SetActorLocation(FVector(20000,20000,1000));}if(PC->GetHUD())PC->GetHUD()->SetActorHiddenInGame(true);PC->SetShowMouseCursor(false);}
    auto* Camera=World->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(60);
    Camera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount=true;
    Camera->GetCameraComponent()->PostProcessSettings.MotionBlurAmount=0;
    Camera->GetCameraComponent()->PostProcessBlendWeight=1;if(PC)PC->SetViewTarget(Camera);
    TMap<FString,ALalalandNpcCharacter*> Cast;
    for(TActorIterator<ALalalandNpcCharacter> It(World);It;++It)Cast.Add(It->GetActorId(),*It);
    auto* Host=Cast.FindRef(TEXT("BARTENDER"));check(Host);
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("BartenderHost_v023"));IFileManager::Get().MakeDirectory(*Dir,true);
    auto State=MakeShared<FHostReview>();
    const FVector Home=LalalandVenue::Named(TEXT("bartender"),92);
    const auto Bar=LalalandVenue::Layout()->GetObjectField(TEXT("barSize"));
    const FVector BarCenter=LalalandVenue::Named(TEXT("barOffset"))+FVector(-220,55,0);
    const float ExitX=BarCenter.X-Bar->GetNumberField(TEXT("width"))*50-80;
    const float FrontY=BarCenter.Y-Bar->GetNumberField(TEXT("depth"))*50-110;
    // The U-shaped counter has real side returns (Y=465..825), so exiting
    // directly sideways intersects its collider. Use the rear staff passage.
    const FVector Inner(-505,Home.Y,92),RearInner(-505,930,92),RearOuter(ExitX,930,92);
    const FVector Clear(ExitX,FrontY,92),Call(ExitX+130,FrontY-140,92);
    State->Points={Home,Inner,RearInner,RearOuter,Clear,Call,Clear,RearOuter,RearInner,Inner,Home};
    State->Phases={TEXT("idle"),TEXT("inner_aisle"),TEXT("rear_inner"),TEXT("rear_exit"),TEXT("clear_bar"),TEXT("host_call"),TEXT("return_front"),TEXT("return_rear"),TEXT("return_inner"),TEXT("return_service"),TEXT("home")};
    HostTarget(Host,Home,-90,Cast);State->Entered=World->GetTimeSeconds();HostCamera(Camera,Host);
    FTimerHandle SoleProbe;World->GetTimerManager().SetTimer(SoleProbe,[Host](){ProbeSole(Host,TEXT("idle"));},2.4f,false);
    // Evidence includes accepted Kiko in the actual futuristic venue, not the returned wood-floor scene.
    if(auto* Kiko=Cast.FindRef(TEXT("A"))){FTimerHandle Aim,Shot;World->GetTimerManager().SetTimer(Aim,[Camera,Kiko](){const FVector Focus=Kiko->GetActorLocation();Camera->SetActorLocation(Focus+FVector(290,-230,50));Camera->SetActorRotation((Focus-Camera->GetActorLocation()).Rotation());},.6f,false);World->GetTimerManager().SetTimer(Shot,[Dir](){FScreenshotRequest::RequestScreenshot(Dir/TEXT("00_Kiko_FutureLounge.png"),false,false);},1.5f,false);}
    FTimerHandle Probe;
    World->GetTimerManager().SetTimer(Probe,[World,Host,Camera,Cast,State,Dir]() {
        const float Now=World->GetTimeSeconds();if(Now<2.f||State->Completed)return;
        HostCamera(Camera,Host);
        auto* Instance=Host->GetMesh()->GetSingleNodeInstance();
        const FString Animation=Instance&&Instance->GetCurrentAsset()?Instance->GetCurrentAsset()->GetName():TEXT("MISSING");
        const FVector P=Host->GetActorLocation();
        if(State->Step==5 && Host->GetVelocity().Size2D()>120 && !State->WalkShot){State->WalkShot=true;ProbeSole(Host,TEXT("walk"));FScreenshotRequest::RequestScreenshot(Dir/TEXT("05b_walking.png"),false,false);}
        State->CSV+=FString::Printf(TEXT("%.4f,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%s\n"),Now,*State->Phases[State->Step],P.X,P.Y,P.Z,Host->GetVelocity().Size2D(),Host->GetActorRotation().Yaw,FVector::Dist2D(Host->GetMesh()->GetSocketLocation(TEXT("pelvis")),P),*Animation);
        const bool At=FVector::Dist2D(P,State->Points[State->Step])<11&&Host->GetVelocity().Size2D()<6;
        if(At&&State->ReachedAt<0) {
            State->ReachedAt=Now;
            if(State->Step==5){Host->TriggerGesture(TEXT("host_call"));Host->ShowDialogue(TEXT("下一局要开始了。想参加的来桌边，不参加也可以继续聊。"),4);FTimerHandle CallShot;World->GetTimerManager().SetTimer(CallShot,[Dir,Host](){ProbeSole(Host,TEXT("host_call"));FScreenshotRequest::RequestScreenshot(Dir/TEXT("06b_host_invitation.png"),false,false);},1.3f,false);}
        }
        const float Hold=State->Step==0?3.f:State->Step==5?4.f:State->Step==State->Points.Num()-1?2.f:.3f;
        if(At&&State->ReachedAt>=0&&Now-State->ReachedAt>Hold) {
            FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("%02d_%s.png"),State->Step+1,*State->Phases[State->Step]),false,false);
            if(State->Step==State->Points.Num()-1){State->Completed=true;FFileHelper::SaveStringToFile(State->CSV,*(Dir/TEXT("movement.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);UE_LOG(LogTemp,Display,TEXT("BARTENDER_HOST_V023_SUCCESS returned through eleven real movement goals; no story/backend confirmation"));FTimerHandle End;World->GetTimerManager().SetTimer(End,[](){FPlatformMisc::RequestExit(false);},.7f,false);Host->SetActorTickEnabled(false);return;}
            State->Step++;State->Entered=Now;State->ReachedAt=-1;HostTarget(Host,State->Points[State->Step],State->Step==5?-90:State->Step==State->Points.Num()-1?-90:0,Cast);
        }
        if(Now>90&&!State->Completed){FFileHelper::SaveStringToFile(State->CSV,*(Dir/TEXT("movement.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);UE_LOG(LogTemp,Error,TEXT("BARTENDER_HOST_V023_FAILED timeout step=%d distance=%.2f"),State->Step,FVector::Dist2D(P,State->Points[State->Step]));FPlatformMisc::RequestExit(false);}
    },.1f,true);
}
