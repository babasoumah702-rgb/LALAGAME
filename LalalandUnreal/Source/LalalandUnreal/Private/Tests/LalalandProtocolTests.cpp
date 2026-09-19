#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LalalandDtos.h"

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
    const FString Json = TEXT("{\"type\":\"state\",\"state\":{\"version\":1,\"sessionId\":\"night-1\",\"cursor\":3,\"characters\":[],\"events\":[],\"replies\":[],\"scene1\":{\"phase\":\"first_meeting\"},\"interaction\":{\"contextId\":\"scene1.first_meeting\",\"groups\":[]}}}");
    FLalalandStateDto State;
    FString Error;
    TestTrue(TEXT("state envelope parses"), FLalalandJson::ParseStateEnvelope(Json, State, Error));
    TestEqual(TEXT("session id"), State.sessionId, FString(TEXT("night-1")));
    TestEqual(TEXT("scene phase"), State.scene1.phase, FString(TEXT("first_meeting")));
    return true;
}

#endif
