#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "LalalandRootWidget.generated.h"

class UBorder;
class UEditableTextBox;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class ULalalandServiceSubsystem;

UCLASS()
class LALALANDUNREAL_API ULalalandActionButton final : public UButton
{
    GENERATED_BODY()
public:
    FString Payload;
    TWeakObjectPtr<class ULalalandRootWidget> Owner;
    void InitializeAction(ULalalandRootWidget* InOwner, const FString& InPayload);
private:
    UFUNCTION() void ForwardClick();
};

UCLASS()
class LALALANDUNREAL_API ULalalandRootWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    UFUNCTION() void HandleAction(const FString& Payload);

private:
    UFUNCTION() void Refresh();
    UFUNCTION() void HandleError(const FString& Message);
    UFUNCTION() void HandleAck(const FString& CommandId, const FString& Reason);
    UFUNCTION() void HandleReject(const FString& CommandId, const FString& Reason);
    UFUNCTION() void SendDialogue();
    UFUNCTION() void SaveModelConfig();

    UTextBlock* AddText(UVerticalBox* Parent, const FString& Text, int32 Size, const FLinearColor& Color);
    ULalalandActionButton* AddButton(UVerticalBox* Parent, const FString& Label, const FString& Payload, bool bEnabled = true, bool bSelected = false);
    ULalalandActionButton* AddButton(UHorizontalBox* Parent, const FString& Label, const FString& Payload, bool bEnabled = true, bool bSelected = false);
    void BuildWidgetTree();
    void BuildIntroPage();
    void BuildGameHud();
    void ChooseIntroValue(const FString& Value);
    void BuildSecondaryOptions(const FString& GroupId);
    void BuildPrimaryRow();
    void UpdateInteractionVisibility();
    void RebuildTargetRow();
    void ExecuteOption(const FString& OptionId);
    void SetSelectedTarget(const FString& Target);

    UPROPERTY() TObjectPtr<ULalalandServiceSubsystem> Service;
    UPROPERTY() TObjectPtr<UBorder> IntroBackdrop;
    UPROPERTY() TObjectPtr<UBorder> GameBackdrop;
    UPROPERTY() TObjectPtr<UVerticalBox> IntroPanel;
    UPROPERTY() TObjectPtr<UVerticalBox> IntroChoices;
    UPROPERTY() TObjectPtr<UTextBlock> IntroPrompt;
    UPROPERTY() TObjectPtr<UTextBlock> IntroProgress;
    UPROPERTY() TObjectPtr<UTextBlock> StatusText;
    UPROPERTY() TObjectPtr<UVerticalBox> GamePanel;
    UPROPERTY() TObjectPtr<UVerticalBox> InteractionPanel;
    UPROPERTY() TObjectPtr<UTextBlock> ObjectiveTitle;
    UPROPERTY() TObjectPtr<UTextBlock> ObjectiveHint;
    UPROPERTY() TObjectPtr<UTextBlock> PlayerDialogue;
    UPROPERTY() TObjectPtr<UVerticalBox> SecondaryOptions;
    UPROPERTY() TObjectPtr<UEditableTextBox> DialogueInput;
    UPROPERTY() TObjectPtr<UEditableTextBox> ApiBaseInput;
    UPROPERTY() TObjectPtr<UEditableTextBox> ModelInput;
    UPROPERTY() TObjectPtr<UEditableTextBox> ApiKeyInput;
    UPROPERTY() TObjectPtr<UHorizontalBox> TargetRow;
    UPROPERTY() TObjectPtr<UHorizontalBox> PrimaryRow;
    UPROPERTY() TObjectPtr<UHorizontalBox> DialogueRow;
    UPROPERTY() TObjectPtr<UTextBlock> TargetPrompt;
    FString SelectedRole;
    FString SelectedIntent;
    FString SelectedStyle;
    FString SelectedTarget;
    FString OpenGroup;
    FString PendingOption;
    FString PendingCommand;
    int32 IntroPage = 0;
};
