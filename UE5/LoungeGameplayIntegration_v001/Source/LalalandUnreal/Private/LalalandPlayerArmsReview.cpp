#include "LalalandCharacterReview.h"
#include "LalalandPlayerArmsComponent.h"
#include "LalalandGlassProp.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "SkeletalRenderPublic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void StartLalalandPlayerArmsReview(UWorld* World)
{
    // No Stage, bar map, navigation, NPCs, story service, or paid API.
    auto* PC=World->GetFirstPlayerController();
    if(PC){if(APawn* Pawn=PC->GetPawn()){Pawn->SetActorHiddenInGame(true);Pawn->SetActorEnableCollision(false);Pawn->SetActorTickEnabled(false);}PC->SetShowMouseCursor(false);}
    auto* Camera=World->SpawnActor<ACameraActor>();Camera->SetActorHiddenInGame(false);Camera->SetActorLocation(FVector(0,0,160));
    auto* Cam=Camera->GetCameraComponent();Cam->SetFieldOfView(85);Cam->PostProcessBlendWeight=1;
    auto& PP=Cam->PostProcessSettings;PP.bOverride_AutoExposureMethod=true;PP.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    PP.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP.AutoExposureApplyPhysicalCameraExposure=false;
    PP.bOverride_AutoExposureBias=true;PP.AutoExposureBias=0;PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=0;
    if(PC)PC->SetViewTarget(Camera);
    auto* Sky=World->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SourceType=SLS_SpecifiedCubemap;Sky->GetLightComponent()->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap")));Sky->GetLightComponent()->SetIntensity(1.2f);
    auto* Light=World->SpawnActor<ADirectionalLight>();Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);Light->SetActorRotation(FRotator(-40,-25,0));Light->GetLightComponent()->SetIntensity(4);Light->GetLightComponent()->SetCastShadows(false);
    // Fixture support only; the component itself never refers to this table.
    auto* Floor=World->SpawnActor<AStaticMeshActor>();Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Floor->SetActorLocation(FVector(150,0,113.21));Floor->SetActorScale3D(FVector(3,4,.08));
    auto* Arms=NewObject<ULalalandPlayerArmsComponent>(Camera);Camera->AddInstanceComponent(Arms);Arms->RegisterComponent();check(Arms->InitializeArms(Cam));
    auto* Glass=World->SpawnActor<ALalalandGlassProp>();check(Glass->BuildNativeMix(TEXT("gin_tonic_fixed"),.7f));
    const FTransform Rest(FQuat::Identity,FVector(40.08,15.03,117.21));Glass->SetActorTransform(Rest);
    const FString Version=FParse::Param(FCommandLine::Get(),TEXT("PlayerArmsV026"))?TEXT("v026"):FParse::Param(FCommandLine::Get(),TEXT("PlayerArmsV025"))?TEXT("v025"):TEXT("v024");
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/(TEXT("PlayerArms_")+Version));IFileManager::Get().MakeDirectory(*Dir,true);
    auto CSV=MakeShared<FString>(TEXT("time,action,finished,cup_x,cup_y,cup_z,rest_gap_cm\n"));
    FTimerHandle Diagnose;World->GetTimerManager().SetTimer(Diagnose,[Arms,Camera,Dir](){auto* M=Arms->GetArmsMesh();TArray<FFinalSkinVertex> V;M->GetCPUSkinnedVertices(V,0);FBox B(ForceInit);FString UV=TEXT("x,y,z,u,v\n");for(auto& Vertex:V){auto P=M->GetComponentTransform().TransformPosition(FVector(Vertex.Position));B+=P;UV+=FString::Printf(TEXT("%.5f,%.5f,%.5f,%.8f,%.8f\n"),P.X,P.Y,P.Z,Vertex.U,Vertex.V);}FFileHelper::SaveStringToFile(UV,*(Dir/TEXT("skin-uv.csv")));UE_LOG(LogTemp,Display,TEXT("PLAYER_ARMS_RENDER visible=%d hidden=%d ownerHidden=%d vertices=%d box=%s hand=%s bounds=%s extent=%s"),M->IsVisible(),M->bHiddenInGame,Camera->IsHidden(),V.Num(),*B.ToString(),*M->GetSocketLocation(TEXT("hand_r")).ToString(),*M->Bounds.Origin.ToString(),*M->Bounds.BoxExtent.ToString());},9.2f,false);
    for(int32 I=0;I<4;++I){const FString Action=I==0?TEXT("Pickup"):I==1?TEXT("Hold"):I==2?TEXT("Sip"):TEXT("Putdown");
        const float Start=4+I*4;FTimerHandle Begin;
        World->GetTimerManager().SetTimer(Begin,[Arms,Glass,Rest,Action](){check(Arms->BeginCupAction(Glass,Rest,Action));},Start,false);
        for(int32 S=0;S<2;++S){FTimerHandle Shot;World->GetTimerManager().SetTimer(Shot,[Dir,Action,S](){FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("%s_%d.png"),*Action,S),false,false);},Start+(S==0?1.35f:2.3f),false);}
    }
    FTimerHandle Probe;World->GetTimerManager().SetTimer(Probe,[World,Arms,Glass,CSV,Rest](){auto V=Glass->GetActorLocation();*CSV+=FString::Printf(TEXT("%.4f,%s,%d,%.3f,%.3f,%.3f,%.3f\n"),World->GetTimeSeconds(),*Arms->GetAction(),Arms->IsFinished(),V.X,V.Y,V.Z,FVector::Dist(V,Rest.GetLocation()));},.05f,true);
    FTimerHandle Turn,Back;World->GetTimerManager().SetTimer(Turn,[Camera](){Camera->SetActorRotation(FRotator(0,20,0));},10.5f,false);World->GetTimerManager().SetTimer(Back,[Camera](){Camera->SetActorRotation(FRotator::ZeroRotator);},11.3f,false);
    FTimerHandle Restart,Cancel;World->GetTimerManager().SetTimer(Restart,[Arms,Glass,Rest](){check(!Arms->BeginCupAction(nullptr,Rest,TEXT("Sip")));check(Arms->BeginCupAction(Glass,Rest,TEXT("Pickup")));check(!Arms->BeginCupAction(Glass,Rest,TEXT("Sip")));},19.4f,false);
    World->GetTimerManager().SetTimer(Cancel,[Arms,Glass,Rest](){Arms->CancelCupAction();check(Arms->IsFinished()&&!Arms->GetArmsMesh()->IsVisible());check(FVector::Dist(Glass->GetActorLocation(),Rest.GetLocation())<.1f);UE_LOG(LogTemp,Display,TEXT("PLAYER_ARMS_V024_CANCEL_AND_BUSY_GUARDS_PASSED"));},20.1f,false);
    FTimerHandle End;World->GetTimerManager().SetTimer(End,[Arms,Glass,Rest,Dir,CSV](){
        const float Gap=FVector::Dist(Glass->GetActorLocation(),Rest.GetLocation());FFileHelper::SaveStringToFile(*CSV,*(Dir/TEXT("sequence.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("PLAYER_ARMS_V024_SEQUENCE_COMPLETE rest_gap_cm=%.4f finished=%d"),Gap,Arms->IsFinished());check(Gap<.1f&&Arms->IsFinished());FPlatformMisc::RequestExit(false);
    },21,false);
}
