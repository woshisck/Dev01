#include "UI/InteractPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "CommonInputSubsystem.h"
#include "Components/Border.h"
#include "UI/InputActionRichTextDecorator.h"
#include "UI/YogCommonRichTextBlock.h"

namespace
{
	const TCHAR* InputActionDecoratorClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InputActionDecorator.BP_InputActionDecorator_C");
}

FText UInteractPromptWidget::MakePromptMarkup(const FText& Label)
{
	if (Label.IsEmptyOrWhitespace())
	{
		return FText::FromString(TEXT("<input action=\"Interact\"/>"));
	}

	return FText::FromString(FString::Printf(TEXT("<input action=\"Interact\"/> %s"), *Label.ToString()));
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

	UClass* DecoratorClass = LoadClass<URichTextBlockDecorator>(nullptr, InputActionDecoratorClassPath);
	if (!DecoratorClass)
	{
		DecoratorClass = UInputActionRichTextDecorator::StaticClass();
	}

	PromptText->EnsureDecoratorClass(DecoratorClass);
}
