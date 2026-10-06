#include "LalalandPlayerArmsComponent.h"
#include "LalalandGlassProp.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ULalalandPlayerArmsComponent::ULalalandPlayerArmsComponent()
{ PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bStartWithTickEnabled=false;PrimaryComponentTick.TickGroup=TG_PostUpdateWork; }

bool ULalalandPlayerArmsComponent::InitializeArms(UCameraComponent* Camera)
{
    if(Arms)return true;
    if(!Camera)return false;
    // Explicit review opt-in; ordinary gameplay remains on the prior candidate.
    if(FParse::Param(FCommandLine::Get(),TEXT("PlayerArmsV025")))AssetVersion=TEXT("v025");
    if(FParse::Param(FCommandLine::Get(),TEXT("PlayerArmsV026")))AssetVersion=TEXT("v026");
    FString Text;TSharedPtr<FJsonObject> Data;
    const FString Root=TEXT("Characters/PLAYER_")+AssetVersion;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/Root/TEXT("grip.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Data))return false;
    auto Mesh=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/")+Root+TEXT("/SK_PlayerArms_")+AssetVersion+TEXT(".SK_PlayerArms_")+AssetVersion));if(!Mesh)return false;
    auto P=Data->GetArrayField(TEXT("grip_translation"));auto Q=Data->GetArrayField(TEXT("grip_rotation"));
    auto R=Data->GetArrayField(TEXT("reach_mesh_cm"));ReachMesh=FVector(R[0]->AsNumber(),R[1]->AsNumber(),R[2]->AsNumber());
    Grip=FTransform(FQuat(Q[0]->AsNumber(),Q[1]->AsNumber(),Q[2]->AsNumber(),Q[3]->AsNumber()),FVector(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber()));
    Arms=NewObject<USkeletalMeshComponent>(GetOwner(),TEXT("FirstPersonArms_v024"));GetOwner()->AddInstanceComponent(Arms);
    Arms->SetupAttachment(Camera);Arms->SetSkeletalMesh(Mesh);Arms->SetCollisionEnabled(ECollisionEnabled::NoCollision);Arms->SetCastShadow(false);
    Arms->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Arms->SetRelativeLocation(FVector(0,0,-148));Arms->SetRelativeRotation(FRotator(0,-90,0));Arms->SetRelativeScale3D(FVector(Data->GetNumberField(TEXT("mesh_scale"))));
    Arms->bComponentUseFixedSkelBounds=true;Arms->SetHiddenInGame(false);Arms->SetRenderInMainPass(true);
    Arms->RegisterComponent();Arms->SetVisibility(false);SetComponentTickEnabled(true);
    return true;
}

bool ULalalandPlayerArmsComponent::BeginCupAction(ALalalandGlassProp* Cup,const FTransform& RestWorld,const FString& Action)
{
    if(!Arms||!IsValid(Cup)||!bFinished)return false;
    if(Action!=TEXT("Pickup")&&Action!=TEXT("Hold")&&Action!=TEXT("Sip")&&Action!=TEXT("Putdown"))return false;
    if(Action!=TEXT("Pickup")&&Glass!=Cup)return false;
    if(Action==TEXT("Pickup")&&!CurrentAction.IsEmpty())return false;
    // This baked candidate has one calibrated reach, not arbitrary runtime IK.
    // Reject an unsupported support instead of silently teleporting the cup.
    if((Action==TEXT("Pickup")||Action==TEXT("Putdown"))&&FVector::Dist(RestWorld.GetLocation(),Arms->GetComponentTransform().TransformPosition(ReachMesh))>2.f)
    {UE_LOG(LogTemp,Warning,TEXT("PLAYER_ARMS_V024_UNSUPPORTED_REACH: align the interaction camera or provide a target-aware IK variant"));return false;}
    auto Seq=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/Characters/PLAYER_")+AssetVersion+TEXT("/AN_Player")+Action+TEXT("_")+AssetVersion+TEXT(".AN_Player")+Action+TEXT("_")+AssetVersion));if(!Seq)return false;
    Glass=Cup;Rest=RestWorld;CurrentAction=Action;Elapsed=0;Duration=Seq->GetPlayLength();bFinished=false;
    Arms->SetVisibility(true);Arms->PlayAnimation(Seq,false);Arms->GetSingleNodeInstance()->SetPlaying(false);UpdateCup();return true;
}

void ULalalandPlayerArmsComponent::UpdateCup()
{
    if(!Arms||!IsValid(Glass))return;
    auto Instance=Arms->GetSingleNodeInstance();if(!Instance)return;
    Instance->SetPosition(FMath::Min(Elapsed,Duration),false);Arms->TickAnimation(0,false);Arms->RefreshBoneTransforms();
    const float T=Elapsed/FMath::Max(.001f,Duration);
    const bool Attached=!(CurrentAction==TEXT("Pickup")&&T<.45f)&&!(CurrentAction==TEXT("Putdown")&&T>=.58f);
    if(Attached){auto Pose=Grip*Arms->GetSocketTransform(TEXT("hand_r"));Glass->SetActorLocationAndRotation(Pose.GetLocation(),Pose.GetRotation());Glass->SetActorScale3D(FVector(1));}
    else Glass->SetActorTransform(Rest);
    bAttached=Attached;
}

void ULalalandPlayerArmsComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Fn)
{
    Super::TickComponent(Dt,Type,Fn);
    if(CurrentAction.IsEmpty())return;
    if(!IsValid(Glass)){CancelCupAction();return;}
    Elapsed=FMath::Min(Duration,Elapsed+Dt);UpdateCup();
    if(Elapsed>=Duration&&!bFinished){bFinished=true;if(CurrentAction==TEXT("Putdown"))CancelCupAction();}
}

void ULalalandPlayerArmsComponent::CancelCupAction()
{
    // Put back on the caller-supplied support, including interrupted sip.
    if(IsValid(Glass))Glass->SetActorTransform(Rest);
    Glass=nullptr;CurrentAction.Reset();bAttached=false;bFinished=true;
    if(Arms){Arms->SetVisibility(false);Arms->Stop();}
}
void ULalalandPlayerArmsComponent::EndPlay(const EEndPlayReason::Type Reason)
{ CancelCupAction();if(Arms)Arms->DestroyComponent();Super::EndPlay(Reason); }
