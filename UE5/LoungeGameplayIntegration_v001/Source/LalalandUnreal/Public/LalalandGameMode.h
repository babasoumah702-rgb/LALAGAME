#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LalalandGameMode.generated.h"

UCLASS()
class LALALANDUNREAL_API ALalalandGameMode final : public AGameModeBase
{
    GENERATED_BODY()
public:
    ALalalandGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void RunAutomatedPlaythrough(float DeltaSeconds);
    void RecordPlaythrough(const FString& Message, bool bOverwrite = false);
    void FailPlaythrough(const FString& Reason);
    void FocusAutomatedCameraOnBounceTable();
    FString SendPlaythroughCommand(const FString& Type, const FString& Target = FString(), const FString& Intent = FString(), const FString& ObjectTarget = FString(), const FString& Text = FString(), double X = 0.0, double Z = 0.0);

    bool bAutomatedPlaythrough = false;
    bool bNormalPlaythrough = false;
    bool bNormalDeclinePlaythrough = false;
    bool bGiftPlaythrough = false;
    bool bBubblePreviewCaptured = false;
    bool bAutomatedRouteAudit = false;
    bool bPlaythroughFinished = false;
    bool bAimSent = false;
    bool bThrowRequested = false;
    bool bInvitationCommandSent = false;
    bool bRoofClimbStarted = false;
    int32 PlaythroughStep = 0;
    float PlaythroughElapsed = 0.f;
    double NextPlaythroughActionAt = 0.0;
    double NextMotionAuditAt = 0.0;
    FString LastLoggedPhase;
    FString LastPendingThrowId;
    FString PlaythroughReportPath;
};
