#include "UI/BubbleMessageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UBubbleMessageWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Build here rather than in NativeConstruct: RebuildWidget runs from TakeWidget, and by
	// NativeConstruct the slate widget has already been built from an empty WidgetTree.
	BuildFallbackLayout();
}

void UBubbleMessageWidget::SetLine(const FText& SpeakerName, const FText& Body)
{
	BuildFallbackLayout();

	if (SpeakerNameText)
	{
		const bool bHasSpeaker = !SpeakerName.IsEmpty();
		SpeakerNameText->SetText(SpeakerName);
		SpeakerNameText->SetVisibility(bHasSpeaker ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (BodyText)
	{
		BodyText->SetText(Body);
	}
}

void UBubbleMessageWidget::SetFadeAlpha(float Alpha)
{
	SetRenderOpacity(FMath::Clamp(Alpha, 0.f, 1.f));
}

void UBubbleMessageWidget::BuildFallbackLayout()
{
	if (BodyText || !WidgetTree)
	{
		return;
	}

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BubbleRoot"));
	Root->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.72f));
	Root->SetPadding(FMargin(12.f, 8.f));

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BubbleStack"));

	SpeakerNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SpeakerNameText"));
	SpeakerNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.45f, 1.f)));
	SpeakerNameText->SetVisibility(ESlateVisibility::Collapsed);
	Stack->AddChildToVerticalBox(SpeakerNameText);

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BodyText"));
	BodyText->SetAutoWrapText(true);
	BodyText->SetWrapTextAt(420.f);
	if (UVerticalBoxSlot* BodySlot = Cast<UVerticalBoxSlot>(Stack->AddChildToVerticalBox(BodyText)))
	{
		BodySlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}

	Root->SetContent(Stack);
	WidgetTree->RootWidget = Root;
}
