#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LalalandDtos.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandBootstrapJsonTest, "Lalaland.Protocol.Bootstrap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLalalandBootstrapJsonTest::RunTest(const FString& Parameters)
{
    const FString Json = TEXT("{\"version\":1,\"title\":\"Lalaland\",\"modelConfigured\":false,\"roles\":[{\"id\":\"passerby\",\"name\":\"临时路过\"}],\"intents\":[],\"styles\":[],\"choices\":[]}");
    FLalalandBootstrapDto Bootstrap;
    FString Error;
    TestTrue(TEXT("valid bootstrap parses"), FLalalandJson::ParseBootstrap(Json, Bootstrap, Error));
    TestEqual(TEXT("protocol version"), Bootstrap.version, 1);
    TestEqual(TEXT("UTF-8 role name"), Bootstrap.roles[0].name, FString(TEXT("临时路过")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandStateJsonTest, "Lalaland.Protocol.StateEnvelope", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLalalandStateJsonTest::RunTest(const FString& Parameters)
{
    const FString Json = TEXT("{\"type\":\"state\",\"state\":{\"version\":1,\"sessionId\":\"night-1\",\"cursor\":3,\"characters\":[],\"events\":[],\"replies\":[],\"firstNight\":{\"contentVersion\":\"handoff-v0.2-2026-09-20\",\"phase\":\"game_round\",\"availableCash\":18,\"playerDrinkStage\":\"sober\",\"voucherCount\":0,\"round\":1,\"turn\":0,\"participants\":[\"USER\",\"A\",\"B\",\"C\",\"D\"],\"pendingThrow\":{\"id\":\"bounce:1:0:USER:0\",\"actor\":\"USER\",\"round\":1,\"aim\":0.67,\"power\":0.58,\"startedAt\":80},\"attitudes\":[{\"id\":\"A\",\"name\":\"Kiko\",\"stage\":\"刚认识\",\"reason\":\"今晚还没有共同做过一件具体的事。\",\"drinkStage\":\"sober\"}],\"evaluations\":[],\"keyActions\":[]},\"interaction\":{\"contextId\":\"first-night.game_round\",\"primaryActionId\":\"\",\"primaryLabel\":\"\",\"primaryTargetRequired\":false,\"suggestions\":[],\"groups\":[]}}}");
    FLalalandStateDto State;
    FString Error;
    TestTrue(TEXT("state envelope parses"), FLalalandJson::ParseStateEnvelope(Json, State, Error));
    TestEqual(TEXT("session id"), State.sessionId, FString(TEXT("night-1")));
    TestEqual(TEXT("first-night content version"), State.firstNight.contentVersion, FString(TEXT("handoff-v0.2-2026-09-20")));
    TestEqual(TEXT("first-night phase"), State.firstNight.phase, FString(TEXT("game_round")));
    TestEqual(TEXT("five physical participants"), State.firstNight.participants.Num(), 5);
    TestEqual(TEXT("pending physical actor"), State.firstNight.pendingThrow.actor, FString(TEXT("USER")));
    TestEqual(TEXT("pending physical round"), State.firstNight.pendingThrow.round, 1);
    TestEqual(TEXT("suggestion count"), State.interaction.suggestions.Num(), 0);
    TestEqual(TEXT("cash hud"), State.firstNight.availableCash, 18);
    TestEqual(TEXT("attitude count"), State.firstNight.attitudes.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandAudioContentTest, "Lalaland.Audio.Content", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLalalandAudioContentTest::RunTest(const FString& Parameters)
{
    const TCHAR* Assets[] = {
        TEXT("/Game/Audio/SW_Arrival.SW_Arrival"), TEXT("/Game/Audio/SW_Cup.SW_Cup"),
        TEXT("/Game/Audio/SW_Door.SW_Door"), TEXT("/Game/Audio/SW_Elevator.SW_Elevator"),
        TEXT("/Game/Audio/SW_Lounge.SW_Lounge"), TEXT("/Game/Audio/SW_Phone.SW_Phone")
    };
    for (const TCHAR* Asset : Assets)
    {
        TestNotNull(FString::Printf(TEXT("audio asset loads: %s"), Asset), LoadObject<USoundBase>(nullptr, Asset));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandCommandIdJsonTest, "Lalaland.Protocol.CommandId", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLalalandCommandIdJsonTest::RunTest(const FString& Parameters)
{
    FLalalandCommandDto Command;
    Command.id = TEXT("deterministic-command-id");
    Command.type = TEXT("opening_ball");
    Command.intent = TEXT("return");
    const FString Json = FLalalandJson::WriteCommand(Command);
    TSharedPtr<FJsonObject> Object;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    TestTrue(TEXT("Command JSON parses"), FJsonSerializer::Deserialize(Reader, Object) && Object.IsValid());
    if (Object.IsValid())
    {
        TestEqual(TEXT("Command JSON carries its idempotency id"), Object->GetStringField(TEXT("id")), Command.id);
    }
    return true;
}

#endif
