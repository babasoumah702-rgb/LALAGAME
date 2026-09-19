#include "LalalandServiceSubsystem.h"
#include "LalalandUnreal.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "IWebSocket.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "WebSocketsModule.h"

namespace
{
    FCriticalSection ServiceEnvironmentMutex;

    FString JsonString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
    {
        FString Value;
        if (Object.IsValid()) Object->TryGetStringField(Field, Value);
        return Value;
    }

    bool ParseObject(const FString& Json, TSharedPtr<FJsonObject>& Object)
    {
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
        return FJsonSerializer::Deserialize(Reader, Object) && Object.IsValid();
    }
}

void ULalalandServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PlayerId = LoadOrCreatePlayerId();
    SetStatus(TEXT("正在准备本地关系世界…"));
    StartLocalService();
}

void ULalalandServiceSubsystem::Deinitialize()
{
    StopLocalService();
    Super::Deinitialize();
}

TStatId ULalalandServiceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(ULalalandServiceSubsystem, STATGROUP_Tickables);
}

void ULalalandServiceSubsystem::Tick(float DeltaTime)
{
    ConsumeServiceOutput();
    if (bHttpFallback && bReady && !State.sessionId.IsEmpty() && !bPollInFlight && FPlatformTime::Seconds() >= NextPollAt)
    {
        bPollInFlight = true;
        NextPollAt = FPlatformTime::Seconds() + 0.2;
        Request(TEXT("GET"), TEXT("/api/state"), FString(), [this](bool bSuccess, const FString& Body)
        {
            bPollInFlight = false;
            FLalalandStateDto Incoming;
            FString Error;
            if (bSuccess && FLalalandJson::ParseStateEnvelope(Body, Incoming, Error)) AcceptState(Incoming);
        });
    }
    if (State.intro.phase == TEXT("elevator") && (bEventChannelReady || bHttpFallback))
    {
        if (!State.intro.ready && !bIntroReadySent)
        {
            FLalalandCommandDto Command;
            Command.type = TEXT("intro_ready");
            bIntroReadySent = !SendCommand(Command).IsEmpty();
        }
        else if (State.intro.progress >= 6.9 && !bIntroCompleteSent)
        {
            FLalalandCommandDto Command;
            Command.type = TEXT("intro_complete");
            bIntroCompleteSent = !SendCommand(Command).IsEmpty();
        }
    }
    if (!bReady && ServiceProcess.IsValid() && FPlatformTime::Seconds() > StartupDeadline)
    {
        Fail(TEXT("本地服务没有及时就绪，请检查运行目录是否完整。"));
        StopLocalService();
    }
    if (ServiceProcess.IsValid() && !FPlatformProcess::IsProcRunning(ServiceProcess) && !bStopping)
    {
        Fail(TEXT("本地服务意外退出。玩家输入和模型密钥没有写入游戏日志。"));
        StopLocalService();
    }
}

void ULalalandServiceSubsystem::StartLocalService()
{
    const FString ServerRoot = ResolveServerRoot();
    const FString NodePath = ResolveNodePath(ServerRoot);
    const FString ScriptPath = FPaths::Combine(ServerRoot, TEXT("dist/server.js"));
    if (NodePath.IsEmpty() || !FPaths::FileExists(NodePath))
    {
        Fail(TEXT("找不到随游戏附带的 Node 运行时。"));
        return;
    }
    if (!FPaths::FileExists(ScriptPath))
    {
        Fail(TEXT("缺少本地 AI 后端，请先构建 BarPrototype/Server。"));
        return;
    }

    SessionToken = FGuid::NewGuid().ToString(EGuidFormats::Digits) + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString LocalData = FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"));
    if (LocalData.IsEmpty()) LocalData = FPlatformProcess::UserSettingsDir();
    const FString DataRoot = FPaths::Combine(LocalData, TEXT("LalalandUnreal"));
    const FString ConfigRoot = FPaths::Combine(DataRoot, TEXT("private"));
    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    Files.CreateDirectoryTree(*ConfigRoot);

    FPlatformProcess::CreatePipe(ServiceReadPipe, ServiceWritePipe);
    FPlatformProcess::CreatePipe(ServiceStdInReadPipe, ServiceStdInWritePipe, true);

    const FString PreviousToken = FPlatformMisc::GetEnvironmentVariable(TEXT("LASTCALL_SESSION_TOKEN"));
    const FString PreviousData = FPlatformMisc::GetEnvironmentVariable(TEXT("LASTCALL_DATA_DIR"));
    const FString PreviousConfig = FPlatformMisc::GetEnvironmentVariable(TEXT("LASTCALL_CONFIG_DIR"));
    {
        FScopeLock Lock(&ServiceEnvironmentMutex);
        FPlatformMisc::SetEnvironmentVar(TEXT("LASTCALL_SESSION_TOKEN"), *SessionToken);
        FPlatformMisc::SetEnvironmentVar(TEXT("LASTCALL_DATA_DIR"), *DataRoot);
        FPlatformMisc::SetEnvironmentVar(TEXT("LASTCALL_CONFIG_DIR"), *ConfigRoot);
        uint32 ProcessId = 0;
        const FString Params = FString::Printf(TEXT("\"%s\" --managed"), *ScriptPath);
        ServiceProcess = FPlatformProcess::CreateProc(*NodePath, *Params, false, true, true, &ProcessId, 0, *ServerRoot, ServiceWritePipe, ServiceStdInReadPipe);
        FPlatformMisc::SetEnvironmentVar(TEXT("LASTCALL_SESSION_TOKEN"), *PreviousToken);
        FPlatformMisc::SetEnvironmentVar(TEXT("LASTCALL_DATA_DIR"), *PreviousData);
        FPlatformMisc::SetEnvironmentVar(TEXT("LASTCALL_CONFIG_DIR"), *PreviousConfig);
    }
    if (!ServiceProcess.IsValid())
    {
        Fail(TEXT("本地服务启动失败。"));
        StopLocalService();
        return;
    }
    StartupDeadline = FPlatformTime::Seconds() + 20.0;
}

void ULalalandServiceSubsystem::StopLocalService()
{
    if (bStopping) return;
    bStopping = true;
    if (EventSocket.IsValid())
    {
        EventSocket->Close();
        EventSocket.Reset();
    }
    if (ServiceStdInWritePipe)
    {
        FPlatformProcess::ClosePipe(ServiceStdInReadPipe, ServiceStdInWritePipe);
        ServiceStdInReadPipe = nullptr;
        ServiceStdInWritePipe = nullptr;
    }
    if (ServiceProcess.IsValid())
    {
        const double Deadline = FPlatformTime::Seconds() + 1.5;
        while (FPlatformProcess::IsProcRunning(ServiceProcess) && FPlatformTime::Seconds() < Deadline)
        {
            FPlatformProcess::Sleep(0.02f);
        }
        if (FPlatformProcess::IsProcRunning(ServiceProcess)) FPlatformProcess::TerminateProc(ServiceProcess, true);
        FPlatformProcess::CloseProc(ServiceProcess);
        ServiceProcess.Reset();
    }
    if (ServiceReadPipe || ServiceWritePipe)
    {
        FPlatformProcess::ClosePipe(ServiceReadPipe, ServiceWritePipe);
        ServiceReadPipe = nullptr;
        ServiceWritePipe = nullptr;
    }
    bReady = false;
    bHttpFallback = false;
    bPollInFlight = false;
    bEventChannelReady = false;
}

void ULalalandServiceSubsystem::ConsumeServiceOutput()
{
    if (!ServiceReadPipe) return;
    ServiceOutputBuffer += FPlatformProcess::ReadPipe(ServiceReadPipe);
    FString Line;
    while (ServiceOutputBuffer.Split(TEXT("\n"), &Line, &ServiceOutputBuffer))
    {
        Line.TrimStartAndEndInline();
        if (!Line.IsEmpty()) HandleServiceLine(Line);
    }
}

void ULalalandServiceSubsystem::HandleServiceLine(const FString& Line)
{
    TSharedPtr<FJsonObject> Object;
    if (!ParseObject(Line, Object)) return;
    const FString DiagnosticType = JsonString(Object, TEXT("type"));
    if (DiagnosticType == TEXT("diagnostic"))
    {
        UE_LOG(LogLalaland, Warning, TEXT("Local service diagnostic: %s"), *JsonString(Object, TEXT("message")).Left(160));
        return;
    }
    bool bServiceReady = false;
    double Port = 0;
    Object->TryGetBoolField(TEXT("ready"), bServiceReady);
    Object->TryGetNumberField(TEXT("port"), Port);
    if (bServiceReady && Port > 0)
    {
        BaseUrl = FString::Printf(TEXT("http://127.0.0.1:%d"), static_cast<int32>(Port));
        FetchBootstrap();
    }
}

void ULalalandServiceSubsystem::FetchBootstrap()
{
    if (BaseUrl.IsEmpty()) return;
    const FString Path = TEXT("/api/bootstrap?playerId=") + FGenericPlatformHttp::UrlEncode(PlayerId);
    Request(TEXT("GET"), Path, FString(), [this](bool bSuccess, const FString& Body)
    {
        FString Error;
        if (!bSuccess || !FLalalandJson::ParseBootstrap(Body, Bootstrap, Error))
        {
            Fail(Error.IsEmpty() ? TEXT("无法读取本地入口配置。") : Error);
            return;
        }
        bReady = true;
        SetStatus(TEXT("准备好了。选择今晚如何入场。"));
        ConnectEvents();
        OnChanged.Broadcast();
#if !UE_BUILD_SHIPPING
        if (FParse::Param(FCommandLine::Get(), TEXT("LalalandAutoStart")))
        {
            OpenNewSession(TEXT("passerby"), TEXT("observe_only"), TEXT("natural"), false);
        }
#endif
    });
}

void ULalalandServiceSubsystem::OpenNewSession(const FString& Role, const FString& Intent, const FString& Style, bool bOnline)
{
    FLalalandSessionRequest Session;
    Session.playerId = PlayerId;
    Session.role = Role.IsEmpty() ? TEXT("passerby") : Role;
    Session.entryIntent = Intent.IsEmpty() ? TEXT("observe_only") : Intent;
    Session.style = Style.IsEmpty() ? TEXT("natural") : Style;
    Session.entryMode = Session.role == TEXT("event_guest") || Session.role == TEXT("staff") ? TEXT("event_guest") : TEXT("solo");
    Session.online = bOnline;
    Request(TEXT("POST"), TEXT("/api/session"), FLalalandJson::WriteSession(Session), [this](bool bSuccess, const FString& Body)
    {
        FString Error;
        FLalalandStateDto Incoming;
        if (!bSuccess || !FLalalandJson::ParseStateEnvelope(Body, Incoming, Error))
        {
            Fail(Error.IsEmpty() ? TEXT("无法开始这一晚。") : Error);
            return;
        }
        PendingCommands.Empty();
        VisibleEvents.Empty();
        bIntroReadySent = false;
        bIntroCompleteSent = false;
        AcceptState(Incoming);
    });
}

void ULalalandServiceSubsystem::ConfigureModel(const FString& ApiBase, const FString& Model, const FString& ApiKey, bool bKeepExistingKey)
{
    TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("base"), ApiBase);
    Body->SetStringField(TEXT("model"), Model);
    Body->SetStringField(TEXT("key"), ApiKey);
    Body->SetBoolField(TEXT("keepKey"), bKeepExistingKey);
    Body->SetBoolField(TEXT("clearKey"), ApiKey.IsEmpty() && !bKeepExistingKey);
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Body, Writer);
    Request(TEXT("POST"), TEXT("/api/model-config"), Json, [this](bool bSuccess, const FString&)
    {
        if (!bSuccess)
        {
            Fail(TEXT("模型配置保存失败，请检查接口地址和模型名。"));
            return;
        }
        SetStatus(TEXT("模型配置已保存。"));
        FetchBootstrap();
    });
}

FString ULalalandServiceSubsystem::SendCommand(FLalalandCommandDto Command)
{
    if (Command.id.IsEmpty()) Command.id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Command.version = 1;
    Command.cursor = State.cursor;
    Command.sessionId = State.sessionId;
    if (!bEventChannelReady && !bHttpFallback)
    {
        if (Command.type == TEXT("position") || Command.type == TEXT("positions")) return FString();
        Fail(TEXT("本地事件连接尚未就绪。"));
        return FString();
    }
    if (PendingCommands.Contains(Command.id)) return Command.id;
    const FString Json = FLalalandJson::WriteCommand(Command);
    PendingCommands.Add(Command.id, Json);
    if (bEventChannelReady && EventSocket.IsValid())
    {
        EventSocket->Send(Json);
    }
    else
    {
        const FString CommandId = Command.id;
        Request(TEXT("POST"), TEXT("/api/command"), Json, [this, CommandId](bool bSuccess, const FString& Body)
        {
            PendingCommands.Remove(CommandId);
            FLalalandStateDto Incoming;
            FString Error;
            if (bSuccess && FLalalandJson::ParseStateEnvelope(Body, Incoming, Error))
            {
                AcceptState(Incoming);
                OnCommandAcknowledged.Broadcast(CommandId, FString());
            }
            else
            {
                const FString Reason = Error.IsEmpty() ? TEXT("本地命令提交失败。") : Error;
                OnCommandRejected.Broadcast(CommandId, Reason);
                Fail(Reason);
            }
        });
    }
    return Command.id;
}

void ULalalandServiceSubsystem::Save()
{
    Request(TEXT("POST"), TEXT("/api/save"), TEXT("{}"), [this](bool bSuccess, const FString&)
    {
        SetStatus(bSuccess ? TEXT("已保存。") : TEXT("保存失败，请稍后重试。"));
    });
}

void ULalalandServiceSubsystem::ConnectEvents()
{
    if (EventSocket.IsValid()) EventSocket->Close();
    bEventChannelReady = false;
    FWebSocketsModule& Module = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
    TMap<FString, FString> Headers;
    // WinHTTP treats Authorization as a reserved upgrade header on some
    // Windows configurations. A dedicated local-only header keeps the token
    // out of the URL and reliably reaches Fastify during the WS handshake.
    Headers.Add(TEXT("X-Lalaland-Token"), SessionToken);
    EventSocket = Module.CreateWebSocket(BaseUrl.Replace(TEXT("http://"), TEXT("ws://")) + TEXT("/api/events"), FString(), Headers);
    EventSocket->OnConnected().AddLambda([this]()
    {
        SetStatus(TEXT("正在建立本地事件通道…"));
    });
    EventSocket->OnConnectionError().AddLambda([this](const FString& Error)
    {
        UE_LOG(LogLalaland, Warning, TEXT("Local event socket failed: %s"), *Error.Left(240));
        bEventChannelReady = false;
        bHttpFallback = true;
        NextPollAt = 0;
        SetStatus(TEXT("本地事件连接已切换到兼容模式。"));
    });
    EventSocket->OnClosed().AddLambda([this](int32, const FString&, bool)
    {
        if (!bStopping)
        {
            bEventChannelReady = false;
            bHttpFallback = true;
            NextPollAt = 0;
            SetStatus(TEXT("本地事件连接已切换到兼容模式。"));
        }
    });
    EventSocket->OnMessage().AddLambda([this](const FString& Message)
    {
        TSharedPtr<FJsonObject> Object;
        if (!ParseObject(Message, Object)) return;
        const FString Type = JsonString(Object, TEXT("type"));
        const FString Id = JsonString(Object, TEXT("id"));
        if (Type == TEXT("ack"))
        {
            bEventChannelReady = true;
            bHttpFallback = false;
            PendingCommands.Remove(Id);
            OnCommandAcknowledged.Broadcast(Id, FString());
            return;
        }
        if (Type == TEXT("error"))
        {
            PendingCommands.Remove(Id);
            const FString Reason = JsonString(Object, TEXT("message"));
            OnCommandRejected.Broadcast(Id, Reason);
            Fail(Reason);
            return;
        }
        FLalalandStateDto Incoming;
        FString Error;
        if (FLalalandJson::ParseStateEnvelope(Message, Incoming, Error))
        {
            bEventChannelReady = true;
            bHttpFallback = false;
            AcceptState(Incoming);
        }
    });
    EventSocket->Connect();
}

void ULalalandServiceSubsystem::AcceptState(const FLalalandStateDto& Incoming)
{
    if (State.sessionId != Incoming.sessionId)
    {
        VisibleEvents.Empty();
        UE_LOG(LogLalaland, Log, TEXT("Session state accepted: chapter phase=%s actors=%d"), *Incoming.scene1.phase, Incoming.characters.Num());
    }
    if (State.intro.phase != Incoming.intro.phase || State.scene1.phase != Incoming.scene1.phase)
    {
        UE_LOG(LogLalaland, Log, TEXT("Story state changed: intro=%s scene1=%s"), *Incoming.intro.phase, *Incoming.scene1.phase);
    }
    for (const FLalalandEventDto& Event : Incoming.events) VisibleEvents.Add(Event.id, Event);
    State = Incoming;
    State.events.Reset();
    VisibleEvents.GenerateValueArray(State.events);
    State.events.Sort([](const FLalalandEventDto& A, const FLalalandEventDto& B) { return A.seq < B.seq; });
    OnChanged.Broadcast();
}

void ULalalandServiceSubsystem::Fail(const FString& Message)
{
    StatusText = Message;
    UE_LOG(LogLalaland, Warning, TEXT("LALALAND_STATUS %s"), *Message);
    OnError.Broadcast(Message);
    OnChanged.Broadcast();
}

void ULalalandServiceSubsystem::SetStatus(const FString& Message)
{
    StatusText = Message;
    OnStatus.Broadcast(Message);
    OnChanged.Broadcast();
}

void ULalalandServiceSubsystem::Request(const FString& Verb, const FString& Path, const FString& Body, TFunction<void(bool, const FString&)> Completion)
{
    if (BaseUrl.IsEmpty())
    {
        Completion(false, FString());
        return;
    }
    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Http = FHttpModule::Get().CreateRequest();
    Http->SetURL(BaseUrl + Path);
    Http->SetVerb(Verb);
    Http->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + SessionToken);
    Http->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
    Http->SetTimeout(20.0f);
    if (!Body.IsEmpty()) Http->SetContentAsString(Body);
    Http->OnProcessRequestComplete().BindLambda([Path, Completion = MoveTemp(Completion)](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
    {
        const bool bSuccess = bConnected && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
        if (!bSuccess)
        {
            const int32 Status = Response.IsValid() ? Response->GetResponseCode() : 0;
            UE_LOG(LogLalaland, Warning, TEXT("Local service request failed: %s status=%d connected=%d"), *Path, Status, bConnected ? 1 : 0);
        }
        Completion(bSuccess, Response.IsValid() ? Response->GetContentAsString() : FString());
    });
    Http->ProcessRequest();
}

FString ULalalandServiceSubsystem::ResolveServerRoot() const
{
    const FString Packaged = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Server")));
    if (FPaths::DirectoryExists(Packaged)) return Packaged;
    const FString PackagedContent = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Server")));
    if (FPaths::DirectoryExists(PackagedContent)) return PackagedContent;
    return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("../BarPrototype/Server")));
}

FString ULalalandServiceSubsystem::ResolveNodePath(const FString& ServerRoot) const
{
#if PLATFORM_WINDOWS
    const FString Bundled = FPaths::Combine(ServerRoot, TEXT("node.exe"));
#else
    const FString Bundled = FPaths::Combine(ServerRoot, TEXT("node"));
#endif
    if (FPaths::FileExists(Bundled)) return Bundled;
#if WITH_EDITOR && PLATFORM_WINDOWS
    if (FPaths::FileExists(TEXT("D:/node.exe"))) return TEXT("D:/node.exe");
#endif
    return FString();
}

FString ULalalandServiceSubsystem::LoadOrCreatePlayerId()
{
    FString Id;
    GConfig->GetString(TEXT("Lalaland.Profile"), TEXT("PlayerId"), Id, GGameIni);
    if (Id.IsEmpty())
    {
        Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        GConfig->SetString(TEXT("Lalaland.Profile"), TEXT("PlayerId"), *Id, GGameIni);
        GConfig->Flush(false, GGameIni);
    }
    return Id;
}
