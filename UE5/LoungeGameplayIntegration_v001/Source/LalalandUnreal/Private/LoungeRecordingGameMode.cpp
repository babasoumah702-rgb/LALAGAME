#include "LoungeRecordingGameMode.h"
#include "LalalandGlassProp.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

ALoungeRecordingHUD::ALoungeRecordingHUD(){RecordingDemo=true;}
ALoungeRecordingGameMode::ALoungeRecordingGameMode(){HUDClass=ALoungeRecordingHUD::StaticClass();}
void AMixologyHUD::OpenRecording()
{
 if(!Station||!Station->StudioCamera.IsValid())return;
 Active=true;EntryMenu=true;OrderMode=false;Amounts.Reset();PendingAmounts.Reset();CommandId.Empty();
 Making=false;Pouring=false;Awaiting=false;CompletionSent=false;BeautyMode=false;
 if(RecordingCup)RecordingCup->SetActorHiddenInGame(true);
 auto PC=GetOwningPlayerController();PreviousView=PC->GetViewTarget();PC->SetViewTargetWithBlend(Station->StudioCamera.Get(),.4f);
 if(auto P=Cast<ACharacter>(PC->GetPawn()))P->GetCharacterMovement()->StopMovementImmediately();
 FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
 PC->SetInputMode(Mode);PC->bShowMouseCursor=true;PC->bEnableClickEvents=true;PC->ClickEventKeys.AddUnique(EKeys::LeftMouseButton);
 if(auto V=GetWorld()->GetGameViewport())V->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
 UWidgetBlueprintLibrary::SetFocusToGameViewport();
 Notice=TEXT("本地试玩：完成制作后才扣费，按 Esc 返回。");
 UE_LOG(LogTemp,Display,TEXT("RECORDING_OPEN entry=1 ready=%d staff_camera_y=%.1f"),Ready,Station->StudioCamera->GetActorLocation().Y);
}
void AMixologyHUD::TickRecording(float Dt)
{
 if(!Station)return;
 const float Now=GetWorld()->GetTimeSeconds();
 if(Making&&!CompletionSent&&Now>=CompletionTime){
  // Single local commit, not a fabricated server acknowledgement.
  CompletionSent=true;Making=false;Cash-=RecordingCost;RecordingCompleted++;
  LastPreparedAmounts=PendingAmounts;LastPreparedName=PendingName;CommandId.Empty();
  if(RecordingCup)RecordingCup->Destroy();
  RecordingCup=GetWorld()->SpawnActor<ALalalandGlassProp>();
  if(!RecordingCup->BuildNativeMix(RecordingRecipeId,.68f)){RecordingCup->Destroy();RecordingCup=nullptr;Notice=TEXT("成品模型加载失败，请重启试玩。");}
  else Notice=TEXT("成品：")+PendingName+TEXT(" / 按 F 自饮，或再次走近吧台。 ");
  RecordingSipStart=-1;RecordingConsumed=false;
  UE_LOG(LogTemp,Display,TEXT("RECORDING_COMMIT cash=%d completed=%d recipe=%s cup=%d"),Cash,RecordingCompleted,*RecordingRecipeId,RecordingCup!=nullptr);
  ReturnToBar();
 }
 if(RecordingCup){
  RecordingCup->SetActorHiddenInGame(Active);
  auto PC=GetOwningPlayerController();FVector Eye;FRotator Rot;PC->GetPlayerViewPoint(Eye,Rot);
  FVector Offset(38,20,-28);FRotator Tilt=FRotator::ZeroRotator;
  if(RecordingSipStart>=0){
   const float T=Now-RecordingSipStart;
   const float Blend=T<.5f?FMath::SmoothStep(0.f,1.f,T/.5f):T<1.3f?1.f:1-FMath::SmoothStep(0.f,1.f,(T-1.3f)/.7f);
   Offset=FMath::Lerp(Offset,FVector(18,5,-11),Blend);Tilt.Pitch=-38*Blend;
   if(T>=1.2f&&!RecordingConsumed){RecordingCup->SetConsumed(true);RecordingConsumed=true;Notice=TEXT("已自饮。R 可重置试玩预算，再录一遍。");UE_LOG(LogTemp,Display,TEXT("RECORDING_SIP consumed=1"));}
   if(T>=2.1f){RecordingCup->Destroy();RecordingCup=nullptr;return;}
  }
  RecordingCup->SetActorLocation(Eye+Rot.RotateVector(Offset));RecordingCup->SetActorRotation(Rot+Tilt);
 }
}
void AMixologyHUD::SipRecording(){if(Active||!RecordingCup||RecordingSipStart>=0)return;RecordingSipStart=GetWorld()->GetTimeSeconds();RecordingCup->BeginSip(FGuid::NewGuid().ToString());}
void AMixologyHUD::ResetRecording(){if(Active)return;if(RecordingCup)RecordingCup->Destroy();RecordingCup=nullptr;Cash=18;RecordingSipStart=-1;Notice=TEXT("试玩预算已重置，不影响正式存档。");}

void ALoungeRecordingGameMode::BeginPlay()
{
 Super::BeginPlay();Auto=FParse::Param(FCommandLine::Get(),TEXT("LoungeRecordingAuto"));At=GetWorld()->GetTimeSeconds();
 UE_LOG(LogTemp,Display,TEXT("RECORDING_READY offline=1 narrative_backend=0 paid_requests=0"));
}
void ALoungeRecordingGameMode::FinishRecordingTest(bool Passed)
{
 const FString Dir=FPaths::ProjectSavedDir()/TEXT("RecordingDemo");IFileManager::Get().MakeDirectory(*Dir,true);
 auto PC=GetWorld()->GetFirstPlayerController();auto H=::Cast<AMixologyHUD>(PC->GetHUD());
 FString Result=FString::Printf(TEXT("{\"passed\":%s,\"completed\":%d,\"cash\":%d,\"offline\":true,\"full_story_integrated\":false,\"paid_requests\":0}"),Passed?TEXT("true"):TEXT("false"),H?H->RecordingCompleted:0,H?H->Cash:0);
 FFileHelper::SaveStringToFile(Result,*(Dir/TEXT("runtime.json")));
 UE_LOG(LogTemp,Display,TEXT("RECORDING_TEST_FINISHED passed=%d"),Passed);Phase=100;FPlatformMisc::RequestExit(false);
}
void ALoungeRecordingGameMode::Tick(float Dt)
{
 Super::Tick(Dt);if(!Auto||Phase==100)return;
 auto PC=GetWorld()->GetFirstPlayerController();auto H=PC?::Cast<AMixologyHUD>(PC->GetHUD()):nullptr;auto P=PC?PC->GetPawn():nullptr;
 if(!H||!P)return;
 const float Now=GetWorld()->GetTimeSeconds();
 auto Key=[&](FKey K,EInputEvent Event=IE_Pressed){PC->InputKey(FInputKeyParams(K,Event,Event==IE_Released?0.:1.));};
 auto Tap=[&](FKey K){Key(K);Key(K,IE_Released);};
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("RecordingDemo")/Name,false,false);};
 auto Click=[&](float X,float Y){int32 W,V;PC->GetViewportSize(W,V);float S=FMath::Min(W/1200.f,V/760.f);return H->UpdateAndDispatchHitBoxClickEvents(FVector2D(X*S,Y*S),IE_Pressed);};
 if(Now>75){FinishRecordingTest(false);return;}
 if(Phase==0&&Now-At>3){MoveStart=P->GetActorLocation();PC->SetControlRotation(FRotator(0,90,0));Key(EKeys::W);At=Now;Phase=1;}
 else if(Phase==1&&P->GetActorLocation().Y>220){Key(EKeys::W,IE_Released);if(!H->CanEnter()||FVector::Dist2D(MoveStart,P->GetActorLocation())<800){FinishRecordingTest(false);return;}Tap(EKeys::E);Phase=2;At=Now;}
 else if(Phase==2&&Now-At>1){Shot(TEXT("01_Entry.png"));if(!H->Active||!H->EntryMenu||!Click(600,381)){FinishRecordingTest(false);return;}Phase=3;At=Now;}
 else if(Phase==3&&Now-At>.8){if(!H->OrderMode||!Click(156,556)){FinishRecordingTest(false);return;}Phase=4;At=Now;}
 else if(Phase==4&&Now-At>.8){Shot(TEXT("02_Recipe.png"));if(H->OrderMode||H->Amounts.FindRef(TEXT("gin"))!=50){FinishRecordingTest(false);return;}Click(270,657);H->AddMaterial(0);Phase=5;At=Now;}
 else if(Phase==5&&Now-At>1.6&&!H->Pouring){H->Amounts={{TEXT("bourbon"),45},{TEXT("lemon_juice"),25},{TEXT("simple_syrup"),10}};Tap(EKeys::Enter);Phase=6;At=Now;}
 else if(Phase==6&&Now-At>.6){if(!H->Making||H->Cash!=18){FinishRecordingTest(false);return;}Tap(EKeys::Enter);Shot(TEXT("03_Shaking.png"));Phase=7;}
 else if(Phase==7&&Now-At>2.5&&H->Making){Shot(TEXT("04_Pouring.png"));Phase=8;}
 else if(Phase==8&&!H->Active){At=Now;Phase=9;}
 else if(Phase==9&&Now-At>1){
  if(H->RecordingCompleted!=1||H->Cash!=11||!H->RecordingCup||PC->bShowMouseCursor||PC->GetViewTarget()!=P){FinishRecordingTest(false);return;}
  Shot(TEXT("05_Served.png"));MoveStart=P->GetActorLocation();Key(EKeys::S);Phase=10;At=Now;
 }
 else if(Phase==10&&Now-At>.7){Key(EKeys::S,IE_Released);if(FVector::Dist2D(MoveStart,P->GetActorLocation())<70){FinishRecordingTest(false);return;}Tap(EKeys::F);Phase=11;At=Now;}
 else if(Phase==11&&Now-At>2.5){if(!H->RecordingConsumed||H->RecordingCup){FinishRecordingTest(false);return;}Key(EKeys::W);At=Now;Phase=12;}
 else if(Phase==12&&Now-At>.7){Key(EKeys::W,IE_Released);Tap(EKeys::E);Phase=13;At=Now;}
 else if(Phase==13&&Now-At>.7){if(!H->EntryMenu){FinishRecordingTest(false);return;}Tap(EKeys::Escape);Phase=14;At=Now;}
 else if(Phase==14&&Now-At>.8){if(H->Active||H->Cash!=11||PC->bShowMouseCursor){FinishRecordingTest(false);return;}Tap(EKeys::E);Phase=15;At=Now;}
 else if(Phase==15&&Now-At>.8){Click(600,381);Phase=16;At=Now;}
 else if(Phase==16&&Now-At>.7){Tap(EKeys::Enter);Phase=17;At=Now;}
 else if(Phase==17&&!H->Active&&Now-At>2){if(H->RecordingCompleted!=2||H->Cash!=5){FinishRecordingTest(false);return;}Shot(TEXT("06_Ordered.png"));Phase=18;At=Now;}
 else if(Phase==18&&Now-At>1){FinishRecordingTest(true);}
}
