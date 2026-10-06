#include "MixologyGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/GameViewportClient.h"
#include "CanvasTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Components/LightComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MixologyPresentation.h"
#include "CanvasItem.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"

#include "LalalandServiceSubsystem.h"
#include "LalalandRootWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/TextRenderActor.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "LalalandGlassProp.h"

namespace {
 FString DisplayUnit(const FString& Unit){return Unit==TEXT("leaf")?TEXT("片"):Unit;}
 FString ChineseMethod(const FString& M){
  if(M==TEXT("shake_then_top"))return TEXT("摇和过滤后补气泡");
  if(M.Contains(TEXT("shake")))return TEXT("摇和并过滤");
  if(M.Contains(TEXT("press_mint")))return TEXT("轻压薄荷后兑和");
  if(M.Contains(TEXT("stir")))return TEXT("杯内轻搅");
  return TEXT("直接兑和");
 }
}


AMixologyHUD::AMixologyHUD(){PrimaryActorTick.bCanEverTick=true;}
AMixologyStation::AMixologyStation(){
 PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
void AMixologyStation::BeginPlay(){
 Super::BeginPlay();
 const bool Recording=GetWorld()->GetMapName().Contains(TEXT("L_RecordableLounge_v002"));
 SetActorLocation(Recording?FVector(0,410,34):FVector(-410,438,41));SetActorRotation(FRotator(0,-90,0));
 auto Prop=[&](const TCHAR* Mesh,const TCHAR* Material,FVector P,FVector Scale=FVector::OneVector){
  auto A=GetWorld()->SpawnActor<AStaticMeshActor>();auto C=A->GetStaticMeshComponent();
  C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Mesh));
  if(Material)C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Material));
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  A->AttachToActor(this,FAttachmentTransformRules::KeepRelativeTransform);
  A->SetActorRelativeLocation(P);A->SetActorRelativeScale3D(Scale);return A;
 };
 BottleActors.SetNum(6);BottleHomes.SetNum(6);
 const TCHAR* Labels[]={TEXT("GIN"),TEXT("RUM"),TEXT("BOURBON"),TEXT("BITTER"),TEXT("VERMOUTH"),TEXT("SODA")};
 for(int I=0;I<6;I++){
  FString Mat=FString::Printf(TEXT("/Game/Mixology/Visual_v004/M_StudioBottle_%d.M_StudioBottle_%d"),I,I);
  BottleHomes[I]=FVector(25,-105+I*42,76);
  BottleActors[I]=Prop(TEXT("/Game/Mixology/Visual_v004/SM_StudioBottle.SM_StudioBottle"),*Mat,BottleHomes[I]);
  if(Recording){
   const int Indices[]={0,3,2,1,1,0};const int Index=Indices[I];
   const FString Path=FString::Printf(TEXT("/Game/Diagnostics/RecordingBottles_v001/%d/SM_Bottle_%d.SM_Bottle_%d"),Index,Index,Index);
   if(auto Mesh=LoadObject<UStaticMesh>(nullptr,*Path)){
    // Root remains at the bottle base, so the existing mouth/pour animation
    // stays in centimeter units regardless of Tripo's exported pivot/scale.
    BottleActors[I]->Destroy();auto Container=GetWorld()->SpawnActor<AActor>();
    auto Root=NewObject<USceneComponent>(Container);Container->SetRootComponent(Root);Root->RegisterComponent();
    auto C=NewObject<UStaticMeshComponent>(Container);Container->AddInstanceComponent(C);C->SetupAttachment(Root);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();
    const auto B=Mesh->GetBoundingBox();float S=32.5f/FMath::Max(.0001f,B.Max.Z-B.Min.Z);
    C->SetRelativeScale3D(FVector(S));C->SetRelativeLocation(FVector(-(B.Min.X+B.Max.X)*S*.5f,-(B.Min.Y+B.Max.Y)*S*.5f,-B.Min.Z*S));
    Container->AttachToActor(this,FAttachmentTransformRules::KeepRelativeTransform);Container->SetActorRelativeLocation(BottleHomes[I]);BottleActors[I]=Container;
   }
  }
  auto Label=GetWorld()->SpawnActor<ATextRenderActor>();Label->AttachToActor(BottleActors[I].Get(),FAttachmentTransformRules::KeepRelativeTransform);
  Label->SetActorRelativeLocation(FVector(-7,0,22));Label->SetActorRelativeRotation(FRotator(0,180,0));
  auto T=Label->GetTextRender();T->SetText(FText::FromString(Labels[I]));T->SetWorldSize(2);
  T->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);T->SetTextRenderColor(FColor(240,225,185));
 }
 ShakerHome=FVector(-25,-46,76);
 Shaker=Prop(TEXT("/Game/Mixology/Visual_v003/SM_ShakerOpen.SM_ShakerOpen"),TEXT("/Game/Mixology/Visual_v004/M_PolishedSteel.M_PolishedSteel"),ShakerHome);
 ShakerCap=Prop(TEXT("/Game/Mixology/Visual_v003/SM_ShakerCap.SM_ShakerCap"),TEXT("/Game/Mixology/Visual_v004/M_PolishedSteel.M_PolishedSteel"),ShakerHome);
 Spoon=Prop(TEXT("/Game/Mixology/Visual_v003/SM_BarSpoon.SM_BarSpoon"),TEXT("/Game/Mixology/Visual_v004/M_PolishedSteel.M_PolishedSteel"),FVector(-32,78,76));
 Prop(TEXT("/Game/Mixology/Visual_v004/SM_RibbedMat.SM_RibbedMat"),TEXT("/Game/Mixology/Visual_v004/M_RibbedRubber.M_RibbedRubber"),FVector(-10,0,76.15),FVector(.7,1.5,1));
 auto Glass=Prop(TEXT("/Game/Mixology/Visual_v005/SM_Tumbler.SM_Tumbler"),TEXT("/Game/Mixology/Visual_v005/M_Glass_v005.M_Glass_v005"),FVector(-49,12,76),FVector(.62f));
 Glass->GetStaticMeshComponent()->SetTranslucentSortPriority(2);Glass->GetStaticMeshComponent()->SetCastShadow(false);
 Liquid=Prop(TEXT("/Game/Mixology/Visual_v005/SM_LiquidBody.SM_LiquidBody"),nullptr,FVector(-49,12,77.426f));
 LiquidSurface=Prop(TEXT("/Game/Mixology/Visual_v005/SM_Meniscus.SM_Meniscus"),nullptr,FVector(-49,12,77.426f),FVector(.62f,.62f,1));
 Stream=Prop(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),nullptr,FVector::ZeroVector);
 auto LC=Cast<AStaticMeshActor>(Liquid.Get())->GetStaticMeshComponent();
 auto SC=Cast<AStaticMeshActor>(LiquidSurface.Get())->GetStaticMeshComponent();
 LiquidMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Mixology/Visual_v005/M_LiquidBody.M_LiquidBody")),this);
 SurfaceMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Mixology/Visual_v005/M_LiquidSurface.M_LiquidSurface")),this);
 LC->SetMaterial(0,LiquidMaterial);SC->SetMaterial(0,SurfaceMaterial);SC->SetTranslucentSortPriority(1);
 Cast<AStaticMeshActor>(Stream.Get())->GetStaticMeshComponent()->SetMaterial(0,LiquidMaterial);
 for(auto A:{Liquid.Get(),LiquidSurface.Get(),Stream.Get()}){A->SetActorHiddenInGame(true);Cast<AStaticMeshActor>(A)->GetStaticMeshComponent()->SetCastShadow(false);}
 DrinkVisualV005=true;
 auto Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->AttachToActor(this,FAttachmentTransformRules::KeepRelativeTransform);StudioCamera=Camera;
 CameraHome=FVector(-165,0,190);CameraClose=FVector(-112,-30,138);
 CameraHomeRotation=(FVector(-25,0,95)-CameraHome).Rotation().Quaternion();
 CameraCloseRotation=(FVector(-49,12,90)-CameraClose).Rotation().Quaternion();
 CameraHomeFOV=65;CameraCloseFOV=48;HasCloseCamera=true;
 Camera->SetActorRelativeLocation(CameraHome);Camera->SetActorRelativeRotation(CameraHomeRotation);
 Camera->GetCameraComponent()->SetFieldOfView(CameraHomeFOV);Camera->GetCameraComponent()->bConstrainAspectRatio=false;
 if(Recording){
  auto& PP=Camera->GetCameraComponent()->PostProcessSettings;
  PP.bOverride_AutoExposureMethod=true;PP.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
  PP.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP.AutoExposureApplyPhysicalCameraExposure=false;
  PP.bOverride_AutoExposureBias=true;PP.AutoExposureBias=-1.5;
 }
 UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_STATION created on bar origin=%s"),*GetActorLocation().ToString());
}
bool AMixologyStation::StartPour(int32 I){
 if(ActivePour||!BottleActors.IsValidIndex(I)||!BottleActors[I].IsValid())return false;
 ActiveBottle=BottleActors[I];ActiveHome=BottleHomes[I];PourStart=GetWorld()->GetTimeSeconds();ActivePour=true;return true;
}
void AMixologyStation::Tick(float Dt){
 Super::Tick(Dt);auto PC=GetWorld()->GetFirstPlayerController();if(!PC)return;auto H=Cast<AMixologyHUD>(PC->GetHUD());if(!H||!H->Active){if(Liquid.IsValid())Liquid->SetActorHiddenInGame(true);if(LiquidSurface.IsValid())LiquidSurface->SetActorHiddenInGame(true);if(Stream.IsValid())Stream->SetActorHiddenInGame(true);return;}
 // Modal shortcuts use the player's input component; polling from Actor Tick
 // could miss or double-dispatch events depending on the tick order.
 if(HasCloseCamera&&StudioCamera.IsValid()){
  FocusBlend=FMath::FInterpTo(FocusBlend,H->Making||H->BeautyMode?1.f:0.f,Dt,6.f);
  StudioCamera->SetActorRelativeLocation(FMath::Lerp(CameraHome,CameraClose,FocusBlend));
  FQuat CloseRot=CameraCloseRotation;
  if(H->RecordingDemo&&H->Making&&H->Sequence.Shake&&H->Sequence.Stage(GetWorld()->GetTimeSeconds())==EMixStage::Mixing)
   CloseRot=(ShakerHome+FVector(0,0,24)-CameraClose).Rotation().Quaternion();
  const FQuat Desired=FQuat::Slerp(CameraHomeRotation,CloseRot,FocusBlend);
  StudioCamera->SetActorRelativeRotation(H->RecordingDemo?FQuat::Slerp(StudioCamera->GetRootComponent()->GetRelativeRotation().Quaternion(),Desired,FMath::Clamp(Dt*8.f,0.f,1.f)):Desired);
  StudioCamera->GetCameraComponent()->SetFieldOfView(FMath::Lerp(CameraHomeFOV,CameraCloseFOV,FocusBlend));
 }
 if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)&&!H->BeautyMode&&!H->EntryMenu&&!H->OrderMode&&!H->Making&&!H->Pouring){float X,Y;if(PC->GetMousePosition(X,Y)){int32 W,VH;PC->GetViewportSize(W,VH);float S=FMath::Min(W/1200.f,VH/760.f);if(X>300*S&&X<895*S&&Y>120*S&&Y<625*S){float Best=35*S;int32 Found=-1;for(int32 I=0;I<BottleActors.Num();I++)if(BottleActors[I].IsValid()){FVector2D P;if(PC->ProjectWorldLocationToScreen(BottleActors[I]->GetActorLocation()+FVector(0,0,20),P)){float D=FVector2D::Distance(P,FVector2D(X,Y));if(D<Best){Best=D;Found=I;}}}if(Found>=0)H->AddMaterial(Found);}}}
 if(ActivePour&&ActiveBottle.IsValid()){
  float T=(GetWorld()->GetTimeSeconds()-PourStart)/1.2f;
  float Blend=T<.35f?T/.35f:T<.65f?1.f:(1.f-T)/.35f;Blend=FMath::Clamp(Blend,0.f,1.f);
  ActiveBottle->SetActorRelativeTransform(MixologyPresentation::PourTransform(ActiveHome,FVector(-49,12,123),ActiveBottle->GetActorScale3D(),32.5f,Blend));
  if(T>=1){ActiveBottle->SetActorRelativeLocation(ActiveHome);ActiveBottle->SetActorRelativeRotation(FRotator::ZeroRotator);ActiveBottle.Reset();ActivePour=false;H->Pouring=false;}
 }
 const float Now=GetWorld()->GetTimeSeconds();const EMixStage Stage=H->Sequence.Stage(Now);const float P=H->Sequence.StageProgress(Now);
 AActor* DirectBottle=nullptr;
 if(!ActivePour){
  int32 DirectIndex=-1;
  if(H->Making&&!H->Sequence.Shake)for(int32 I=0;I<6;I++)if(H->Catalog.Materials.IsValidIndex(I)&&H->PendingAmounts.FindRef(H->Catalog.Materials[I].Id)>0&&BottleActors[I].IsValid()){DirectIndex=I;break;}
  for(int32 I=0;I<BottleActors.Num();I++)if(BottleActors[I].IsValid()){
   float Blend=0;if(I==DirectIndex){DirectBottle=BottleActors[I].Get();if(Stage==EMixStage::Serving)Blend=FMath::SmoothStep(0.f,1.f,FMath::Min(P/.2f,1.f));else if(Stage==EMixStage::Mixing)Blend=1-FMath::SmoothStep(0.f,1.f,FMath::Min(P/.3f,1.f));else if(Stage==EMixStage::Finishing&&!H->Sequence.Stir)Blend=1-FMath::SmoothStep(0.f,1.f,P);}
   BottleActors[I]->SetActorRelativeTransform(MixologyPresentation::PourTransform(BottleHomes[I],FVector(-49,12,123),BottleActors[I]->GetActorScale3D(),32.5f,Blend));
  }
 }
 if(Shaker.IsValid()){
  FVector Pos=ShakerHome;FRotator Rot=FRotator::ZeroRotator;
  if(H->Making&&H->Sequence.Shake){
   if(Stage==EMixStage::Mixing){Pos+=FVector(0,FMath::Sin(Now*25)*4,12);Rot.Pitch=FMath::Sin(Now*25)*15;}
   else if(Stage==EMixStage::Serving||Stage==EMixStage::Finishing){float Blend=Stage==EMixStage::Serving?FMath::SmoothStep(0.f,1.f,FMath::Min(P/.2f,1.f)):1-FMath::SmoothStep(0.f,1.f,P);const FTransform Pose=MixologyPresentation::PourTransform(ShakerHome,FVector(-49,12,121),Shaker->GetActorScale3D(),17.f,Blend);Pos=Pose.GetLocation();Rot=Pose.Rotator();}
  }
  Shaker->SetActorRelativeLocation(Pos);Shaker->SetActorRelativeRotation(Rot);
  if(ShakerCap.IsValid()){
   FVector CapPos=Pos;FRotator CapRot=Rot;
   if(H->Making&&H->Sequence.Shake){
    if(Stage==EMixStage::Uncapping){CapPos=FMath::Lerp(ShakerHome,ShakerHome+FVector(0,-38,-27),FMath::SmoothStep(0.f,1.f,P));CapPos.Z+=FMath::Sin(P*PI)*26;}
    else if(Stage==EMixStage::Serving){CapPos=ShakerHome+FVector(0,-38,-27);CapRot=FRotator::ZeroRotator;}
    else if(Stage==EMixStage::Finishing){CapPos=FMath::Lerp(ShakerHome+FVector(0,-38,-27),ShakerHome,FMath::SmoothStep(0.f,1.f,P));CapPos.Z+=FMath::Sin(P*PI)*26;CapRot=FRotator::ZeroRotator;}
   }
   ShakerCap->SetActorRelativeLocation(CapPos);ShakerCap->SetActorRelativeRotation(CapRot);
  }
 }
 if(Spoon.IsValid()){
  if(H->Making&&H->Sequence.Stir&&Stage==EMixStage::Mixing){Spoon->SetActorRelativeLocation(FVector(-49+FMath::Cos(Now*9)*2,12+FMath::Sin(Now*9)*2,80));Spoon->SetActorRelativeRotation(FRotator(12,FMath::RadiansToDegrees(Now*9),0));}
  else{Spoon->SetActorRelativeLocation(FVector(-32,78,76));Spoon->SetActorRelativeRotation(FRotator::ZeroRotator);}
 }
 if(Liquid.IsValid()){
  const auto& DisplayAmounts=H->Making?H->PendingAmounts:H->OrderMode||H->Amounts.IsEmpty()?H->LastPreparedAmounts:H->Amounts;
  int32 Ml=MixologyPresentation::Volume(H->Catalog,DisplayAmounts);float Height=DrinkVisualV005?MixologyPresentation::LiquidHeight(Ml):FMath::Clamp(Ml/12.f,0.f,18.f);
  if(H->Making)Height*=H->Sequence.FillProgress(Now);
  Liquid->SetActorHiddenInGame(Height==0);Liquid->SetActorScale3D(DrinkVisualV005?FVector(.62f,.62f,FMath::Max(.002f,Height/100.f)):FVector(.102,.102,FMath::Max(.002f,Height/100.f)));Liquid->SetActorRelativeLocation(FVector(-49,12,DrinkVisualV005?77.426f:77.426f+Height/2));
  const FLinearColor Tint=MixologyPresentation::LiquidColor(H->Catalog,DisplayAmounts);
  const float Opacity=MixologyPresentation::LiquidOpacity(H->Catalog,DisplayAmounts);
  if(LiquidMaterial){LiquidMaterial->SetVectorParameterValue(TEXT("LiquidColor"),Tint);if(DrinkVisualV005)LiquidMaterial->SetScalarParameterValue(TEXT("LiquidOpacity"),Opacity);}
  if(LiquidSurface.IsValid()){LiquidSurface->SetActorHiddenInGame(Height==0);LiquidSurface->SetActorRelativeLocation(FVector(-49,12,77.426f+Height));if(SurfaceMaterial){SurfaceMaterial->SetVectorParameterValue(TEXT("LiquidColor"),Tint);SurfaceMaterial->SetScalarParameterValue(TEXT("LiquidOpacity"),FMath::Min(Opacity+.10f,.8f));}}
  if(Stream.IsValid()){
   const float ManualPhase=ActivePour?(Now-PourStart)/1.2f:0;
   bool ManualFlow=ActivePour&&ActiveBottle.IsValid()&&ManualPhase>.35f&&ManualPhase<.65f;
   bool Flowing=(H->Making&&Stage==EMixStage::Serving&&P>.2f&&P<.93f)||ManualFlow;Stream->SetActorHiddenInGame(!Flowing);
   if(Flowing){FVector Target(-49,12,77.426f+Height),Source=ManualFlow?ActiveBottle->GetRootComponent()->GetRelativeTransform().TransformPosition(FVector(0,0,32.5)):H->Sequence.Shake&&Shaker.IsValid()?Shaker->GetRootComponent()->GetRelativeTransform().TransformPosition(FVector(0,0,17)):DirectBottle?DirectBottle->GetRootComponent()->GetRelativeTransform().TransformPosition(FVector(0,0,32.5)):FVector(-49,12,134);FVector D=Target-Source;Stream->SetActorRelativeLocation((Source+Target)*.5f);Stream->SetActorRelativeRotation(FRotationMatrix::MakeFromZ(D).Rotator());Stream->SetActorScale3D(FVector(.006,.006,D.Size()/100.f));}
  }
 }
}
void AMixologyHUD::AddMaterial(int32 I){
 if(!Active||EntryMenu||Awaiting||!Ready||Making||Pouring||BeautyMode||OrderMode||!Catalog.Materials.IsValidIndex(I))return;
 auto& M=Catalog.Materials[I];if(M.Id==TEXT("ice")||M.Id==TEXT("citrus_garnish")){Notice=TEXT("冰和装饰暂未实现，不计入液体配方。");return;}
 Selection=I;int32 Old=Amounts.FindRef(M.Id);int32 Next=FMath::Min(Old+(M.Unit==TEXT("leaf")?2:5),M.Unit==TEXT("leaf")?20:200);if(Next==Old)return;Amounts.Add(M.Id,Next);
 if(Station)Pouring=Station->StartPour(I);Notice=M.Name+TEXT("已加入；确认制作前不会扣费。");
}

void AMixologyHUD::BeginPlay(){
 Super::BeginPlay();
 if(FParse::Param(FCommandLine::Get(),TEXT("LalalandPlayerArmsReview"))){SetActorTickEnabled(false);SetActorHiddenInGame(true);return;}
 FString Error;Ready=Catalog.Load(Error,TEXT("zh-CN"));if(!Ready)Notice=Error;
 if(RecordingDemo){Cash=18;Station=GetWorld()->SpawnActor<AMixologyStation>();Notice=Ready?TEXT("走近吧台，按 E 开始。") : Error;return;}
 Service=GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
 Service->OnCommandRejected.AddDynamic(this,&AMixologyHUD::Rejected);
 Service->OnCommandAcknowledged.AddDynamic(this,&AMixologyHUD::Acknowledged);
 Station=GetWorld()->SpawnActor<AMixologyStation>();
}
bool AMixologyHUD::CanEnter() const {
 if(RecordingDemo){auto PC=GetOwningPlayerController();auto P=PC?PC->GetPawn():nullptr;return Ready&&!Active&&P&&P->GetActorLocation().Z<140&&FVector::Dist2D(P->GetActorLocation(),FVector(0,225,0))<180;}
 if(!Service||!Ready||Active||!EntryCommandId.IsEmpty())return false;
 const auto& State=Service->GetState();
 auto PC=GetOwningPlayerController();auto Pawn=PC?PC->GetPawn():nullptr;
 if(!Pawn||State.sessionId.IsEmpty()||State.paused||State.intro.phase==TEXT("elevator")||Pawn->GetActorLocation().Z>140)return false;
 const auto& N=State.firstNight;
 if(N.phase!=TEXT("arrival")&&N.phase!=TEXT("free_time")&&N.phase!=TEXT("post_game"))return false;
 if(!N.pendingThrow.id.IsEmpty())return false;
 // Local movement can lead the authoritative position by a report interval.
 // Do not advertise an interaction until both positions pass the server gate.
 const auto* User=State.characters.FindByPredicate([](const auto& A){return A.id==TEXT("USER");});
 const auto* Bartender=State.characters.FindByPredicate([](const auto& A){return A.id==TEXT("BARTENDER");});
 if(!User||!Bartender)return false;
 const FVector ServerUser(User->x*100,User->z*100,User->y*100);
 const FVector ServerBartender(Bartender->x*100,Bartender->z*100,Bartender->y*100);
 const FVector Entry(-410,335,0);
 const bool ServerNear=(User->area==TEXT("bar")&&FMath::Abs(User->y)<.4&&FVector::Dist(ServerUser,Entry)<=135)
  ||FVector::Dist(ServerUser,ServerBartender)<=240;
 const bool LocalNear=FVector::Dist2D(Pawn->GetActorLocation(),Entry)<=135
  ||FVector::Dist2D(Pawn->GetActorLocation(),ServerBartender)<=240;
 return ServerNear&&LocalNear;
}
void AMixologyHUD::InteractAtBar(){
 if(RecordingDemo){if(CanEnter())OpenRecording();return;}
 if(!CanEnter())return;
 FLalalandCommandDto C;C.type=TEXT("craft_drink");C.intent=TEXT("open");
 EntryCommandId=Service->SendCommand(C);Notice=TEXT("正在打开吧台服务…");EntryFeedbackUntil=GetWorld()->GetTimeSeconds()+12;
 UE_LOG(LogTemp,Display,TEXT("NATIVE_BAR_ENTRY requested id=%s"),*EntryCommandId);
}
void AMixologyHUD::SelectEntry(bool IsOrder){
 if(!Active||Making||Awaiting)return;
 EntryMenu=false;OrderMode=IsOrder;
 Notice=IsOrder?TEXT("选择酒单饮品，或载入参考配方自己调。"):TEXT("已切到吧台内侧；选材料、调整用量，再确认制作。");
 UE_LOG(LogTemp,Display,TEXT("NATIVE_BAR_ENTRY selected mode=%s camera_inside=1"),IsOrder?TEXT("order"):TEXT("craft"));
}
void AMixologyHUD::Tick(float Dt){
 Super::Tick(Dt);if(RecordingDemo){TickRecording(Dt);return;}if(!Service||!Station)return;
 Cash=Service->GetState().firstNight.availableCash;
 if(FParse::Param(FCommandLine::Get(),TEXT("LalalandMixingSmoke")))RunSmoke();
 bool Open=Service->GetState().firstNight.craft.open&&Service->GetState().firstNight.catalogVersion==TEXT("lalaland-drink-catalog-v0.3");
 if(Open&&!Active){
  Active=true;EntryMenu=true;EntryCommandId.Empty();Making=false;Pouring=false;Awaiting=false;CompletionSent=false;BeautyMode=false;Amounts.Reset();PendingAmounts.Reset();CommandId.Empty();
  Notice=TEXT("自由选材或载入参考配方；演出完成后统一结算。");
  TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(),Widgets,ULalalandRootWidget::StaticClass(),true);
  MainWidgets.Reset();for(auto W:Widgets){MainWidgets.Add(W);W->SetVisibility(ESlateVisibility::Collapsed);}
  auto PC=GetOwningPlayerController();PreviousView=PC->GetViewTarget();PC->SetViewTargetWithBlend(Station->StudioCamera.Get(),.4f);
  FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);
  PC->SetShowMouseCursor(true);PC->bEnableClickEvents=true;PC->ClickEventKeys.AddUnique(EKeys::LeftMouseButton);
  if(auto Viewport=GetWorld()->GetGameViewport())Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
  UWidgetBlueprintLibrary::SetFocusToGameViewport();
  // Keep input alive for Enter/Escape/V and pointer events. Block movement/look
  // in the player handlers, not by removing its entire input component.
 }
 if(!Open&&Active){
  Active=false;EntryMenu=false;Making=false;Pouring=false;BeautyMode=false;Awaiting=false;
  Station->ActivePour=false;if(Station->ActiveBottle.IsValid()){Station->ActiveBottle->SetActorRelativeLocation(Station->ActiveHome);Station->ActiveBottle->SetActorRelativeRotation(FRotator::ZeroRotator);Station->ActiveBottle.Reset();}
  for(auto W:MainWidgets)if(W)W->SetVisibility(ESlateVisibility::SelfHitTestInvisible);MainWidgets.Reset();
  auto PC=GetOwningPlayerController();if(PreviousView.IsValid())PC->SetViewTargetWithBlend(PreviousView.Get(),.4f);
  FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);
  if(auto Viewport=GetWorld()->GetGameViewport())Viewport->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
  PC->SetShowMouseCursor(true);UWidgetBlueprintLibrary::SetFocusToGameViewport();
 }
 if(!Active){
  if(!EntryCommandId.IsEmpty()&&GetWorld()->GetTimeSeconds()>EntryFeedbackUntil){EntryCommandId.Empty();Notice=TEXT("入口确认超时，请重试；没有扣款。");EntryFeedbackUntil=GetWorld()->GetTimeSeconds()+4;}
  return;
 }
 if(Making&&!CompletionSent&&GetWorld()->GetTimeSeconds()>=CompletionTime){
  FLalalandCommandDto C;C.id=CommandId;C.type=TEXT("craft_drink");C.intent=TEXT("finish_mix");C.objectTarget=OrderMode?TEXT("order"):TEXT("craft");C.mixIngredients=PendingAmounts;
  CompletionSent=true;Awaiting=true;SentAt=FPlatformTime::Seconds();
  if(Service->SendCommand(C).IsEmpty()){CompletionSent=false;Awaiting=false;Making=false;Notice=TEXT("连接未就绪，没有本地扣款；可重新确认。");}
  else Notice=TEXT("制作完成，等待服务器确认扣款和杯子。");
 }
 if(Awaiting&&FPlatformTime::Seconds()-SentAt>12){Awaiting=false;Making=false;Notice=TEXT("确认超时；再次确认会使用同一交易编号，不重复扣款。");}
}
void AMixologyHUD::Acknowledged(const FString& Id,const FString& Reason){
 if(Id==EntryCommandId){EntryCommandId.Empty();}
 if(Id==CommandId){Awaiting=false;Making=false;LastPreparedAmounts=PendingAmounts;LastPreparedName=PendingName;Notice=TEXT("成品已同步酒吧，可自饮或先询问赠饮。");CommandId.Empty();}
}
void AMixologyHUD::Rejected(const FString& Id,const FString& Reason){
 if(Id==EntryCommandId){EntryCommandId.Empty();Notice=Reason;EntryFeedbackUntil=GetWorld()->GetTimeSeconds()+5;UE_LOG(LogTemp,Warning,TEXT("NATIVE_BAR_ENTRY rejected reason=%s"),*Reason);}
 if(Id==CommandId){Awaiting=false;Making=false;CompletionSent=false;CommandId.Empty();Notice=Reason;}
}
void AMixologyHUD::Confirm(){
 if(!Active||EntryMenu||!Ready||Making||Pouring||BeautyMode||Awaiting)return;
 const FMixRecipe* R=OrderMode?(Catalog.Recipes.IsValidIndex(MenuIndex)?&Catalog.Recipes[MenuIndex]:nullptr):Catalog.Resolve(Amounts);
 if(!R){Notice=TEXT("暂未匹配现有配方，没有扣款。");return;}
 int Cost=OrderMode?R->OrderCost:R->CraftCost;
 if(!RecordingDemo&&OrderMode&&Service->GetState().firstNight.voucherCount>0)Cost=0;
 if(Cost<0||Cash<Cost){Notice=TEXT("Cash 不足或这款不能直接点单。");return;}
 if(CommandId.IsEmpty()){
  CommandId=FGuid::NewGuid().ToString(EGuidFormats::Digits);PendingAmounts=R->Amounts;PendingName=R->Name;
  if(RecordingDemo){RecordingCost=Cost;RecordingRecipeId=R->Id;}
  Sequence.Begin(R->Method,GetWorld()->GetTimeSeconds());Making=true;CompletionSent=false;CompletionTime=Sequence.Start+Sequence.Duration();
 }else{
  // Uncertain acknowledgement retries exactly the original immutable transaction.
  FLalalandCommandDto C;C.id=CommandId;C.type=TEXT("craft_drink");C.intent=TEXT("finish_mix");C.objectTarget=OrderMode?TEXT("order"):TEXT("craft");C.mixIngredients=PendingAmounts;
  Awaiting=true;SentAt=FPlatformTime::Seconds();Service->SendCommand(C);
 }
}
void AMixologyHUD::ReturnToBar(){
 if(!Active||Making||Pouring||Awaiting||!CommandId.IsEmpty())return;
 if(RecordingDemo){
  Active=false;EntryMenu=false;BeautyMode=false;
  auto PC=GetOwningPlayerController();if(PreviousView.IsValid())PC->SetViewTargetWithBlend(PreviousView.Get(),.25f);
  PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;PC->bEnableClickEvents=false;
  if(auto V=GetWorld()->GetGameViewport()){V->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);V->SetMouseLockMode(EMouseLockMode::LockOnCapture);}
  UWidgetBlueprintLibrary::SetFocusToGameViewport();
  UE_LOG(LogTemp,Display,TEXT("RECORDING_RETURN active=0 cursor=0"));return;
 }
 FLalalandCommandDto C;C.type=TEXT("craft_drink");C.intent=TEXT("cancel");Service->SendCommand(C);
}
void AMixologyHUD::ResetEvening(){ReturnToBar();}
void AMixologyHUD::RunSmoke(){
#if !UE_BUILD_SHIPPING
 const auto& State=Service->GetState();float Now=GetWorld()->GetTimeSeconds();
 auto Send=[&](const TCHAR* Type,const TCHAR* Intent=TEXT(""),const FString& Target=FString(),const FString& Object=FString()){
  FLalalandCommandDto C;C.type=Type;C.intent=Intent;C.target=Target;C.objectTarget=Object;Service->SendCommand(C);
 };
 auto Shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("MixingReview"),Name),true,false);};
 auto Click=[&](float X,float Y){
  int32 W,VH;GetOwningPlayerController()->GetViewportSize(W,VH);
  const float Scale=FMath::Min(W/1200.f,VH/760.f);
  return UpdateAndDispatchHitBoxClickEvents(FVector2D(X*Scale,Y*Scale),IE_Pressed);
 };
 auto SendKey=[&](FKey K){auto PC=GetOwningPlayerController();PC->InputKey(FInputKeyParams(K,IE_Pressed,1.));PC->InputKey(FInputKeyParams(K,IE_Released,0.));};
 if(Now>120&&SmokePhase!=11){UE_LOG(LogTemp,Error,TEXT("NATIVE_MIX_SMOKE FAILED timeout phase=%d"),SmokePhase);SmokePhase=11;FPlatformMisc::RequestExit(false);return;}
 if(SmokePhase==0&&!State.sessionId.IsEmpty()&&State.intro.phase==TEXT("bar")){
  // The new screen intentionally hides the bartender from the elevator.
  // Use an ordinary location route first; do not bypass visibility or teleport.
  FLalalandCommandDto C;C.type=TEXT("move_to");C.location=TEXT("bar");Service->SendCommand(C);
  SmokePhase=1;SmokeAt=Now;
 }
 else if(SmokePhase==1){
  const FLalalandActorDto *User=nullptr,*Bartender=nullptr;
  for(const auto& A:State.characters){if(A.id==TEXT("USER"))User=&A;if(A.id==TEXT("BARTENDER"))Bartender=&A;}
  if(User&&User->area==TEXT("bar")&&User->route.IsEmpty()&&!CanEnter()){
   if(auto Pawn=GetOwningPlayerController()->GetPawn()){
    FVector Delta=FVector(-480,245,Pawn->GetActorLocation().Z)-Pawn->GetActorLocation();
    if(Delta.Size2D()>12)Pawn->AddMovementInput(Delta.GetSafeNormal2D(),1.f);
   }
  }
  if(User&&Bartender&&CanEnter()){
   TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(),Widgets,ULalalandRootWidget::StaticClass(),true);
   bool Handled=false;const FKeyEvent Key(EKeys::E,FModifierKeysState(),0,false,69,69);
   for(auto W:Widgets)if(auto Root=Cast<ULalalandRootWidget>(W))Handled|=Root->NativeOnPreviewKeyDown(FGeometry(),Key).IsEventHandled();
   UE_LOG(LogTemp,Display,TEXT("NATIVE_BAR_E_ROUTE pass=%d"),Handled&&!EntryCommandId.IsEmpty());
   if(!Handled){UE_LOG(LogTemp,Error,TEXT("NATIVE_BAR_E_ROUTE FAILED"));FPlatformMisc::RequestExit(false);return;}
   SmokePhase=2;SmokeAt=Now;
  }
 }
 else if(SmokePhase==2&&Active&&Now-SmokeAt>1){
  UE_LOG(LogTemp,Display,TEXT("NATIVE_BAR_ENTRY_MENU pass=%d"),EntryMenu);Shot(TEXT("00_BarEntry.png"));SmokePhase=20;SmokeAt=Now;
 }
 else if(SmokePhase==20&&Now-SmokeAt>.8){
  const bool Hit=Click(599,381);
  UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_POINTER_MENU pass=%d"),Hit&&OrderMode&&!EntryMenu);
  if(!Hit||!OrderMode){UE_LOG(LogTemp,Error,TEXT("NATIVE_MIX_POINTER_MENU FAILED"));FPlatformMisc::RequestExit(false);return;}
  SmokePhase=201;SmokeAt=Now;
 }
 else if(SmokePhase==201&&Now-SmokeAt>.4){
  const bool Hit=Click(217,502);UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_POINTER_NEXT pass=%d"),Hit&&MenuIndex==1);
  if(!Hit||MenuIndex!=1){UE_LOG(LogTemp,Error,TEXT("NATIVE_MIX_POINTER_NEXT FAILED"));FPlatformMisc::RequestExit(false);return;}
  SmokePhase=202;SmokeAt=Now;
 }
 else if(SmokePhase==202&&Now-SmokeAt>.4){
  const bool Hit=Click(92,502);UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_POINTER_PREV pass=%d"),Hit&&MenuIndex==0);
  if(!Hit||MenuIndex!=0){UE_LOG(LogTemp,Error,TEXT("NATIVE_MIX_POINTER_PREV FAILED"));FPlatformMisc::RequestExit(false);return;}
  SmokePhase=203;SmokeAt=Now;
 }
 else if(SmokePhase==203&&Now-SmokeAt>.4){
  const bool Hit=Click(156,556);UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_POINTER_GUIDE pass=%d"),Hit&&!OrderMode&&Amounts.FindRef(TEXT("gin"))==50);
  if(!Hit||OrderMode){UE_LOG(LogTemp,Error,TEXT("NATIVE_MIX_POINTER_GUIDE FAILED"));FPlatformMisc::RequestExit(false);return;}
  SmokePhase=21;SmokeAt=Now;
 }
 else if(SmokePhase==21&&Now-SmokeAt>.8){
  const bool Hit=Click(270,657);UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_POINTER_CLEAR pass=%d"),Hit&&Amounts.IsEmpty());
  Shot(TEXT("01_BarStation.png"));AddMaterial(0);AddMaterial(0);
  UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_ADD_LOCK pass=%d"),Amounts.FindRef(TEXT("gin"))==5&&Pouring&&Cash==18);SmokeAt=Now;SmokePhase=3;
 }
 else if(SmokePhase==3&&!Pouring&&Now-SmokeAt>1.6){
  Amounts={{TEXT("bourbon"),45},{TEXT("lemon_juice"),25},{TEXT("simple_syrup"),10}};SendKey(EKeys::Enter);SmokePhase=4;
 }
 else if(SmokePhase==4&&Making&&Now-Sequence.Start>.5){UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_KEY_CONFIRM pass=%d"),Cash==18);Confirm();Shot(TEXT("02_Shaking.png"));SmokePhase=5;}
 else if(SmokePhase==5&&Making&&Now-Sequence.Start>1.6){Shot(TEXT("03_Uncapping.png"));SmokePhase=6;}
 else if(SmokePhase==6&&Making&&Now-Sequence.Start>2.4){Shot(TEXT("04_Pouring.png"));SmokePhase=67;}
 else if(SmokePhase==67&&Making&&Now-Sequence.Start>3.3){Shot(TEXT("06_FilledGlass.png"));SmokePhase=7;}
 else if(SmokePhase==7&&!Active&&Cash==11){
  SmokePhase=70;SmokeAt=Now;
 }
 else if(SmokePhase==70&&Now-SmokeAt>.8f){
  const auto Cup=State.firstNight.drinks.FindByPredicate([](const auto& D){return D.owner==TEXT("USER")&&D.status==TEXT("served")&&D.id.StartsWith(TEXT("whiskey_sour"));});
  if(Cup&&Cup->propConfirmed){
   UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_CUP_READY pass=1 cash=%d id=%s camera_restored=%d"),Cash,*Cup->instanceId,GetOwningPlayerController()->GetViewTarget()==PreviousView.Get());
   Shot(TEXT("05_BackInBar.png"));Send(TEXT("consume_drink"),TEXT(""),TEXT(""),Cup->instanceId);SmokePhase=8;
  }
 }
 else if(SmokePhase==8){
  if(State.firstNight.drinks.ContainsByPredicate([](const auto& D){return D.owner==TEXT("USER")&&D.status==TEXT("consumed")&&D.id.StartsWith(TEXT("whiskey_sour"));})){
   UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_SIP_EFFECT pass=1"));Send(TEXT("craft_drink"),TEXT("open"));SmokePhase=9;
  }
 }
 else if(SmokePhase==9&&Active&&EntryMenu){SendKey(EKeys::Escape);SmokePhase=10;}
 else if(SmokePhase==10&&!Active){
  UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_SMOKE SUCCESS cash=%d cancel_no_charge=%d"),Cash,Cash==11);SmokePhase=11;SmokeAt=Now;
 }
 else if(SmokePhase==11&&Now-SmokeAt>3)FPlatformMisc::RequestExit(false);
#endif
}
void AMixologyHUD::NotifyHitBoxClick(FName Name){
 UE_LOG(LogTemp,Display,TEXT("NATIVE_MIX_POINTER id=%s active=%d entry=%d ready=%d making=%d pouring=%d awaiting=%d transaction=%d"),*Name.ToString(),Active,EntryMenu,Ready,Making,Pouring,Awaiting,!CommandId.IsEmpty());
 if(Active&&EntryMenu){if(Name==TEXT("entry_order"))SelectEntry(true);else if(Name==TEXT("entry_craft"))SelectEntry(false);else if(Name==TEXT("reset"))ReturnToBar();return;}
 if(!Active&&Name==TEXT("bar_entry")){InteractAtBar();return;}
 const FString Id=Name.ToString();if(Id==TEXT("view")){BeautyMode=!BeautyMode;return;}
 if(!Active||Awaiting||!CommandId.IsEmpty()||Making||Pouring||BeautyMode||!Ready){if(Id==TEXT("confirm")&&!Awaiting&&!Making)Confirm();return;}
 if(Id==TEXT("confirm")){Confirm();return;}if(Id==TEXT("reset")){ResetEvening();return;}if(Id==TEXT("clear")){Amounts.Reset();return;}
 if(Id==TEXT("mode")){OrderMode=!OrderMode;return;}if(Id==TEXT("prev")){MenuIndex=(MenuIndex+17)%18;return;}if(Id==TEXT("next")){MenuIndex=(MenuIndex+1)%18;return;}
 if(Id==TEXT("guide")&&Catalog.Recipes.IsValidIndex(MenuIndex)){Amounts=Catalog.Recipes[MenuIndex].Amounts;OrderMode=false;Notice=TEXT("已载入参考配方，可调整或确认；目前没有扣费。");return;}
 FString Prefix,Index;if(!Id.Split(TEXT("_"),&Prefix,&Index))return;int32 I=FCString::Atoi(*Index);if(!Catalog.Materials.IsValidIndex(I))return;auto& M=Catalog.Materials[I];int32 Step=M.Unit==TEXT("leaf")?2:5;
 if(Prefix==TEXT("add")){AddMaterial(I);return;}
 if(M.Id==TEXT("ice")||M.Id==TEXT("citrus_garnish"))return;
 Selection=I;int32 Delta=Prefix==TEXT("minus")?-Step:Step;Amounts.FindOrAdd(M.Id)=FMath::Clamp(Amounts.FindRef(M.Id)+Delta,0,M.Unit==TEXT("leaf")?20:200);if(Amounts.FindRef(M.Id)==0)Amounts.Remove(M.Id);
}
void AMixologyHUD::Button(const FString& Id,const FString& Text,float X,float Y,float W,float H,bool Enabled){
 const float S=FMath::Min(Canvas->SizeX/1200.f,Canvas->SizeY/760.f);float MX=0,MY=0;bool Hover=false;
 if(auto PC=GetOwningPlayerController())if(PC->GetMousePosition(MX,MY))Hover=MX>=X*S&&MX<=(X+W)*S&&MY>=Y*S&&MY<=(Y+H)*S;
 bool Selected=Id==FString::Printf(TEXT("add_%d"),Selection)&&!OrderMode;
 DrawRect(!Enabled?FLinearColor(.065f,.07f,.075f,.9f):Hover||Selected?FLinearColor(.21f,.16f,.105f,.96f):FLinearColor(.09f,.105f,.11f,.96f),X,Y,W,H);
 if(Selected)DrawRect(FLinearColor(.72f,.47f,.22f,1),X,Y,2,H);
 Write(Text,Enabled?FLinearColor(.92f,.86f,.74f):FLinearColor(.39f,.43f,.44f),X+9,Y+(H-15)/2,.85f);
 if(Enabled)AddHitBox(FVector2D(X*S,Y*S),FVector2D(W*S,H*S),FName(*Id),true);
}
void AMixologyHUD::Write(const FString& Text,FLinearColor Color,float X,float Y,float Scale){
 if(!Canvas)return;
 // Canvas requires a non-null runtime UFont even when a Slate font file is supplied.
 if(!RuntimeFont){
  RuntimeFont=NewObject<UFont>(this);RuntimeFont->FontCacheType=EFontCacheType::Runtime;
  RuntimeFont->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Emplace(TEXT("Regular"),FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansFallback.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
 }
 const FSlateFontInfo Info(RuntimeFont.Get(),FMath::Max(8,FMath::RoundToInt(13*Scale)),TEXT("Regular"));
 FCanvasTextItem Item(FVector2D(X,Y),FText::FromString(Text),Info,Color);
 Item.Font=RuntimeFont;
 Canvas->DrawItem(Item);
}
void AMixologyHUD::DrawHUD(){
 Super::DrawHUD();if(!Canvas)return;
 if(!Active){
  if(RecordingDemo){Write(TEXT("WASD 移动 · 鼠标视角 · 靠近吧台 E · F 自饮 · R 重置试玩预算"),FLinearColor(.95f,.87f,.75f),24,24,.85f);Write(FString::Printf(TEXT("本地试玩 / %d Cash / %s"),Cash,*Notice),FLinearColor(.9f,.8f,.66f),24,Canvas->SizeY-40,.8f);}
  if(CanEnter()||!EntryCommandId.IsEmpty()||GetWorld()->GetTimeSeconds()<EntryFeedbackUntil){
   const float S=FMath::Min(Canvas->SizeX/1200.f,Canvas->SizeY/760.f);Canvas->Canvas->PushAbsoluteTransform(FScaleMatrix(FVector(S,S,1)));
   Button(TEXT("bar_entry"),EntryCommandId.IsEmpty()?TEXT("E / 吧台：点单 · 自己调酒"):TEXT("正在打开吧台服务…"),420,100,360,42,CanEnter());
   if(GetWorld()->GetTimeSeconds()<EntryFeedbackUntil)Write(Notice,FLinearColor(.9f,.75f,.5f),420,149,.95f);
   Canvas->Canvas->PopTransform();
  }
  return;
 }
 const float S=FMath::Min(Canvas->SizeX/1200.f,Canvas->SizeY/760.f);Canvas->Canvas->PushAbsoluteTransform(FScaleMatrix(FVector(S,S,1)));
 const FLinearColor Gold(.77f,.55f,.28f),Ink(.023f,.031f,.035f,.95f),White(.91f,.89f,.83f),Muted(.50f,.58f,.59f);
 const bool Busy=Making||Pouring||Awaiting||!CommandId.IsEmpty();
 auto Text=[&](const FString& T,float X,float Y,float Scale=1.f){Write(T,White,X,Y,Scale);};
 auto Panel=[&](float X,float Y,float W,float H){DrawRect(Ink,X,Y,W,H);DrawRect(Gold,X,Y,W,1);};
 if(EntryMenu){
  Panel(360,250,480,245);Text(TEXT("今晚想喝点什么？"),389,279,1.45f);
  Write(TEXT("点单，或到吧台内侧亲手调一杯。"),Muted,389,317,1.f);
  Button(TEXT("entry_order"),TEXT("浏览酒单 / 点单"),389,360,420,42);
  Button(TEXT("entry_craft"),TEXT("自己调酒 / 选材料"),389,414,280,42);
  Button(TEXT("reset"),TEXT("返回酒吧"),683,414,126,42);
  Canvas->Canvas->PopTransform();return;
 }
 if(Making||BeautyMode){
  const float Now=GetWorld()->GetTimeSeconds();
  Panel(24,24,300,45);Text(TEXT("LALA-LAND  /  调酒近景"),40,36,1.f);
  Panel(970,24,206,45);Write(FString::Printf(TEXT("当晚预算   %02d Cash"),Cash),Gold,984,36,.9f);
  Panel(24,666,1152,70);Text(Making?Sequence.Label(Now):LastPreparedName.IsEmpty()?TEXT("调酒台近景"):TEXT("成品  /  ")+LastPreparedName,40,679,1.1f);
  if(Making){float Progress=FMath::Clamp((Now-Sequence.Start)/Sequence.Duration(),0.f,1.f);DrawRect(FLinearColor(.13f,.17f,.18f),40,710,1136,3);DrawRect(Gold,40,710,1136*Progress,3);Write(TEXT("制作中  /  完成后自动返回操作界面"),Muted,40,718,.72f);}
  else{Write(TEXT("展示模式不扣费、不改变配方  /  按 V 返回操作界面"),Muted,40,709,.8f);Button(TEXT("view"),TEXT("返回操作"),1038,699,122,27,true);}
  Canvas->Canvas->PopTransform();return;
 }
 Panel(24,24,1152,67);Text(TEXT("LALA-LAND  /  调酒工作室"),40,38,1.2f);
 Write(TEXT("酒吧原位调配  /  统一预算与成品"),Muted,40,69,.8f);
 Write(FString::Printf(TEXT("当晚预算   %02d Cash"),Cash),Gold,900,45,1.f);
 Panel(24,120,265,503);Text(OrderMode?TEXT("01  /  酒单"):TEXT("01  /  材料"),38,135,.9f);
 const FMixRecipe* R=OrderMode?(Catalog.Recipes.IsValidIndex(MenuIndex)?&Catalog.Recipes[MenuIndex]:nullptr):Catalog.Resolve(Amounts);
 if(!OrderMode){
  Write(TEXT("点击加入材料，演出完成后统一结算。"),Muted,38,160,.72f);
  for(int32 I=0;I<Catalog.Materials.Num();I++){
   const auto& M=Catalog.Materials[I];bool Reserved=M.Id==TEXT("ice")||M.Id==TEXT("citrus_garnish");
   FString Label=M.Name.Left(25)+(Reserved?TEXT(" *"):TEXT(""));
   Button(FString::Printf(TEXT("add_%d"),I),Label,36,187+I*25,241,23,!Busy&&Ready);
  }
  Write(TEXT("* 冰与装饰暂未实现"),Muted,38,601,.72f);
 }else if(R){
  FString Base,Variant;if(!R->Name.Split(TEXT(" / "),&Base,&Variant))Base=R->Name;
  Text(Base.Left(28),38,183,1.15f);Write(Variant.Left(32),Gold,38,213,.85f);
  Text(ChineseMethod(R->Method),38,245,.8f);Text(TEXT("参考配方"),38,282,.9f);
  int32 Y=310;for(const auto& M:Catalog.Materials)if(int32 Amount=R->Amounts.FindRef(M.Id)){Text(FString::Printf(TEXT("%s  %d %s"),*M.Name.Left(22),Amount,*DisplayUnit(M.Unit)),38,Y,.8f);Y+=26;}
  Button(TEXT("prev"),TEXT("< 上一款"),36,485,113,34,!Busy);Button(TEXT("next"),TEXT("下一款 >"),158,485,119,34,!Busy);
  Button(TEXT("guide"),TEXT("载入配方，自己调"),36,536,241,40,!Busy);
  Write(TEXT("酒单仅展示常规款；隐藏款需自己调。"),Muted,38,595,.72f);
 }
 Panel(906,120,270,204);Text(TEXT("02  /  调整用量"),920,136,.85f);
 if(Catalog.Materials.IsValidIndex(Selection)&&!OrderMode){
  const auto& M=Catalog.Materials[Selection];Text(M.Name.Left(29),920,168,1.f);
  bool Reserved=M.Id==TEXT("ice")||M.Id==TEXT("citrus_garnish");
  Write(FString::Printf(TEXT("%d %s"),Amounts.FindRef(M.Id),*DisplayUnit(M.Unit)),Gold,920,199,1.4f);
  FString Step=M.Unit==TEXT("leaf")?TEXT("2 片"):TEXT("5 ml");
  Button(FString::Printf(TEXT("minus_%d"),Selection),TEXT("- ")+Step,920,246,116,32,!Busy&&!Reserved);
  Button(FString::Printf(TEXT("plus_%d"),Selection),TEXT("+ ")+Step,1044,246,118,32,!Busy&&!Reserved);
  Write(FString::Printf(TEXT("材料计价：%.0f Cash / 配方份"),M.Cost),Muted,920,298,.73f);
 }else{Text(TEXT("选择酒单中的饮品。"),920,178,.85f);Text(TEXT("也可以载入配方，自己调配。"),920,205,.85f);}
 Panel(906,341,270,282);Text(TEXT("03  /  当前配方"),920,356,.9f);
 const auto& Mix=OrderMode&&R?R->Amounts:Amounts;int32 Y=385,Count=0;bool Alcohol=false;
 for(const auto& M:Catalog.Materials)if(int32 Amount=Mix.FindRef(M.Id)){Alcohol|=M.Alcoholic;if(Count<6){Text(FString::Printf(TEXT("%s  %d %s"),*M.Name.Left(22),Amount,*DisplayUnit(M.Unit)),920,Y,.78f);Y+=22;}Count++;}
 if(Count==0)Write(TEXT("还没有加入材料。"),Muted,920,390,.85f);
 if(Count>6)Write(FString::Printf(TEXT("另有 %d 种材料"),Count-6),Muted,920,520,.76f);
 Write(FString::Printf(TEXT("%d ml  /  %s"),MixologyPresentation::Volume(Catalog,Mix),Alcohol?TEXT("含酒精"):TEXT("无酒精")),Gold,920,548,.8f);
 FString Result=R?R->Name:TEXT("暂未匹配现有配方");Text(Result.Left(32),920,576,.8f);
 Write(R?ChineseMethod(R->Method):TEXT("未匹配的组合不会扣费。"),Muted,920,601,.72f);
 // Keep the 3D work surface unobscured by the ingredient panels.
 Write(TEXT("你的调酒台"),Gold,456,125,.9f);
 Write(TEXT("点击瓶子或从左侧选择材料"),Muted,367,152,.8f);
 if(Busy){
  DrawRect(Ink,385,544,430,65);Text(Pouring?TEXT("加入材料  /  酒瓶正在归位"):Sequence.Label(GetWorld()->GetTimeSeconds()),402,557,.85f);
  float Progress=Making?(GetWorld()->GetTimeSeconds()-Sequence.Start)/Sequence.Duration():0;
  if(Pouring)if(auto G=Station.Get())Progress=(GetWorld()->GetTimeSeconds()-G->PourStart)/1.2f;
  DrawRect(FLinearColor(.13f,.17f,.18f),402,589,396,3);DrawRect(Gold,402,589,396*FMath::Clamp(Progress,0.f,1.f),3);
 }else if(!LastPreparedName.IsEmpty()){DrawRect(Ink,355,552,490,52);Text(TEXT("成品托盘  /  ")+LastPreparedName.Left(44),372,566,.85f);}
 const int32 Cost=R?(!RecordingDemo&&OrderMode&&Service&&Service->GetState().firstNight.voucherCount>0?0:OrderMode?R->OrderCost:R->CraftCost):Amounts.Num()?Catalog.Quote(Amounts):0;
 Button(TEXT("mode"),OrderMode?TEXT("自己调酒"):TEXT("浏览酒单"),24,638,181,38,!Busy&&Ready);
 Button(TEXT("clear"),TEXT("清空配料"),213,638,115,38,!Busy&&!OrderMode);
 Button(TEXT("confirm"),FString::Printf(TEXT("%s  /  %d Cash"),OrderMode?TEXT("直接点单"):TEXT("确认制作"),Cost),906,638,178,38,!Busy&&Ready&&R&&Cost>=0&&Cash>=Cost);
 Button(TEXT("reset"),TEXT("返回"),1092,638,84,38,!Busy);
 Write(Notice.Left(130),Gold,32,696,.83f);
 if(!CommandId.IsEmpty()&&!Awaiting&&!Making)Button(TEXT("confirm"),TEXT("重试结算（不重复扣款）"),600,638,295,38,true);
 Write(RecordingDemo?TEXT("本地试玩 / Enter 确认 / Esc 返回 / V 近景 / 完成后回到酒吧，按 F 自饮"):TEXT("精确配方  /  Enter 确认  /  Esc 返回  /  V 近景  /  完成后回到酒吧，自饮或先询问赠饮"),Muted,32,727,.75f);
 Canvas->Canvas->PopTransform();
}
