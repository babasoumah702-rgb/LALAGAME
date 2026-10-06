#include "LalalandCharacterReview.h"
#include "LalalandNpcCharacter.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Components/SkyLightComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/SkeletalMeshRenderData.h"

namespace
{
USkeletalMesh* CurrentMesh(const FString& Role)
{
    const FString Dir=Role==TEXT("A") ? Role : Role+TEXT("_2026");
    return LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s/SK_%s"),*Dir,*Dir));
}
USkeletalMesh* SuppliedMesh(const FString& Role)
{
    if(Role==TEXT("A")) return LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/orc_character/Women_Motified_Ultimate/tripo_convert_990a7ce5-2cf1-4992-8c37-2335c7420d46"));
    return LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/CharacterReview_v012/%s/SK_%s_Review"),*Role,*Role));
}
UAnimSequence* Action(USkeletalMesh* Mesh,const FString& Role,const FString& Semantic)
{
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    FString Folder=Role==TEXT("A") ? TEXT("/Game/Characters/A") : FString::Printf(TEXT("/Game/Characters/%s_2026/RetargetedAligned"),*Role);
    Registry.ScanPathsSynchronous({Folder},false);
    TArray<FAssetData> Assets;Registry.GetAssetsByPath(FName(*Folder),Assets,true);
    if(Assets.IsEmpty() && Role!=TEXT("A"))
    {
        Folder=FString::Printf(TEXT("/Game/Characters/%s_2026/Retargeted"),*Role);
        Registry.ScanPathsSynchronous({Folder},false);Registry.GetAssetsByPath(FName(*Folder),Assets,true);
    }
    Assets.Sort([](const FAssetData& A,const FAssetData& B){return A.AssetName.ToString().Len()<B.AssetName.ToString().Len();});
    const TArray<FString> Tokens=Semantic==TEXT("talk") ? TArray<FString>{TEXT("greet"),TEXT("agree")} : Semantic==TEXT("fold") ? TArray<FString>{TEXT("fold_arms"),TEXT("wait")} : TArray<FString>{TEXT("standing_relax"),TEXT("idle"),TEXT("wait")};
    for(const FString& Token:Tokens) for(const auto& Data:Assets)
        if(Data.AssetName.ToString().Contains(Token)) if(auto* Anim=Cast<UAnimSequence>(Data.GetAsset()))
            if(Anim->GetSkeleton()==Mesh->GetSkeleton()) return Anim;
    return nullptr;
}
void Show(USkeletalMeshComponent* Component,USkeletalMesh* Mesh,float Y)
{
    Component->SetLeaderPoseComponent(nullptr,true);
    Component->SetSkeletalMeshAsset(Mesh);Component->EmptyOverrideMaterials();
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetCastShadow(false);
    Component->SetForceRefPose(true);
    if(!Mesh) return;
    const auto B=Mesh->GetBounds();const float Scale=170.f/FMath::Max(.01f,2.f*B.BoxExtent.Z);
    Component->SetWorldScale3D(FVector(Scale));
    Component->SetWorldLocation(FVector(0,Y,-(B.Origin.Z-B.BoxExtent.Z)*Scale));
    Component->SetWorldRotation(FRotator(0,-90,0));
}
}
void StartLalalandCharacterReview(UWorld* World)
{
    auto* PC=World->GetFirstPlayerController();
    if(PC) {if(APawn* Pawn=PC->GetPawn()){Pawn->SetActorHiddenInGame(true);Pawn->SetActorEnableCollision(false);}PC->SetShowMouseCursor(false);if(PC->GetHUD())PC->GetHUD()->SetActorHiddenInGame(true);}
    auto* Camera=World->SpawnActor<ACameraActor>(FVector(780,0,96),FRotator(0,180,0));
    Camera->GetCameraComponent()->SetFieldOfView(62.f);
    auto& Settings=Camera->GetCameraComponent()->PostProcessSettings;
    Settings.bOverride_AutoExposureMinBrightness=true;Settings.AutoExposureMinBrightness=1;
    Settings.bOverride_AutoExposureMaxBrightness=true;Settings.AutoExposureMaxBrightness=1;
    if(PC){PC->SetViewTarget(Camera);PC->ConsoleCommand(TEXT("viewmode unlit"));PC->ConsoleCommand(TEXT("r.SetRes 1920x1080w"));}
    for(const auto Rotation:{FRotator(-20,180,0),FRotator(15,160,0)})
    {
        auto* Light=World->SpawnActor<ADirectionalLight>(FVector(100,0,300),Rotation);
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);Light->GetLightComponent()->SetIntensity(4.f);Light->GetLightComponent()->SetCastShadows(false);
    }
    TArray<USkeletalMeshComponent*> Components;
    for(int32 I=0;I<3;++I)Components.Add(World->SpawnActor<ASkeletalMeshActor>()->GetSkeletalMeshComponent());
    const bool bSynced=FParse::Param(FCommandLine::Get(),TEXT("LalalandReviewSynced"));
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("CharacterReview_v012"))+(bSynced?TEXT("/Synced"):TEXT(""));
    IFileManager::Get().MakeDirectory(*Dir,true);
    // Camera is looking down -X: screen left=current animation, center=supplied
    // rest, screen right=current rest. Unlit mode removes scene lighting as a variable.
    const TArray<FString> Roles={TEXT("B"),TEXT("C"),TEXT("D"),TEXT("A")};
    const TArray<FString> Semantics={TEXT("idle"),TEXT("talk"),TEXT("fold")};
    for(int32 Index=0;Index<12;++Index)
    {
        const FString Role=Roles[Index/3],Semantic=Semantics[Index%3];FTimerHandle Setup,Shot;
        World->GetTimerManager().SetTimer(Setup,[Components,Role,Semantic,bSynced]()
        {
            auto* Old=CurrentMesh(Role);auto* New=SuppliedMesh(Role);
            Show(Components[0],Old,-260.f);Show(Components[1],New,0.f);Show(Components[2],Old,260.f);
            if(Old)if(auto* Anim=Action(Old,Role,Semantic))
            {
                Components[2]->SetForceRefPose(false);Components[2]->PlayAnimation(Anim,false);
                Components[2]->SetPosition(Anim->GetPlayLength()*.5f,false);Components[2]->SetPlayRate(0.f);
                // Diagnostic only: drive both 61-bone meshes with identical component
                // bone transforms, without changing either asset's Skeleton assignment.
                if(bSynced && New && Role!=TEXT("A") && Components[1]->GetNumBones()==Components[2]->GetNumBones())
                {Components[1]->SetForceRefPose(false);Components[1]->SetLeaderPoseComponent(Components[2],true,true);}
                if(bSynced && New && Role==TEXT("A"))
                    if(auto* OwnIdle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/orc_character/Women_Motified_Ultimate/A/MM_Idle")))
                        if(OwnIdle->GetSkeleton()==New->GetSkeleton())
                        {Components[1]->SetForceRefPose(false);Components[1]->PlayAnimation(OwnIdle,false);Components[1]->SetPosition(OwnIdle->GetPlayLength()*.5f,false);Components[1]->SetPlayRate(0.f);UE_LOG(LogTemp,Display,TEXT("KIKO_SUPPLIED_OWN_IDLE_REVIEW"));}
                UE_LOG(LogTemp,Display,TEXT("CHARACTER_REVIEW role=%s semantic=%s old=%s new=%s animation=%s"),*Role,*Semantic,*Old->GetPathName(),New?*New->GetPathName():TEXT("MISSING"),*Anim->GetPathName());
            }
        },1.f+Index*3.5f,false);
        World->GetTimerManager().SetTimer(Shot,[Dir,Role,Semantic]() {FScreenshotRequest::RequestScreenshot(Dir/(Role+TEXT("_")+Semantic+TEXT(".png")),false,false);},3.f+Index*3.5f,false);
    }
    FTimerHandle Exit;World->GetTimerManager().SetTimer(Exit,[](){FPlatformMisc::RequestExit(false);},44.f,false);
}

void StartLalalandReplacementReview(UWorld* World)
{
    const bool NoShadows=FParse::Param(FCommandLine::Get(),TEXT("LalalandReplacementNoShadows"));
    const bool Unlit=FParse::Param(FCommandLine::Get(),TEXT("LalalandReplacementUnlit"));
    const bool Shading=FParse::Param(FCommandLine::Get(),TEXT("LalalandShadingReview"));
    const bool Scaled=FParse::Param(FCommandLine::Get(),TEXT("LalalandShadingScaled"));
    const bool Imported=FParse::Param(FCommandLine::Get(),TEXT("LalalandShadingImportedNormals"));
    auto* PC=World->GetFirstPlayerController();
    if(PC){if(APawn* Pawn=PC->GetPawn()){Pawn->SetActorHiddenInGame(true);Pawn->SetActorEnableCollision(false);}if(PC->GetHUD())PC->GetHUD()->SetActorHiddenInGame(true);PC->SetShowMouseCursor(false);}
    auto* Camera=World->SpawnActor<ACameraActor>(FVector(780,0,96),FRotator(0,180,0));
    Camera->GetCameraComponent()->SetFieldOfView(62.f);
    auto& Exposure=Camera->GetCameraComponent()->PostProcessSettings;
    Exposure.bOverride_AutoExposureMinBrightness=true;Exposure.AutoExposureMinBrightness=1;
    Exposure.bOverride_AutoExposureMaxBrightness=true;Exposure.AutoExposureMaxBrightness=1;
    if(PC){PC->SetViewTarget(Camera);PC->ConsoleCommand(Unlit?TEXT("viewmode unlit"):TEXT("viewmode lit"));}
    auto* Sky=World->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SourceType=SLS_SpecifiedCubemap;
    Sky->GetLightComponent()->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap")));
    Sky->GetLightComponent()->SetIntensity(1.f);
    Sky->GetLightComponent()->SetCastShadows(!NoShadows);
    // Diagnostic fill, not a change to the actual bar's lighting.
    auto* Fill=World->SpawnActor<APointLight>(FVector(350,0,150),FRotator::ZeroRotator);
    Fill->PointLightComponent->SetMobility(EComponentMobility::Movable);
    Fill->PointLightComponent->SetIntensity(15000.f);
    Fill->PointLightComponent->SetAttenuationRadius(1500.f);
    Fill->PointLightComponent->SetCastShadows(false);
    for(int32 I=0;I<3;++I)
    {
        auto* Light=World->SpawnActor<ADirectionalLight>(FVector(100,0,300),I==0?FRotator(-25,180,0):I==1?FRotator(-20,95,0):FRotator(-15,-80,0));
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Light->GetLightComponent()->SetIntensity(I==0?3.f:1.5f);
        Light->GetLightComponent()->SetCastShadows(I==0&&!NoShadows);
    }
    TArray<USkeletalMeshComponent*> Components;
    for(int32 I=0;I<3;++I)
    {
        auto* Component=World->SpawnActor<ASkeletalMeshActor>()->GetSkeletalMeshComponent();
        Components.Add(Component);
    }
    // Check the real NPC load/cache/selection route, not only asset existence.
    for(const FString Role:{TEXT("B"),TEXT("C"),TEXT("D")})
    {
        auto* Npc=World->SpawnActor<ALalalandNpcCharacter>();Npc->InitializeActor(Role,FLinearColor::White);
        Npc->TriggerGesture(TEXT("walk"));
        auto* Single=Npc->GetMesh()->GetSingleNodeInstance();
        UE_LOG(LogTemp,Display,TEXT("REPLACEMENT_RUNTIME_V014 role=%s mesh=%s walk=%s"),*Role,*Npc->GetMesh()->GetSkeletalMeshAsset()->GetPathName(),Single&&Single->GetCurrentAsset()?*Single->GetCurrentAsset()->GetPathName():TEXT("MISSING"));
        Npc->Destroy();
    }
    const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/(Shading?TEXT("CharacterShading_v015"):TEXT("CharacterReplacement_v014")))+(Shading?(Imported?TEXT("/ImportedNormals"):Scaled?TEXT("/ScaledMaterials"):TEXT("/Materials")):Unlit?TEXT("/UnlitCorrected"):NoShadows?TEXT("/LitFillNoShadows"):TEXT("/LitFill"));
    IFileManager::Get().MakeDirectory(*Dir,true);
    for(int32 Index=0;Index<18;++Index)
    {
        const FString Role=Index/6==0?TEXT("B"):Index/6==1?TEXT("C"):TEXT("D");
        const int32 Sample=Index%6;
        const bool Walk=Sample>=2;
        const float Fraction=Walk?.125f+(Sample-2)*.25f:.5f;
        const FString Label=Walk?FString::Printf(TEXT("walk_%d"),Sample-2):Sample==0?TEXT("idle_front"):TEXT("idle_side");
        FTimerHandle Setup,Shot;
        World->GetTimerManager().SetTimer(Setup,[Components,Role,Walk,Fraction,Sample,NoShadows,Shading,Scaled,Imported]()
        {
            auto* Old=CurrentMesh(Role);
            auto* New=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v014/SK_%s_v014"),*Role,*Role));
            if(Shading&&Scaled)New=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/CharacterReview_v015/%s_Scaled/SK_%s_Scaled"),*Role,*Role));
            if(Shading&&Imported)New=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/CharacterReview_v015/%s_Normals/SK_%s_Normals"),*Role,*Role));
            auto* OldAnim=Walk?LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_2026/RetargetedAligned/LL_Walk_Grounded"),*Role)):Action(Old,Role,TEXT("idle"));
            auto* SameAnim=OldAnim?LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v014/UnreviewedLegacy/Legacy_%s"),*Role,*OldAnim->GetName())):nullptr;
            auto* NewAnim=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/%s_v014/Basic/LL_%s_v014"),*Role,Walk?TEXT("Walk"):TEXT("Idle")));
            if(Shading&&Scaled)NewAnim=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/CharacterReview_v015/%s_Scaled/Basic/LL_%s_v014"),*Role,Walk?TEXT("Walk"):TEXT("Idle")));
            // Looking along -X, camera right is -Y. Keep visual order explicit:
            // screen left=old mesh/old action; center=fresh mesh/old action;
            // screen right=fresh mesh/new action.
            Show(Components[0],Old,260.f);Show(Components[1],New,0.f);Show(Components[2],New,-260.f);
            TArray<UAnimSequence*> Anims={OldAnim,SameAnim,NewAnim};
            if(Shading)
            {
                // Same mesh and pose in all three columns. Only material differs.
                Show(Components[0],New,260.f);
                Show(Components[1],New,0.f);Show(Components[2],New,-260.f);
                Anims={NewAnim,NewAnim,NewAnim};
                auto* Grey=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/CharacterReview_v015/M_Grey"));
                auto* Normals=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/CharacterReview_v015/M_VertexNormalWS"));
                check(Grey&&Normals);
                for(int32 Slot=0;Slot<New->GetMaterials().Num();++Slot)
                {Components[1]->SetMaterial(Slot,Grey);Components[2]->SetMaterial(Slot,Normals);}
                if(Sample==0)
                {
                    auto* Data=New->GetResourceForRendering();
                    if(Data&&Data->LODRenderData.Num()>0)
                    {
                        const auto& Buffer=Data->LODRenderData[0].StaticVertexBuffers.StaticMeshVertexBuffer;
                        int32 Bad=0;float Minimum=1.f;
                        if(Buffer.GetTangentData())
                        {
                            for(uint32 V=0;V<Buffer.GetNumVertices();++V)
                            {
                                const auto N=Buffer.VertexTangentZ(V);
                                const float Length=FVector3f(N.X,N.Y,N.Z).Size();
                                Minimum=FMath::Min(Minimum,Length);if(Length<.9f)++Bad;
                            }
                            UE_LOG(LogTemp,Display,TEXT("SHADING_NORMALS_V015 role=%s vertices=%u shortNormals=%d minimumLength=%.6f"),*Role,Buffer.GetNumVertices(),Bad,Minimum);
                        }
                        else UE_LOG(LogTemp,Display,TEXT("SHADING_NORMALS_V015 role=%s CPU_BUFFER_UNAVAILABLE"),*Role);
                    }
                }
            }
            for(int32 I=0;I<3;++I)
            {
                Components[I]->SetCastShadow(!NoShadows);
                if(Sample==1)Components[I]->SetWorldRotation(FRotator(0,0,0));
                if(Anims[I]&&Anims[I]->GetSkeleton()==Components[I]->GetSkeletalMeshAsset()->GetSkeleton())
                {
                    Components[I]->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
                    Components[I]->SetForceRefPose(false);Components[I]->PlayAnimation(Anims[I],false);Components[I]->SetPosition(Anims[I]->GetPlayLength()*Fraction,false);Components[I]->SetPlayRate(0.f);
                    Components[I]->TickAnimation(0.f,false);Components[I]->RefreshBoneTransforms();Components[I]->UpdateBounds();
                }
                else UE_LOG(LogTemp,Error,TEXT("REPLACEMENT_V014_MISSING role=%s column=%d"),*Role,I);
            }
            UE_LOG(LogTemp,Display,TEXT("REPLACEMENT_V014_POSE role=%s walk=%d fraction=%.3f old=%s new=%s"),*Role,Walk,Fraction,OldAnim?*OldAnim->GetPathName():TEXT("MISSING"),NewAnim?*NewAnim->GetPathName():TEXT("MISSING"));
        },1.f+Index*2.5f,false);
        World->GetTimerManager().SetTimer(Shot,[Dir,Role,Label,Components]()
        {
            for(int32 I=0;I<3;++I)
            {
                const FVector S=Components[I]->GetSocketLocation(TEXT("upperarm_r")),E=Components[I]->GetSocketLocation(TEXT("lowerarm_r"));
                const FVector D=(E-S).GetSafeNormal();
                UE_LOG(LogTemp,Display,TEXT("REPLACEMENT_RENDER_BONE role=%s sample=%s column=%d angle=%.3f"),*Role,*Label,I,FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(-D.Z,-1.f,1.f))));
            }
            FScreenshotRequest::RequestScreenshot(Dir/(Role+TEXT("_")+Label+TEXT(".png")),false,false);
        },2.8f+Index*2.5f,false);
    }
    FTimerHandle Exit;World->GetTimerManager().SetTimer(Exit,[](){FPlatformMisc::RequestExit(false);},48.f,false);
}
