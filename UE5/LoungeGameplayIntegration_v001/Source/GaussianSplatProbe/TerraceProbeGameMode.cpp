#include "TerraceProbeGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

ATerraceProbeCharacter::ATerraceProbeCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(34,96);
    auto* Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("TerraceCamera"));
    Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(0,0,60));
    Camera->bUsePawnControlRotation=true;
    Camera->FieldOfView=80;
    Camera->PostProcessSettings.bOverride_AutoExposureMethod=true;
    Camera->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    Camera->PostProcessSettings.bOverride_AutoExposureBias=true;
    Camera->PostProcessSettings.AutoExposureBias=-1.5;
    Camera->PostProcessBlendWeight=1;
    GetCharacterMovement()->MaxWalkSpeed=220;
    GetCharacterMovement()->MaxStepHeight=35;
}
void ATerraceProbeCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis("MoveForward",this,&ATerraceProbeCharacter::WalkForward);
    Input->BindAxis("MoveRight",this,&ATerraceProbeCharacter::WalkRight);
    Input->BindAxis("Turn",this,&ATerraceProbeCharacter::Turn);
    Input->BindAxis("LookUp",this,&ATerraceProbeCharacter::Look);
    Input->BindAction("Exit",IE_Pressed,this,&ATerraceProbeCharacter::Exit);
}
void ATerraceProbeCharacter::WalkForward(float V) { if(Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V,true); }
void ATerraceProbeCharacter::WalkRight(float V) { if(Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V,true); }
void ATerraceProbeCharacter::Turn(float V) { AddControllerYawInput(V); }
void ATerraceProbeCharacter::Look(float V) { AddControllerPitchInput(V); }
void ATerraceProbeCharacter::Exit() { FGenericPlatformMisc::RequestExit(false); }
ATerraceProbeGameMode::ATerraceProbeGameMode()
{
    DefaultPawnClass=ATerraceProbeCharacter::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}
void ATerraceProbeGameMode::BeginPlay()
{
    Super::BeginPlay();
    Auto=FParse::Param(FCommandLine::Get(),TEXT("TerraceAuto"));
    Started=FPlatformTime::Seconds();
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))
    {
        PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
        PC->SetControlRotation(FRotator(0,90,0));
        if(APawn* Pawn=PC->GetPawn()) { Pawn->SetActorLocation(FVector(0,-300,798));if(Auto) Pawn->DisableInput(PC); }
    }
}
void ATerraceProbeGameMode::Capture(const FString& Name)
{
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    if(!PC||!PC->GetPawn()) return;
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(Name+TEXT(".png")),false,false);
    auto Entry=MakeShared<FJsonObject>();
    Entry->SetStringField(TEXT("name"),Name);
    Entry->SetStringField(TEXT("capsule_center_cm"),PC->GetPawn()->GetActorLocation().ToString());
    Entry->SetStringField(TEXT("rotation"),PC->GetControlRotation().ToString());
    Entry->SetStringField(TEXT("pawn_class"),PC->GetPawn()->GetClass()->GetName());
    Entry->SetBoolField(TEXT("pawn_has_controller"),PC->GetPawn()->GetController()!=nullptr);
    Entry->SetStringField(TEXT("view_target"),PC->GetViewTarget()?PC->GetViewTarget()->GetName():TEXT("none"));
    if(auto* C=PC->GetPawn()->FindComponentByClass<UCameraComponent>())
    {
        Entry->SetStringField(TEXT("camera_world_rotation"),C->GetComponentRotation().ToString());
        Entry->SetStringField(TEXT("camera_world_location"),C->GetComponentLocation().ToString());
    }
    Captures.Add(MakeShared<FJsonValueObject>(Entry));
}
void ATerraceProbeGameMode::Tick(float Delta)
{
    Super::Tick(Delta);if(!Auto) return;
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto* Pawn=PC?Cast<ATerraceProbeCharacter>(PC->GetPawn()):nullptr;if(!Pawn) return;
    const double T=FPlatformTime::Seconds()-Started;
    if(Stage==0&&T>16) { PC->SetViewTarget(Pawn);Capture(TEXT("01_Entrance"));WalkStart=Pawn->GetActorLocation();Stage=1; }
    else if(Stage==1&&T<19) Pawn->WalkForward(1);
    else if(Stage==1) { WalkEnd=Pawn->GetActorLocation();Stage=2; }
    else if(Stage==2&&T>23) { Capture(TEXT("02_RailApproach"));Stage=3; }
    else if(Stage==3&&T<27) Pawn->WalkForward(1);
    else if(Stage==3) { RailEnd=Pawn->GetActorLocation();PC->SetControlRotation(FRotator(-42,90,0));Stage=4; }
    // Screenshots render after this tick. Hold each view for two seconds after
    // requesting it; changing the camera immediately captures the next view.
    else if(Stage==4&&T>31) { Capture(TEXT("03_LookDown"));SideStart=Pawn->GetActorLocation();Stage=5; }
    else if(Stage==5&&T<33) {}
    else if(Stage==5&&T<36) { PC->SetControlRotation(FRotator(0,90,0));Pawn->WalkRight(-1); }
    else if(Stage==5) { SideEnd=Pawn->GetActorLocation();Stage=6; }
    else if(Stage==6&&T>40) { Capture(TEXT("04_LeftEnd"));Stage=7; }
    else if(Stage==7&&T<42) {}
    else if(Stage==7&&T<47) Pawn->WalkRight(1);
    else if(Stage==7) { Stage=8; }
    else if(Stage==8&&T>51) { Capture(TEXT("05_RightEnd"));Stage=9; }
    else if(Stage==9&&T>53) { PC->SetControlRotation(FRotator(65,90,0));Stage=10; }
    else if(Stage==10&&T>57) { Capture(TEXT("06_Sky"));Stage=11; }
    else if(Stage==11&&T>59) { PC->SetControlRotation(FRotator(0,-90,0));Stage=12; }
    else if(Stage==12&&T>63) { Capture(TEXT("07_Back"));Stage=13; }
    else if(Stage==13&&T>66) Finish();
}
void ATerraceProbeGameMode::Finish()
{
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto* Pawn=PC?Cast<ATerraceProbeCharacter>(PC->GetPawn()):nullptr;
    auto R=MakeShared<FJsonObject>();
    R->SetStringField(TEXT("map"),GetWorld()->GetMapName());
    R->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    R->SetBoolField(TEXT("uses_character_movement"),true);
    R->SetStringField(TEXT("walk_start_cm"),WalkStart.ToString());
    R->SetStringField(TEXT("walk_end_cm"),WalkEnd.ToString());
    R->SetStringField(TEXT("rail_end_cm"),RailEnd.ToString());
    R->SetNumberField(TEXT("walk_displacement_cm"),FVector::Dist(WalkStart,WalkEnd));
    R->SetNumberField(TEXT("side_displacement_cm"),FVector::Dist(SideStart,SideEnd));
    R->SetBoolField(TEXT("rail_blocks_forward"),RailEnd.Y<546&&RailEnd.Y>480);
    R->SetBoolField(TEXT("grounded_at_finish"),Pawn&&Pawn->GetCharacterMovement()->IsMovingOnGround());
    R->SetBoolField(TEXT("automated_inputs_not_manual_keyboard_test"),true);
    R->SetBoolField(TEXT("formal_gameplay_integrated"),false);
    R->SetBoolField(TEXT("art_approved"),false);
    R->SetArrayField(TEXT("captures"),Captures);
    FString S;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&S));
    FFileHelper::SaveStringToFile(S,*(FPaths::ProjectSavedDir()/TEXT("terrace-runtime.json")));
    UE_LOG(LogTemp,Display,TEXT("TERRACE_REPORT %s"),*S);
    FGenericPlatformMisc::RequestExit(false);
}
