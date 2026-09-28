#include "LalalandRootWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "LalalandDtos.h"
#include "LalalandServiceSubsystem.h"
#include "LalalandNpcCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

namespace
{
    FSlateFontInfo LalalandFont(int32 Size)
    {
        const FString FontPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Fonts/NotoSansSC.ttf"));
        return FPaths::FileExists(FontPath) ? FSlateFontInfo(FontPath, Size) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
    }
}

void ULalalandActionButton::InitializeAction(ULalalandRootWidget* InOwner, const FString& InPayload)
{
    Owner = InOwner;
    Payload = InPayload;
    OnClicked.AddDynamic(this, &ULalalandActionButton::ForwardClick);
}

void ULalalandActionButton::ForwardClick()
{
    if (Owner.IsValid()) Owner->HandleAction(Payload);
}

void ULalalandRootWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Service = GetGameInstance()->GetSubsystem<ULalalandServiceSubsystem>();
    if (Service)
    {
        Service->OnChanged.AddDynamic(this, &ULalalandRootWidget::Refresh);
        Service->OnError.AddDynamic(this, &ULalalandRootWidget::HandleError);
        Service->OnCommandAcknowledged.AddDynamic(this, &ULalalandRootWidget::HandleAck);
        Service->OnCommandRejected.AddDynamic(this, &ULalalandRootWidget::HandleReject);
    }
    Refresh();
}

TSharedRef<SWidget> ULalalandRootWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget) BuildWidgetTree();
    return Super::RebuildWidget();
}

void ULalalandRootWidget::BuildWidgetTree()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
    WidgetTree->RootWidget = Root;

    IntroBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IntroBackdrop"));
    IntroBackdrop->SetBrushColor(FLinearColor(.015f, .018f, .022f, .96f));
    Root->AddChild(IntroBackdrop);
    IntroPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntroPanel"));
    IntroBackdrop->SetContent(IntroPanel);
    AddText(IntroPanel, TEXT("LALALAND"), 46, FLinearColor(.92f, .78f, .58f));
    IntroProgress = AddText(IntroPanel, TEXT("酒吧首夜 · 新版"), 15, FLinearColor(.55f, .58f, .62f));
    IntroPrompt = AddText(IntroPanel, TEXT("今晚，先从一件小事开始。"), 28, FLinearColor::White);
    IntroChoices = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntroChoices"));
    IntroPanel->AddChildToVerticalBox(IntroChoices);

    UBorder* ConfigBorder = WidgetTree->ConstructWidget<UBorder>();
    ConfigBorder->SetBrushColor(FLinearColor(.045f, .05f, .06f, .94f));
    IntroPanel->AddChildToVerticalBox(ConfigBorder);
    UVerticalBox* Config = WidgetTree->ConstructWidget<UVerticalBox>();
    ConfigBorder->SetContent(Config);
    AddText(Config, TEXT("模型 API（可稍后填写）"), 18, FLinearColor(.8f, .82f, .85f));
    ApiBaseInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    ApiBaseInput->SetHintText(FText::FromString(TEXT("https://api.openai.com/v1")));
    Config->AddChildToVerticalBox(ApiBaseInput);
    ModelInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    ModelInput->SetHintText(FText::FromString(TEXT("模型 ID")));
    Config->AddChildToVerticalBox(ModelInput);
    ApiKeyInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    ApiKeyInput->SetHintText(FText::FromString(TEXT("API Key（不会写入游戏日志）")));
    ApiKeyInput->SetIsPassword(true);
    Config->AddChildToVerticalBox(ApiKeyInput);
    AddButton(Config, TEXT("保存模型配置"), TEXT("system:model"));
    StatusText = AddText(IntroPanel, TEXT("正在准备本地关系世界…"), 15, FLinearColor(.72f, .75f, .78f));

    USizeBox* GameSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GameSize"));
    GameSize->SetWidthOverride(430.f);
    GameBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GameBackdrop"));
    GameBackdrop->SetBrushColor(FLinearColor(.012f, .016f, .022f, .86f));
    GameBackdrop->SetPadding(FMargin(8.f));
    GameSize->SetContent(GameBackdrop);
    UOverlaySlot* GameSlot = Root->AddChildToOverlay(GameSize);
    GameSlot->SetHorizontalAlignment(HAlign_Left);
    GameSlot->SetVerticalAlignment(VAlign_Top);
    GameSlot->SetPadding(FMargin(22.f));
    GamePanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GamePanel"));
    GameBackdrop->SetContent(GamePanel);
    ObjectiveTitle = AddText(GamePanel, TEXT("当前目标"), 24, FLinearColor(.95f, .78f, .45f));
    ObjectiveHint = AddText(GamePanel, TEXT("等待场景状态…"), 16, FLinearColor(.88f, .88f, .9f));
    StatusHud = AddText(GamePanel, TEXT("Cash 18 · 清醒"), 14, FLinearColor(.82f, .72f, .48f));
    AttitudeHud = AddText(GamePanel, TEXT("靠近并看向一个人，即可开始交谈"), 13, FLinearColor(.7f, .72f, .75f));
    UHorizontalBox* SystemRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    GamePanel->AddChildToVerticalBox(SystemRow);
    AddButton(SystemRow, TEXT("暂停"), TEXT("system:pause"));
    AddButton(SystemRow, TEXT("微醺视效"), TEXT("system:intox"));

    SettlementBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettlementBackdrop"));
    SettlementBackdrop->SetBrushColor(FLinearColor(.012f, .014f, .02f, .94f));
    SettlementBackdrop->SetPadding(FMargin(28.f));
    SettlementBackdrop->SetVisibility(ESlateVisibility::Collapsed);
    UOverlaySlot* SettlementSlot = Root->AddChildToOverlay(SettlementBackdrop);
    SettlementSlot->SetHorizontalAlignment(HAlign_Center);
    SettlementSlot->SetVerticalAlignment(VAlign_Center);
    SettlementPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettlementPanel"));
    SettlementBackdrop->SetContent(SettlementPanel);

    USizeBox* BottomSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BottomSize"));
    BottomSize->SetWidthOverride(780.f);
    UBorder* BottomBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BottomBackdrop"));
    BottomBackdrop->SetBrushColor(FLinearColor(.018f, .02f, .025f, .76f));
    BottomBackdrop->SetPadding(FMargin(10.f));
    BottomSize->SetContent(BottomBackdrop);
    UOverlaySlot* BottomSlot = Root->AddChildToOverlay(BottomSize);
    BottomSlot->SetHorizontalAlignment(HAlign_Center);
    BottomSlot->SetVerticalAlignment(VAlign_Bottom);
    BottomSlot->SetPadding(FMargin(18.f, 18.f, 18.f, 26.f));
    InteractionPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InteractionPanel"));
    BottomBackdrop->SetContent(InteractionPanel);

    PrimaryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PrimaryRow"));
    InteractionPanel->AddChildToVerticalBox(PrimaryRow);
    BuildPrimaryRow();

    TargetPrompt = AddText(InteractionPanel, TEXT("靠近并看向一个人，即可开始交谈"), 14, FLinearColor(.72f, .70f, .66f));
    TargetRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(TargetRow);
    SecondaryOptions = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(SecondaryOptions);

    DialogueRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(DialogueRow);
    DialogueInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    DialogueInput->SetHintText(FText::FromString(TEXT("自由输入你想说的话…")));
    DialogueInput->OnTextCommitted.AddDynamic(this, &ULalalandRootWidget::HandleDialogueCommitted);
    UHorizontalBoxSlot* InputSlot = DialogueRow->AddChildToHorizontalBox(DialogueInput);
    InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ULalalandActionButton* Send = AddButton(DialogueRow, TEXT("发送"), TEXT("system:send"));
    Send->SetToolTipText(FText::FromString(TEXT("只让当前选择的人回应")));
    PlayerDialogue = AddText(InteractionPanel, FString(), 15, FLinearColor(.82f, .82f, .84f));
    UpdateInteractionVisibility();
}

UTextBlock* ULalalandRootWidget::AddText(UVerticalBox* Parent, const FString& Text, int32 Size, const FLinearColor& Color)
{
    UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
    Block->SetText(FText::FromString(Text));
    Block->SetFont(LalalandFont(Size));
    Block->SetColorAndOpacity(FSlateColor(Color));
    Block->SetAutoWrapText(true);
    UVerticalBoxSlot* TextSlot = Parent->AddChildToVerticalBox(Block);
    TextSlot->SetPadding(FMargin(18, 7));
    return Block;
}

ULalalandActionButton* ULalalandRootWidget::AddButton(UVerticalBox* Parent, const FString& Label, const FString& Payload, bool bEnabled, bool bSelected)
{
    ULalalandActionButton* Button = WidgetTree->ConstructWidget<ULalalandActionButton>();
    Button->InitializeAction(this, Payload);
    Button->SetIsEnabled(bEnabled && !bSelected);
    Button->SetBackgroundColor(bSelected ? FLinearColor(.18f, .18f, .19f, .8f) : FLinearColor(.08f, .09f, .11f, .86f));
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Label));
    Text->SetFont(LalalandFont(17));
    Text->SetColorAndOpacity(FSlateColor(bEnabled ? FLinearColor::White : FLinearColor(.45f, .45f, .47f)));
    Button->SetContent(Text);
    UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
    ButtonSlot->SetPadding(FMargin(6, 4));
    return Button;
}

ULalalandActionButton* ULalalandRootWidget::AddButton(UHorizontalBox* Parent, const FString& Label, const FString& Payload, bool bEnabled, bool bSelected)
{
    ULalalandActionButton* Button = WidgetTree->ConstructWidget<ULalalandActionButton>();
    Button->InitializeAction(this, Payload);
    Button->SetIsEnabled(bEnabled && !bSelected);
    Button->SetBackgroundColor(bSelected ? FLinearColor(.18f, .18f, .19f, .8f) : FLinearColor(.08f, .09f, .11f, .86f));
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Label));
    Text->SetFont(LalalandFont(16));
    Text->SetColorAndOpacity(FSlateColor(bEnabled ? FLinearColor::White : FLinearColor(.45f, .45f, .47f)));
    Button->SetContent(Text);
    UHorizontalBoxSlot* ButtonSlot = Parent->AddChildToHorizontalBox(Button);
    ButtonSlot->SetPadding(FMargin(5, 5));
    return Button;
}

void ULalalandRootWidget::Refresh()
{
    if (!Service) return;
    StatusText->SetText(FText::FromString(Service->GetStatusText()));
    const bool bInGame = !Service->GetState().sessionId.IsEmpty();
    IntroBackdrop->SetVisibility(bInGame ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    GameBackdrop->SetVisibility(bInGame ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (!bInGame)
    {
        if (ApiBaseInput->GetText().IsEmpty() && !Service->GetBootstrap().modelBase.IsEmpty()) ApiBaseInput->SetText(FText::FromString(Service->GetBootstrap().modelBase));
        if (ModelInput->GetText().IsEmpty() && !Service->GetBootstrap().model.IsEmpty()) ModelInput->SetText(FText::FromString(Service->GetBootstrap().model));
        BuildIntroPage();
        return;
    }
    const bool bInElevator = Service->GetState().intro.phase == TEXT("elevator");
    if (bInElevator)
    {
        ObjectiveTitle->SetText(FText::FromString(TEXT("电梯正在上行")));
        const FString ElevatorHint = Service->GetState().intro.hint.IsEmpty() ? TEXT("门开后进入今晚的酒吧。") : Service->GetState().intro.hint;
        ObjectiveHint->SetText(FText::FromString(ElevatorHint));
    }
    else
    {
        ObjectiveTitle->SetText(FText::FromString(Service->GetState().interaction.nextTitle.IsEmpty() ? TEXT("今晚的酒吧") : Service->GetState().interaction.nextTitle));
        ObjectiveHint->SetText(FText::FromString(Service->GetState().interaction.nextHint));
    }
    InteractionPanel->SetVisibility(bInElevator ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    const FLalalandFirstNightDto& Night = Service->GetState().firstNight;
    if (StatusHud)
    {
        const FString Voucher = Night.voucherCount > 0 ? FString::Printf(TEXT(" · 特调券 %d"), Night.voucherCount) : FString();
        const FString Feign = Night.feigningDrunk ? TEXT(" · 装醉") : FString();
        const FString Assist = Night.cupAssist ? TEXT(" · 杯口已放大") : FString();
        StatusHud->SetText(FText::FromString(FString::Printf(TEXT("Cash %d · %s%s%s%s"), Night.availableCash, *DrinkStageLabel(Night.playerDrinkStage), *Voucher, *Feign, *Assist)));
    }
    bool bReplyWaiting = false;
    FString ReplyError;
    FString RetryId;
    const bool bHasReplyState = GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId);
    if (!DialogueInput->HasKeyboardFocus() && PendingCommand.IsEmpty() && !bHasReplyState) RefreshFocusedTarget();
    RebuildTargetRow();
    BuildPrimaryRow();
    BuildSecondaryOptions(TEXT("interact"));
    RefreshSettlement();
    if (Service->GetState().events.Num())
    {
        const FLalalandEventDto& Last = Service->GetState().events.Last();
        if (Last.actor == TEXT("USER")) PlayerDialogue->SetText(FText::FromString(Last.text));
    }
    if (bReplyWaiting)
    {
        PlayerDialogue->SetText(FText::FromString(TEXT("等待 ") + GetSelectedTargetName() + TEXT(" 回应…")));
    }
    else if (!ReplyError.IsEmpty())
    {
        PlayerDialogue->SetText(FText::FromString(ReplyError + TEXT("。你刚才的话已保留，可以重试。")));
    }
    UpdateInteractionVisibility();
}

FString ULalalandRootWidget::DrinkStageLabel(const FString& Stage) const
{
    if (Stage == TEXT("light")) return TEXT("微醺");
    if (Stage == TEXT("impaired")) return TEXT("明显醉意");
    return TEXT("清醒");
}

void ULalalandRootWidget::RefreshSettlement()
{
    if (!SettlementBackdrop || !SettlementPanel || !Service) return;
    const FLalalandFirstNightDto& Night = Service->GetState().firstNight;
    const bool bSettled = Night.phase == TEXT("settled") || !Night.ending.IsEmpty();
    SettlementBackdrop->SetVisibility(bSettled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (!bSettled) return;
    SettlementPanel->ClearChildren();
    AddText(SettlementPanel, TEXT("这一晚的结算"), 28, FLinearColor(.95f, .78f, .45f));
    AddText(SettlementPanel, Night.settlementSummary.IsEmpty() ? Night.ending : Night.settlementSummary, 16, FLinearColor(.9f, .9f, .92f));
    if (Night.keyActions.Num())
    {
        AddText(SettlementPanel, TEXT("今晚做过的事"), 18, FLinearColor(.82f, .72f, .48f));
        for (const FString& Action : Night.keyActions) AddText(SettlementPanel, TEXT("· ") + Action, 14, FLinearColor(.8f, .82f, .85f));
    }
    if (Night.evaluations.Num())
    {
        AddText(SettlementPanel, TEXT("她们基于亲见事件的评价"), 18, FLinearColor(.82f, .72f, .48f));
        for (const FLalalandEvaluationDto& Line : Night.evaluations) AddText(SettlementPanel, Line.text, 14, FLinearColor(.86f, .86f, .88f));
    }
    AddText(SettlementPanel, TEXT("下一站"), 18, FLinearColor(.82f, .72f, .48f));
    if (Night.nextStations.Num())
    {
        for (const FLalalandStationDto& Station : Night.nextStations)
        {
            const FString Label = Station.open ? Station.label : Station.label + TEXT("（未开放）");
            AddButton(SettlementPanel, Label, TEXT("option:station_") + Station.id, Station.open);
        }
    }
    else
    {
        AddText(SettlementPanel, TEXT("室内攀岩馆与温泉尚未开放。关系可以留到下次。"), 14, FLinearColor(.8f, .82f, .85f));
    }
}

void ULalalandRootWidget::BuildIntroPage()
{
    IntroChoices->ClearChildren();
    if (!Service->IsReady()) return;
    IntroProgress->SetText(FText::FromString(TEXT("酒吧首夜 · 流星雨")));
    IntroPrompt->SetText(FText::FromString(TEXT("不填写身份问卷。进入酒吧后，用行动决定今晚怎样开始。")));
    if (Service->GetBootstrap().modelConfigured)
    {
        AddButton(IntroChoices, TEXT("使用在线 AI 开始新夜晚"), TEXT("intro:start"));
    }
    else
    {
        AddButton(IntroChoices, TEXT("保存模型配置后使用在线 AI"), TEXT("intro:online_unavailable"), false);
        AddButton(IntroChoices, TEXT("明确使用离线规则开始"), TEXT("intro:offline"));
    }
}

void ULalalandRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!Service || Service->GetState().sessionId.IsEmpty() || Service->GetState().intro.phase == TEXT("elevator")) return;
    FocusRefreshRemaining -= InDeltaTime;
    if (FocusRefreshRemaining > 0.f) return;
    FocusRefreshRemaining = .15f;
    bool bReplyWaiting = false;
    FString ReplyError;
    FString RetryId;
    if ((DialogueInput && DialogueInput->HasKeyboardFocus()) || !PendingCommand.IsEmpty()
        || GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId)) return;
    const FString Previous = SelectedTarget;
    RefreshFocusedTarget();
    if (Previous != SelectedTarget) { BuildPrimaryRow(); BuildSecondaryOptions(TEXT("interact")); }
}

void ULalalandRootWidget::HandleAction(const FString& Payload)
{
    if (Payload.StartsWith(TEXT("intro:"))) { ChooseIntroValue(Payload.Mid(6)); return; }
    if (Payload.StartsWith(TEXT("group:")))
    {
        const FString Requested = Payload.Mid(6);
        OpenGroup = OpenGroup == Requested ? FString() : Requested;
        BuildPrimaryRow();
        BuildSecondaryOptions(OpenGroup);
        return;
    }
    if (Payload.StartsWith(TEXT("target:"))) { SetSelectedTarget(Payload.Mid(7)); return; }
    if (Payload.StartsWith(TEXT("option:"))) { ExecuteOption(Payload.Mid(7)); return; }
    if (Payload == TEXT("system:send")) { SendDialogue(); return; }
    if (Payload == TEXT("system:retry"))
    {
        if (LastRetryReplyId.IsEmpty() || !PendingCommand.IsEmpty()) return;
        FLalalandCommandDto Command;
        Command.type = TEXT("retry_reply");
        Command.requestId = LastRetryReplyId;
        PendingOption = TEXT("retry_reply");
        PendingCommand = Service->SendCommand(Command);
        BuildPrimaryRow();
        return;
    }
    if (Payload == TEXT("system:model")) { SaveModelConfig(); return; }
    if (Payload == TEXT("system:save")) { Service->Save(); return; }
    if (Payload == TEXT("system:pause"))
    {
        FLalalandCommandDto Command; Command.type = TEXT("pause"); Command.paused = !Service->GetState().paused; Service->SendCommand(Command); return;
    }
    if (Payload == TEXT("system:intox"))
    {
        FLalalandCommandDto Command; Command.type = TEXT("set_intox_fx");
        const FString Current = Service->GetState().firstNight.intoxFx;
        Command.intent = Current == TEXT("full") || Current.IsEmpty() ? TEXT("low") : Current == TEXT("low") ? TEXT("off") : TEXT("full");
        Service->SendCommand(Command);
        return;
    }
}

void ULalalandRootWidget::ChooseIntroValue(const FString& Value)
{
    if (Value == TEXT("start") && Service->GetBootstrap().modelConfigured)
        Service->OpenNewSession(TEXT("passerby"), TEXT("observe_only"), TEXT("natural"), true);
    else if (Value == TEXT("offline"))
        Service->OpenNewSession(TEXT("passerby"), TEXT("observe_only"), TEXT("natural"), false);
}

void ULalalandRootWidget::BuildSecondaryOptions(const FString& GroupId)
{
    SecondaryOptions->ClearChildren();
    const FLalalandInteractionDto& Interaction = Service->GetState().interaction;
    int32 Added = 0;
    for (const FLalalandInteractionOptionDto& Option : Interaction.suggestions)
    {
        if (Added++ >= 3) break;
        const bool bPending = PendingOption == Option.id;
        FString VisibleLabel = Option.label;
        if (Option.id.StartsWith(TEXT("drink_")) && Option.id != TEXT("drink_next"))
        {
            FString Name;
            for (const FLalalandActorDto& Actor : Service->GetState().characters) if (Actor.id == SelectedTarget) { Name = Actor.name; break; }
            VisibleLabel = Name.IsEmpty() ? TEXT("自己点 · ") + VisibleLabel : TEXT("请 ") + Name + TEXT(" · ") + VisibleLabel;
        }
        ULalalandActionButton* Button = AddButton(SecondaryOptions, VisibleLabel, TEXT("option:") + Option.id, Option.enabled && !bPending, Option.selected || bPending);
        if (!Option.enabled && !Option.disabledReason.IsEmpty()) Button->SetToolTipText(FText::FromString(Option.disabledReason));
    }
    UpdateInteractionVisibility();
}

void ULalalandRootWidget::BuildPrimaryRow()
{
    if (!PrimaryRow || !Service) return;
    PrimaryRow->ClearChildren();
    bool bReplyWaiting = false;
    FString ReplyError;
    FString RetryId;
    if (GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId))
    {
        LastRetryReplyId = RetryId;
        if (!ReplyError.IsEmpty())
        {
            AddButton(PrimaryRow, TEXT("重试 AI 回复"), TEXT("system:retry"), PendingCommand.IsEmpty(), PendingOption == TEXT("retry_reply"));
        }
        else
        {
            AddButton(PrimaryRow, TEXT("等待回应…"), TEXT("system:waiting"), false, true);
        }
        return;
    }
    LastRetryReplyId.Empty();
    const FLalalandInteractionDto& Interaction = Service->GetState().interaction;
    if (!Interaction.primaryActionId.IsEmpty() && Interaction.suggestions.Num() == 0)
    {
        const bool bHasTarget = !Interaction.primaryTargetRequired || !SelectedTarget.IsEmpty();
        AddButton(PrimaryRow, Interaction.primaryLabel, TEXT("option:") + Interaction.primaryActionId, bHasTarget && PendingCommand.IsEmpty(), PendingOption == Interaction.primaryActionId);
    }
}

void ULalalandRootWidget::UpdateInteractionVisibility()
{
    if (!TargetRow || !TargetPrompt || !DialogueRow) return;
    const bool bHasTarget = !SelectedTarget.IsEmpty();
    FString TargetName = SelectedTarget;
    FString Attitude;
    if (Service) for (const FLalalandActorDto& Actor : Service->GetState().characters)
    {
        if (Actor.id == SelectedTarget) { TargetName = Actor.name.IsEmpty() ? Actor.id : Actor.name; break; }
    }
    if (Service) for (const FLalalandAttitudeDto& Item : Service->GetState().firstNight.attitudes)
    {
        if (Item.id != SelectedTarget) continue;
        Attitude = Item.stage + TEXT(" · ") + Item.reason;
        TargetName = Item.name.IsEmpty() ? TargetName : Item.name;
        break;
    }
    TargetPrompt->SetVisibility(ESlateVisibility::Visible);
    TargetPrompt->SetText(FText::FromString(bHasTarget ? TEXT("交谈对象 · ") + TargetName : TEXT("靠近并看向一个人，即可自由交谈")));
    if (AttitudeHud) AttitudeHud->SetText(FText::FromString(bHasTarget && !Attitude.IsEmpty() ? TargetName + TEXT(" · ") + Attitude : TEXT("靠近并看向一个人，即可开始交谈")));
    TargetRow->SetVisibility(ESlateVisibility::Collapsed);
    DialogueRow->SetVisibility(ESlateVisibility::Visible);
    bool bReplyWaiting = false;
    FString ReplyError;
    FString RetryId;
    GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId);
    DialogueInput->SetIsEnabled(bHasTarget && PendingCommand.IsEmpty() && !bReplyWaiting && ReplyError.IsEmpty());
    const FString Hint = !bHasTarget ? TEXT("靠近并看向一个人…")
        : bReplyWaiting ? TEXT("正在等待对方回应…")
        : !ReplyError.IsEmpty() ? TEXT("请先重试这次回复…")
        : TEXT("自由输入你想说的话，按 Enter 发送…");
    DialogueInput->SetHintText(FText::FromString(Hint));
}

void ULalalandRootWidget::ExecuteOption(const FString& OptionId)
{
    if (!PendingOption.IsEmpty()) return;
    if (OptionId == TEXT("talk")) { DialogueInput->SetKeyboardFocus(); return; }
    FLalalandCommandDto Command;
    if (OptionId == TEXT("observe_room") || OptionId == TEXT("observe_target")) { Command.type = TEXT("observe"); Command.target = SelectedTarget; }
    else if (OptionId == TEXT("ball_return") || OptionId == TEXT("ball_try") || OptionId == TEXT("ball_ignore"))
    {
        Command.type = TEXT("opening_ball");
        Command.intent = OptionId == TEXT("ball_return") ? TEXT("return") : OptionId == TEXT("ball_try") ? TEXT("try") : TEXT("ignore");
    }
    else if (OptionId == TEXT("bounce_join") || OptionId == TEXT("bounce_watch") || OptionId == TEXT("bounce_decline"))
    {
        Command.type = TEXT("bounce_choice");
        Command.intent = OptionId == TEXT("bounce_join") ? TEXT("join") : OptionId == TEXT("bounce_watch") ? TEXT("watch") : TEXT("decline");
    }
    else if (OptionId == TEXT("open_drinks") || OptionId == TEXT("drink_next"))
    {
        Command.type = TEXT("drink_menu"); Command.intent = OptionId == TEXT("drink_next") ? TEXT("next") : TEXT("open");
    }
    else if (OptionId.StartsWith(TEXT("drink_")) && OptionId != TEXT("drink_next"))
    {
        Command.type = TEXT("order_drink"); Command.objectTarget = OptionId.Mid(6); Command.target = SelectedTarget;
    }
    else if (OptionId.StartsWith(TEXT("consume_")))
    {
        Command.type = TEXT("consume_drink"); Command.objectTarget = OptionId.Mid(8);
    }
    else if (OptionId == TEXT("aim_left") || OptionId == TEXT("aim_center") || OptionId == TEXT("aim_right"))
    {
        Command.type = TEXT("set_throw_aim"); Command.x = OptionId == TEXT("aim_left") ? .48 : OptionId == TEXT("aim_center") ? .67 : .84;
    }
    else if (OptionId == TEXT("power_light") || OptionId == TEXT("power_medium") || OptionId == TEXT("power_heavy"))
    {
        Command.type = TEXT("throw_ball"); Command.z = OptionId == TEXT("power_light") ? .42 : OptionId == TEXT("power_medium") ? .58 : .76;
    }
    else if (OptionId.StartsWith(TEXT("reward_")) || OptionId.StartsWith(TEXT("penalty_")))
    {
        Command.type = TEXT("post_game_choice"); Command.intent = OptionId;
    }
    else if (OptionId == TEXT("feign_drunk")) { Command.type = TEXT("feign_drunk"); }
    else if (OptionId == TEXT("sit_view")) { Command.type = TEXT("sit_view"); }
    else if (OptionId.StartsWith(TEXT("song_"))) { Command.type = TEXT("pick_song"); Command.objectTarget = OptionId.Mid(5); }
    else if (OptionId.StartsWith(TEXT("activity_"))) { Command.type = TEXT("pick_activity"); Command.objectTarget = OptionId.Mid(9); }
    else if (OptionId == TEXT("npc_romance") || OptionId == TEXT("npc_friend") || OptionId == TEXT("npc_decline"))
    {
        Command.type = TEXT("npc_invite_reply");
        Command.intent = OptionId == TEXT("npc_romance") ? TEXT("accept_romance") : OptionId == TEXT("npc_friend") ? TEXT("accept_friend") : TEXT("decline");
    }
    else if (OptionId.StartsWith(TEXT("station_"))) { Command.type = TEXT("choose_next"); Command.intent = OptionId.Mid(8); }
    else if (OptionId == TEXT("invite_rooftop")) { Command.type = TEXT("invite_rooftop"); Command.target = SelectedTarget; }
    else if (OptionId == TEXT("go_rooftop")) { Command.type = TEXT("go_rooftop"); }
    else if (OptionId == TEXT("end_first_night")) { Command.type = TEXT("end_first_night"); }
    else if (OptionId == TEXT("observe_third")) { Command.type = TEXT("observe_object"); Command.objectTarget = TEXT("third_drink"); }
    else if (OptionId == TEXT("observe_seat")) { Command.type = TEXT("observe_object"); Command.objectTarget = TEXT("reserved_seat"); }
    else if (OptionId == TEXT("approach")) { Command.type = TEXT("approach_target"); Command.target = SelectedTarget; }
    else if (OptionId == TEXT("move_main")) { Command.type = TEXT("move_to"); Command.location = TEXT("main_table"); }
    else if (OptionId == TEXT("sit_reserved")) { Command.type = TEXT("sit_reserved"); }
    else return;
    PendingOption = OptionId;
    PendingCommand = Service->SendCommand(Command);
    if (PendingCommand.IsEmpty()) PendingOption.Empty();
    BuildSecondaryOptions(OpenGroup);
}

void ULalalandRootWidget::SetSelectedTarget(const FString& Target)
{
    SelectedTarget = Target;
    RebuildTargetRow();
}

void ULalalandRootWidget::RefreshFocusedTarget()
{
    SelectedTarget.Empty();
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->PlayerCameraManager) return;
    const FVector Start = PC->PlayerCameraManager->GetCameraLocation();
    const FVector End = Start + PC->PlayerCameraManager->GetCameraRotation().Vector() * 450.f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LalalandFocus), true);
    if (APawn* Pawn = PC->GetPawn()) Params.AddIgnoredActor(Pawn);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (const ALalalandNpcCharacter* Npc = Cast<ALalalandNpcCharacter>(Hit.GetActor())) SelectedTarget = Npc->GetActorId();
    }
}

void ULalalandRootWidget::RebuildTargetRow()
{
    TargetRow->ClearChildren();
}

void ULalalandRootWidget::SendDialogue()
{
    const FString Text = DialogueInput->GetText().ToString().TrimStartAndEnd();
    bool bReplyWaiting = false;
    FString ReplyError;
    FString RetryId;
    if (Text.IsEmpty() || SelectedTarget.IsEmpty() || !PendingCommand.IsEmpty()
        || GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId)) return;
    FLalalandCommandDto Command;
    Command.type = TEXT("talk");
    Command.target = SelectedTarget;
    Command.text = Text.Left(200);
    PendingOption = TEXT("talk");
    PendingCommand = Service->SendCommand(Command);
    if (!PendingCommand.IsEmpty())
    {
        PlayerDialogue->SetText(FText::FromString(Command.text));
        DialogueInput->SetText(FText::GetEmpty());
    }
}

void ULalalandRootWidget::HandleDialogueCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter) SendDialogue();
}

bool ULalalandRootWidget::GetSelectedReplyState(bool& bWaiting, FString& Error, FString& RequestId) const
{
    bWaiting = false;
    Error.Empty();
    RequestId.Empty();
    if (!Service || SelectedTarget.IsEmpty()) return false;
    const TArray<FLalalandReplyDto>& Replies = Service->GetState().replies;
    for (int32 Index = Replies.Num() - 1; Index >= 0; --Index)
    {
        const FLalalandReplyDto& Reply = Replies[Index];
        if (Reply.actor != SelectedTarget) continue;
        if (Reply.status == TEXT("queued") || Reply.status == TEXT("running") || Reply.status == TEXT("ready"))
        {
            bWaiting = true;
            RequestId = Reply.id;
            return true;
        }
        if (Reply.status == TEXT("error"))
        {
            Error = Reply.error.IsEmpty() ? TEXT("AI 回复失败") : Reply.error;
            RequestId = Reply.id;
            return true;
        }
        return false;
    }
    return false;
}

FString ULalalandRootWidget::GetSelectedTargetName() const
{
    if (!Service) return SelectedTarget;
    for (const FLalalandActorDto& Actor : Service->GetState().characters)
    {
        if (Actor.id == SelectedTarget) return Actor.name.IsEmpty() ? Actor.id : Actor.name;
    }
    return SelectedTarget;
}

void ULalalandRootWidget::SaveModelConfig()
{
    Service->ConfigureModel(ApiBaseInput->GetText().ToString(), ModelInput->GetText().ToString(), ApiKeyInput->GetText().ToString(), ApiKeyInput->GetText().IsEmpty());
    ApiKeyInput->SetText(FText::GetEmpty());
}

void ULalalandRootWidget::HandleError(const FString& Message)
{
    if (StatusText) StatusText->SetText(FText::FromString(Message));
}

void ULalalandRootWidget::HandleAck(const FString& CommandId, const FString& Reason)
{
    if (CommandId != PendingCommand) return;
    PendingCommand.Empty();
    if (PendingOption == TEXT("talk"))
    {
        PendingOption.Empty();
        BuildSecondaryOptions(OpenGroup);
        return;
    }
    FTimerHandle ClearPendingTimer;
    TWeakObjectPtr<ULalalandRootWidget> WeakThis(this);
    GetWorld()->GetTimerManager().SetTimer(ClearPendingTimer, [WeakThis]()
    {
        if (!WeakThis.IsValid()) return;
        WeakThis->PendingOption.Empty();
        WeakThis->BuildSecondaryOptions(WeakThis->OpenGroup);
    }, .35f, false);
}

void ULalalandRootWidget::HandleReject(const FString& CommandId, const FString& Reason)
{
    if (CommandId != PendingCommand) return;
    PendingCommand.Empty();
    PendingOption.Empty();
    PlayerDialogue->SetText(FText::FromString(Reason));
    BuildSecondaryOptions(OpenGroup);
}
