// Offline diagnostics only. No edits to production meshes, weights or animations.
#include "LalalandCharacterReview.h"
#include "LalalandNpcCharacter.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
FString Weights(const FSkeletalMeshLODRenderData& LOD, const USkeletalMesh* Mesh, uint32 Vertex)
{
    const auto* W=LOD.GetSkinWeightVertexBuffer();
    for(const auto& Section:LOD.RenderSections)
    {
        if(Vertex<Section.BaseVertexIndex||Vertex>=Section.BaseVertexIndex+Section.NumVertices)continue;
        FString Result;
        for(uint32 I=0;I<W->GetMaxBoneInfluences();++I)
        {
            const uint16 Weight=W->GetBoneWeight(Vertex,I);if(!Weight)continue;
            const uint32 Bone=W->GetBoneIndex(Vertex,I);
            if(Section.BoneMap.IsValidIndex(Bone))
                Result+=FString::Printf(TEXT("%s:%.6f|"),*Mesh->GetRefSkeleton().GetBoneName(Section.BoneMap[Bone]).ToString(),Weight/65535.f);
        }
        return Result;
    }
    return TEXT("NO_SECTION");
}
void Audit(USkeletalMeshComponent* C, const FString& Role, const FString& Pose, const FString& Dir)
{
    auto* Mesh=C->GetSkeletalMeshAsset();auto* Data=Mesh->GetResourceForRendering();
    if(!Data||!Data->LODRenderData.Num())return;
    const auto& LOD=Data->LODRenderData[0];
    const auto& Buffer=LOD.StaticVertexBuffers.PositionVertexBuffer;
    if(Pose==TEXT("ref"))
    {
        const auto& Tangents=LOD.StaticVertexBuffers.StaticMeshVertexBuffer;
        int32 Bad=0;float Minimum=1.f;
        for(uint32 V=0;V<Tangents.GetNumVertices();++V)
        {const auto N=Tangents.VertexTangentZ(V);const float L=FVector3f(N.X,N.Y,N.Z).Size();Minimum=FMath::Min(Minimum,L);if(L<.9f)++Bad;}
        UE_LOG(LogTemp,Display,TEXT("STRETCH_NORMALS_V016 role=%s shortNormals=%d minimumLength=%.6f"),*Role,Bad,Minimum);
        if(Mesh->GetPathName().Contains(TEXT("CharacterReview_v016"))||Mesh->GetPathName().Contains(TEXT("_v016/")))
        {
            auto* Previous=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v015/SK_%s_v015"),*Role,*Role));
            const auto& OldLOD=Previous->GetResourceForRendering()->LODRenderData[0];
            const auto& OldBuffer=OldLOD.StaticVertexBuffers.PositionVertexBuffer;
            bool SamePositions=OldBuffer.GetNumVertices()==Buffer.GetNumVertices();float MaxPositionDelta=0,MaxNormalDelta=0;
            if(SamePositions)for(uint32 V=0;V<Buffer.GetNumVertices();++V)
            {
                MaxPositionDelta=FMath::Max(MaxPositionDelta,(Buffer.VertexPosition(V)-OldBuffer.VertexPosition(V)).Size());
                const auto N=Tangents.VertexTangentZ(V),O=OldLOD.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(V);
                MaxNormalDelta=FMath::Max(MaxNormalDelta,FVector3f(N.X-O.X,N.Y-O.Y,N.Z-O.Z).Size());
            }
            const auto* OldIndices=OldLOD.MultiSizeIndexContainer.GetIndexBuffer();const auto* NewIndices=LOD.MultiSizeIndexContainer.GetIndexBuffer();
            bool SameIndices=OldIndices->Num()==NewIndices->Num();
            if(SameIndices)for(int32 I=0;I<OldIndices->Num();++I)if(OldIndices->Get(I)!=NewIndices->Get(I)){SameIndices=false;break;}
            UE_LOG(LogTemp,Display,TEXT("STRETCH_PRESERVED_V016 role=%s sameVertexCount=%d maxPositionDelta=%.9f maxNormalDelta=%.9f sameIndices=%d sameSkeleton=%d"),*Role,SamePositions,MaxPositionDelta,MaxNormalDelta,SameIndices,Mesh->GetSkeleton()==Previous->GetSkeleton());
        }
    }
    if(!Buffer.GetVertexData()||!LOD.GetSkinWeightVertexBuffer()->GetDataVertexBuffer()->GetWeightData())
    {UE_LOG(LogTemp,Error,TEXT("STRETCH_V016_CPU_BUFFER_MISSING role=%s"),*Role);return;}
    TArray<FMatrix44f> Matrices;C->GetCurrentRefToLocalMatrices(Matrices,0);
    TArray<FVector3f> P;USkinnedMeshComponent::ComputeSkinnedPositions(C,P,Matrices,LOD,*LOD.GetSkinWeightVertexBuffer());
    const auto* Indices=LOD.MultiSizeIndexContainer.GetIndexBuffer();
    FString CSV=TEXT("triangle,vertex_a,vertex_b,rest_length,posed_length,ratio,rest_a_x,rest_a_y,rest_a_z,rest_b_x,rest_b_y,rest_b_z,posed_a_x,posed_a_y,posed_a_z,posed_b_x,posed_b_y,posed_b_z,weights_a,weights_b\n");
    int32 Count=0;float MaxRatio=0,MaxEdge=0;
    for(uint32 I=0;I+2<static_cast<uint32>(Indices->Num());I+=3)for(uint32 E=0;E<3;++E)
    {
        const uint32 A=Indices->Get(I+E),B=Indices->Get(I+(E+1)%3);
        if(!P.IsValidIndex(A)||!P.IsValidIndex(B))continue;
        const auto RA=Buffer.VertexPosition(A),RB=Buffer.VertexPosition(B);
        const float Rest=(RA-RB).Size(),Posed=(P[A]-P[B]).Size(),Ratio=Posed/FMath::Max(Rest,1.e-8f);
        MaxRatio=FMath::Max(MaxRatio,Ratio);MaxEdge=FMath::Max(MaxEdge,Posed);
        // Normalized mesh is approximately one unit tall. Ignore short UV seam edges.
        if(Posed>.02f && Ratio>5.f)
        {
            ++Count;
            CSV+=FString::Printf(TEXT("%u,%u,%u,%.9f,%.9f,%.4f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%s,%s\n"),I/3,A,B,Rest,Posed,Ratio,RA.X,RA.Y,RA.Z,RB.X,RB.Y,RB.Z,P[A].X,P[A].Y,P[A].Z,P[B].X,P[B].Y,P[B].Z,*Weights(LOD,Mesh,A),*Weights(LOD,Mesh,B));
        }
    }
    FFileHelper::SaveStringToFile(CSV,*(Dir/(Role+TEXT("_")+Pose+TEXT("_edges.csv"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("STRETCH_V016 role=%s pose=%s vertices=%d anomalousEdges=%d maxRatio=%.3f maxEdge=%.6f"),*Role,*Pose,P.Num(),Count,MaxRatio,MaxEdge);
}
}

void StartLalalandStretchReview(UWorld* World)
{
    auto* PC=World->GetFirstPlayerController();
    if(PC){if(APawn* Pawn=PC->GetPawn()){Pawn->SetActorHiddenInGame(true);Pawn->SetActorEnableCollision(false);}if(PC->GetHUD())PC->GetHUD()->SetActorHiddenInGame(true);PC->SetShowMouseCursor(false);PC->ConsoleCommand(TEXT("viewmode unlit"));}
    auto* Camera=World->SpawnActor<ACameraActor>(FVector(350,0,140),FRotator(0,180,0));
    Camera->GetCameraComponent()->SetFieldOfView(48.f);
    if(PC)PC->SetViewTarget(Camera);
    TArray<USkeletalMeshComponent*> Components;
    for(int32 I=0;I<3;++I)Components.Add(World->SpawnActor<ASkeletalMeshActor>()->GetSkeletalMeshComponent());
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("CharacterStretch_v016"));
    const bool Promoted=FParse::Param(FCommandLine::Get(),TEXT("LalalandStretchPromoted"));
    const bool Fixed=Promoted||FParse::Param(FCommandLine::Get(),TEXT("LalalandStretchFixed"));
    const FString Output=Dir+(Promoted?TEXT("/Promoted"):Fixed?TEXT("/Fixed"):TEXT("/AlignedBaseline"));
    IFileManager::Get().MakeDirectory(*Output,true);
    if(Promoted)for(const FString Role:{TEXT("B"),TEXT("C"),TEXT("D")})
    {
        auto* Npc=World->SpawnActor<ALalalandNpcCharacter>(FVector(20000,0,0),FRotator::ZeroRotator);
        Npc->InitializeActor(Role,FLinearColor::White);Npc->TriggerGesture(TEXT("walk"));
        auto* Single=Npc->GetMesh()->GetSingleNodeInstance();auto* Mesh=Npc->GetMesh()->GetSkeletalMeshAsset();
        auto* Walk=Single?Single->GetCurrentAsset():nullptr;
        const FString Expected=FString::Printf(TEXT("/Game/Characters/%s_v%s/SK_%s_v%s"),*Role,Role==TEXT("D")?TEXT("015"):TEXT("016"),*Role,Role==TEXT("D")?TEXT("015"):TEXT("016"));
        const bool Passed=Mesh&&Mesh->GetPathName().StartsWith(Expected)&&Walk&&(Walk->GetName()==TEXT("LL_Walk_v014")||Walk->GetName()==TEXT("LL_Walk_v017")||Walk->GetName()==TEXT("BS_Locomotion_v018"))&&Walk->GetSkeleton()==Mesh->GetSkeleton();
        UE_LOG(LogTemp,Display,TEXT("STRETCH_RUNTIME_V016 role=%s passed=%d mesh=%s walk=%s"),*Role,Passed,Mesh?*Mesh->GetPathName():TEXT("MISSING"),Walk?*Walk->GetPathName():TEXT("MISSING"));
        if(!Passed)UE_LOG(LogTemp,Error,TEXT("STRETCH_RUNTIME_V016_WRONG_ASSET role=%s"),*Role);
        Npc->Destroy();
    }
    for(int32 Index=0;Index<6;++Index)
    {
        const FString Role=Index/2==0?TEXT("B"):Index/2==1?TEXT("C"):TEXT("D");const bool Side=(Index%2)!=0;
        FTimerHandle Setup,Shot;
        World->GetTimerManager().SetTimer(Setup,[Components,Role,Side,Output,Fixed,Promoted]()
        {
            auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v015/SK_%s_v015"),*Role,*Role));
            if(Fixed&&Role!=TEXT("D"))Mesh=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/CharacterReview_v016/%s/SK_%s_Weights"),*Role,*Role));
            if(Promoted&&Role!=TEXT("D"))Mesh=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v016/SK_%s_v016"),*Role,*Role));
            check(Mesh);
            const auto Bounds=Mesh->GetBounds();const float Scale=170.f/(2.f*Bounds.BoxExtent.Z);
            for(int32 I=0;I<3;++I)
            {
                auto* C=Components[I];C->SetSkeletalMeshAsset(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetWorldScale3D(FVector(Scale));
                C->SetWorldLocation(FVector(0,(1-I)*110.f,-(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale));C->SetWorldRotation(FRotator(0,Side?0:-90,0));
                C->SetForceRefPose(true);C->Stop();
                if(I)
                {
                    auto* Anim=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v014/Basic/LL_%s_v014"),*Role,I==1?TEXT("Idle"):TEXT("Walk")));
                    check(Anim&&Anim->GetSkeleton()==Mesh->GetSkeleton());C->SetForceRefPose(false);C->PlayAnimation(Anim,false);C->SetPosition(Anim->GetPlayLength()*.5f,false);C->SetPlayRate(0.f);
                }
                C->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
                C->TickAnimation(0.f,false);C->RefreshBoneTransforms();C->UpdateBounds();
                if(!Side)Audit(C,Role,I==0?TEXT("ref"):I==1?TEXT("idle"):TEXT("walk"),Output);
                if(!Side&&I)
                {
                    auto* Anim=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v014/Basic/LL_%s_v014"),*Role,I==1?TEXT("Idle"):TEXT("Walk")));
                    for(int32 Sample=0;Sample<=32;++Sample)
                    {
                        C->SetPosition(Anim->GetPlayLength()*Sample/32.f,false);C->TickAnimation(0.f,false);C->RefreshBoneTransforms();
                        Audit(C,Role,FString::Printf(TEXT("%s_%02d"),I==1?TEXT("idle"):TEXT("walk"),Sample),Output);
                    }
                    C->SetPosition(Anim->GetPlayLength()*.5f,false);C->TickAnimation(0.f,false);C->RefreshBoneTransforms();C->UpdateBounds();
                }
                // Center each head in its column despite animation pelvis offsets;
                // this changes camera composition only, not the skinning audit above.
                const FVector Head=C->GetSocketLocation(TEXT("head"));
                C->SetWorldLocation(C->GetComponentLocation()+FVector(0,(1-I)*110.f-Head.Y,155.f-Head.Z));
            }
        },1.f+Index*4.f,false);
        World->GetTimerManager().SetTimer(Shot,[Output,Role,Side](){FScreenshotRequest::RequestScreenshot(Output/(Role+(Side?TEXT("_side.png"):TEXT("_front.png"))),false,false);},3.f+Index*4.f,false);
    }
    FTimerHandle Exit;World->GetTimerManager().SetTimer(Exit,[](){FPlatformMisc::RequestExit(false);},26.f,false);
}
