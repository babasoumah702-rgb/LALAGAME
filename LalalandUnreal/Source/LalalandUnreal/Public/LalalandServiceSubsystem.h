#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LalalandDtos.h"
#include "LalalandServiceSubsystem.generated.h"

class IWebSocket;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLalalandServiceChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLalalandServiceMessage, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLalalandCommandResult, const FString&, CommandId, const FString&, Reason);

UCLASS(BlueprintType)
class LALALANDUNREAL_API ULalalandServiceSubsystem final : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }

    UPROPERTY(BlueprintAssignable) FLalalandServiceChanged OnChanged;
    UPROPERTY(BlueprintAssignable) FLalalandServiceMessage OnError;
    UPROPERTY(BlueprintAssignable) FLalalandServiceMessage OnStatus;
    UPROPERTY(BlueprintAssignable) FLalalandCommandResult OnCommandAcknowledged;
    UPROPERTY(BlueprintAssignable) FLalalandCommandResult OnCommandRejected;

    UFUNCTION(BlueprintPure) bool IsReady() const { return bReady; }
    UFUNCTION(BlueprintPure) const FLalalandBootstrapDto& GetBootstrap() const { return Bootstrap; }
    UFUNCTION(BlueprintPure) const FLalalandStateDto& GetState() const { return State; }
    UFUNCTION(BlueprintPure) FString GetStatusText() const { return StatusText; }
    UFUNCTION(BlueprintPure) FString GetPlayerId() const { return PlayerId; }

    UFUNCTION(BlueprintCallable) void FetchBootstrap();
    UFUNCTION(BlueprintCallable) void OpenNewSession(const FString& Role, const FString& Intent, const FString& Style, bool bOnline);
    UFUNCTION(BlueprintCallable) void ConfigureModel(const FString& ApiBase, const FString& Model, const FString& ApiKey, bool bKeepExistingKey);
    UFUNCTION(BlueprintCallable) FString SendCommand(FLalalandCommandDto Command);
    UFUNCTION(BlueprintCallable) void Save();

private:
    void StartLocalService();
    void StopLocalService();
    void ConsumeServiceOutput();
    void HandleServiceLine(const FString& Line);
    void ConnectEvents();
    void AcceptState(const FLalalandStateDto& Incoming);
    void Fail(const FString& Message);
    void SetStatus(const FString& Message);
    void Request(const FString& Verb, const FString& Path, const FString& Body, TFunction<void(bool, const FString&)> Completion);
    FString ResolveServerRoot() const;
    FString ResolveNodePath(const FString& ServerRoot) const;
    FString LoadOrCreatePlayerId();

    FLalalandBootstrapDto Bootstrap;
    FLalalandStateDto State;
    FString StatusText;
    FString PlayerId;
    FString BaseUrl;
    FString SessionToken;
    FString ServiceOutputBuffer;
    FProcHandle ServiceProcess;
    void* ServiceReadPipe = nullptr;
    void* ServiceWritePipe = nullptr;
    void* ServiceStdInReadPipe = nullptr;
    void* ServiceStdInWritePipe = nullptr;
    TSharedPtr<IWebSocket> EventSocket;
    TMap<FString, FString> PendingCommands;
    TMap<FString, FLalalandEventDto> VisibleEvents;
    double StartupDeadline = 0;
    bool bReady = false;
    bool bStopping = false;
    bool bIntroReadySent = false;
    bool bIntroCompleteSent = false;
    bool bHttpFallback = false;
    bool bPollInFlight = false;
    bool bEventChannelReady = false;
    double NextPollAt = 0;
};
