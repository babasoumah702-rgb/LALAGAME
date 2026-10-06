// Camera-only compatibility module. Receiver uses a panorama, not Cesium splats.
// Isolated adaptation: original GaussianSplatProbe source remains untouched.
#include "SplatProbeGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

ASplatProbePawn::ASplatProbePawn()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(RootComponent);Camera->FieldOfView=80;
    Camera->bUsePawnControlRotation=true;
    Camera->PostProcessSettings.bOverride_AutoExposureMethod=true;
    Camera->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
}
void ASplatProbePawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis("MoveForward",this,&ASplatProbePawn::Forward);
    Input->BindAxis("MoveRight",this,&ASplatProbePawn::Right);
    Input->BindAxis("MoveUp",this,&ASplatProbePawn::Up);
    Input->BindAxis("Turn",this,&ASplatProbePawn::Turn);
    Input->BindAxis("LookUp",this,&ASplatProbePawn::Look);
    Input->BindAction("Exit",IE_Pressed,this,&ASplatProbePawn::Exit);
}
void ASplatProbePawn::Forward(float V){if(Controller)AddActorWorldOffset(Controller->GetControlRotation().Vector()*V*200*GetWorld()->GetDeltaSeconds());}
void ASplatProbePawn::Right(float V){if(Controller)AddActorWorldOffset(FRotationMatrix(Controller->GetControlRotation()).GetUnitAxis(EAxis::Y)*V*200*GetWorld()->GetDeltaSeconds());}
void ASplatProbePawn::Up(float V){AddActorWorldOffset(FVector(0,0,V*200*GetWorld()->GetDeltaSeconds()));}
void ASplatProbePawn::Turn(float V){AddControllerYawInput(V);}
void ASplatProbePawn::Look(float V){AddControllerPitchInput(V);}
void ASplatProbePawn::Exit(){FGenericPlatformMisc::RequestExit(false);}
ASplatProbeGameMode::ASplatProbeGameMode(){DefaultPawnClass=ASplatProbePawn::StaticClass();}
void ASplatProbeGameMode::BeginPlay(){Super::BeginPlay();}
void ASplatProbeGameMode::Tick(float D){Super::Tick(D);}
