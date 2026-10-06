#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LalalandDtos.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "LalalandDialogueLayout.h"

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
    const FString SipJson=TEXT("{\"state\":{\"version\":1,\"sessionId\":\"sip-session\",\"firstNight\":{\"drinks\":[{\"id\":\"terrace_breeze\",\"instanceId\":\"cup-1\",\"owner\":\"USER\",\"status\":\"served\",\"sipActionId\":\"sip-1\",\"sipStartedAt\":12.5},{\"id\":\"lime_soda_0\",\"instanceId\":\"legacy-cup\",\"owner\":\"USER\",\"status\":\"served\"}]}}}");
    FLalalandStateDto SipState;
    TestTrue(TEXT("sip action parses"),FLalalandJson::ParseStateEnvelope(SipJson,SipState,Error));
    if(SipState.firstNight.drinks.Num()==2)
    {
        TestEqual(TEXT("sip bound to cup"),SipState.firstNight.drinks[0].instanceId,FString(TEXT("cup-1")));
        TestEqual(TEXT("sip action id"),SipState.firstNight.drinks[0].sipActionId,FString(TEXT("sip-1")));
        TestEqual(TEXT("sip start time"),SipState.firstNight.drinks[0].sipStartedAt,12.5);
        TestTrue(TEXT("legacy cup does not replay sip"),SipState.firstNight.drinks[1].sipActionId.IsEmpty());
    }
    else AddError(TEXT("sip cup records missing"));
    const FString GiftJson=TEXT("{\"state\":{\"version\":1,\"sessionId\":\"gift-session\",\"firstNight\":{\"drinks\":[{\"instanceId\":\"cup-2\",\"owner\":\"USER\",\"status\":\"served\",\"deliveryOfferId\":\"offer-2\",\"deliveryActionId\":\"handoff-2\",\"deliveryTarget\":\"B\",\"deliveryStartedAt\":15.5}]}}}");
    FLalalandStateDto GiftState;
    TestTrue(TEXT("physical gift parses"),FLalalandJson::ParseStateEnvelope(GiftJson,GiftState,Error));
    if(GiftState.firstNight.drinks.Num()==1)
    {
        TestEqual(TEXT("giver owns pending cup"),GiftState.firstNight.drinks[0].owner,FString(TEXT("USER")));
        TestEqual(TEXT("handoff bound to action"),GiftState.firstNight.drinks[0].deliveryActionId,FString(TEXT("handoff-2")));
        TestEqual(TEXT("handoff target"),GiftState.firstNight.drinks[0].deliveryTarget,FString(TEXT("B")));
        TestEqual(TEXT("handoff time"),GiftState.firstNight.drinks[0].deliveryStartedAt,15.5);
    }
    else AddError(TEXT("physical gift cup missing"));
    TestTrue(TEXT("legacy cup does not replay handoff"),SipState.firstNight.drinks.Num()==2&&SipState.firstNight.drinks[1].deliveryActionId.IsEmpty());
    const FString NpcCupJson=TEXT(R"({"state":{"version":1,"sessionId":"npc-cup","firstNight":{"relationshipVersion":"first-night-reaction-v0.2","attitudes":[{"id":"B","affinity":16}],"drinks":[{"owner":"B","status":"consumed","sipPhase":"lower","cupPlacement":"hand"}]}}})");
    FLalalandStateDto NpcCupState;
    TestTrue(TEXT("NPC cup and relationship parse"),FLalalandJson::ParseStateEnvelope(NpcCupJson,NpcCupState,Error));
    if(NpcCupState.firstNight.drinks.Num()==1)
    {
        TestEqual(TEXT("separate putdown stage"),NpcCupState.firstNight.drinks[0].sipPhase,FString(TEXT("lower")));
        TestEqual(TEXT("cup remains held after consumption"),NpcCupState.firstNight.drinks[0].cupPlacement,FString(TEXT("hand")));
    }
    else AddError(TEXT("NPC cup missing"));
    TestEqual(TEXT("new relation contract"),NpcCupState.firstNight.relationshipVersion,FString(TEXT("first-night-reaction-v0.2")));
    if(NpcCupState.firstNight.attitudes.Num()==1)TestEqual(TEXT("affinity is a derived integer"),NpcCupState.firstNight.attitudes[0].affinity,16);
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
        TestFalse(TEXT("unspecified actor is absent, never an invalid empty id"),Object->HasField(TEXT("actor")));
    }
    Command.actor=TEXT("D");Command.type=TEXT("drink_effect");
    Object.Reset();
    const auto ActorReader=TJsonReaderFactory<>::Create(FLalalandJson::WriteCommand(Command));
    TestTrue(TEXT("explicit NPC actor serializes"),FJsonSerializer::Deserialize(ActorReader,Object));
    if(Object.IsValid())TestEqual(TEXT("NPC effect keeps its actor"),Object->GetStringField(TEXT("actor")),FString(TEXT("D")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandBodyReportTest, "Lalaland.Protocol.BodyReportsAndReplies", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLalalandBodyReportTest::RunTest(const FString& Parameters)
{
    FLalalandCommandDto Command;
    Command.id = TEXT("body-frame"); Command.type = TEXT("positions");
    FLalalandPositionDto Position;
    Position.actor = TEXT("B"); Position.x = 6.3; Position.z = -6.25; Position.area = TEXT("corridor");
    Command.items.Add(Position);
    TSharedPtr<FJsonObject> Object;
    auto Reader = TJsonReaderFactory<>::Create(FLalalandJson::WriteCommand(Command));
    TestTrue(TEXT("body report is valid JSON"), FJsonSerializer::Deserialize(Reader, Object));
    if (Object.IsValid())
    {
        const auto& Items = Object->GetArrayField(TEXT("items"));
        TestEqual(TEXT("one real body report"), Items.Num(), 1);
        if (Items.Num()) TestEqual(TEXT("report retains actor"), Items[0]->AsObject()->GetStringField(TEXT("actor")), FString(TEXT("B")));
    }
    FLalalandStateDto State;
    FString Error;
    TestTrue(TEXT("reply ownership parses"), FLalalandJson::ParseStateEnvelope(TEXT("{\"state\":{\"version\":1,\"sessionId\":\"reply-test\",\"replies\":[{\"actor\":\"B\",\"playerInitiated\":true},{\"actor\":\"C\"}]}}"), State, Error));
    if (State.replies.Num() == 2)
    {
        TestTrue(TEXT("direct player reply"), State.replies[0].playerInitiated);
        TestFalse(TEXT("old or NPC reply does not lock player input"), State.replies[1].playerInitiated);
    }
    else AddError(TEXT("Reply DTO list did not parse"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandCommandErrorTest, "Lalaland.Protocol.CommandError", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLalalandCommandErrorTest::RunTest(const FString& Parameters)
{
    FLalalandStateDto State;
    FString Error;
    TestFalse(TEXT("rejection is not a state"), FLalalandJson::ParseStateEnvelope(TEXT("{\"error\":\"请先靠近对方再交流\"}"), State, Error));
    TestEqual(TEXT("server reason survives HTTP rejection"), Error, FString(TEXT("请先靠近对方再交流")));
    Error.Reset();
    TestFalse(TEXT("missing error is safe"), FLalalandJson::ParseStateEnvelope(TEXT("{}"), State, Error));
    TestFalse(TEXT("missing error has a useful fallback"), Error.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLalalandBubbleLayoutTest, "Lalaland.UI.BubbleLayout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLalalandBubbleLayoutTest::RunTest(const FString& Parameters)
{
    for (const FVector2D Viewport : {FVector2D(1280,720), FVector2D(1920,1080), FVector2D(1280,800)})
    {
        TArray<FBox2D> Occupied = {FBox2D(FVector2D(20,20),FVector2D(340,290)),
                                 FBox2D(FVector2D(Viewport.X*.5-390,Viewport.Y-190),FVector2D(Viewport.X*.5+390,Viewport.Y))};
        const FVector2D Size(280,92);
        for (int32 Speaker=0;Speaker<4;++Speaker)
        {
            const FVector2D Anchor(Viewport.X*.5+Speaker*22,Viewport.Y*.65);
            FVector2D TopLeft;
            const bool bFits=LalalandDialogueLayout::Place(Anchor,Size,Viewport,Occupied,TopLeft);
            TestTrue(TEXT("four nearby speakers fit without overlap"),bFits);
            if (!bFits) continue;
            const FBox2D Box(TopLeft,TopLeft+Size);
            for (const FBox2D& Other:Occupied) TestFalse(TEXT("bubble avoids HUD and earlier speakers"),Box.Intersect(Other));
            TestTrue(TEXT("bubble stays above speaker"),TopLeft.Y+Size.Y<=Anchor.Y);
            TestTrue(TEXT("bubble stays on screen"),TopLeft.X>=0&&TopLeft.Y>=0&&TopLeft.X+Size.X<=Viewport.X&&TopLeft.Y+Size.Y<=Viewport.Y);
            Occupied.Add(Box);
        }
        FVector2D Unused;
        TestFalse(TEXT("overflow is hidden rather than overlapping"),LalalandDialogueLayout::Place(FVector2D(20,20),Size,Viewport,{FBox2D(FVector2D::ZeroVector,Viewport)},Unused));
    }
    return true;
}

#endif
