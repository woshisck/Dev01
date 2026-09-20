#include "UI/InteractPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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
	SetHoldProgress(0.f);

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

void UInteractPromptWidget::SetHoldProgress(float Normalized)
{
	if (!HoldProgressBar)
	{
		return;
	}

	const float Clamped = FMath::Clamp(Normalized, 0.f, 1.f);
	HoldProgressBar->SetPercent(Clamped);
	HoldProgressBar->SetVisibility(Clamped > KINDA_SMALL_NUMBER
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
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

	HoldProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(),
		TEXT("HoldProgressBar"));
	// SProgressBar takes its desired height from the style brush, so the bar thickness has to
	// be baked into ImageSize here rather than set on the UProgressBar itself.
	const FVector2f BarImageSize(64.f, HoldProgressBarHeight);
	FProgressBarStyle HoldStyle = HoldProgressBar->GetWidgetStyle();
	HoldStyle.BackgroundImage = FSlateRoundedBoxBrush(PromptFillColor, 2.f, BarImageSize);
	HoldStyle.FillImage = FSlateRoundedBoxBrush(HoldProgressFillColor, 2.f, BarImageSize);
	HoldProgressBar->SetWidgetStyle(HoldStyle);
	HoldProgressBar->SetPercent(0.f);
	HoldProgressBar->SetVisibility(ESlateVisibility::Collapsed);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("PromptRoot"));
	Root->AddChildToVerticalBox(PromptBorder);
	if (UVerticalBoxSlot* BarSlot = Cast<UVerticalBoxSlot>(Root->AddChildToVerticalBox(HoldProgressBar)))
	{
		BarSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	WidgetTree->RootWidget = Root;
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
