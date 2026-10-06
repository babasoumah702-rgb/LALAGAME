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
#include "MixologyGameMode.h"
#include "InputCoreTypes.h"
#include "LalalandNpcCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "EngineUtils.h"
#include "LalalandDialogueLayout.h"
#include "LalalandBounceGame.h"
#include "Components/ProgressBar.h"

namespace
{
    FSlateFontInfo LalalandFont(int32 Size)
    {
        const FString FontPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Fonts/NotoSansSC.ttf"));
        return FPaths::FileExists(FontPath) ? FSlateFontInfo(FontPath, Size) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
    }
    void StyleInput(UEditableTextBox* Input)
    {
        FEditableTextBoxStyle Style = Input->GetWidgetStyle();
        Style.SetFont(LalalandFont(17));
        Style.SetBackgroundColor(FSlateColor(FLinearColor(.045f,.055f,.075f,1.f)));
        Style.SetForegroundColor(FSlateColor(FLinearColor(.95f,.95f,.97f)));
        Style.SetFocusedForegroundColor(FSlateColor(FLinearColor::White));
        Style.SetReadOnlyForegroundColor(FSlateColor(FLinearColor(.72f,.74f,.78f)));
        Style.TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor(.95f,.95f,.97f)));
        Input->SetWidgetStyle(Style);
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
    USizeBox* IntroSize = WidgetTree->ConstructWidget<USizeBox>();
    IntroSize->SetWidthOverride(520.f);
    IntroSize->SetContent(IntroBackdrop);
    IntroBackdrop->SetPadding(FMargin(20.f));
    UOverlaySlot* IntroSlot = Root->AddChildToOverlay(IntroSize);
    IntroSlot->SetHorizontalAlignment(HAlign_Center);
    IntroSlot->SetVerticalAlignment(VAlign_Center);
    IntroPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntroPanel"));
    IntroBackdrop->SetContent(IntroPanel);
    AddText(IntroPanel, TEXT("LALALAND"), 46, FLinearColor(.92f, .78f, .58f));
    IntroProgress = AddText(IntroPanel, TEXT("酒吧首夜 · 新版"), 15, FLinearColor(.55f, .58f, .62f));
    IntroPrompt = AddText(IntroPanel, TEXT("今晚，先从一件小事开始。"), 28, FLinearColor::White);
    IntroChoices = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntroChoices"));
    IntroPanel->AddChildToVerticalBox(IntroChoices);

    ConfigBorder = WidgetTree->ConstructWidget<UBorder>();
    ConfigBorder->SetBrushColor(FLinearColor(.045f, .05f, .06f, .94f));
    IntroPanel->AddChildToVerticalBox(ConfigBorder);
    UVerticalBox* Config = WidgetTree->ConstructWidget<UVerticalBox>();
    ConfigBorder->SetContent(Config);
    AddText(Config, TEXT("高级设置 · 自带模型 API"), 18, FLinearColor(.8f, .82f, .85f));
    ApiBaseInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    StyleInput(ApiBaseInput);
    ApiBaseInput->SetHintText(FText::FromString(TEXT("https://api.openai.com/v1")));
    Config->AddChildToVerticalBox(ApiBaseInput);
    ModelInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    StyleInput(ModelInput);
    ModelInput->SetHintText(FText::FromString(TEXT("模型 ID")));
    Config->AddChildToVerticalBox(ModelInput);
    ApiKeyInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    StyleInput(ApiKeyInput);
    ApiKeyInput->SetHintText(FText::FromString(TEXT("API Key（不会写入游戏日志）")));
    ApiKeyInput->SetIsPassword(true);
    Config->AddChildToVerticalBox(ApiKeyInput);
    AddText(Config,TEXT("填写自己的兼容 Chat Completions 接口；Key 仅保存在本机。先保存，再测试。测试会消耗少量 API 额度。"),13,FLinearColor(.7f,.72f,.75f));
    AddButton(Config, TEXT("保存模型配置"), TEXT("system:model"));
    AddButton(Config,TEXT("测试已保存的连接"),TEXT("system:model_test"));
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
    PauseButton = AddButton(SystemRow, TEXT("暂停"), TEXT("system:pause"));
    AddButton(SystemRow, TEXT("今晚见闻"), TEXT("system:journal"));
    AddButton(SystemRow, TEXT("设置"), TEXT("system:settings"));

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

    NarrativeText = AddText(InteractionPanel, FString(), 18, FLinearColor(.98f, .9f, .74f));
    NarrativeText->RemoveFromParent();
    USizeBox* NarrativeSpace=WidgetTree->ConstructWidget<USizeBox>();
    NarrativeSpace->SetMinDesiredHeight(72.f);
    NarrativeSpace->SetContent(NarrativeText);
    UVerticalBoxSlot* NarrativeSlot=InteractionPanel->AddChildToVerticalBox(NarrativeSpace);
    NarrativeSlot->SetPadding(FMargin(18.f,7.f));
    NarrativeText->SetVisibility(ESlateVisibility::Hidden);

    PrimaryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PrimaryRow"));
    InteractionPanel->AddChildToVerticalBox(PrimaryRow);
    BuildPrimaryRow();

    TargetPrompt = AddText(InteractionPanel, TEXT("靠近并看向一个人，即可开始交谈"), 14, FLinearColor(.72f, .70f, .66f));
    ThrowCharge=WidgetTree->ConstructWidget<UProgressBar>();
    InteractionPanel->AddChildToVerticalBox(ThrowCharge);
    ThrowCharge->SetFillColorAndOpacity(FLinearColor(.85f,.58f,.22f));
    ThrowCharge->SetVisibility(ESlateVisibility::Collapsed);
    TargetRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(TargetRow);
    SecondaryOptions = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(SecondaryOptions);

    DialogueRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(DialogueRow);
    DialogueInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    StyleInput(DialogueInput);
    DialogueInput->SetHintText(FText::FromString(TEXT("自由输入你想说的话…")));
    DialogueInput->OnTextCommitted.AddDynamic(this, &ULalalandRootWidget::HandleDialogueCommitted);
    DialogueInput->OnTextChanged.AddDynamic(this, &ULalalandRootWidget::HandleDialogueChanged);
    UHorizontalBoxSlot* InputSlot = DialogueRow->AddChildToHorizontalBox(DialogueInput);
    InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ULalalandActionButton* Send = AddButton(DialogueRow, TEXT("发送"), TEXT("system:send"));
    Send->SetToolTipText(FText::FromString(TEXT("只让当前选择的人回应")));
    DiscardDraftButton = AddButton(DialogueRow, TEXT("取消草稿"), TEXT("system:discard_draft"));
    DiscardDraftButton->SetVisibility(ESlateVisibility::Collapsed);
    PlayerDialogue = AddText(InteractionPanel, FString(), 15, FLinearColor(.82f, .82f, .84f));
    // Explicit wrapping participates in the desired-size prepass. Automatic
    // wrapping alone can report one-line height on the first visible frame.
    PlayerDialogue->SetAutoWrapText(false);
    PlayerDialogue->SetWrapTextAt(700.f);
    NarrativeText->SetAutoWrapText(false);
    NarrativeText->SetWrapTextAt(700.f);
    JournalBackdrop = WidgetTree->ConstructWidget<UBorder>();
    JournalBackdrop->SetBrushColor(FLinearColor(.015f, .018f, .024f, .98f));
    JournalBackdrop->SetPadding(FMargin(20.f));
    USizeBox* JournalSize = WidgetTree->ConstructWidget<USizeBox>();
    JournalSize->SetWidthOverride(680.f);
    JournalSize->SetHeightOverride(450.f);
    JournalBackdrop->SetContent(JournalSize);
    UVerticalBox* JournalLayout = WidgetTree->ConstructWidget<UVerticalBox>();
    JournalSize->SetContent(JournalLayout);
    AddText(JournalLayout, TEXT("今晚见闻 · 你实际听见和看见的事"), 20, FLinearColor(.95f, .78f, .45f));
    AddButton(JournalLayout, TEXT("回到酒吧"), TEXT("system:journal"));
    UScrollBox* JournalScroll = WidgetTree->ConstructWidget<UScrollBox>();
    JournalLayout->AddChildToVerticalBox(JournalScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    JournalEntries = WidgetTree->ConstructWidget<UVerticalBox>();
    JournalScroll->AddChild(JournalEntries);
    UOverlaySlot* JournalSlot = Root->AddChildToOverlay(JournalBackdrop);
    JournalSlot->SetHorizontalAlignment(HAlign_Center);
    JournalSlot->SetVerticalAlignment(VAlign_Center);
    JournalBackdrop->SetVisibility(ESlateVisibility::Collapsed);
    CraftBackdrop=WidgetTree->ConstructWidget<UBorder>();
    CraftBackdrop->SetBrushColor(FLinearColor(.025f,.035f,.045f,.99f));
    CraftBackdrop->SetPadding(FMargin(24));
    USizeBox* CraftSize=WidgetTree->ConstructWidget<USizeBox>();CraftSize->SetWidthOverride(580.f);
    CraftPanel=WidgetTree->ConstructWidget<UVerticalBox>();CraftSize->SetContent(CraftPanel);CraftBackdrop->SetContent(CraftSize);
    UOverlaySlot* CraftSlot=Root->AddChildToOverlay(CraftBackdrop);CraftSlot->SetHorizontalAlignment(HAlign_Center);CraftSlot->SetVerticalAlignment(VAlign_Center);
    CraftBackdrop->SetVisibility(ESlateVisibility::Collapsed);
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
    BuildCraftPanel();
    StatusText->SetText(FText::FromString(Service->GetStatusText()));
    if (PauseButton) if (UTextBlock* Label = Cast<UTextBlock>(PauseButton->GetContent()))
        Label->SetText(FText::FromString(Service->GetState().paused ? TEXT("继续") : TEXT("暂停")));
    const bool bInGame = !Service->GetState().sessionId.IsEmpty() && !bSettingsOpen;
    ConfigBorder->SetVisibility((bSettingsOpen || bAdvancedOpen) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    IntroBackdrop->SetVisibility(bInGame ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    GameBackdrop->SetVisibility(bInGame ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (!bInGame)
    {
        InteractionPanel->SetVisibility(ESlateVisibility::Collapsed);
        if (ApiBaseInput->GetText().IsEmpty() && !Service->GetBootstrap().modelBase.IsEmpty()) ApiBaseInput->SetText(FText::FromString(Service->GetBootstrap().modelBase));
        if (ModelInput->GetText().IsEmpty() && !Service->GetBootstrap().model.IsEmpty()) ModelInput->SetText(FText::FromString(Service->GetBootstrap().model));
        if (bSettingsOpen)
        {
            IntroChoices->ClearChildren();
            IntroPrompt->SetText(FText::FromString(TEXT("模型与对话设置")));
            AddButton(IntroChoices, TEXT("返回当前夜晚"), TEXT("system:settings"));
            AddButton(IntroChoices, TEXT("保存进度"), TEXT("system:save"));
            AddButton(IntroChoices, TEXT("微醺视效"), TEXT("system:intox"));
            AddButton(IntroChoices, TEXT("使用在线 AI"), TEXT("system:online"), Service->GetBootstrap().modelConfigured);
            AddButton(IntroChoices, TEXT("明确改用离线规则"), TEXT("system:offline"));
        }
        else BuildIntroPage();
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
        ObjectiveHint->SetText(FText::FromString(Service->GetState().paused ? TEXT("已暂停：点击继续恢复人物、剧情与时间。") : Service->GetState().interaction.nextHint));
    }
    InteractionPanel->SetVisibility(ESlateVisibility::Visible);
    InteractionPanel->SetIsEnabled(Service->IsReady());
    if (!Service->IsReady()) ObjectiveHint->SetText(FText::FromString(Service->GetStatusText()));
    const FLalalandFirstNightDto& Night = Service->GetState().firstNight;
    if (StatusHud)
    {
        const FString Voucher = Night.voucherCount > 0 ? FString::Printf(TEXT(" · 特调券 %d"), Night.voucherCount) : FString();
        const FString Feign = Night.feigningDrunk ? TEXT(" · 装醉") : FString();
        StatusHud->SetText(FText::FromString(FString::Printf(TEXT("Cash %d · %s%s%s"), Night.availableCash, *DrinkStageLabel(Night.playerDrinkStage), *Voucher, *Feign)));
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
    RefreshNarrative();
    if (Service->GetState().events.Num())
    {
        const FLalalandEventDto& Last = Service->GetState().events.Last();
        if (Last.actor == TEXT("USER") && Last.type == TEXT("speech")) PlayerDialogue->SetText(FText::FromString(Last.text));
        else if (!bHasReplyState) PlayerDialogue->SetText(FText::GetEmpty());
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

void ULalalandRootWidget::RefreshNarrative()
{
    const FLalalandStateDto& State = Service->GetState();
    if (NarrativeSession != State.sessionId)
    {
        NarrativeSession = State.sessionId;
        NarrativeSeen.Empty();
        NarrativeQueue.Empty();
        NarrativeRemaining = 0;
        for (const FLalalandEventDto& Event : State.events) NarrativeSeen.Add(Event.id);
        // Resume the objective, without replaying old dialogue or environment events.
        if (State.firstNight.phase == TEXT("arrival")) NarrativeQueue.Add(TEXT("今晚有一场流星雨。先认识这里的人；愿意的话，稍后可以邀请一个人上露台。"));
        return;
    }
    for (const FLalalandEventDto& Event : State.events)
    {
        if (NarrativeSeen.Contains(Event.id)) continue;
        NarrativeSeen.Add(Event.id);
        if (Event.text.IsEmpty()) continue;
        // Spoken lines use the head bubbles (NPC) and player subtitle only. Do not
        // duplicate them in the environment/objective queue.
        if (Event.type == TEXT("speech")) continue;
        const bool bSpeech = Event.type == TEXT("speech") || Event.type == TEXT("message");
        const bool bImportant = Event.type == TEXT("system") || Event.target == TEXT("USER") || Event.actor == TEXT("USER");
        if (!bSpeech && !bImportant) continue;
        const FString Line = bSpeech ? Event.name + TEXT("：") + Event.text : Event.text;
        // Decisions must remain legible even when several ambient NPC events arrive together.
        if (Event.type == TEXT("system")) { NarrativeQueue.Insert(Line, 0); NarrativeRemaining = FMath::Min(NarrativeRemaining, 2.f); }
        else if (NarrativeQueue.Num() < 8) NarrativeQueue.Add(Line);
    }
    if (bJournalOpen) RefreshJournal();
}

void ULalalandRootWidget::RefreshJournal()
{
    JournalBackdrop->SetVisibility(bJournalOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (!bJournalOpen) return;
    JournalEntries->ClearChildren();
    if(!Service->GetState().firstNight.relationshipVersion.IsEmpty()){
        AddText(JournalEntries,TEXT("人物笔记 · 今晚的相处"),19,FLinearColor(.95f,.78f,.45f));
        for(const auto& Person:Service->GetState().firstNight.attitudes){
            const auto* Actor=Service->GetState().characters.FindByPredicate([&](const FLalalandActorDto& A){return A.id==Person.id;});
            if(!Actor)continue;
            AddText(JournalEntries,FString::Printf(TEXT("%s · 好感 %d/100 · %s\n%s"),*Actor->name,Person.affinity,*Person.stage,*Person.reason),15,FLinearColor(.9f,.9f,.92f));
        }
        AddText(JournalEntries,TEXT("实际见闻"),19,FLinearColor(.95f,.78f,.45f));
    }

    for (const FLalalandEventDto& Event : Service->GetState().events)
    {
        if (Event.text.IsEmpty()) continue;
        const FString Line = Event.time + TEXT(" · ") + Event.name + TEXT("\n") + Event.text;
        AddText(JournalEntries, Line, 15, FLinearColor(.9f, .9f, .92f));
    }
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
    IntroPrompt->SetText(FText::FromString(TEXT("城市入夜，故事才刚开始。")));
    if (Service->GetBootstrap().modelConfigured)
    {
        AddButton(IntroChoices, TEXT("开始新夜晚"), TEXT("intro:start"));
    }
    else
    {
        AddButton(IntroChoices, TEXT("在线游玩 · 请先填写自己的 API"), TEXT("intro:online_unavailable"), false);
        AddButton(IntroChoices, TEXT("离线体验（规则对话）"), TEXT("intro:offline"));
    }
    AddButton(IntroChoices, bAdvancedOpen ? TEXT("收起高级设置") : TEXT("高级设置"), TEXT("system:advanced"));
}

void ULalalandRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if(Service && Service->GetState().firstNight.craft.open && CraftFill)
    {
        CraftVisualFill=FMath::FInterpTo(CraftVisualFill,Service->GetState().firstNight.craft.ingredients/static_cast<float>(FMath::Max(1,Service->GetState().firstNight.craft.steps)),InDeltaTime,3.f);
        CraftFill->SetPercent(CraftVisualFill);
    }
    LayoutDialogueBubbles(MyGeometry);
    for (TActorIterator<ALalalandBounceGame> It(GetWorld());It;++It)
    {
        ThrowCharge->SetVisibility(It->IsPlayerThrowReady()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        ThrowCharge->SetPercent(It->GetCharge());
        const FString Prompt=It->GetWorldPrompt();
        if (!Prompt.IsEmpty() && AllowsWorldInteraction()) TargetPrompt->SetText(FText::FromString(Prompt));
    }
    if (!Service || Service->GetState().sessionId.IsEmpty() || Service->GetState().intro.phase == TEXT("elevator")) return;
    if (!Service->GetState().paused && !bJournalOpen)
    {
        NarrativeRemaining -= InDeltaTime;
        if (NarrativeRemaining <= 0 && NarrativeQueue.Num())
        {
            const FString Line = NarrativeQueue[0];
            NarrativeQueue.RemoveAt(0);
            // Keep a single readable notice; full text stays in the journal.
            const int32 VisibleLimit=76;
            if(Line.Len()>VisibleLimit)NarrativeQueue.Insert(Line.Mid(VisibleLimit),0);
            NarrativeText->SetText(FText::FromString(Line.Left(VisibleLimit)));
            NarrativeText->SetVisibility(ESlateVisibility::HitTestInvisible);
            NarrativeRemaining = FMath::Clamp(3.f + Line.Len() * .11f, 5.f, 16.f);
        }
        else if (NarrativeRemaining <= 0) NarrativeText->SetVisibility(ESlateVisibility::Hidden);
    }
    FocusRefreshRemaining -= InDeltaTime;
    if (FocusRefreshRemaining > 0.f) return;
    FocusRefreshRemaining = .15f;
    bool bReplyWaiting = false;
    FString ReplyError;
    FString RetryId;
    GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId);
    if (!DraftTarget.IsEmpty() || bReplyWaiting || (DialogueInput && DialogueInput->HasKeyboardFocus()) || !PendingCommand.IsEmpty()) return;
    const FString Previous = SelectedTarget;
    RefreshFocusedTarget();
    if (Previous != SelectedTarget) { BuildPrimaryRow(); BuildSecondaryOptions(TEXT("interact")); }
}

void ULalalandRootWidget::HandleAction(const FString& Payload)
{
    if(Payload==TEXT("bar:open")){if(auto PC=GetOwningPlayer())if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))H->InteractAtBar();return;}
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
    if (Payload == TEXT("system:discard_draft"))
    {
        if (!PendingCommand.IsEmpty()) return;
        DialogueInput->SetText(FText::GetEmpty());
        DraftTarget.Empty();
        RefreshFocusedTarget();
        UpdateInteractionVisibility();
        return;
    }
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
    if(Payload.StartsWith(TEXT("craft:")))
    {
        if(!PendingCommand.IsEmpty())return;
        FLalalandCommandDto Command;Command.type=TEXT("craft_drink");Command.intent=Payload.Mid(6);
        if(Command.intent.StartsWith(TEXT("recipe:"))) { Command.objectTarget=Command.intent.Mid(7);Command.intent=TEXT("recipe");CraftVisualFill=0; }
        PendingCommand=Service->SendCommand(Command);return;
    }
    if (Payload == TEXT("system:drink_cancel")) { ConfirmDrinkId.Empty(); Refresh(); return; }
    if (Payload == TEXT("system:drink_confirm"))
    {
        if (ConfirmDrinkId.IsEmpty() || !PendingCommand.IsEmpty()) return;
        FLalalandCommandDto Command; Command.type=TEXT("order_drink"); Command.objectTarget=ConfirmDrinkId; Command.target=ConfirmDrinkTarget;
        PendingOption=TEXT("drink_")+ConfirmDrinkId;
        PendingCommand=Service->SendCommand(Command);
        if (!PendingCommand.IsEmpty()) ConfirmDrinkId.Empty();
        Refresh(); return;
    }
    if (Payload==TEXT("system:skip_intro"))
    {
        FLalalandCommandDto Command;Command.type=TEXT("intro_complete");Command.intent=TEXT("skip");Service->SendCommand(Command);return;
    }
    if (Payload == TEXT("system:advanced")) { bAdvancedOpen = !bAdvancedOpen; Refresh(); return; }
    if (Payload == TEXT("system:model_test")) { Service->TestModelConnection(); return; }
    if (Payload == TEXT("system:model")) { SaveModelConfig(); return; }
    if (Payload == TEXT("system:settings")) { bSettingsOpen = !bSettingsOpen; Refresh(); return; }
    if (Payload == TEXT("system:online") || Payload == TEXT("system:offline"))
    {
        FLalalandCommandDto Command; Command.type = TEXT("mode"); Command.online = Payload == TEXT("system:online");
        Service->SendCommand(Command); bSettingsOpen = false; Refresh(); return;
    }
    if (Payload == TEXT("system:drink_self")) { DrinkRecipient.Empty(); BuildPrimaryRow(); BuildSecondaryOptions(TEXT("interact")); return; }
    if (Payload == TEXT("system:drink_gift")) { DrinkRecipient = SelectedTarget; BuildPrimaryRow(); BuildSecondaryOptions(TEXT("interact")); return; }
    if (Payload == TEXT("system:drink_close")) { FLalalandCommandDto Command; Command.type = TEXT("drink_menu"); Command.intent = TEXT("close"); Service->SendCommand(Command); return; }
    if (Payload == TEXT("system:journal"))
    {
        bJournalOpen = !bJournalOpen;
        RefreshJournal();
        return;
    }
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
    if (!ConfirmDrinkId.IsEmpty())
    {
        if (!Service->GetState().interaction.nextTitle.StartsWith(TEXT("酒单"))) ConfirmDrinkId.Empty();
        else
        {
            ObjectiveTitle->SetText(FText::FromString(TEXT("确认点单")));
            ObjectiveHint->SetText(FText::FromString(ConfirmDrinkLabel+TEXT("。确认后提交，取消不会扣除 Cash 或特调券。")));
            AddButton(SecondaryOptions,TEXT("确认点单"),TEXT("system:drink_confirm"),PendingCommand.IsEmpty());
            AddButton(SecondaryOptions,TEXT("取消"),TEXT("system:drink_cancel"),PendingCommand.IsEmpty());
            return;
        }
    }
    const FLalalandInteractionDto& Interaction = Service->GetState().interaction;
    int32 Added = 0;
    for (const FLalalandInteractionOptionDto& Option : Interaction.suggestions)
    {
        if (Added++ >= 3) break;
        const bool bPending = PendingOption == Option.id;
        FString VisibleLabel = Option.label;
        if (Option.id.StartsWith(TEXT("drink_")) && Option.id!=TEXT("drink_next"))
        {
            TArray<FString> Parts; Option.label.ParseIntoArray(Parts,TEXT(" · "));
            if (Parts.Num()>=3) VisibleLabel=Parts[0]+TEXT(" · ")+Parts[1]+TEXT("\n")+Parts[2];
        }
        if (Option.id.StartsWith(TEXT("drink_")) && Option.id != TEXT("drink_next"))
        {
            FString Name;
            for (const FLalalandActorDto& Actor : Service->GetState().characters) if (Actor.id == SelectedTarget) { Name = Actor.name; break; }
            for (const FLalalandActorDto& Actor : Service->GetState().characters) if (Actor.id == DrinkRecipient) { Name = Actor.name; break; }
            if (DrinkRecipient.IsEmpty()) Name.Empty();
            VisibleLabel = Name.IsEmpty() ? TEXT("自己点 · ") + VisibleLabel : TEXT("请 ") + Name + TEXT(" · ") + VisibleLabel;
        }
        const bool bHasTarget = !Option.targetRequired || !SelectedTarget.IsEmpty();
        ULalalandActionButton* Button = AddButton(SecondaryOptions, VisibleLabel, TEXT("option:") + Option.id, Option.enabled && PendingCommand.IsEmpty() && !bPending && bHasTarget, Option.selected || bPending);
        if (UHorizontalBoxSlot* OptionSlot=UWidgetLayoutLibrary::SlotAsHorizontalBoxSlot(Button)) OptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        if (UTextBlock* Label=Cast<UTextBlock>(Button->GetContent())) { Label->SetAutoWrapText(true); Label->SetJustification(ETextJustify::Center); }
        Button->SetToolTipText(FText::FromString(Option.label));
        if (!bHasTarget) Button->SetToolTipText(FText::FromString(TEXT("先走近并看向一个人")));
        if (!Option.enabled && !Option.disabledReason.IsEmpty()) Button->SetToolTipText(FText::FromString(Option.disabledReason));
    }
    UpdateInteractionVisibility();
}

void ULalalandRootWidget::BuildPrimaryRow()
{
    if (!PrimaryRow || !Service) return;
    PrimaryRow->ClearChildren();
    if(Service->GetState().intro.phase==TEXT("elevator"))
    { AddButton(PrimaryRow,TEXT("跳过电梯演出"),TEXT("system:skip_intro"));return; }
    if (!ConfirmDrinkId.IsEmpty()) return;
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
    }
    else LastRetryReplyId.Empty();
    const FLalalandInteractionDto& Interaction = Service->GetState().interaction;
    if (!Interaction.primaryActionId.IsEmpty() && Interaction.suggestions.Num() == 0)
    {
        const bool bHasTarget = !Interaction.primaryTargetRequired || !SelectedTarget.IsEmpty();
        AddButton(PrimaryRow, Interaction.primaryLabel, TEXT("option:") + Interaction.primaryActionId, bHasTarget && PendingCommand.IsEmpty(), PendingOption == Interaction.primaryActionId);
    }
    if(auto PC=GetOwningPlayer())if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))if(H->CanEnter())
        AddButton(PrimaryRow,TEXT("吧台：点单 / 自己调酒"),TEXT("bar:open"));
    if (Interaction.nextTitle.StartsWith(TEXT("酒单")))
    {
        AddButton(PrimaryRow, TEXT("给自己点"), TEXT("system:drink_self"), !DrinkRecipient.IsEmpty(), DrinkRecipient.IsEmpty());
        AddButton(PrimaryRow, TEXT("请眼前的人"), TEXT("system:drink_gift"), !SelectedTarget.IsEmpty() && DrinkRecipient != SelectedTarget, !DrinkRecipient.IsEmpty());
        AddButton(PrimaryRow,TEXT("自己做一杯"),TEXT("craft:open"));
        AddButton(PrimaryRow, TEXT("收起酒单"), TEXT("system:drink_close"));
    }
}

FReply ULalalandRootWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Key)
{
    // GameAndUI button focus previously prevented the HUD's key poll from seeing E.
    // Never steal E from chat or API text fields.
    if(Key.GetKey()==EKeys::E&&!Key.IsRepeat()
        &&(!DialogueInput||(!DialogueInput->HasKeyboardFocus()&&!DialogueInput->HasUserFocusedDescendants(GetOwningPlayer())))
        &&(!ConfigBorder||!ConfigBorder->IsVisible()))
        if(auto PC=GetOwningPlayer())if(auto H=Cast<AMixologyHUD>(PC->GetHUD()))if(H->CanEnter())
        {H->InteractAtBar();return FReply::Handled();}
    return Super::NativeOnPreviewKeyDown(Geometry,Key);
}
void ULalalandRootWidget::UpdateInteractionVisibility()
{
    if (!TargetRow || !TargetPrompt || !DialogueRow) return;
    if(Service && Service->GetState().intro.phase==TEXT("elevator"))
    {
        PrimaryRow->SetVisibility(ESlateVisibility::Visible);
        SecondaryOptions->SetVisibility(ESlateVisibility::Collapsed);
        DialogueRow->SetVisibility(ESlateVisibility::Collapsed);
        TargetPrompt->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    SecondaryOptions->SetVisibility(ESlateVisibility::Visible);
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
        Attitude = Item.stage + (Service->GetState().firstNight.relationshipVersion.IsEmpty()?FString():FString::Printf(TEXT(" · 好感 %d/100"),Item.affinity)) + TEXT(" · ") + Item.reason;
        TargetName = Item.name.IsEmpty() ? TargetName : Item.name;
        break;
    }
    TargetPrompt->SetVisibility(ESlateVisibility::Visible);
    TargetPrompt->SetText(FText::FromString(bHasTarget ? (DraftTarget.IsEmpty() ? TEXT("交谈对象 · ") : TEXT("草稿对象已锁定 · ")) + TargetName : TEXT("靠近并看向一个人，即可自由交谈")));
    if (AttitudeHud) AttitudeHud->SetText(FText::FromString(bHasTarget && !Attitude.IsEmpty() ? TargetName + TEXT(" · ") + Attitude : TEXT("靠近并看向一个人，即可开始交谈")));
    TargetRow->ClearChildren();
    TargetRow->SetVisibility(ESlateVisibility::Collapsed);
    if (DiscardDraftButton)
    {
        DiscardDraftButton->SetVisibility(DraftTarget.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        DiscardDraftButton->SetIsEnabled(PendingCommand.IsEmpty());
    }
    DialogueRow->SetVisibility(ESlateVisibility::Visible);
    bool bReplyWaiting = false;
    FString ReplyError;
    FString RetryId;
    GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId);
    const bool bAnswerQuestion = Service && Service->GetState().firstNight.postGameStep == TEXT("question");
    DialogueInput->SetIsEnabled((bHasTarget || bAnswerQuestion) && PendingCommand.IsEmpty() && !bReplyWaiting);
    const FString Hint = bAnswerQuestion ? TEXT("直接输入你的回答，按 Enter 发送…") : !bHasTarget ? TEXT("靠近并看向一个人…")
        : bReplyWaiting ? TEXT("正在等待对方回应…")
        : !ReplyError.IsEmpty() ? TEXT("可以重试，或说一句新的话…")
        : TEXT("自由输入你想说的话，按 Enter 发送…");
    DialogueInput->SetHintText(FText::FromString(Hint));
}

void ULalalandRootWidget::ExecuteOption(const FString& OptionId)
{
    if (!PendingOption.IsEmpty()) return;
    if (OptionId == TEXT("talk")) { DialogueInput->SetKeyboardFocus(); return; }
    if (OptionId==TEXT("ball_return"))
    {
        for (TActorIterator<ALalalandBounceGame> It(GetWorld());It;++It) It->RequestOpeningReturn();
        return;
    }
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
        if (OptionId == TEXT("open_drinks")) DrinkRecipient.Empty();
        Command.type = TEXT("drink_menu"); Command.intent = OptionId == TEXT("drink_next") ? TEXT("next") : TEXT("open");
    }
    else if (OptionId.StartsWith(TEXT("drink_")) && OptionId != TEXT("drink_next"))
    {
        for (const FLalalandInteractionOptionDto& Option : Service->GetState().interaction.suggestions)
            if (Option.id==OptionId && Option.enabled)
            {
                ConfirmDrinkId=OptionId.Mid(6); ConfirmDrinkTarget=DrinkRecipient;
                FString Recipient=TEXT("自己");
                for (const FLalalandActorDto& Actor : Service->GetState().characters) if (Actor.id==ConfirmDrinkTarget) Recipient=Actor.name;
                ConfirmDrinkLabel=TEXT("给 ")+Recipient+TEXT(" · ")+Option.label;
                Refresh(); return;
            }
        return;
    }
    else if (OptionId.StartsWith(TEXT("gift_pay_")) || OptionId.StartsWith(TEXT("gift_cancel_")) || OptionId.StartsWith(TEXT("gift_deliver_")))
    {
        Command.type=TEXT("gift_drink");
        if(OptionId.StartsWith(TEXT("gift_pay_"))){Command.intent=TEXT("pay");Command.objectTarget=OptionId.Mid(9);}
        else if(OptionId.StartsWith(TEXT("gift_cancel_"))){Command.intent=TEXT("cancel");Command.objectTarget=OptionId.Mid(12);}
        else{Command.intent=TEXT("deliver");Command.objectTarget=OptionId.Mid(13);}
    }
    else if (OptionId.StartsWith(TEXT("offer_owned_")))
    {
        Command.requestId=OptionId.Mid(12);
        for (const FLalalandDrinkPropDto& Drink : Service->GetState().firstNight.drinks)
            if (Drink.instanceId==Command.requestId && Drink.owner==TEXT("USER") && Drink.status==TEXT("served")) Command.objectTarget=Drink.id;
        if (Command.objectTarget.IsEmpty() || SelectedTarget.IsEmpty()) return;
        Command.type=TEXT("order_drink"); Command.intent=TEXT("offer_owned"); Command.target=SelectedTarget;
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
    else if (OptionId == TEXT("stand_up")) { Command.type=TEXT("cancel_move"); }
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
    if (!DraftTarget.IsEmpty() && Target != DraftTarget) return;
    SelectedTarget = Target;
    RebuildTargetRow();
}

void ULalalandRootWidget::RefreshFocusedTarget()
{
    if (!DraftTarget.IsEmpty()) { SelectedTarget = DraftTarget; return; }
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
        if (const ALalalandNpcCharacter* Npc = Cast<ALalalandNpcCharacter>(Hit.GetActor()))
            for (const FLalalandActorDto& Actor : Service->GetState().characters)
                if (Actor.id == Npc->GetActorId() && Actor.interactable && (Actor.id == TEXT("A") || Actor.id == TEXT("B") || Actor.id == TEXT("C") || Actor.id == TEXT("D"))) SelectedTarget = Actor.id;
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
    const bool bAnswerQuestion = Service->GetState().firstNight.postGameStep == TEXT("question");
    GetSelectedReplyState(bReplyWaiting, ReplyError, RetryId);
    if (Text.IsEmpty() || (SelectedTarget.IsEmpty() && !bAnswerQuestion) || !PendingCommand.IsEmpty() || bReplyWaiting) return;
    FLalalandCommandDto Command;
    Command.type = bAnswerQuestion ? TEXT("answer_question") : TEXT("talk");
    Command.target = DraftTarget.IsEmpty() ? SelectedTarget : DraftTarget;
    Command.text = Text.Left(200);
    SubmittedDialogue = Command.text;
    SubmittedTarget = Command.target;
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

void ULalalandRootWidget::HandleDialogueChanged(const FText& Text)
{
    if (!PendingCommand.IsEmpty()) return;
    const FString Previous = DraftTarget;
    if (Text.ToString().TrimStartAndEnd().IsEmpty()) DraftTarget.Empty();
    else if (DraftTarget.IsEmpty() && !SelectedTarget.IsEmpty()) DraftTarget = SelectedTarget;
    if (Previous != DraftTarget) UpdateInteractionVisibility();
}

bool ULalalandRootWidget::AllowsWorldInteraction() const
{
    return Service && !Service->GetState().firstNight.craft.open && !Service->GetState().sessionId.IsEmpty() && !Service->GetState().paused
        && !bSettingsOpen && !bJournalOpen && PendingCommand.IsEmpty()
        && (!DialogueInput || !DialogueInput->HasKeyboardFocus());
}

void ULalalandRootWidget::LayoutDialogueBubbles(const FGeometry& Geometry)
{
    if (!GetWorld() || !GetOwningPlayer()) return;
    struct FEntry { ALalalandNpcCharacter* Npc; FVector2D Anchor; FVector2D Size; };
    TArray<FEntry> Entries;
    TArray<FBox2D> Occupied;
    const bool bHidden = !Service || Service->GetState().sessionId.IsEmpty() || bSettingsOpen || bJournalOpen || Service->GetState().firstNight.phase == TEXT("settled");
    for (TActorIterator<ALalalandNpcCharacter> It(GetWorld()); It; ++It)
    {
        FVector WorldAnchor; FVector2D Size, Anchor;
        if (bHidden || !It->GetDialogueLayout(WorldAnchor, Size) || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(), WorldAnchor, Anchor, true))
        { It->ApplyDialogueLayout(FVector2D::ZeroVector, false); continue; }
        Entries.Add({*It, Anchor, Size});
        Occupied.Add(FBox2D(Anchor + FVector2D(-32,8), Anchor + FVector2D(32,90)));
    }
    for (UWidget* Panel : {static_cast<UWidget*>(GameBackdrop.Get()), static_cast<UWidget*>(InteractionPanel.Get())})
    {
        if (!Panel || !Panel->IsVisible()) continue;
        const FGeometry& G = Panel->GetCachedGeometry();
        const FVector2D Min = Geometry.AbsoluteToLocal(G.LocalToAbsolute(FVector2D::ZeroVector));
        const FVector2D Max = Geometry.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
        if (Max.X > Min.X && Max.Y > Min.Y) Occupied.Add(FBox2D(Min-FVector2D(12,12), Max+FVector2D(12,12)));
    }
    Entries.Sort([this](const FEntry& A, const FEntry& B)
    {
        if ((A.Npc->GetActorId() == SelectedTarget) != (B.Npc->GetActorId() == SelectedTarget)) return A.Npc->GetActorId() == SelectedTarget;
        return A.Npc->GetActorId() < B.Npc->GetActorId();
    });
    for (const FEntry& Entry : Entries)
    {
        FVector2D TopLeft;
        const bool bFits = LalalandDialogueLayout::Place(Entry.Anchor, Entry.Size, Geometry.GetLocalSize(), Occupied, TopLeft);
        UE_LOG(LogTemp, VeryVerbose, TEXT("LALALAND_BUBBLE_LAYOUT actor=%s anchor=%s size=%s viewport=%s fits=%d"), *Entry.Npc->GetActorId(), *Entry.Anchor.ToString(), *Entry.Size.ToString(), *Geometry.GetLocalSize().ToString(), bFits);
        Entry.Npc->ApplyDialogueLayout(bFits ? TopLeft - (Entry.Anchor - FVector2D(Entry.Size.X*.5, Entry.Size.Y)) : FVector2D::ZeroVector, bFits);
        if (bFits) Occupied.Add(FBox2D(TopLeft, TopLeft+Entry.Size));
    }
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
        if (Reply.actor != SelectedTarget || !Reply.playerInitiated) continue;
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
    if (PlayerDialogue) PlayerDialogue->SetText(FText::FromString(Message));
}

void ULalalandRootWidget::HandleAck(const FString& CommandId, const FString& Reason)
{
    if (CommandId != PendingCommand) return;
    PendingCommand.Empty();
    if (PendingOption == TEXT("talk"))
    {
        DraftTarget.Empty();
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
    if (PendingOption == TEXT("talk") && DialogueInput->GetText().IsEmpty())
    {
        DraftTarget = SubmittedTarget;
        SelectedTarget = SubmittedTarget;
        DialogueInput->SetText(FText::FromString(SubmittedDialogue));
    }
    PendingOption.Empty();
    PlayerDialogue->SetText(FText::FromString(Reason));
    BuildSecondaryOptions(OpenGroup);
}

void ULalalandRootWidget::BuildCraftPanel()
{
    if(!CraftBackdrop||!Service)return;
    // Native station owns the craft interface in this pilot; keep legacy panel for other hosts.
    if(GetOwningPlayer()&&Cast<AMixologyHUD>(GetOwningPlayer()->GetHUD())&&Service->GetState().firstNight.catalogVersion==TEXT("lalaland-drink-catalog-v0.3"))
    {CraftBackdrop->SetVisibility(ESlateVisibility::Collapsed);CraftFill=nullptr;return;}
    const auto& Craft=Service->GetState().firstNight.craft;
    CraftBackdrop->SetVisibility(Craft.open?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(!Craft.open){CraftFill=nullptr;CraftVisualFill=0;return;}
    CraftPanel->ClearChildren();
    AddText(CraftPanel,TEXT("吧台 · 自己做一杯"),25,FLinearColor(.95f,.78f,.45f));
    AddText(CraftPanel,TEXT("制作期间夜晚暂停。取消不扣款，完成后回到原位置。"),14,FLinearColor(.8f,.8f,.82f));
    if(Craft.recipe.IsEmpty())
    {
        for(const auto& Choice:Craft.choices)AddButton(CraftPanel,Choice.label,TEXT("craft:recipe:")+Choice.id);
        AddButton(CraftPanel,TEXT("其他配方"),TEXT("craft:next"));
    }
    else
    {
        const bool Alcohol=Craft.alcoholic;
        const int32 Cost=Craft.cost;
        const FString Name=Craft.title;
        AddText(CraftPanel,FString::Printf(TEXT("%s · %d Cash · %s"),*Name,Cost,Alcohol?TEXT("含酒精"):TEXT("无酒精")),18,FLinearColor::White);
        CraftFill=WidgetTree->ConstructWidget<UProgressBar>();CraftFill->SetFillColorAndOpacity(FLinearColor(.8f,.52f,.14f));CraftFill->SetPercent(CraftVisualFill);
        CraftPanel->AddChildToVerticalBox(CraftFill)->SetPadding(FMargin(18,15));
        const TArray<FString>& Ingredients=Craft.ingredientNames;
        if(Craft.ingredients<Craft.steps)AddButton(CraftPanel,TEXT("加入 ")+(Ingredients.IsValidIndex(Craft.ingredients)?Ingredients[Craft.ingredients]:TEXT("配方材料")),TEXT("craft:add"));
        else if(!Craft.mixed)AddButton(CraftPanel,TEXT("搅拌均匀"),TEXT("craft:stir"));
        else AddButton(CraftPanel,FString::Printf(TEXT("确认完成 · 支付 %d Cash"),Cost),TEXT("craft:finish"),Service->GetState().firstNight.availableCash>=Cost);
    }
    AddButton(CraftPanel,TEXT("取消并返回"),TEXT("craft:cancel"));
}
