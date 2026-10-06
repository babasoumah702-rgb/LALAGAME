#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "MixologyCatalog.h"
#include "MixologySequence.h"
#include "MixologyGameMode.generated.h"
UCLASS()
class AMixologyHUD : public AHUD {
 GENERATED_BODY()
public:
 AMixologyHUD();
 // Explicit standalone recording adapter, never an online-service fallback.
 bool RecordingDemo=false;
 int32 RecordingCost=0, RecordingCompleted=0;
 FString RecordingRecipeId;
 UPROPERTY() TObjectPtr<class ALalalandGlassProp> RecordingCup;
 float RecordingSipStart=-1;
 bool RecordingConsumed=false;
 void OpenRecording();
 void TickRecording(float DeltaSeconds);
 void SipRecording();
 void ResetRecording();
 virtual void DrawHUD() override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void NotifyHitBoxClick(FName BoxName) override;
 bool Active=false;
 bool EntryMenu=false;
 FString EntryCommandId;
 float EntryFeedbackUntil=0;
 bool CanEnter() const;
 void InteractAtBar();
 void SelectEntry(bool IsOrder);
 bool Awaiting=false;
 bool CompletionSent=false;
 FString CommandId;
 double SentAt=0;
 UPROPERTY() TObjectPtr<class ULalalandServiceSubsystem> Service;
 UPROPERTY() TObjectPtr<class AMixologyStation> Station;
 UPROPERTY() TArray<TObjectPtr<class UUserWidget>> MainWidgets;
 TWeakObjectPtr<AActor> PreviousView;
 UFUNCTION() void Rejected(const FString& Id,const FString& Reason);
 UFUNCTION() void Acknowledged(const FString& Id,const FString& Reason);
 void ReturnToBar();
 void RunSmoke();
 int32 SmokePhase=0;
 float SmokeAt=0;
 void Confirm();
 void ResetEvening();
 void AddMaterial(int32 Index);
 // Display mirror only. All payment and voucher changes belong to the server.
 int32 Cash=0;
 FMixCatalog Catalog;
 TMap<FString,int32> Amounts;
 int32 Selection=0, MenuIndex=0;
 bool OrderMode=false, Making=false, Ready=false;
 bool Pouring=false;
 bool BeautyMode=false;
 bool ShakeRequested=false;
 float CompletionTime=0;
 FString Notice=TEXT("选择材料加入，再调整用量；确认制作时才扣费。");
 TArray<FString> Cups;
 TMap<FString,int32> LastPreparedAmounts;
 FString LastPreparedName;
 TMap<FString,int32> PendingAmounts;
 FString PendingName, PendingResult;
 FMixSequence Sequence;
 UPROPERTY() TObjectPtr<class UFont> RuntimeFont;
 void Write(const FString& Text,FLinearColor Color,float X,float Y,float Scale=1.f);
 void Button(const FString& Id,const FString& Text,float X,float Y,float W,float H,bool Enabled=true);
};
UCLASS()
class AMixologyStation : public AActor {
 GENERATED_BODY()
public:
 AMixologyStation();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 bool StartPour(int32 MaterialIndex);
 TArray<TWeakObjectPtr<AActor>> BottleActors;
 TArray<FVector> BottleHomes;
 TWeakObjectPtr<AActor> ActiveBottle, Shaker, ShakerCap, Spoon, Liquid, Stream, LiquidSurface;
 bool DrinkVisualV005=false;
 TWeakObjectPtr<class ACameraActor> StudioCamera;
 FVector CameraHome=FVector::ZeroVector, CameraClose=FVector::ZeroVector;
 FQuat CameraHomeRotation=FQuat::Identity, CameraCloseRotation=FQuat::Identity;
 float CameraHomeFOV=66, CameraCloseFOV=52, FocusBlend=0;
 bool HasCloseCamera=false;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> LiquidMaterial;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> SurfaceMaterial;
 FVector ActiveHome=FVector::ZeroVector, ShakerHome=FVector::ZeroVector;
 float PourStart=0;
 bool ActivePour=false;
 int32 SmokePhase=0;
 float SmokeStart=0;
 void SetupVisualActors();
};
