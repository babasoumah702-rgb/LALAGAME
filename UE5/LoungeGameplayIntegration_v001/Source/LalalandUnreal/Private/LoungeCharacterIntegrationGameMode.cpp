#include "LoungeCharacterIntegrationGameMode.h"
#include "LalalandNpcCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "MixologyGameMode.h"
#include "Components/InputComponent.h"

ALoungeInspectionPawn::ALoungeInspectionPawn()
{
    GetCapsuleComponent()->InitCapsuleSize(38,92);
    GetCharacterMovement()->MaxWalkSpeed=280;
    bUseControllerRotationYaw=true;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("InspectionCamera"));
    Camera->SetupAttachment(GetCapsuleComponent());Camera->SetRelativeLocation(FVector(0,0,68));
    Camera->bUsePawnControlRotation=true;Camera->FieldOfView=80;
    Camera->PostProcessSettings.bOverride_AutoExposureMethod=true;
    Camera->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    Camera->PostProcessSettings.bOverride_AutoExposureBias=true;
    Camera->PostProcessSettings.AutoExposureBias=-1.5;
}
void ALoungeInspectionPawn::SetupPlayerInputComponent(UInputComponent* I)
{
    Super::SetupPlayerInputComponent(I);
    I->BindAxis(TEXT("MoveForward"),this,&ALoungeInspectionPawn::Forward);
    I->BindAxis(TEXT("MoveRight"),this,&ALoungeInspectionPawn::Right);
    I->BindAxis(TEXT("Turn"),this,&ALoungeInspectionPawn::Turn);
    I->BindAxis(TEXT("LookUp"),this,&ALoungeInspectionPawn::Look);
    I->BindAction(TEXT("Exit"),IE_Pressed,this,&ALoungeInspectionPawn::Exit);
    I->BindKey(EKeys::E,IE_Pressed,this,&ALoungeInspectionPawn::Interact);
    I->BindKey(EKeys::Enter,IE_Pressed,this,&ALoungeInspectionPawn::ConfirmMix);
    I->BindKey(EKeys::V,IE_Pressed,this,&ALoungeInspectionPawn::ViewMix);
    I->BindKey(EKeys::F,IE_Pressed,this,&ALoungeInspectionPawn::Sip);
    I->BindKey(EKeys::R,IE_Pressed,this,&ALoungeInspectionPawn::ResetDemo);
}
namespace { AMixologyHUD* MixHUD(const APawn* P){auto PC=Cast<APlayerController>(P->GetController());return PC?Cast<AMixologyHUD>(PC->GetHUD()):nullptr;} }
bool ALoungeInspectionPawn::Modal()const{auto H=MixHUD(this);return H&&H->Active;}
void ALoungeInspectionPawn::Forward(float V){if(!Modal())AddMovementInput(GetActorForwardVector(),V);}
void ALoungeInspectionPawn::Right(float V){if(!Modal())AddMovementInput(GetActorRightVector(),V);}
void ALoungeInspectionPawn::Turn(float V){if(!Modal())AddControllerYawInput(V);}
void ALoungeInspectionPawn::Look(float V){if(!Modal())AddControllerPitchInput(V);}
void ALoungeInspectionPawn::Exit(){if(auto H=MixHUD(this)){if(H->Active)H->ReturnToBar();return;}FGenericPlatformMisc::RequestExit(false);}
void ALoungeInspectionPawn::Interact(){if(auto H=MixHUD(this))H->InteractAtBar();}
void ALoungeInspectionPawn::ConfirmMix(){if(auto H=MixHUD(this))H->Confirm();}
void ALoungeInspectionPawn::ViewMix(){if(auto H=MixHUD(this))if(H->Active&&!H->EntryMenu&&!H->Making&&!H->Pouring)H->BeautyMode=!H->BeautyMode;}
void ALoungeInspectionPawn::Sip(){if(auto H=MixHUD(this))if(H->RecordingDemo)H->SipRecording();}
void ALoungeInspectionPawn::ResetDemo(){if(auto H=MixHUD(this))if(H->RecordingDemo)H->ResetRecording();}

namespace {
const TCHAR* RoleIds[]={TEXT("A"),TEXT("B"),TEXT("C"),TEXT("D"),TEXT("BARTENDER")};
const TCHAR* ViewNames[]={TEXT("01_Hall"),TEXT("02_Kiko"),TEXT("03_X"),TEXT("04_Wansai"),TEXT("05_Yitong"),TEXT("06_Bartender"),TEXT("07_Walk"),TEXT("08_Return")};
TArray<TSharedPtr<FJsonValue>> Vec(const FVector& V){return {MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};}
}
ALoungeCharacterIntegrationGameMode::ALoungeCharacterIntegrationGameMode()
{
    DefaultPawnClass=ALoungeInspectionPawn::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}
void ALoungeCharacterIntegrationGameMode::BeginPlay()
{
    Super::BeginPlay();
    Automated=FParse::Param(FCommandLine::Get(),TEXT("LoungeCharacterAuto"));
    StartedAt=GetWorld()->GetTimeSeconds();
    for(const TCHAR* Id:RoleIds)
    {
        const FName Tag(*FString::Printf(TEXT("LoungeAnchor_%s"),Id));
        TArray<AActor*> Found;UGameplayStatics::GetAllActorsWithTag(this,Tag,Found);
        if(Found.Num()!=1){UE_LOG(LogTemp,Error,TEXT("LOUNGE_ANCHOR_INVALID %s count=%d"),Id,Found.Num());Finish();return;}
        const FVector Ground=Found[0]->GetActorLocation();Anchors.Add(Id,Ground);
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* N=GetWorld()->SpawnActor<ALalalandNpcCharacter>(ALalalandNpcCharacter::StaticClass(),Ground+FVector(0,0,92),FRotator(0,-90,0),Params);
        if(!N){UE_LOG(LogTemp,Error,TEXT("LOUNGE_SPAWN_FAILED %s"),Id);Finish();return;}
        N->InitializeActor(Id,FLinearColor::White);Characters.Add(N);Cast.Add(Id,N);
    }
    for(ALalalandNpcCharacter* N:Characters)ApplyGoal(N,Anchors[N->GetActorId()],-90);
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))
    {
        PC->bShowMouseCursor=false;PC->SetInputMode(FInputModeGameOnly());
        if(APawn* P=PC->GetPawn()){P->SetActorLocation(FVector(0,-550,94));if(Automated)P->DisableInput(PC);}
        PC->SetControlRotation(FRotator(0,90,0));
    }
    Ready=true;SetView(0);
    UE_LOG(LogTemp,Display,TEXT("LOUNGE_CHARACTER_READY count=%d old_stage_spawned=0 production_gameplay=0"),Characters.Num());
}
void ALoungeCharacterIntegrationGameMode::ApplyGoal(ALalalandNpcCharacter* N,const FVector& Ground,float Yaw)
{
    FLalalandActorDto D;D.id=N->GetActorId();D.name=D.id;D.area=TEXT("bar");D.posture=TEXT("stand");
    // Explicit UE centimeters -> legacy server coordinates. No old map offsets.
    D.x=Ground.X/100.;D.y=Ground.Z/100.;D.z=Ground.Y/100.;D.yaw=90-Yaw;
    N->ApplyState(D,Cast);Goals.Add(D.id,Ground);GoalYaw.Add(D.id,Yaw);
}
void ALoungeCharacterIntegrationGameMode::SetMotionPhase(int32 Phase)
{
    MotionPhase=Phase;
    for(ALalalandNpcCharacter* N:Characters)
    {
        FVector Goal=Anchors[N->GetActorId()];float Yaw=-90;
        if(N->GetActorId()==TEXT("BARTENDER"))
        {
            if(Phase==1){Goal.X+=200;Yaw=0;}
            if(Phase==2){Goal.X-=150;Yaw=180;}
        }
        else
        {
            if(Phase==1){Goal.X+=200;Yaw=0;}
            if(Phase==2){Goal+=FVector(200,170,0);Yaw=90;}
        }
        ApplyGoal(N,Goal,Yaw);
    }
    UE_LOG(LogTemp,Display,TEXT("LOUNGE_MOTION_PHASE %d"),Phase);
}
void ALoungeCharacterIntegrationGameMode::CheckMotionPhase(int32 Phase)
{
    for(ALalalandNpcCharacter* N:Characters)
    {
        const float Distance=FVector::Dist2D(N->GetActorLocation(),Goals[N->GetActorId()]);
        const float YawError=FMath::Abs(FMath::FindDeltaAngleDegrees(N->GetActorRotation().Yaw,GoalYaw[N->GetActorId()]));
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("role"),N->GetActorId());R->SetNumberField(TEXT("phase"),Phase);
        R->SetNumberField(TEXT("distance_to_goal_cm"),Distance);R->SetNumberField(TEXT("speed_cm_s"),N->GetVelocity().Size2D());
        R->SetNumberField(TEXT("yaw_error_degrees"),YawError);
        R->SetBoolField(TEXT("walkable_floor"),N->GetCharacterMovement()->CurrentFloor.IsWalkableFloor());
        R->SetNumberField(TEXT("steering_phase"),N->GetLocomotionSteeringPhase());
        R->SetArrayField(TEXT("capsule_position_cm"),Vec(N->GetActorLocation()));
        R->SetBoolField(TEXT("passed"),Distance<=25&&YawError<=10&&N->GetCharacterMovement()->CurrentFloor.IsWalkableFloor()&&N->GetVelocity().Size2D()<8);
        Checks.Add(MakeShared<FJsonValueObject>(R));
        UE_LOG(LogTemp,Display,TEXT("LOUNGE_MOTION_CHECK role=%s phase=%d distance=%.1f speed=%.1f floor=%d"),*N->GetActorId(),Phase,Distance,N->GetVelocity().Size2D(),N->GetCharacterMovement()->CurrentFloor.IsWalkableFloor());
    }
}
void ALoungeCharacterIntegrationGameMode::SetView(int32 V)
{
    auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC||!PC->GetPawn())return;
    FVector Eye(0,-740,165),Target(0,180,140);
    if(V>=1&&V<=5)
    {
        FVector Ground=Anchors[RoleIds[V-1]];
        if(V==5){Eye=FVector(0,150,165);Target=Ground+FVector(0,0,150);}
        else{Eye=Ground+FVector(120,-240,165);Target=Ground+FVector(0,0,115);}
    }
    // View-only QA captures may float; never claim these teleports test player routes.
    auto* P=::Cast<ACharacter>(PC->GetPawn());if(P&&Automated)P->GetCharacterMovement()->DisableMovement();
    PC->GetPawn()->SetActorLocation(Eye-FVector(0,0,68));PC->SetControlRotation((Target-Eye).Rotation());
}
void ALoungeCharacterIntegrationGameMode::Tick(float D)
{
    Super::Tick(D);if(!Ready||Finished)return;
    // Source Stage continuously applies state. Preserve that contract even in
    // this offline fixture, especially A's legacy movement-facing branch.
    // ApplyState teleports only the FIRST state, not these subsequent refreshes.
    for(ALalalandNpcCharacter* N:Characters)
        ApplyGoal(N,Goals[N->GetActorId()],GoalYaw[N->GetActorId()]);
    if(!Automated)return;
    const double T=GetWorld()->GetTimeSeconds()-StartedAt;
    if(CaptureIndex<6&&T>=18+CaptureIndex*5)
    {
        const FString Name=ViewNames[CaptureIndex];FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("CharacterIntegration")/(Name+TEXT(".png")),false,false);
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("name"),Name);R->SetNumberField(TEXT("world_seconds"),T);Captures.Add(MakeShared<FJsonValueObject>(R));
        ++CaptureIndex;
    }
    // Move camera a full second after each deferred screenshot.
    if(CaptureIndex>0&&CaptureIndex<6&&T>=19+(CaptureIndex-1)*5)SetView(CaptureIndex);
    if(T>=48&&MotionPhase==0){SetMotionPhase(1);SetView(0);}
    if(T>=53&&CaptureIndex==6){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("CharacterIntegration/07_Walk.png"),false,false);++CaptureIndex;}
    if(T>=58&&MotionPhase==1){CheckMotionPhase(1);SetMotionPhase(2);}
    if(T>=68&&MotionPhase==2){CheckMotionPhase(2);SetMotionPhase(3);}
    if(T>=80&&MotionPhase==3){CheckMotionPhase(3);MotionPhase=4;}
    if(T>=84&&CaptureIndex==7){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("CharacterIntegration/08_Return.png"),false,false);++CaptureIndex;}
    if(T>=88)Finish();
}
void ALoungeCharacterIntegrationGameMode::Finish()
{
    if(Finished)return;Finished=true;
    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("map"),GetWorld()->GetMapName());R->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    R->SetBoolField(TEXT("offline_character_probe_only"),true);R->SetBoolField(TEXT("full_gameplay_integrated"),false);R->SetBoolField(TEXT("production_player_arms_enabled"),false);
    R->SetBoolField(TEXT("old_stage_spawned"),false);R->SetNumberField(TEXT("paid_requests"),0);R->SetArrayField(TEXT("motion_checks"),Checks);R->SetArrayField(TEXT("captures"),Captures);
    TArray<TSharedPtr<FJsonValue>> Meshes;
    for(ALalalandNpcCharacter* N:Characters)
    {
        auto M=MakeShared<FJsonObject>();M->SetStringField(TEXT("role"),N->GetActorId());
        USkeletalMesh* Mesh=N->GetMesh()->GetSkeletalMeshAsset();M->SetStringField(TEXT("mesh"),GetPathNameSafe(Mesh));
        const auto Bounds=Mesh?Mesh->GetBounds():FBoxSphereBounds(ForceInit);
        M->SetNumberField(TEXT("reference_height_cm"),Bounds.BoxExtent.Z*2*N->GetMesh()->GetComponentScale().Z);
        M->SetBoolField(TEXT("skeletal_mesh_loaded"),Mesh!=nullptr);Meshes.Add(MakeShared<FJsonValueObject>(M));
    }
    R->SetArrayField(TEXT("characters"),Meshes);
    bool Passed=Ready&&Characters.Num()==5&&Checks.Num()==15;
    for(auto C:Checks)Passed&=C->AsObject()->GetBoolField(TEXT("passed"));
    R->SetBoolField(TEXT("basic_movement_passed"),Passed);
    FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("CharacterIntegration");IFileManager::Get().MakeDirectory(*Dir,true);
    FFileHelper::SaveStringToFile(Json,*(Dir/TEXT("runtime.json")));
    UE_LOG(LogTemp,Display,TEXT("LOUNGE_CHARACTER_FINISHED basic_movement_passed=%d checks=%d"),Passed,Checks.Num());
    if(Automated)FGenericPlatformMisc::RequestExit(false);
}
