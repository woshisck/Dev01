#include "UI/InteractPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "UI/InputActionRichTextDecorator.h"
#include "UI/YogCommonRichTextBlock.h"

namespace
{
	const TCHAR* InfoPopupTextStyleClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InfoPopupTextStyle.BP_InfoPopupTextStyle_C");
}

FText UInteractPromptWidget::MakePromptMarkup(const FText& Label)
{
	if (Label.IsEmptyOrWhitespace())
	{
		return FText::FromString(TEXT("<input action=\"Interact\"/>"));
	}

	return FText::FromString(FString::Printf(TEXT("<input action=\"Interact\"/> %s"), *Label.ToString()));
}

void UInteractPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Build the fallback layout here (before RebuildWidget runs from TakeWidget) so the SObjectWidget
	// has a valid RootWidget the first time the screen layer takes our widget. NativeConstruct is too
	// late — by then the slate widget has already been built from an empty WidgetTree.
	// Same reason for the input decorator: it must be appended to DecoratorClasses before
	// SRichTextBlock is built, otherwise the <input action=.../> tag is rendered as raw text.
	BuildFallbackLayout();
	EnsureInputDecorator();
}

void UInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildFallbackLayout();
	EnsureInputDecorator();
	RefreshPrompt();

	if (UCommonInputSubsystem* InputSub =
		ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(GetOwningLocalPlayer()))
	{
		InputSub->OnInputMethodChangedNative.AddUObject(this, &UInteractPromptWidget::RefreshPrompt);
	}
}

void UInteractPromptWidget::NativeDestruct()
{
	if (UCommonInputSubsystem* InputSub =
		ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(GetOwningLocalPlayer()))
	{
		InputSub->OnInputMethodChangedNative.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UInteractPromptWidget::SetPromptLabel(const FText& InLabel)
{
	PromptLabel = InLabel;
	RefreshPrompt();
}

void UInteractPromptWidget::BuildFallbackLayout()
{
	if (PromptText || !WidgetTree)
	{
		return;
	}

	PromptBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptBorder"));
	PromptBorder->SetPadding(FMargin(12.f, 7.f));
	PromptBorder->SetBrush(FSlateRoundedBoxBrush(PromptFillColor, 4.f, PromptBorderColor, 1.f));

	PromptText = WidgetTree->ConstructWidget<UYogCommonRichTextBlock>(
		UYogCommonRichTextBlock::StaticClass(),
		TEXT("PromptText"));
	if (UClass* TextStyleClass = LoadClass<UCommonTextStyle>(nullptr, InfoPopupTextStyleClassPath))
	{
		PromptText->FontStyleClass = TextStyleClass;
	}
	PromptText->OverrideFontSize = PromptFontSize;
	PromptText->OverrideColor = PromptTextColor;

	PromptBorder->SetContent(PromptText);
	WidgetTree->RootWidget = PromptBorder;
}

void UInteractPromptWidget::RefreshPrompt(ECommonInputType /*NewInputType*/)
{
	if (PromptText)
	{
		PromptText->SetText(MakePromptMarkup(PromptLabel));
	}
}

void UInteractPromptWidget::EnsureInputDecorator()
{
	if (!PromptText)
	{
		return;
	}

	PromptText->EnsureDecoratorClass(UInputActionRichTextDecorator::StaticClass());
}
