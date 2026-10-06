#include "CityComparisonGameMode.h"
#include "SplatProbeGameMode.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace {
const TCHAR* LoungeNames[]={TEXT("01_Hall"),TEXT("02_RightGlass"),TEXT("03_LeftRibs"),TEXT("04_WindowClose"),TEXT("05_WindowDown"),TEXT("06_Canopy")};
const FVector LoungePositions[]={FVector(-180,-660,165),FVector(350,-150,165),FVector(-620,-80,165),FVector(1200,-100,165),FVector(1200,-100,165),FVector(200,-60,165)};
const FRotator LoungeRotations[]={FRotator(6,70,0),FRotator(0,0,0),FRotator(0,180,0),FRotator(0,0,0),FRotator(-25,0,0),FRotator(42,167,0)};
const TCHAR* TerraceNames[]={TEXT("01_Entrance"),TEXT("02_RightCity"),TEXT("03_LeftCity"),TEXT("04_RailClose"),TEXT("05_LookDown"),TEXT("06_Sky")};
const FVector TerracePositions[]={FVector(0,-300,858),FVector(600,-100,858),FVector(-600,0,858),FVector(800,-100,858),FVector(800,-100,858),FVector(800,-100,858)};
const FRotator TerraceRotations[]={FRotator(0,70,0),FRotator(0,0,0),FRotator(0,180,0),FRotator(0,0,0),FRotator(-25,0,0),FRotator(65,0,0)};
const TCHAR* MergeNames[]={TEXT("01_Hall"),TEXT("02_Window"),TEXT("03_PillarCrown"),TEXT("04_StairBelow"),TEXT("05_Opening"),TEXT("06_CounterFront"),TEXT("07_WorkSide"),TEXT("08_LeftSeating")};
const FVector MergePositions[]={FVector(-180,-660,165),FVector(350,-150,165),FVector(-480,-400,165),FVector(-190,-160,165),FVector(-950,-240,560),FVector(0,120,165),FVector(0,555,165),FVector(-620,-80,165)};
const FVector MergeTargets[]={FVector(0,250,260),FVector(3000,-150,165),FVector(-790,-620,600),FVector(-650,360,470),FVector(-650,190,615),FVector(0,520,160),FVector(0,390,100),FVector(-1100,300,80)};
}
ACityComparisonGameMode::ACityComparisonGameMode() {
    DefaultPawnClass=ASplatProbePawn::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}
void ACityComparisonGameMode::BeginPlay() {
    Super::BeginPlay();
    Terrace=GetWorld()->GetMapName().Contains(TEXT("TerraceCity"));
    InteriorMerge=FParse::Param(FCommandLine::Get(),TEXT("InteriorMergeProbe"));
    Automated=FParse::Param(FCommandLine::Get(),TEXT("CityCompareAuto"));
    StartedAt=FPlatformTime::Seconds();
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0)) {
        PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
        if(APawn* Pawn=PC->GetPawn()) {
            if(Automated) Pawn->DisableInput(PC);
            if(auto* C=Pawn->FindComponentByClass<UCameraComponent>()) {
                C->FieldOfView=80;
                C->PostProcessSettings.bOverride_AutoExposureBias=true;
                C->PostProcessSettings.AutoExposureBias=-1.5;
            }
        }
    }
    SetView(0);
}
void ACityComparisonGameMode::SetView(int32 Index) {
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0)) {
        const FVector Position=InteriorMerge?MergePositions[Index]:(Terrace?TerracePositions[Index]:LoungePositions[Index]);
        if(APawn* Pawn=PC->GetPawn()) Pawn->SetActorLocation(Position);
        PC->SetControlRotation(InteriorMerge?(MergeTargets[Index]-Position).Rotation():(Terrace?TerraceRotations[Index]:LoungeRotations[Index]));
    }
}
void ACityComparisonGameMode::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);if(!Automated)return;
    const double T=FPlatformTime::Seconds()-StartedAt;
    // Capture after shader/texture settling, and do not mutate the view until
    // three seconds after the deferred screenshot request.
    const double CaptureAt=18+ViewIndex*8;
    if(!Captured&&T>CaptureAt) {
        const FString Name=InteriorMerge?MergeNames[ViewIndex]:(Terrace?TerraceNames[ViewIndex]:LoungeNames[ViewIndex]);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(Name+TEXT(".png")),false,false);
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);
        if(auto* PC=UGameplayStatics::GetPlayerController(this,0)) {
            if(APawn* Pawn=PC->GetPawn())R->SetStringField(TEXT("eye_cm"),Pawn->GetActorLocation().ToString());
            R->SetStringField(TEXT("rotation"),PC->GetControlRotation().ToString());
        }
        R->SetNumberField(TEXT("capture_seconds"),T);Captures.Add(MakeShared<FJsonValueObject>(R));Captured=true;
    } else if(Captured&&T>CaptureAt+3) {
        ++ViewIndex;if(ViewIndex==(InteriorMerge?8:6)){Finish();return;}
        SetView(ViewIndex);Captured=false;
    }
}
void ACityComparisonGameMode::Finish() {
    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("map"),GetWorld()->GetMapName());
    R->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    R->SetNumberField(TEXT("eye_height_above_floor_cm"),Terrace?158:165);
    R->SetNumberField(TEXT("fov"),80);R->SetNumberField(TEXT("exposure_bias"),-1.5);
    R->SetBoolField(TEXT("visual_camera_probe_only"),true);
    R->SetBoolField(TEXT("interior_merge_probe"),InteriorMerge);
    R->SetBoolField(TEXT("walking_or_stair_connection_verified"),false);
    R->SetBoolField(TEXT("formal_gameplay_integrated"),false);
    R->SetBoolField(TEXT("art_approved"),false);R->SetArrayField(TEXT("captures"),Captures);
    FString S;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&S));
    FFileHelper::SaveStringToFile(S,*(FPaths::ProjectSavedDir()/TEXT("city-comparison-runtime.json")));
    FGenericPlatformMisc::RequestExit(false);
}
