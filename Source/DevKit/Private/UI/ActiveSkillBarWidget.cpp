#include "UI/ActiveSkillBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UActiveSkillBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildRuntimeLayout();
	RefreshSkillSlots();
}

void UActiveSkillBarWidget::NativeDestruct()
{
	UnbindCurrentComponent();
	Super::NativeDestruct();
}

void UActiveSkillBarWidget::BindToActiveSkillComponent(UPlayerActiveSkillComponent* InActiveSkillComponent)
{
	if (BoundActiveSkillComponent == InActiveSkillComponent)
	{
		RefreshSkillSlots();
		return;
	}

	UnbindCurrentComponent();
	BoundActiveSkillComponent = InActiveSkillComponent;
	if (BoundActiveSkillComponent)
	{
		BoundActiveSkillComponent->OnSkillSlotsChanged.AddDynamic(this, &UActiveSkillBarWidget::HandleSkillSlotsChanged);
		BoundActiveSkillComponent->OnSkillUsed.AddDynamic(this, &UActiveSkillBarWidget::HandleSkillUsed);
		BoundActiveSkillComponent->OnSkillUseFailed.AddDynamic(this, &UActiveSkillBarWidget::HandleSkillUseFailed);
	}

	RefreshSkillSlots();
}

void UActiveSkillBarWidget::RefreshSkillSlots()
{
	BuildRuntimeLayout();
	const TArray<FActiveSkillSlotView> Slots = BoundActiveSkillComponent
		? BoundActiveSkillComponent->GetSlotViews()
		: TArray<FActiveSkillSlotView>();
	UpdateSlotWidgets(Slots);
	BP_OnSkillSlotsChanged(Slots);
}

void UActiveSkillBarWidget::UnbindCurrentComponent()
{
	if (!BoundActiveSkillComponent)
	{
		return;
	}

	BoundActiveSkillComponent->OnSkillSlotsChanged.RemoveDynamic(this, &UActiveSkillBarWidget::HandleSkillSlotsChanged);
	BoundActiveSkillComponent->OnSkillUsed.RemoveDynamic(this, &UActiveSkillBarWidget::HandleSkillUsed);
	BoundActiveSkillComponent->OnSkillUseFailed.RemoveDynamic(this, &UActiveSkillBarWidget::HandleSkillUseFailed);
	BoundActiveSkillComponent = nullptr;
}

void UActiveSkillBarWidget::BuildRuntimeLayout()
{
	if (RuntimeRoot || !WidgetTree)
	{
		return;
	}

	RuntimeRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActiveSkillRuntimeRoot"));
	WidgetTree->RootWidget = RuntimeRoot;

	RuntimeSlotWidgets.Reset();
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FRuntimeSkillSlotWidget& SlotWidget = RuntimeSlotWidgets.AddDefaulted_GetRef();

		SlotWidget.RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotBorder_%d"), Index));
		SlotWidget.RootBorder->SetPadding(FMargin(6.0f));

		UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotOverlay_%d"), Index));
		SlotWidget.RootBorder->SetContent(SlotOverlay);

		UVerticalBox* Vertical = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotVertical_%d"), Index));
		if (UOverlaySlot* OverlaySlot = SlotOverlay->AddChildToOverlay(Vertical))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}

		SlotWidget.IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotIcon_%d"), Index));
		if (UVerticalBoxSlot* IconSlot = Vertical->AddChildToVerticalBox(SlotWidget.IconImage))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
			IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		SlotWidget.NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotName_%d"), Index));
		SlotWidget.NameText->SetJustification(ETextJustify::Center);
		SlotWidget.NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo NameFont = SlotWidget.NameText->GetFont();
		NameFont.Size = 12;
		SlotWidget.NameText->SetFont(NameFont);
		Vertical->AddChildToVerticalBox(SlotWidget.NameText);

		SlotWidget.KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotKey_%d"), Index));
		SlotWidget.KeyText->SetJustification(ETextJustify::Right);
		SlotWidget.KeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.86f, 1.0f, 1.0f)));
		FSlateFontInfo KeyFont = SlotWidget.KeyText->GetFont();
		KeyFont.Size = 13;
		SlotWidget.KeyText->SetFont(KeyFont);
		if (UOverlaySlot* KeySlot = SlotOverlay->AddChildToOverlay(SlotWidget.KeyText))
		{
			KeySlot->SetHorizontalAlignment(HAlign_Right);
			KeySlot->SetVerticalAlignment(VAlign_Bottom);
			KeySlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 4.0f));
		}

		SlotWidget.CooldownBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotCooldownBar_%d"), Index));
		SlotWidget.CooldownBar->SetPercent(0.0f);
		if (UOverlaySlot* CooldownBarSlot = SlotOverlay->AddChildToOverlay(SlotWidget.CooldownBar))
		{
			CooldownBarSlot->SetHorizontalAlignment(HAlign_Fill);
			CooldownBarSlot->SetVerticalAlignment(VAlign_Bottom);
			CooldownBarSlot->SetPadding(FMargin(4.0f));
		}

		SlotWidget.CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("ActiveSkillSlotCooldownText_%d"), Index));
		SlotWidget.CooldownText->SetJustification(ETextJustify::Center);
		SlotWidget.CooldownText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		if (UOverlaySlot* CooldownTextSlot = SlotOverlay->AddChildToOverlay(SlotWidget.CooldownText))
		{
			CooldownTextSlot->SetHorizontalAlignment(HAlign_Center);
			CooldownTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UHorizontalBoxSlot* RootSlot = RuntimeRoot->AddChildToHorizontalBox(SlotWidget.RootBorder))
		{
			RootSlot->SetPadding(FMargin(4.0f));
			RootSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}
}

void UActiveSkillBarWidget::UpdateSlotWidgets(const TArray<FActiveSkillSlotView>& Slots)
{
	bool bAnyUnlocked = false;
	for (const FActiveSkillSlotView& SkillView : Slots)
	{
		bAnyUnlocked |= !SkillView.bLocked;
	}
	SetVisibility(bAnyUnlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	for (int32 Index = 0; Index < RuntimeSlotWidgets.Num(); ++Index)
	{
		FRuntimeSkillSlotWidget& SlotWidget = RuntimeSlotWidgets[Index];
		const bool bHasSlot = Slots.IsValidIndex(Index);
		const FActiveSkillSlotView SkillSlot = bHasSlot ? Slots[Index] : FActiveSkillSlotView();
		const bool bEquipped = bHasSlot && SkillSlot.SkillId != NAME_None;

		const FLinearColor BorderColor = SkillSlot.bLocked
			? FLinearColor(0.02f, 0.02f, 0.02f, 0.45f)
			: SkillSlot.bSelected
				? FLinearColor(0.25f, 0.62f, 1.0f, 0.82f)
				: FLinearColor(0.04f, 0.04f, 0.04f, 0.70f);
		if (SlotWidget.RootBorder)
		{
			SlotWidget.RootBorder->SetBrushColor(BorderColor);
			SlotWidget.RootBorder->SetRenderOpacity(bHasSlot ? 1.0f : 0.35f);
		}

		if (SlotWidget.IconImage)
		{
			if (SkillSlot.Icon)
			{
				SlotWidget.IconImage->SetBrushFromTexture(SkillSlot.Icon);
				SlotWidget.IconImage->SetColorAndOpacity(SkillSlot.bLocked ? FLinearColor(0.35f, 0.35f, 0.35f, 1.0f) : FLinearColor::White);
			}
			else
			{
				SlotWidget.IconImage->SetBrush(FSlateBrush());
				SlotWidget.IconImage->SetColorAndOpacity(FLinearColor(0.16f, 0.20f, 0.24f, 1.0f));
			}
		}

		if (SlotWidget.NameText)
		{
			SlotWidget.NameText->SetText(SkillSlot.bLocked
				? FText::FromString(TEXT("Locked"))
				: bEquipped ? GetShortDisplayName(SkillSlot) : FText::FromString(TEXT("Empty")));
		}
		if (SlotWidget.KeyText)
		{
			SlotWidget.KeyText->SetText(Index == 0
				? FText::FromString(TEXT("R/RB"))
				: FText::FromString(TEXT("T/R3")));
		}

		const bool bCoolingDown = bHasSlot && SkillSlot.CooldownRemaining > 0.0f && SkillSlot.CooldownDuration > 0.0f;
		if (SlotWidget.CooldownBar)
		{
			SlotWidget.CooldownBar->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			SlotWidget.CooldownBar->SetPercent(bCoolingDown ? SkillSlot.CooldownRemaining / SkillSlot.CooldownDuration : 0.0f);
		}
		if (SlotWidget.CooldownText)
		{
			SlotWidget.CooldownText->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			SlotWidget.CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(SkillSlot.CooldownRemaining)));
		}
	}
}

FText UActiveSkillBarWidget::GetShortDisplayName(const FActiveSkillSlotView& SkillSlot) const
{
	if (!SkillSlot.DisplayName.IsEmpty())
	{
		return SkillSlot.DisplayName;
	}

	return SkillSlot.SkillId != NAME_None ? FText::FromName(SkillSlot.SkillId) : FText::GetEmpty();
}

void UActiveSkillBarWidget::HandleSkillSlotsChanged(const TArray<FActiveSkillSlotView>& Slots)
{
	UpdateSlotWidgets(Slots);
	BP_OnSkillSlotsChanged(Slots);
}

void UActiveSkillBarWidget::HandleSkillUsed(int32 SlotIndex, FActiveSkillSlotView SkillSlot)
{
	BP_OnSkillUsed(SlotIndex, SkillSlot);
}

void UActiveSkillBarWidget::HandleSkillUseFailed(int32 SlotIndex, FText Reason)
{
	BP_OnSkillUseFailed(SlotIndex, Reason);
}
