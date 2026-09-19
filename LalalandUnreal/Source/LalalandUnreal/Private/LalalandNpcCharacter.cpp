#include "LalalandNpcCharacter.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Modules/ModuleManager.h"
#include "UObject/ConstructorHelpers.h"

ALalalandNpcCharacter::ALalalandNpcCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);
    GetCharacterMovement()->MaxWalkSpeed = 240.f;
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
        if (UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BARTENDER/SM_BARTENDER.SM_BARTENDER")))
        {
            const FBoxSphereBounds SourceBounds = Asset->GetBounds();
            const float SourceHeight = FMath::Max(.01f, SourceBounds.BoxExtent.Z * 2.f);
            const float ImportScale = 172.f / SourceHeight;
            const float MeshBottom = (SourceBounds.Origin.Z - SourceBounds.BoxExtent.Z) * ImportScale;
            PlaceholderBody->SetStaticMesh(Asset);
            PlaceholderBody->SetRelativeScale3D(FVector(ImportScale));
            PlaceholderBody->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - MeshBottom));
            PlaceholderBody->SetRelativeRotation(FRotator(0, -90.f, 0));
            PlaceholderBody->SetVisibility(true);
            UE_LOG(LogTemp, Log, TEXT("LALALAND_SUPPORTING_CAST_READY BARTENDER height=172.0 scale=%.2f"), ImportScale);
        }
        return;
    }
    const FString Path = FString::Printf(TEXT("/Game/Characters/%s/SK_%s.SK_%s"), *ActorId, *ActorId, *ActorId);
    if (USkeletalMesh* Asset = LoadObject<USkeletalMesh>(nullptr, *Path))
    {
        GetMesh()->SetSkeletalMeshAsset(Asset);
        const FBoxSphereBounds SourceBounds = Asset->GetBounds();
        const float TargetHeight = ActorId == TEXT("A") ? 168.f : ActorId == TEXT("B") ? 174.f : ActorId == TEXT("C") ? 180.f : 172.f;
        const float SourceHeight = FMath::Max(.01f, SourceBounds.BoxExtent.Z * 2.f);
        const float ImportScale = TargetHeight / SourceHeight;
        GetMesh()->SetRelativeScale3D(FVector(ImportScale));
        const float MeshBottom = (SourceBounds.Origin.Z - SourceBounds.BoxExtent.Z) * ImportScale;
        const float FootAlignedZ = -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - MeshBottom;
        GetMesh()->SetRelativeLocation(FVector(0, 0, FootAlignedZ));
        GetMesh()->SetRelativeRotation(FRotator(0, -90.f, 0));
        PlaceholderBody->SetVisibility(false);
        CacheAnimations();
        UE_LOG(LogTemp, Log, TEXT("LALALAND_CHARACTER_READY %s animations=%d height=%.1f scale=%.2f footOffset=%.2f"), *ActorId, AnimationAssets.Num(), TargetHeight, ImportScale, FootAlignedZ);

        FName HeadBone = NAME_None;
        for (int32 BoneIndex = 0; BoneIndex < GetMesh()->GetNumBones(); ++BoneIndex)
        {
            const FName Candidate = GetMesh()->GetBoneName(BoneIndex);
            if (Candidate.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase))
            {
                HeadBone = Candidate;
                break;
            }
        }
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
    const FString CharacterPath = FString::Printf(TEXT("/Game/Characters/%s"), *ActorId);
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
        if (UAnimSequence* Sequence = Cast<UAnimSequence>(Data.GetAsset())) AnimationAssets.Add(Sequence);
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

void ALalalandNpcCharacter::PlaySemanticAnimation(const FString& Semantic, bool bLooping)
{
    if (AnimationAssets.IsEmpty() || (CurrentAnimation == Semantic && GetMesh()->IsPlaying())) return;
    TArray<FString> Tokens;
    const FString Lower = Semantic.ToLower();
    if (Lower.Contains(TEXT("walk")) || Lower.Contains(TEXT("move"))) Tokens = {TEXT("walk")};
    else if (Lower.Contains(TEXT("sit"))) Tokens = {TEXT("sit")};
    else if (Lower.Contains(TEXT("phone")) || Lower.Contains(TEXT("mobile"))) Tokens = {TEXT("make_a_call"), TEXT("play_mobile_game")};
    else if (Lower.Contains(TEXT("turn"))) Tokens = {TEXT("turn")};
    else if (Lower.Contains(TEXT("look")) || Lower.Contains(TEXT("observe"))) Tokens = {TEXT("look_around")};
    else if (Lower.Contains(TEXT("fold")) || Lower.Contains(TEXT("listen"))) Tokens = {TEXT("fold_arms"), TEXT("wait")};
    else if (Lower.Contains(TEXT("greet")) || Lower.Contains(TEXT("talk"))) Tokens = {TEXT("greet"), TEXT("agree")};
    else if (Lower.Contains(TEXT("agree"))) Tokens = {TEXT("agree")};
    else Tokens = {TEXT("standing_relax"), TEXT("idle"), TEXT("wait")};
    if (UAnimSequence* Sequence = FindAnimation(Tokens))
    {
        CurrentAnimation = Semantic;
        GetMesh()->PlayAnimation(Sequence, bLooping);
        const float CharacterRate = ActorId == TEXT("A") ? .92f : ActorId == TEXT("B") ? 1.06f : ActorId == TEXT("C") ? .86f : 1.f;
        GetMesh()->SetPlayRate(CharacterRate);
        if (!bLooping) GestureRemaining = FMath::Max(.5f, Sequence->GetPlayLength() / CharacterRate);
    }
}

void ALalalandNpcCharacter::ApplyState(const FLalalandActorDto& Dto, const TMap<FString, ALalalandNpcCharacter*>& Cast)
{
    const bool bHide = Dto.id == TEXT("D") && !Dto.interactable;
    SetActorHiddenInGame(bHide);
    SetActorEnableCollision(!bHide);
    DesiredLocation = ServerToWorld(Dto.x, Dto.y, Dto.z);
    if (!bReceivedInitialState || FVector::DistSquared(GetActorLocation(), DesiredLocation) > FMath::Square(800.f))
    {
        SetActorLocation(DesiredLocation, false, nullptr, ETeleportType::TeleportPhysics);
        bReceivedInitialState = true;
    }
    DesiredRotation = FRotator(0, Dto.yaw, 0);
    DesiredPosture = Dto.posture;
    if (!Dto.gesture.IsEmpty() && Dto.gestureAt > LastGestureAt)
    {
        LastGestureAt = Dto.gestureAt;
        PlaySemanticAnimation(Dto.gesture, false);
    }
    if (!Dto.conversationTarget.IsEmpty())
    {
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
    const FVector Delta = DesiredLocation - GetActorLocation();
    if (Delta.SizeSquared2D() > FMath::Square(10.f))
    {
        AddMovementInput(Delta.GetSafeNormal2D(), 1.f, true);
        DesiredRotation = Delta.Rotation();
    }
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, 4.f));
    if (GestureRemaining > 0)
    {
        GestureRemaining -= DeltaSeconds;
    }
    else
    {
        const bool bWalking = GetVelocity().SizeSquared2D() > FMath::Square(8.f);
        if (bWalking)
        {
            IdleElapsed = 0;
            PlaySemanticAnimation(TEXT("walk"), true);
        }
        else if (DesiredPosture.Contains(TEXT("sit"), ESearchCase::IgnoreCase))
        {
            IdleElapsed = 0;
            PlaySemanticAnimation(TEXT("sit"), true);
        }
        else
        {
            IdleElapsed += DeltaSeconds;
            if (IdleElapsed >= NextIdleGesture)
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
    if (BubbleRemaining > 0)
    {
        BubbleRemaining -= DeltaSeconds;
        if (BubbleRemaining <= 0) BubbleText->SetVisibility(false);
    }
    if (APlayerCameraManager* Camera = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->PlayerCameraManager : nullptr)
    {
        const FRotator FaceCamera = (Camera->GetCameraLocation() - BubbleText->GetComponentLocation()).Rotation();
        BubbleText->SetWorldRotation(FRotator(0, FaceCamera.Yaw + 180.f, 0));
        NameLabel->SetWorldRotation(FRotator(0, FaceCamera.Yaw + 180.f, 0));
    }
}

void ALalalandNpcCharacter::ShowDialogue(const FString& Text, float Seconds)
{
    BubbleText->SetText(FText::FromString(Text.Left(90)));
    BubbleText->SetVisibility(true);
    BubbleRemaining = Seconds;
}

FVector ALalalandNpcCharacter::ServerToWorld(double X, double Y, double Z) const
{
    return FVector(X * 100.0, Z * 100.0, Y * 100.0 + 92.0);
}
