#include "LalalandNpcCharacter.h"
#include "LalalandLocomotionSteering.h"
#include "LalalandServiceSubsystem.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace1D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Modules/ModuleManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ALalalandNpcCharacter::ALalalandNpcCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->MaxWalkSpeed = 160.f;
    GetCharacterMovement()->MaxAcceleration = 520.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 800.f;
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 140.f;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(GetCapsuleComponent());
    PlaceholderBody->SetRelativeLocation(FVector::ZeroVector);
    PlaceholderBody->SetRelativeScale3D(FVector(.42f, .42f, .82f));
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaceholderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (PlaceholderMesh.Succeeded()) PlaceholderBody->SetStaticMesh(PlaceholderMesh.Object);

    NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
    NameLabel->SetupAttachment(GetCapsuleComponent());
    NameLabel->SetRelativeLocation(FVector(0, 0, 125.f));
    NameLabel->SetHorizontalAlignment(EHTA_Center);
    NameLabel->SetWorldSize(18.f);
    NameLabel->SetTextRenderColor(FColor::White);
    // Identity is revealed through witnessed introductions and the interaction
    // UI. Floating debug letters made the scene look unfinished and leaked IDs.
    NameLabel->SetVisibility(false);

    BubbleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BubbleText"));
    // Placeholder actors (the bartender) do not have a skeletal mesh. Keep a
    // safe capsule anchor until an A-D head bone is available.
    BubbleText->SetupAttachment(GetCapsuleComponent());
    BubbleText->SetRelativeLocation(FVector(0, 0, 160.f));
    BubbleText->SetHorizontalAlignment(EHTA_Center);
    BubbleText->SetVerticalAlignment(EVRTA_TextCenter);
    BubbleText->SetWorldSize(13.f);
    BubbleText->SetTextRenderColor(FColor(245, 240, 232));
    BubbleText->SetVisibility(false);
}

void ALalalandNpcCharacter::BeginPlay()
{
    Super::BeginPlay();
    DialogueWidget = NewObject<UWidgetComponent>(this, TEXT("HeadDialogue"));
    DialogueWidget->SetupAttachment(GetCapsuleComponent());
    DialogueWidget->SetWidgetSpace(EWidgetSpace::Screen);
    DialogueWidget->SetDrawSize(FVector2D(320, 150));
    DialogueWidget->SetDrawAtDesiredSize(true);
    DialogueWidget->SetPivot(FVector2D(.5f, 1.f));
    DialogueWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DialogueWidget->RegisterComponent();
    UUserWidget* Widget = NewObject<ULalalandDialogueWidget>(this);
    Widget->WidgetTree = NewObject<UWidgetTree>(Widget);
    UBorder* Border = Widget->WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(FLinearColor(.015f,.018f,.025f,.9f));
    Border->SetPadding(FMargin(12.f,8.f));
    DialogueLabel = Widget->WidgetTree->ConstructWidget<UTextBlock>();
    DialogueLabel->SetFont(FSlateFontInfo(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Fonts/NotoSansSC.ttf")), 17));
    DialogueLabel->SetWrapTextAt(280.f);
    DialogueLabel->SetColorAndOpacity(FSlateColor(FLinearColor(.98f,.94f,.86f)));
    Border->SetContent(DialogueLabel);
    Widget->WidgetTree->RootWidget = Border;
    DialogueWidget->SetWidget(Widget);
    DialogueWidget->SetVisibility(false);
}

void ALalalandNpcCharacter::InitializeActor(const FString& InActorId, const FLinearColor& Color)
{
    ActorId = InActorId;
    NameLabel->SetText(FText::FromString(InActorId));
    PlaceholderBody->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
    const int32 CharacterIndex = FMath::Max(0, InActorId.Len() == 1 ? InActorId[0] - TCHAR('A') : 0);
    NextIdleGesture = 9.f + CharacterIndex * 2.75f;
    LoadCharacterMesh();
}

void ALalalandNpcCharacter::LoadCharacterMesh()
{
    if (ActorId.IsEmpty()) return;
    if (ActorId == TEXT("BARTENDER"))
    {
        USkeletalMesh* BartenderAsset=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/BARTENDER_v022/SK_Bartender_v022.SK_Bartender_v022"));
        if(!BartenderAsset)BartenderAsset=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/BARTENDER_v002/SK_Bartender_v002.SK_Bartender_v002"));
        if(USkeletalMesh* NewAsset=BartenderAsset)
        {
            GetMesh()->SetSkeletalMeshAsset(NewAsset);const auto Bounds=NewAsset->GetBounds();
            const float Scale=172.f/FMath::Max(.01f,Bounds.BoxExtent.Z*2.f);
            BaseMeshFootZ=-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale;
            GetMesh()->SetRelativeScale3D(FVector(Scale));GetMesh()->SetRelativeLocation(FVector(0,0,BaseMeshFootZ));
            GetMesh()->SetRelativeRotation(FRotator(0,-90,0));PlaceholderBody->SetVisibility(false);
            HeadAnchor=TEXT("head");RightHandAnchor=TEXT("hand_r");
            for(const TCHAR* Path:{TEXT("/Game/Characters/BARTENDER_v023/Basic/LL_Idle_v023"),TEXT("/Game/Characters/BARTENDER_v023/Basic/LL_Walk_v023"),TEXT("/Game/Characters/BARTENDER_v023/LL_HostCall_v023")})
                if(auto Clip=LoadObject<UAnimSequence>(nullptr,Path))if(Clip->GetSkeleton()==NewAsset->GetSkeleton())AnimationAssets.Add(Clip);
            if(AnimationAssets.IsEmpty())if(auto Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/BARTENDER_v002/AN_Bartender_idle_v002.AN_Bartender_idle_v002")))
                if(Idle->GetSkeleton()==NewAsset->GetSkeleton())AnimationAssets.Add(Idle);
            PlaySemanticAnimation(TEXT("idle"),true);
            UE_LOG(LogTemp,Display,TEXT("BARTENDER_V023_READY height=172 bones=%d clips=%d mesh=%s"),GetMesh()->GetNumBones(),AnimationAssets.Num(),*NewAsset->GetPathName());
            return;
        }
        if (UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BARTENDER/SM_BARTENDER.SM_BARTENDER")))
        {
            const FBoxSphereBounds SourceBounds = Asset->GetBounds();
            const float SourceHeight = FMath::Max(.01f, SourceBounds.BoxExtent.Z * 2.f);
            const float ImportScale = 172.f / SourceHeight;
            const float MeshBottom = (SourceBounds.Origin.Z - SourceBounds.BoxExtent.Z) * ImportScale;
            PlaceholderBody->SetStaticMesh(Asset);
            PlaceholderBody->EmptyOverrideMaterials();
            PlaceholderBody->SetRelativeScale3D(FVector(ImportScale));
            PlaceholderBody->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - MeshBottom));
            PlaceholderBody->SetRelativeRotation(FRotator(0, -90.f, 0));
            PlaceholderBody->SetVisibility(true);
            UE_LOG(LogTemp, Log, TEXT("LALALAND_SUPPORTING_CAST_READY BARTENDER height=172.0 scale=%.2f"), ImportScale);
        }
        return;
    }
    const bool bHasReplacement = ActorId == TEXT("B") || ActorId == TEXT("C") || ActorId == TEXT("D");
    const FString ReplacementPath = FString::Printf(TEXT("/Game/Characters/%s_2026/SK_%s_2026.SK_%s_2026"), *ActorId, *ActorId, *ActorId);
    const FString LegacyPath = FString::Printf(TEXT("/Game/Characters/%s/SK_%s.SK_%s"), *ActorId, *ActorId, *ActorId);
    // Functionally replace B/C/D with the supplied originals, without erasing
    // old packages. The explicit legacy flag provides a reversible comparison.
    const FString RebuiltPath=FString::Printf(TEXT("/Game/Characters/%s_v014/SK_%s_v014"),*ActorId,*ActorId);
    const FString NormalFixedPath=FString::Printf(TEXT("/Game/Characters/%s_v015/SK_%s_v015"),*ActorId,*ActorId);
    const FString WeightFixedPath=FString::Printf(TEXT("/Game/Characters/%s_v016/SK_%s_v016"),*ActorId,*ActorId);
    const bool bUseNormalFixed=bHasReplacement && !FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacyCharacters"))
        && !FParse::Param(FCommandLine::Get(),TEXT("LalalandRecomputedNormals"));
    USkeletalMesh* Asset=bUseNormalFixed&&(ActorId==TEXT("B")||ActorId==TEXT("C"))
        && !FParse::Param(FCommandLine::Get(),TEXT("LalalandOriginalWeights"))
        ? LoadObject<USkeletalMesh>(nullptr,*WeightFixedPath):nullptr;
    if(!Asset)Asset = bUseNormalFixed
        ? LoadObject<USkeletalMesh>(nullptr,*NormalFixedPath) : nullptr;
    if (!Asset && bHasReplacement && !FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacyCharacters"))) Asset=LoadObject<USkeletalMesh>(nullptr,*RebuiltPath);
    if (!Asset) Asset = bHasReplacement ? LoadObject<USkeletalMesh>(nullptr, *ReplacementPath) : nullptr;
    if(!Asset && ActorId==TEXT("A") && !FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacyCharacters")))
        Asset=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/A_v021/SK_A_v021.SK_A_v021"));
    if (!Asset) Asset = LoadObject<USkeletalMesh>(nullptr, *LegacyPath);
    if (Asset)
    {
        GetMesh()->SetSkeletalMeshAsset(Asset);
        const FBoxSphereBounds SourceBounds = Asset->GetBounds();
        const float TargetHeight = ActorId == TEXT("A") ? 168.f : ActorId == TEXT("B") ? 174.f : ActorId == TEXT("C") ? 180.f : 172.f;
        const float SourceHeight = FMath::Max(.01f, SourceBounds.BoxExtent.Z * 2.f);
        const float ImportScale = TargetHeight / SourceHeight;
        GetMesh()->SetRelativeScale3D(FVector(ImportScale));
        const float MeshBottom = (SourceBounds.Origin.Z - SourceBounds.BoxExtent.Z) * ImportScale;
        const float FootAlignedZ = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - MeshBottom;
        BaseMeshFootZ = FootAlignedZ;
        GetMesh()->SetRelativeLocation(FVector(0, 0, FootAlignedZ));
        GetMesh()->SetRelativeRotation(FRotator(0, -90.f, 0));
        PlaceholderBody->SetVisibility(false);
        CacheAnimations();
        UE_LOG(LogTemp, Log, TEXT("LALALAND_CHARACTER_READY %s animations=%d height=%.1f scale=%.2f footOffset=%.2f mesh=%s"), *ActorId, AnimationAssets.Num(), TargetHeight, ImportScale, FootAlignedZ,*Asset->GetPathName());

        FName HeadBone = NAME_None;
        for (int32 BoneIndex = 0; BoneIndex < GetMesh()->GetNumBones(); ++BoneIndex)
        {
            const FName Candidate = GetMesh()->GetBoneName(BoneIndex);
            const FString Bone = Candidate.ToString().ToLower();
            if (Bone.Contains(TEXT("hand")) && (Bone.Contains(TEXT("right")) || Bone.EndsWith(TEXT("_r")) || Bone.StartsWith(TEXT("r_")))) RightHandAnchor = Candidate;
            if (Candidate.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase))
            {
                HeadBone = Candidate;
            }
        }
        HeadAnchor = HeadBone;
        if (!HeadBone.IsNone())
        {
            BubbleText->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HeadBone);
            BubbleText->SetAbsolute(false, false, true);
            BubbleText->SetRelativeLocation(FVector(0, 0, 32.f / ImportScale));
            BubbleText->SetWorldScale3D(FVector::OneVector);
        }
        PlaySemanticAnimation(TEXT("idle"), true);
    }
}

void ALalalandNpcCharacter::CacheAnimations()
{
    AnimationAssets.Empty();
    FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    const bool bUsesReplacementSkeleton =
        ActorId == TEXT("B") || ActorId == TEXT("C") || ActorId == TEXT("D");
    const FString AlignedPath=FString::Printf(TEXT("/Game/Characters/%s_2026/RetargetedAligned"),*ActorId);
    RegistryModule.Get().ScanPathsSynchronous({AlignedPath},false);
    TArray<FAssetData> AlignedAssets;
    RegistryModule.Get().GetAssetsByPath(FName(*AlignedPath),AlignedAssets,true);
    // v015 fixes mesh normals only and intentionally shares the v014 skeleton/actions.
    const FString MeshPath=GetMesh()->GetSkeletalMeshAsset()->GetPathName();
    const bool bUsesRebuilt=MeshPath.Contains(TEXT("_v014/"))||MeshPath.Contains(TEXT("_v015/"))||MeshPath.Contains(TEXT("_v016/"));
    const FString CharacterPath = bUsesRebuilt ? FString::Printf(TEXT("/Game/Characters/%s_v014"),*ActorId) : bUsesReplacementSkeleton && !AlignedAssets.IsEmpty() ? AlignedPath : bUsesReplacementSkeleton
        ? FString::Printf(TEXT("/Game/Characters/%s_2026/Retargeted"), *ActorId)
        : FString::Printf(TEXT("/Game/Characters/%s"), *ActorId);
    RegistryModule.Get().ScanPathsSynchronous({CharacterPath}, false);
    FARFilter Filter;
    Filter.PackagePaths.Add(*CharacterPath);
    Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    TArray<FAssetData> Assets;
    RegistryModule.Get().GetAssets(Filter, Assets);
    Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
    {
        return Left.AssetName.ToString().Len() < Right.AssetName.ToString().Len();
    });
    for (const FAssetData& Data : Assets)
    {
        if (UAnimSequence* Sequence = Cast<UAnimSequence>(Data.GetAsset()))
        {
            // A retargeted sequence must belong to the mesh's own skeleton.
            // Mixing the legacy 41-bone actions with a 61-bone replacement
            // mesh makes the character collapse even though both assets load.
            if (Sequence->GetSkeleton() == GetMesh()->GetSkeletalMeshAsset()->GetSkeleton())
            {
                Sequence->bEnableRootMotion = false;
                Sequence->bForceRootLock = true;
                AnimationAssets.Add(Sequence);
            }
        }
    }
}

UAnimSequence* ALalalandNpcCharacter::FindAnimation(const TArray<FString>& Tokens) const
{
    for (const FString& Token : Tokens)
    {
        for (UAnimSequence* Sequence : AnimationAssets)
        {
            if (Sequence && Sequence->GetName().Contains(Token, ESearchCase::IgnoreCase)) return Sequence;
        }
    }
    return nullptr;
}

bool ALalalandNpcCharacter::UsesReviewedBasicMotion() const
{
    const auto* CharacterMesh=GetMesh()->GetSkeletalMeshAsset();
    if(!CharacterMesh || FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacyBasicMotion")))return false;
    const FString Path=CharacterMesh->GetPathName();
    if(ActorId==TEXT("BARTENDER"))return (Path.Contains(TEXT("_v022/"))||Path.Contains(TEXT("_v002/"))) && FindAnimation({TEXT("LL_Walk_v023")})!=nullptr;
    return (ActorId==TEXT("B") || ActorId==TEXT("C") || ActorId==TEXT("D")) &&
        (Path.Contains(TEXT("_v015/")) || Path.Contains(TEXT("_v016/")));
}

void ALalalandNpcCharacter::PlaySemanticAnimation(const FString& Semantic, bool bLooping)
{
    if (AnimationAssets.IsEmpty() || (CurrentAnimation == Semantic && (GetMesh()->IsPlaying() || Semantic==TEXT("cup_putdown") || Semantic==TEXT("sip_contact")))) return;
    TArray<FString> Tokens;
    const FString Lower = Semantic.ToLower();
    if(UsesReviewedBasicMotion() && !FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacyMotionCuts")) &&
        !FParse::Param(FCommandLine::Get(),TEXT("LalalandVenueAnchorCandidate")) && (Lower==TEXT("idle") || Lower==TEXT("walk")))
    {
        const FString Path=ActorId==TEXT("BARTENDER")?TEXT("/Game/Characters/BARTENDER_v023/Basic/BS_Locomotion_v023.BS_Locomotion_v023"):FString::Printf(TEXT("/Game/Characters/%s_v018/Basic/BS_Locomotion_v018.BS_Locomotion_v018"),*ActorId);
        if(auto* Blend=LoadObject<UBlendSpace1D>(nullptr,*Path))if(Blend->GetSkeleton()==GetMesh()->GetSkeletalMeshAsset()->GetSkeleton())
        {
            auto* Instance=GetMesh()->GetSingleNodeInstance();
            // Keep the same player alive across idle/walk; do not restart its phase.
            if(!Instance || Instance->GetCurrentAsset()!=Blend){GetMesh()->PlayAnimation(Blend,true);Instance=GetMesh()->GetSingleNodeInstance();LocomotionWalkWeight=0.f;}
            if(Instance)Instance->SetBlendSpacePosition(FVector(LocomotionWalkWeight,0,0));
            CurrentAnimation=Semantic;bDrinkGripAnimation=false;
            if(Lower==TEXT("idle"))GetMesh()->SetPlayRate(ActorId==TEXT("B")?1.06f:ActorId==TEXT("C")?.86f:1.f);
            return;
        }
    }
    if(ActorId==TEXT("BARTENDER") && (Lower==TEXT("host_call")||Lower==TEXT("greet")||Lower==TEXT("talk")))Tokens={TEXT("LL_HostCall_v023")};
    else if (Lower==TEXT("cup_walk")) Tokens={TEXT("LL_Cup_Walk")};
    else if (Lower==TEXT("cup_hold") || Lower==TEXT("receive")) Tokens={TEXT("LL_Cup_Hold")};
    else if (Lower==TEXT("cup_putdown")) Tokens={TEXT("LL_Cup_Putdown")};
    else if (Lower==TEXT("sip_contact")) Tokens={TEXT("LL_Sip_Contact")};
    else if (Lower.Contains(TEXT("walk")) || Lower.Contains(TEXT("move"))) Tokens = {TEXT("LL_Walk_v014"),TEXT("LL_Walk_Grounded"),TEXT("walk")};
    else if (Lower.Contains(TEXT("drink")) || Lower.Contains(TEXT("sip")) || Lower.Contains(TEXT("toast"))) Tokens = {TEXT("drink"), TEXT("sip"), TEXT("toast")};
    else if (Lower.Contains(TEXT("sit"))) Tokens = {TEXT("sit")};
    else if (Lower.Contains(TEXT("phone")) || Lower.Contains(TEXT("mobile"))) Tokens = {TEXT("make_a_call"), TEXT("play_mobile_game")};
    else if (Lower.Contains(TEXT("turn"))) Tokens = {TEXT("turn")};
    else if (Lower.Contains(TEXT("look")) || Lower.Contains(TEXT("observe"))) Tokens = {TEXT("look_around")};
    else if (Lower.Contains(TEXT("fold")) || Lower.Contains(TEXT("listen"))) Tokens = {TEXT("fold_arms"), TEXT("wait")};
    else if (Lower.Contains(TEXT("greet")) || Lower.Contains(TEXT("talk"))) Tokens = {TEXT("greet"), TEXT("agree")};
    else if (Lower.Contains(TEXT("throw")) || Lower.Contains(TEXT("toss"))) Tokens = {TEXT("throw"), TEXT("agree"), TEXT("greet")};
    else if (Lower.Contains(TEXT("agree"))) Tokens = {TEXT("agree")};
    else Tokens = {TEXT("LL_Idle_v014"),TEXT("standing_relax"), TEXT("idle"), TEXT("wait")};
    UAnimSequence* Sequence = FindAnimation(Tokens);
    // Reference-root bake is restricted to rebuilt B/C/D basic idle and walk.
    // Retain an explicit historical-motion rollback; never reassign skeletons.
    const bool bAnchorCandidate=FParse::Param(FCommandLine::Get(),TEXT("LalalandVenueAnchorCandidate"));
    if ((bAnchorCandidate || !FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacyBasicMotion"))) &&
        (ActorId==TEXT("B") || ActorId==TEXT("C") || ActorId==TEXT("D")) &&
        (Lower==TEXT("idle") || Lower==TEXT("walk")))
    {
        const FString Action=Lower==TEXT("walk")?TEXT("Walk"):TEXT("Idle");
        const FString Folder=bAnchorCandidate?FString::Printf(TEXT("/Game/CharacterReview_v017/%s"),*ActorId):FString::Printf(TEXT("/Game/Characters/%s_v017/Basic"),*ActorId);
        const FString Path=FString::Printf(TEXT("%s/LL_%s_v017.LL_%s_v017"),*Folder,*Action,*Action);
        if (auto* Candidate=LoadObject<UAnimSequence>(nullptr,*Path))
            if (Candidate->GetSkeleton()==GetMesh()->GetSkeletalMeshAsset()->GetSkeleton())Sequence=Candidate;
    }
    if (Sequence)
    {
        // Compatibility clips have a Legacy_ prefix after the fresh import.
        bDrinkGripAnimation = Sequence->GetName().Contains(TEXT("LL_Cup_")) || Sequence->GetName().Contains(TEXT("drink"), ESearchCase::IgnoreCase) || Sequence->GetName().Contains(TEXT("sip"), ESearchCase::IgnoreCase) || Sequence->GetName().Contains(TEXT("toast"), ESearchCase::IgnoreCase);
        CurrentAnimation = Semantic;
        GetMesh()->PlayAnimation(Sequence, bLooping);
        const float CharacterRate = ActorId == TEXT("A") ? .92f : ActorId == TEXT("B") ? 1.06f : ActorId == TEXT("C") ? .86f : 1.f;
        GetMesh()->SetPlayRate(CharacterRate);
        if (!bLooping) GestureRemaining = FMath::Max(.5f, Sequence->GetPlayLength() / CharacterRate);
    }
}

FVector ALalalandNpcCharacter::GetMouthLocation() const
{
    return GetMesh()->GetSocketLocation(HeadAnchor)+GetActorForwardVector()*8.f-FVector(0,0,5.f);
}
float ALalalandNpcCharacter::GetCupAnimationProgress() const
{
    auto* Instance=GetMesh()->GetSingleNodeInstance();
    return Instance && Instance->GetLength()>0.f ? Instance->GetCurrentTime()/Instance->GetLength() : 0.f;
}
bool ALalalandNpcCharacter::HasCupAction(const FString& Phase) const
{
    return HasDrinkGrip() && CurrentAnimation==(Phase==TEXT("lower")?TEXT("cup_putdown"):TEXT("sip_contact"));
}
void ALalalandNpcCharacter::ApplyState(const FLalalandActorDto& Dto, const TMap<FString, ALalalandNpcCharacter*>& Cast)
{
    const bool bHide = false;
    SetActorHiddenInGame(bHide);
    SetActorEnableCollision(!bHide);
    NameLabel->SetText(FText::FromString(Dto.name.IsEmpty() ? Dto.id : Dto.name));
    DesiredLocation = ServerToWorld(Dto.x, Dto.y, Dto.z);
    if (!bReceivedInitialState)
    {
        SetActorLocation(DesiredLocation, false, nullptr, ETeleportType::TeleportPhysics);
        LastMovementLocation=GetActorLocation();
        bReceivedInitialState = true;
    }
    if (Dto.route.Num()) DesiredLocation = ServerToWorld(Dto.route[0].x, Dto.route[0].y, Dto.route[0].z);
    DesiredRotation = FRotator(0, 90.f - Dto.yaw, 0);
    ConversationTarget = Dto.conversationTarget;
    DesiredPosture = Dto.posture;
    if (!Dto.gesture.IsEmpty() && Dto.gestureAt > LastGestureAt)
    {
        LastGestureAt = Dto.gestureAt;
        PlaySemanticAnimation(Dto.gesture, false);
    }
    if (!Dto.conversationTarget.IsEmpty())
    {
        if (Dto.conversationTarget == TEXT("USER") && GetWorld()->GetFirstPlayerController())
        {
            if (APawn* Player = GetWorld()->GetFirstPlayerController()->GetPawn()) DesiredRotation = (Player->GetActorLocation() - GetActorLocation()).Rotation();
            DesiredRotation.Pitch = DesiredRotation.Roll = 0;
        }
        if (ALalalandNpcCharacter* const* Target = Cast.Find(Dto.conversationTarget))
        {
            DesiredRotation = ((*Target)->GetActorLocation() - GetActorLocation()).Rotation();
            DesiredRotation.Pitch = DesiredRotation.Roll = 0;
        }
    }
}

void ALalalandNpcCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bReceivedInitialState) return;
    bool bBodyAction=false;bool bHoldingCup=false;FString CupPhase;
    if (const ULalalandServiceSubsystem* Service = GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>())
    {
        const bool bPaused = Service->GetState().paused;
        bBodyAction=!Service->GetState().firstNight.pendingThrow.id.IsEmpty()&&Service->GetState().firstNight.pendingThrow.actor==ActorId;
        for(const FLalalandDrinkPropDto& Drink:Service->GetState().firstNight.drinks)
            {
                if(!Drink.deliveryActionId.IsEmpty()&&Drink.deliveryTarget==ActorId)bBodyAction=true;
                if(Drink.owner==ActorId)
                {
                    if(!Drink.sipActionId.IsEmpty())CupPhase=Drink.sipPhase;
                    bHoldingCup|=Drink.cupPlacement==TEXT("hand") || (!Drink.sipActionId.IsEmpty()&&Drink.sipPhase!=TEXT("approach")&&Drink.sipPhase!=TEXT("queued"));
                    bBodyAction|=!Drink.sipActionId.IsEmpty()&&(Drink.sipPhase==TEXT("sip")||Drink.sipPhase==TEXT("lower"));
                }
            }
        GetMesh()->bPauseAnims = bPaused;
        if (bPaused) { GetCharacterMovement()->StopMovementImmediately();LastMovementLocation=GetActorLocation();StalledMovementSeconds=0;return; }
    }
    const FVector Delta = DesiredLocation - GetActorLocation();
    // CharacterMovement deliberately keeps the collision capsule a small
    // distance above a walkable floor. Render the sole at that floor instead
    // of inheriting the capsule clearance; do not move/resize the capsule.
    const auto& Floor=GetCharacterMovement()->CurrentFloor;
    if(GetCharacterMovement()->IsMovingOnGround()&&Floor.IsWalkableFloor())
    {
        FVector MeshLocation=GetMesh()->GetRelativeLocation();
        const float GoalZ=BaseMeshFootZ-FMath::Clamp(Floor.FloorDist,0.f,5.f);
        MeshLocation.Z=FMath::FInterpTo(MeshLocation.Z,GoalZ,DeltaSeconds,12.f);
        GetMesh()->SetRelativeLocation(MeshLocation);
    }
    const float ActualStep=FVector::Dist2D(LastMovementLocation,GetActorLocation());
    LastMovementLocation=GetActorLocation();
    AvoidanceRecoverySeconds=FMath::Max(0.f,AvoidanceRecoverySeconds-DeltaSeconds);
    MovementDiagnosticCooldown=FMath::Max(0.f,MovementDiagnosticCooldown-DeltaSeconds);
    StalledMovementSeconds=!bBodyAction&&!bLocomotionTurnGate&&Delta.SizeSquared2D()>FMath::Square(25.f)&&ActualStep<.5f ? StalledMovementSeconds+DeltaSeconds : 0.f;
    if(StalledMovementSeconds>.7f)
    {
        // RVO considers neighbouring characters, not furniture. It can steer
        // a valid southbound route back into a shelf for many seconds. Briefly
        // follow the server route directly; capsule collision remains active.
        AvoidanceRecoverySeconds=1.5f;StalledMovementSeconds=0.f;
        if(MovementDiagnosticCooldown<=0.f)
        {
            MovementDiagnosticCooldown=3.f;
            TArray<FHitResult> Blockers;
            FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(LalalandStallBlocker),false,this);
            GetWorld()->SweepMultiByChannel(Blockers,GetActorLocation(),GetActorLocation()+Delta.GetSafeNormal2D()*50.f,FQuat::Identity,
                ECC_Pawn,FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(),GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),TraceParams);
            FString Contacts;
            for(const auto& Hit:Blockers)if(Hit.bBlockingHit)
                Contacts+=FString::Printf(TEXT(" [%s/%s normal=%s penetrating=%d]"),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),*Hit.ImpactNormal.ToCompactString(),Hit.bStartPenetrating?1:0);
            UE_LOG(LogTemp,Log,TEXT("LALALAND_MOVEMENT_RECOVERY actor=%s body=%s target=%s velocity=%s floor=%s avoidance=%d contacts=%s"),
                *ActorId,*GetActorLocation().ToCompactString(),*DesiredLocation.ToCompactString(),*GetVelocity().ToCompactString(),
                *GetNameSafe(GetCharacterMovement()->CurrentFloor.HitResult.GetComponent()),GetCharacterMovement()->bUseRVOAvoidance?1:0,*Contacts);
        }
    }
    // Anticipate furniture before RVO can steer the capsule under an
    // overhang. Character avoidance has no knowledge of those obstacles.
    FCollisionObjectQueryParams GeometryTypes;
    GeometryTypes.AddObjectTypesToQuery(ECC_WorldStatic);GeometryTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
    FCollisionQueryParams GeometryParams(SCENE_QUERY_STAT(LalalandMovementClearance),false,this);
    FHitResult GeometryHit;
    const FVector ProbeDirection=GetVelocity().SizeSquared2D()>25.f?GetVelocity().GetSafeNormal2D():Delta.GetSafeNormal2D();
    const bool bNearGeometry=GetWorld()->SweepSingleByObjectType(GeometryHit,GetActorLocation(),GetActorLocation()+ProbeDirection*45.f,FQuat::Identity,
        GeometryTypes,FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius()+2.f,GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),GeometryParams)
        &&GeometryHit.ImpactNormal.Z<.5f;
    const bool bUseAvoidance=!bBodyAction&&!bNearGeometry&&AvoidanceRecoverySeconds<=0.f;
    if(GetCharacterMovement()->bUseRVOAvoidance!=bUseAvoidance)GetCharacterMovement()->SetAvoidanceEnabled(bUseAvoidance);
    const bool bReviewedSteering=UsesReviewedBasicMotion()&&!bBodyAction&&!bHoldingCup
        &&!FParse::Param(FCommandLine::Get(),TEXT("LalalandLegacySteering"));
    if(bReviewedSteering)
    {
        LalalandLocomotion::FSteering Steering;Steering.bTurnGate=bLocomotionTurnGate;
        Steering.Update(Delta.Size2D(),Delta.Rotation().Yaw,DesiredRotation.Yaw,GetActorRotation().Yaw,GetVelocity(),DeltaSeconds);
        bLocomotionTurnGate=Steering.bTurnGate;LocomotionSteeringPhase=static_cast<int32>(Steering.Phase);
        GetCharacterMovement()->MaxWalkSpeed=Steering.SpeedLimit;
        if(bLocomotionTurnGate)GetCharacterMovement()->SetAvoidanceEnabled(false);
        if(Steering.bMove)AddMovementInput(Delta.GetSafeNormal2D(),1.f,true);
        SetActorRotation(FRotator(0,Steering.FacingYaw,0));
    }
    else
    {
        bLocomotionTurnGate=false;LocomotionSteeringPhase=-1;GetCharacterMovement()->MaxWalkSpeed=160.f;
        if(bBodyAction)GetCharacterMovement()->StopMovementImmediately();
        else if (Delta.SizeSquared2D() > FMath::Square(10.f))
        {
            AddMovementInput(Delta.GetSafeNormal2D(), 1.f, true);
            DesiredRotation = Delta.Rotation();
        }
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, 4.f));
    }
    if (GetVelocity().SizeSquared2D() > FMath::Square(8.f))
    {
        GestureRemaining = 0;
        IdleElapsed = 0;
        PlaySemanticAnimation(bHoldingCup?TEXT("cup_walk"):TEXT("walk"), true);
        GetMesh()->SetPlayRate(bReviewedSteering?LalalandLocomotion::WalkingPlayRate(GetVelocity().Size2D()):FMath::Clamp(GetVelocity().Size2D() / 135.f, .35f, 1.4f));
    }
    else if(CupPhase==TEXT("lower")||CupPhase==TEXT("sip"))
    {
        // Hold the last action frame until its physical effect is acknowledged.
        // Idle must not reset progress just before the cup reaches the counter.
        PlaySemanticAnimation(CupPhase==TEXT("lower")?TEXT("cup_putdown"):TEXT("sip_contact"),false);
        GestureRemaining=FMath::Max(.05f,GestureRemaining-DeltaSeconds);
    }
    else if (GestureRemaining > 0)
    {
        GestureRemaining -= DeltaSeconds;
    }
    else
    {
        const bool bWalking = GetVelocity().SizeSquared2D() > FMath::Square(8.f);
        if (bWalking)
        {
            IdleElapsed = 0;
            PlaySemanticAnimation(bHoldingCup?TEXT("cup_walk"):TEXT("walk"), true);
        }
        else if(bHoldingCup)
        {
            IdleElapsed=0;PlaySemanticAnimation(TEXT("cup_hold"),true);
        }
        else if (DesiredPosture.Contains(TEXT("sit"), ESearchCase::IgnoreCase))
        {
            IdleElapsed = 0;
            PlaySemanticAnimation(TEXT("sit"), true);
        }
        else
        {
            IdleElapsed += DeltaSeconds;
            if (IdleElapsed >= NextIdleGesture && !UsesReviewedBasicMotion())
            {
                IdleElapsed = 0;
                NextIdleGesture += ActorId == TEXT("C") ? 3.f : 1.5f;
                const FString Variation = ActorId == TEXT("A") ? TEXT("fold_arms") : ActorId == TEXT("B") ? TEXT("agree") : ActorId == TEXT("C") ? TEXT("look_around") : TEXT("phone");
                PlaySemanticAnimation(Variation, false);
            }
            else
            {
                PlaySemanticAnimation(TEXT("idle"), true);
            }
        }
    }
    if(auto* Instance=GetMesh()->GetSingleNodeInstance())if(Cast<UBlendSpace1D>(Instance->GetCurrentAsset()))
    {
        // Hysteresis prevents micro-velocity toggling; a bounded 0.22s weight ramp.
        const float Speed=GetVelocity().Size2D();
        const float Goal=Speed>12.f?1.f:Speed<6.f?0.f:LocomotionWalkWeight;
        LocomotionWalkWeight=FMath::FInterpConstantTo(LocomotionWalkWeight,Goal,DeltaSeconds,1.f/.22f);
        Instance->SetBlendSpacePosition(FVector(LocomotionWalkWeight,0,0));
    }
    if (BubbleRemaining > 0)
    {
        BubbleRemaining -= DeltaSeconds;
        if (BubbleRemaining <= 0 && DialoguePages.Num())
        {
            DialogueLabel->SetText(FText::FromString(DialoguePages[0]));
            DialoguePages.RemoveAt(0);
            BubbleRemaining = 7.f;
        }
    }
    if (APlayerCameraManager* Camera = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->PlayerCameraManager : nullptr)
    {
        const FVector Anchor = HeadAnchor.IsNone() ? GetActorLocation()+FVector(0,0,100) : GetMesh()->GetSocketLocation(HeadAnchor);
        DialogueWidget->SetWorldLocation(Anchor + FVector(0,0,26));
        const FVector ToHead = Anchor - Camera->GetCameraLocation();
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LalalandDialogueVisibility), false, this);
        Params.AddIgnoredActor(GetWorld()->GetFirstPlayerController()->GetPawn());
        FHitResult Hit;
        const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Camera->GetCameraLocation(), Anchor, ECC_Visibility, Params);
        bDialogueEligible = BubbleRemaining > 0 && ToHead.SizeSquared() < FMath::Square(850.f) && FVector::DotProduct(ToHead.GetSafeNormal(), Camera->GetCameraRotation().Vector()) > .2f && !bBlocked;
        if (!bDialogueEligible) DialogueWidget->SetVisibility(false);
        const FRotator FaceCamera = (Camera->GetCameraLocation() - BubbleText->GetComponentLocation()).Rotation();
        BubbleText->SetWorldRotation(FRotator(0, FaceCamera.Yaw + 180.f, 0));
        NameLabel->SetWorldRotation(FRotator(0, FaceCamera.Yaw + 180.f, 0));
    }
}

bool ALalalandNpcCharacter::GetDialogueLayout(FVector& Anchor, FVector2D& Size) const
{
    if (!bDialogueEligible || !DialogueWidget || !DialogueWidget->GetUserWidgetObject()) return false;
    Anchor = DialogueWidget->GetComponentLocation();
    UUserWidget* Widget = DialogueWidget->GetUserWidgetObject();
    // Screen-space components do not construct Slate while hidden. Measure
    // before first display as well, otherwise the initial desired size is zero.
    const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
    Widget->ForceLayoutPrepass();
    Size = SlateWidget->GetDesiredSize();
    UE_LOG(LogTemp, VeryVerbose, TEXT("LALALAND_BUBBLE_MEASURE actor=%s size=%s"), *ActorId, *Size.ToString());
    return Size.X > 0 && Size.Y > 0;
}

void ALalalandNpcCharacter::ApplyDialogueLayout(const FVector2D& Offset, bool bShow)
{
    if (!DialogueWidget) return;
    DialogueWidget->SetVisibility(bShow && bDialogueEligible);
    if (UUserWidget* Widget = DialogueWidget->GetUserWidgetObject()) Widget->SetRenderTranslation(Offset);
}

void ALalalandNpcCharacter::ShowDialogue(const FString& Text, float Seconds)
{
    if (!DialogueLabel || Text.IsEmpty()) return;
    DialoguePages.Empty();
    for (int32 Offset = 0; Offset < Text.Len(); Offset += 48) DialoguePages.Add(Text.Mid(Offset,48));
    DialogueLabel->SetText(FText::FromString(DialoguePages[0]));
    DialoguePages.RemoveAt(0);
    BubbleRemaining = FMath::Max(Seconds, 7.f);
}

FVector ALalalandNpcCharacter::ServerToWorld(double X, double Y, double Z) const
{
    return FVector(X * 100.0, Z * 100.0, Y * 100.0 + 92.0);
}
