// Opt-in physical contact preview, not a backend or player-input acceptance test.
#include "LalalandCharacterReview.h"
#include "LalalandStage.h"
#include "LalalandNpcCharacter.h"
#include "LalalandGlassProp.h"
#include "MixologyGameMode.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
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

void StartLalalandCupReview(UWorld* World)
{
    auto* Stage=World->SpawnActor<ALalalandStage>();Stage->SetActorTickEnabled(false);
    // Remove the separate mixing station only in this fixture to expose contacts.
    for(TActorIterator<AMixologyStation> It(World);It;++It){It->SetActorHiddenInGame(true);It->SetActorTickEnabled(false);TArray<AActor*> Children;It->GetAttachedActors(Children,true,true);for(auto* Child:Children)Child->SetActorHiddenInGame(true);}
    auto* PC=World->GetFirstPlayerController();
    if(PC){if(APawn* Pawn=PC->GetPawn()){Pawn->SetActorHiddenInGame(true);Pawn->SetActorEnableCollision(false);Pawn->SetActorLocation(FVector(20000,20000,1000));}if(PC->GetHUD())PC->GetHUD()->SetActorHiddenInGame(true);PC->SetShowMouseCursor(false);}
    auto* Camera=World->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(45.f);if(PC)PC->SetViewTarget(Camera);
    ALalalandNpcCharacter* Npc=nullptr;
    for(TActorIterator<ALalalandNpcCharacter> It(World);It;++It){It->SetActorTickEnabled(false);It->SetActorEnableCollision(false);It->GetCharacterMovement()->StopMovementImmediately();if(It->GetActorId()==TEXT("C"))Npc=*It;else It->SetActorHiddenInGame(true);}
    check(Npc);Npc->SetActorLocation(FVector(-460,362,92),false,nullptr,ETeleportType::TeleportPhysics);Npc->SetActorRotation(FRotator(0,90,0));
    // Constants exported by Prepare-Cup-v020.py, normalized source skeleton units.
    const FTransform Grip(FQuat(-.163793325993,.012455356352,-.986415404470,.001122623436),FVector(-.039695883749,-.057109438364,-.030182881568));
    const FVector Reach(-.085808960776,.262886226092,.649682772704);
    const FVector Mouth(-.019174830242,.085195211335,.823918193844);
    const FVector Rest=Npc->GetMesh()->GetComponentTransform().TransformPosition(Reach);
    auto* Glass=World->SpawnActor<ALalalandGlassProp>();
    check(Glass->BuildNativeMix(TEXT("gin_tonic_fixed"),.7f));
    Glass->SetActorLocation(Rest);
    const bool Side=FParse::Param(FCommandLine::Get(),TEXT("LalalandCupSideReview"));
    const FVector CameraOffset(Side?-105:105,105,18);
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("CharacterCup_v020")/(Side?TEXT("Side"):TEXT("Front")));IFileManager::Get().MakeDirectory(*Dir,true);
    const auto CSV=MakeShared<FString>(TEXT("action,fraction,world_time,cup_x,cup_y,cup_z,rim_mouth_gap_cm,rest_gap_cm,hand_cup_gap_cm\n"));
    for(int32 A=0;A<4;++A)
    {
        const FString Action=A==0?TEXT("Pickup"):A==1?TEXT("Hold"):A==2?TEXT("Sip"):TEXT("Putdown");const float Duration=A==0?2.4f:A==1?1.2f:A==2?2.8f:2.6f;const float Start=2.f+A*4.f;
        auto* Seq=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/Characters/C_v020/Cup/LL_Cup_")+Action+TEXT("_v020.LL_Cup_")+Action+TEXT("_v020")));check(Seq);
        FTimerHandle Begin;World->GetTimerManager().SetTimer(Begin,[Npc,Seq,World](){
            // HUD creates its station after GameMode BeginPlay, so hide it here.
            for(TActorIterator<AMixologyStation> It(World);It;++It){It->SetActorHiddenInGame(true);It->SetActorTickEnabled(false);TArray<AActor*> Children;It->GetAttachedActors(Children,true,true);for(auto* Child:Children)Child->SetActorHiddenInGame(true);}
            Npc->GetMesh()->PlayAnimation(Seq,false);Npc->GetMesh()->GetSingleNodeInstance()->SetPlaying(false);
        },Start,false);
        for(int32 I=0;I<=90;++I)
        {
            const float T=I/90.f;FTimerHandle Probe;
            World->GetTimerManager().SetTimer(Probe,[Npc,Glass,Camera,CameraOffset,Grip,Reach,Mouth,Rest,Seq,Duration,Action,CSV,T,World](){
                auto* Mesh=Npc->GetMesh();auto* Instance=Mesh->GetSingleNodeInstance();Instance->SetPosition(Duration*T,false);Mesh->TickAnimation(0.f,false);Mesh->RefreshBoneTransforms();
                const FTransform HandCup=Grip*Mesh->GetSocketTransform(TEXT("hand_r"));
                const bool Attached=!(Action==TEXT("Pickup")&&T<.45f)&&!(Action==TEXT("Putdown")&&T>=.58f);
                if(Attached){Glass->SetActorLocationAndRotation(HandCup.GetLocation(),HandCup.GetRotation());Glass->SetActorScale3D(FVector(1));}else{Glass->SetActorLocationAndRotation(Rest,FQuat::Identity);}
                const FVector Rim=Glass->GetActorTransform().TransformPosition(FVector(0,-3.813f,13.95f));const FVector M=Mesh->GetComponentTransform().TransformPosition(Mouth);
                const FVector Bottom=Glass->GetActorLocation();
                *CSV+=FString::Printf(TEXT("%s,%.6f,%.6f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n"),*Action,T,World->GetTimeSeconds(),Bottom.X,Bottom.Y,Bottom.Z,FVector::Dist(Rim,M),FVector::Dist(Bottom,Rest),FVector::Dist(Bottom,HandCup.GetLocation()));
                const FVector Look=Action==TEXT("Sip")?(Rim+M)*.5f:Bottom+FVector(0,0,8);
                Camera->SetActorLocation(Look+CameraOffset);Camera->SetActorRotation((Look-Camera->GetActorLocation()).Rotation());
            },Start+.05f+Duration*T,false);
        }
        for(int32 S=0;S<3;++S){const float Times[]={.28f,.5f,.92f};FTimerHandle Shot;World->GetTimerManager().SetTimer(Shot,[Dir,Action,S](){FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("%s_%d.png"),*Action,S),false,false);},Start+.09f+Duration*Times[S],false);}
    }
    FTimerHandle Exit;World->GetTimerManager().SetTimer(Exit,[CSV,Dir](){FFileHelper::SaveStringToFile(*CSV,*(Dir/TEXT("contact.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);FPlatformMisc::RequestExit(false);},18.f,false);
}
