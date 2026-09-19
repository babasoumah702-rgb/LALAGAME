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
#include "LalalandServiceSubsystem.h"
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
    IntroProgress = AddText(IntroPanel, TEXT("1 / 3"), 15, FLinearColor(.55f, .58f, .62f));
    IntroPrompt = AddText(IntroPanel, TEXT("今晚以什么身份来？"), 28, FLinearColor::White);
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
    GameSize->SetWidthOverride(460.f);
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
    UHorizontalBox* SystemRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    GamePanel->AddChildToVerticalBox(SystemRow);
    AddButton(SystemRow, TEXT("线索册"), TEXT("system:clues"));
    AddButton(SystemRow, TEXT("保存"), TEXT("system:save"));
    AddButton(SystemRow, TEXT("暂停"), TEXT("system:pause"));

    InteractionPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InteractionPanel"));
    GamePanel->AddChildToVerticalBox(InteractionPanel);

    PrimaryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PrimaryRow"));
    InteractionPanel->AddChildToVerticalBox(PrimaryRow);
    BuildPrimaryRow();

    TargetPrompt = AddText(InteractionPanel, TEXT("选择对象"), 14, FLinearColor(.65f, .67f, .7f));
    TargetRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(TargetRow);
    SecondaryOptions = WidgetTree->ConstructWidget<UVerticalBox>();
    InteractionPanel->AddChildToVerticalBox(SecondaryOptions);

    DialogueRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    InteractionPanel->AddChildToVerticalBox(DialogueRow);
    DialogueInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    DialogueInput->SetHintText(FText::FromString(TEXT("对所选人物说……")));
    UHorizontalBoxSlot* InputSlot = DialogueRow->AddChildToHorizontalBox(DialogueInput);
    InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ULalalandActionButton* Send = AddButton(DialogueRow, TEXT("发送"), TEXT("system:send"));
    Send->SetToolTipText(FText::FromString(TEXT("只让当前选择的人回应")));
    PlayerDialogue = AddText(GamePanel, FString(), 17, FLinearColor(.95f, .95f, .96f));
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
    Button->SetBackgroundColor(bSelected ? FLinearColor(.24f, .24f, .26f) : FLinearColor(.15f, .18f, .22f));
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(bSelected ? TEXT("✓ ") + Label : Label));
    Text->SetFont(LalalandFont(17));
    Text->SetColorAndOpacity(FSlateColor(bEnabled ? FLinearColor::White : FLinearColor(.45f, .45f, .47f)));
    Button->SetContent(Text);
    UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
    ButtonSlot->SetPadding(FMargin(18, 4));
    return Button;
}

ULalalandActionButton* ULalalandRootWidget::AddButton(UHorizontalBox* Parent, const FString& Label, const FString& Payload, bool bEnabled, bool bSelected)
{
    ULalalandActionButton* Button = WidgetTree->ConstructWidget<ULalalandActionButton>();
    Button->InitializeAction(this, Payload);
    Button->SetIsEnabled(bEnabled && !bSelected);
    Button->SetBackgroundColor(bSelected ? FLinearColor(.24f, .24f, .26f) : FLinearColor(.15f, .18f, .22f));
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(bSelected ? TEXT("✓ ") + Label : Label));
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
    RebuildTargetRow();
    BuildPrimaryRow();
    BuildSecondaryOptions(OpenGroup);
    if (Service->GetState().events.Num())
    {
        const FLalalandEventDto& Last = Service->GetState().events.Last();
        if (Last.actor == TEXT("USER")) PlayerDialogue->SetText(FText::FromString(Last.text));
    }
}

void ULalalandRootWidget::BuildIntroPage()
{
    IntroChoices->ClearChildren();
    const FLalalandBootstrapDto& Bootstrap = Service->GetBootstrap();
    if (!Service->IsReady()) return;
    IntroProgress->SetText(FText::FromString(FString::Printf(TEXT("%d / 3"), IntroPage + 1)));
    const TArray<FLalalandEntryDto>* Entries = nullptr;
    FString Prompt;
    if (IntroPage == 0) { Entries = &Bootstrap.roles; Prompt = TEXT("今晚以什么身份来？"); }
    else if (IntroPage == 1) { Entries = &Bootstrap.intents; Prompt = TEXT("今晚想做什么？"); }
    else { Entries = &Bootstrap.styles; Prompt = TEXT("希望怎样聊天？"); }
    IntroPrompt->SetText(FText::FromString(Prompt));
    for (const FLalalandEntryDto& Entry : *Entries)
    {
        const FString Current = IntroPage == 0 ? SelectedRole : IntroPage == 1 ? SelectedIntent : SelectedStyle;
        AddButton(IntroChoices, Entry.name, TEXT("intro:") + Entry.id, true, Entry.id == Current);
    }
    AddButton(IntroChoices, TEXT("跳过"), TEXT("intro:skip"));
    if (IntroPage > 0) AddButton(IntroChoices, TEXT("返回"), TEXT("intro:back"));
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
    if (Payload == TEXT("system:model")) { SaveModelConfig(); return; }
    if (Payload == TEXT("system:save")) { Service->Save(); return; }
    if (Payload == TEXT("system:pause"))
    {
        FLalalandCommandDto Command; Command.type = TEXT("pause"); Command.paused = !Service->GetState().paused; Service->SendCommand(Command); return;
    }
}

void ULalalandRootWidget::ChooseIntroValue(const FString& Value)
{
    if (Value == TEXT("back")) { IntroPage = FMath::Max(0, IntroPage - 1); BuildIntroPage(); return; }
    const FString Chosen = Value == TEXT("skip") ? (IntroPage == 0 ? TEXT("passerby") : IntroPage == 1 ? TEXT("observe_only") : TEXT("natural")) : Value;
    if (IntroPage == 0) SelectedRole = Chosen;
    else if (IntroPage == 1) SelectedIntent = Chosen;
    else SelectedStyle = Chosen;
    if (IntroPage < 2) { ++IntroPage; BuildIntroPage(); }
    else Service->OpenNewSession(SelectedRole, SelectedIntent, SelectedStyle, Service->GetBootstrap().modelConfigured);
}

void ULalalandRootWidget::BuildSecondaryOptions(const FString& GroupId)
{
    SecondaryOptions->ClearChildren();
    const FLalalandInteractionDto& Interaction = Service->GetState().interaction;
    for (const FLalalandInteractionGroupDto& Group : Interaction.groups)
    {
        if (Group.id != GroupId) continue;
        for (const FLalalandInteractionOptionDto& Option : Group.options)
        {
            const bool bPending = PendingOption == Option.id;
            ULalalandActionButton* Button = AddButton(SecondaryOptions, Option.label, TEXT("option:") + Option.id, Option.enabled && !bPending, Option.selected || bPending);
            if (!Option.enabled && !Option.disabledReason.IsEmpty()) Button->SetToolTipText(FText::FromString(Option.disabledReason));
        }
    }
    UpdateInteractionVisibility();
}

void ULalalandRootWidget::BuildPrimaryRow()
{
    if (!PrimaryRow) return;
    PrimaryRow->ClearChildren();
    AddButton(PrimaryRow, TEXT("观察"), TEXT("group:observe"), true, OpenGroup == TEXT("observe"));
    AddButton(PrimaryRow, TEXT("移动"), TEXT("group:move"), true, OpenGroup == TEXT("move"));
    AddButton(PrimaryRow, TEXT("互动"), TEXT("group:interact"), true, OpenGroup == TEXT("interact"));
}

void ULalalandRootWidget::UpdateInteractionVisibility()
{
    if (!TargetRow || !TargetPrompt || !DialogueRow) return;
    bool bNeedsTarget = false;
    if (Service)
    {
        for (const FLalalandInteractionGroupDto& Group : Service->GetState().interaction.groups)
        {
            if (Group.id != OpenGroup) continue;
            bNeedsTarget = Group.options.ContainsByPredicate([](const FLalalandInteractionOptionDto& Option)
            {
                return Option.targetRequired;
            });
            break;
        }
    }
    const ESlateVisibility TargetVisibility = bNeedsTarget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    TargetPrompt->SetVisibility(TargetVisibility);
    TargetRow->SetVisibility(TargetVisibility);
    DialogueRow->SetVisibility(OpenGroup == TEXT("interact") ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void ULalalandRootWidget::ExecuteOption(const FString& OptionId)
{
    if (!PendingOption.IsEmpty()) return;
    if (OptionId == TEXT("talk")) { DialogueInput->SetKeyboardFocus(); return; }
    FLalalandCommandDto Command;
    if (OptionId == TEXT("observe_room") || OptionId == TEXT("observe_target")) { Command.type = TEXT("observe"); Command.target = SelectedTarget; }
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

void ULalalandRootWidget::RebuildTargetRow()
{
    TargetRow->ClearChildren();
    TArray<const FLalalandActorDto*> Targets;
    for (const FLalalandActorDto& Actor : Service->GetState().characters)
    {
        if (Actor.id != TEXT("USER") && Actor.id != TEXT("OWNER") && Actor.interactable) Targets.Add(&Actor);
    }
    if (Targets.Num() && !Targets.ContainsByPredicate([this](const FLalalandActorDto* Actor) { return Actor->id == SelectedTarget; }))
        SelectedTarget = Targets[0]->id;
    for (const FLalalandActorDto* Actor : Targets)
    {
        const FString Label = Actor->name.IsEmpty() ? Actor->id : Actor->name;
        AddButton(TargetRow, Label, TEXT("target:") + Actor->id, true, Actor->id == SelectedTarget);
    }
}

void ULalalandRootWidget::SendDialogue()
{
    const FString Text = DialogueInput->GetText().ToString().TrimStartAndEnd();
    if (Text.IsEmpty() || !PendingCommand.IsEmpty()) return;
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
