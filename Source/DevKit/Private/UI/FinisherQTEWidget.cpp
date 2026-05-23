#include "UI/FinisherQTEWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/InputActionRichTextDecorator.h"
#include "UI/YogCommonRichTextBlock.h"

void UFinisherQTEWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!PromptPanel || !KeyText || !PromptText || !WindowProgressBar || !IsKeyPromptRichText())
	{
		BuildRuntimeLayout(true);
	}

	HidePrompt();
}

void UFinisherQTEWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bPromptActive)
	{
		return;
	}

	const float Elapsed = FMath::Max(0.f, GetCurrentRealTime() - StartRealTime);
	const float RemainingAlpha = Duration > 0.f
		? FMath::Clamp(1.f - (Elapsed / Duration), 0.f, 1.f)
		: 0.f;
	RefreshVisuals(RemainingAlpha);

	if (RemainingAlpha <= 0.f)
	{
		HidePrompt();
	}
}

void UFinisherQTEWidget::ShowPrompt(float WindowDuration)
{
	BuildRuntimeLayout(!IsKeyPromptRichText());

	Duration = FMath::Max(0.05f, WindowDuration);
	StartRealTime = GetCurrentRealTime();
	bPromptActive = true;
	bConfirmed = false;

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshVisuals(1.f);
}

void UFinisherQTEWidget::HidePrompt()
{
	bPromptActive = false;
	bConfirmed = false;
	Duration = 0.f;
	SetVisibility(ESlateVisibility::Collapsed);
	RefreshVisuals(0.f);
}

void UFinisherQTEWidget::MarkConfirmed()
{
	bConfirmed = true;
	RefreshVisuals(1.f);
}

float UFinisherQTEWidget::GetCurrentRealTime() const
{
	return GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
}

void UFinisherQTEWidget::RefreshVisuals(float RemainingAlpha)
{
	if (KeyText)
	{
		SetTextIfSupported(KeyText, KeyLabel);
	}

	if (PromptText)
	{
		SetTextIfSupported(PromptText, bConfirmed ? ConfirmedPrompt : ActivePrompt);
	}

	if (PromptPanel)
	{
		const float IntroAlpha = Duration > 0.f
			? FMath::Clamp((GetCurrentRealTime() - StartRealTime) / 0.12f, 0.f, 1.f)
			: 1.f;
		const float SmoothIntro = FMath::InterpEaseInOut(0.f, 1.f, IntroAlpha, 2.f);
		const float ConfirmPulse = bConfirmed ? 1.04f : 1.f;
		const float CurrentScale = FMath::Lerp(0.94f, ConfirmPulse, SmoothIntro);
		PromptPanel->SetRenderOpacity(SmoothIntro);
		PromptPanel->SetRenderScale(FVector2D(CurrentScale, CurrentScale));
		PromptPanel->SetBrushColor(bConfirmed
			? FLinearColor(0.025f, 0.090f, 0.065f, 0.88f)
			: FLinearColor(0.015f, 0.016f, 0.020f, 0.82f));
	}

	if (WindowProgressBar)
	{
		WindowProgressBar->SetPercent(FMath::Clamp(RemainingAlpha, 0.f, 1.f));
		WindowProgressBar->SetFillColorAndOpacity(bConfirmed
			? FLinearColor(0.25f, 0.85f, 0.62f, 1.f)
			: FLinearColor(0.94f, 0.76f, 0.26f, 1.f));
	}
}

bool UFinisherQTEWidget::IsKeyPromptRichText() const
{
	return Cast<UYogCommonRichTextBlock>(KeyText) != nullptr;
}

void UFinisherQTEWidget::SetTextIfSupported(UWidget* Widget, const FText& Text) const
{
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
	{
		TextBlock->SetText(Text);
		return;
	}

	if (UYogCommonRichTextBlock* RichTextBlock = Cast<UYogCommonRichTextBlock>(Widget))
	{
		RichTextBlock->EnsureDecoratorClass(UInputActionRichTextDecorator::StaticClass());
		RichTextBlock->SetText(Text);
	}
}

void UFinisherQTEWidget::BuildRuntimeLayout(bool bForceRebuild)
{
	if (!WidgetTree || (!bForceRebuild && PromptPanel && KeyText && PromptText && WindowProgressBar))
	{
		return;
	}

	if (WidgetTree->RootWidget)
	{
		WidgetTree->RemoveWidget(WidgetTree->RootWidget);
		WidgetTree->RootWidget = nullptr;
	}

	RuntimeRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FinisherQTERoot"));
	WidgetTree->RootWidget = RuntimeRoot;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PromptPanelSize"));
	PanelSize->SetWidthOverride(280.f);
	PanelSize->SetHeightOverride(96.f);

	PromptPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptPanel"));
	PromptPanel->SetBrushColor(FLinearColor(0.015f, 0.016f, 0.020f, 0.82f));
	PromptPanel->SetPadding(FMargin(12.f, 10.f));

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PromptStack"));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PromptRow"));
	UBorder* KeyBack = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("KeyBack"));
	KeyBack->SetBrushColor(FLinearColor(0.92f, 0.74f, 0.25f, 0.95f));
	KeyBack->SetPadding(FMargin(12.f, 5.f));

	UYogCommonRichTextBlock* KeyRichText = WidgetTree->ConstructWidget<UYogCommonRichTextBlock>(UYogCommonRichTextBlock::StaticClass(), TEXT("KeyText"));
	KeyRichText->EnsureDecoratorClass(UInputActionRichTextDecorator::StaticClass());
	KeyRichText->OverrideFontSize = 22;
	KeyRichText->OverrideColor = FLinearColor(0.02f, 0.018f, 0.012f, 1.f);
	KeyText = KeyRichText;
	KeyBack->SetContent(KeyText);

	UTextBlock* PromptTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
	PromptTextBlock->SetJustification(ETextJustify::Left);
	PromptTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.96f, 0.92f, 1.f)));
	PromptTextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
	PromptTextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	FSlateFontInfo PromptFont = PromptTextBlock->GetFont();
	PromptFont.Size = 19;
	PromptTextBlock->SetFont(PromptFont);
	PromptText = PromptTextBlock;

	WindowProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("WindowProgressBar"));
	WindowProgressBar->SetPercent(1.f);
	WindowProgressBar->SetFillColorAndOpacity(FLinearColor(0.94f, 0.76f, 0.26f, 1.f));

	RuntimeRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	PanelSize->AddChild(PromptPanel);
	PromptPanel->SetContent(Stack);

	if (UVerticalBoxSlot* RowSlot = Stack->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
	if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyBack))
	{
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	if (UHorizontalBoxSlot* PromptSlot = Row->AddChildToHorizontalBox(PromptText))
	{
		PromptSlot->SetVerticalAlignment(VAlign_Center);
		PromptSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UVerticalBoxSlot* BarSlot = Stack->AddChildToVerticalBox(WindowProgressBar))
	{
		BarSlot->SetPadding(FMargin(0.f));
	}

	if (UCanvasPanelSlot* CanvasSlot = RuntimeRoot->AddChildToCanvas(PanelSize))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(0.f, 170.f));
		CanvasSlot->SetSize(FVector2D(280.f, 96.f));
		CanvasSlot->SetAutoSize(false);
	}
}
