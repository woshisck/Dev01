#include "UI/CombatItemBarWidget.h"

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

void UCombatItemBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildRuntimeLayout();
	RefreshItemSlots();
}

void UCombatItemBarWidget::NativeDestruct()
{
	UnbindCurrentComponent();
	Super::NativeDestruct();
}

void UCombatItemBarWidget::BindToCombatItemComponent(UCombatItemComponent* InCombatItemComponent)
{
	if (BoundCombatItemComponent == InCombatItemComponent)
	{
		RefreshItemSlots();
		return;
	}

	UnbindCurrentComponent();
	BoundCombatItemComponent = InCombatItemComponent;
	if (BoundCombatItemComponent)
	{
		BoundCombatItemComponent->OnItemSlotsChanged.AddDynamic(this, &UCombatItemBarWidget::HandleItemSlotsChanged);
		BoundCombatItemComponent->OnItemUsed.AddDynamic(this, &UCombatItemBarWidget::HandleItemUsed);
		BoundCombatItemComponent->OnItemUseFailed.AddDynamic(this, &UCombatItemBarWidget::HandleItemUseFailed);
	}

	RefreshItemSlots();
}

void UCombatItemBarWidget::RefreshItemSlots()
{
	BuildRuntimeLayout();
	const TArray<FCombatItemSlotView> Slots = BoundCombatItemComponent
		? BoundCombatItemComponent->GetSlotViews()
		: TArray<FCombatItemSlotView>();
	UpdateSlotWidgets(Slots);
	BP_OnItemSlotsChanged(Slots);
}

void UCombatItemBarWidget::UnbindCurrentComponent()
{
	if (!BoundCombatItemComponent)
	{
		return;
	}

	BoundCombatItemComponent->OnItemSlotsChanged.RemoveDynamic(this, &UCombatItemBarWidget::HandleItemSlotsChanged);
	BoundCombatItemComponent->OnItemUsed.RemoveDynamic(this, &UCombatItemBarWidget::HandleItemUsed);
	BoundCombatItemComponent->OnItemUseFailed.RemoveDynamic(this, &UCombatItemBarWidget::HandleItemUseFailed);
	BoundCombatItemComponent = nullptr;
}

void UCombatItemBarWidget::BuildRuntimeLayout()
{
	if (RuntimeRoot || !WidgetTree)
	{
		return;
	}

	RuntimeRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CombatItemRuntimeRoot"));
	WidgetTree->RootWidget = RuntimeRoot;

	RuntimeSlotWidgets.Reset();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FRuntimeSlotWidget& SlotWidget = RuntimeSlotWidgets.AddDefaulted_GetRef();

		SlotWidget.RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("ItemSlotBorder_%d"), Index));
		SlotWidget.RootBorder->SetPadding(FMargin(6.0f));

		UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("ItemSlotOverlay_%d"), Index));
		SlotWidget.RootBorder->SetContent(SlotOverlay);

		UVerticalBox* Vertical = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("ItemSlotVertical_%d"), Index));
		if (UOverlaySlot* OverlaySlot = SlotOverlay->AddChildToOverlay(Vertical))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}

		SlotWidget.IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("ItemSlotIcon_%d"), Index));
		if (UVerticalBoxSlot* IconSlot = Vertical->AddChildToVerticalBox(SlotWidget.IconImage))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
			IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		SlotWidget.NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("ItemSlotName_%d"), Index));
		SlotWidget.NameText->SetJustification(ETextJustify::Center);
		SlotWidget.NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo NameFont = SlotWidget.NameText->GetFont();
		NameFont.Size = 12;
		SlotWidget.NameText->SetFont(NameFont);
		Vertical->AddChildToVerticalBox(SlotWidget.NameText);

		SlotWidget.CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("ItemSlotCount_%d"), Index));
		SlotWidget.CountText->SetJustification(ETextJustify::Right);
		SlotWidget.CountText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.9f, 0.55f, 1.0f)));
		FSlateFontInfo CountFont = SlotWidget.CountText->GetFont();
		CountFont.Size = 14;
		SlotWidget.CountText->SetFont(CountFont);
		if (UOverlaySlot* CountSlot = SlotOverlay->AddChildToOverlay(SlotWidget.CountText))
		{
			CountSlot->SetHorizontalAlignment(HAlign_Right);
			CountSlot->SetVerticalAlignment(VAlign_Bottom);
			CountSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 4.0f));
		}

		SlotWidget.CooldownBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *FString::Printf(TEXT("ItemSlotCooldownBar_%d"), Index));
		SlotWidget.CooldownBar->SetPercent(0.0f);
		if (UOverlaySlot* CooldownBarSlot = SlotOverlay->AddChildToOverlay(SlotWidget.CooldownBar))
		{
			CooldownBarSlot->SetHorizontalAlignment(HAlign_Fill);
			CooldownBarSlot->SetVerticalAlignment(VAlign_Bottom);
			CooldownBarSlot->SetPadding(FMargin(4.0f));
		}

		SlotWidget.CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("ItemSlotCooldownText_%d"), Index));
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

void UCombatItemBarWidget::UpdateSlotWidgets(const TArray<FCombatItemSlotView>& Slots)
{
	for (int32 Index = 0; Index < RuntimeSlotWidgets.Num(); ++Index)
	{
		FRuntimeSlotWidget& SlotWidget = RuntimeSlotWidgets[Index];
		const bool bHasSlot = Slots.IsValidIndex(Index);
		const FCombatItemSlotView ItemSlot = bHasSlot ? Slots[Index] : FCombatItemSlotView();

		const FLinearColor BorderColor = ItemSlot.bSelected
			? FLinearColor(0.95f, 0.78f, 0.28f, 0.82f)
			: FLinearColor(0.04f, 0.04f, 0.04f, 0.70f);
		if (SlotWidget.RootBorder)
		{
			SlotWidget.RootBorder->SetBrushColor(BorderColor);
			SlotWidget.RootBorder->SetRenderOpacity(bHasSlot ? 1.0f : 0.35f);
		}

		if (SlotWidget.IconImage)
		{
			if (ItemSlot.Icon)
			{
				SlotWidget.IconImage->SetBrushFromTexture(ItemSlot.Icon);
				SlotWidget.IconImage->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				SlotWidget.IconImage->SetColorAndOpacity(FLinearColor(0.22f, 0.22f, 0.22f, 1.0f));
			}
		}

		if (SlotWidget.NameText)
		{
			SlotWidget.NameText->SetText(bHasSlot ? GetShortDisplayName(ItemSlot) : FText::GetEmpty());
		}
		if (SlotWidget.CountText)
		{
			SlotWidget.CountText->SetText(bHasSlot
				? FText::Format(FText::FromString(TEXT("{0}")), FText::AsNumber(ItemSlot.Charges))
				: FText::GetEmpty());
		}

		const bool bCoolingDown = bHasSlot && ItemSlot.CooldownRemaining > 0.0f && ItemSlot.CooldownDuration > 0.0f;
		if (SlotWidget.CooldownBar)
		{
			SlotWidget.CooldownBar->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			SlotWidget.CooldownBar->SetPercent(bCoolingDown ? ItemSlot.CooldownRemaining / ItemSlot.CooldownDuration : 0.0f);
		}
		if (SlotWidget.CooldownText)
		{
			SlotWidget.CooldownText->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			SlotWidget.CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(ItemSlot.CooldownRemaining)));
		}
	}
}

FText UCombatItemBarWidget::GetShortDisplayName(const FCombatItemSlotView& ItemSlot) const
{
	if (!ItemSlot.DisplayName.IsEmpty())
	{
		return ItemSlot.DisplayName;
	}

	switch (ItemSlot.EffectType)
	{
	case ECombatItemEffectType::OilBottle:
		return FText::FromString(TEXT("Oil"));
	case ECombatItemEffectType::ThunderStone:
		return FText::FromString(TEXT("Thunder"));
	case ECombatItemEffectType::SmokeBomb:
		return FText::FromString(TEXT("Smoke"));
	default:
		return FText::FromName(ItemSlot.ItemId);
	}
}

void UCombatItemBarWidget::HandleItemSlotsChanged(const TArray<FCombatItemSlotView>& Slots)
{
	UpdateSlotWidgets(Slots);
	BP_OnItemSlotsChanged(Slots);
}

void UCombatItemBarWidget::HandleItemUsed(int32 SlotIndex, FCombatItemSlotView ItemSlot)
{
	BP_OnItemUsed(SlotIndex, ItemSlot);
}

void UCombatItemBarWidget::HandleItemUseFailed(int32 SlotIndex, FText Reason)
{
	BP_OnItemUseFailed(SlotIndex, Reason);
}
