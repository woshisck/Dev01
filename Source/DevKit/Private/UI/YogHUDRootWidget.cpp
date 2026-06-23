#include "UI/YogHUDRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Styling/SlateBrush.h"
#include "UI/PlayerCommonInfoWidget.h"
#include "UI/WidgetReflectorDebugUtils.h"
#include "UI/WeaponGlassIconWidget.h"
#include "UI/WeaponComboTextUtils.h"
#include "UI/YogCommonRichTextBlock.h"

namespace
{
	void ConfigureWeaponSlotText(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(ETextJustify::Center);
		TextBlock->SetAutoWrapText(false);
		TextBlock->SetClipping(EWidgetClipping::ClipToBounds);
		TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = FontSize;
		TextBlock->SetFont(FontInfo);
	}

	void ConfigureWeaponIcon(UImage* Icon)
	{
		if (!Icon)
		{
			return;
		}

		Icon->SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
		Icon->SetColorAndOpacity(FLinearColor(0.26f, 0.28f, 0.32f, 1.f));
	}

	FText GetWeaponDisplayName(const UWeaponDefinition* WeaponDefinition)
	{
		if (!WeaponDefinition)
		{
			return FText::GetEmpty();
		}

		if (const UWeaponInfoDA* WeaponInfo = WeaponDefinition->WeaponInfo.Get())
		{
			if (!WeaponInfo->WeaponName.IsEmpty())
			{
				return WeaponInfo->WeaponName;
			}
		}

		return FText::FromString(WeaponDefinition->GetName());
	}

	UTexture2D* GetWeaponThumbnail(const UWeaponDefinition* WeaponDefinition)
	{
		const UWeaponInfoDA* WeaponInfo = WeaponDefinition ? WeaponDefinition->WeaponInfo.Get() : nullptr;
		return WeaponInfo ? WeaponInfo->Thumbnail.Get() : nullptr;
	}

	void UpdateWeaponSlot(UWidget* Slot, UImage* Icon, UTextBlock* NameText, const UWeaponDefinition* WeaponDefinition, bool bActive)
	{
		if (Slot)
		{
			Slot->SetRenderOpacity(WeaponDefinition ? (bActive ? 1.f : 0.68f) : 0.35f);
		}

		if (Icon)
		{
			if (UTexture2D* Thumbnail = GetWeaponThumbnail(WeaponDefinition))
			{
				Icon->SetBrushFromTexture(Thumbnail, true);
				Icon->SetColorAndOpacity(bActive ? FLinearColor::White : FLinearColor(0.72f, 0.76f, 0.82f, 1.f));
			}
			else
			{
				Icon->SetBrush(FSlateBrush());
				Icon->SetColorAndOpacity(bActive
					? FLinearColor(0.34f, 0.30f, 0.24f, 1.f)
					: FLinearColor(0.18f, 0.20f, 0.24f, 1.f));
			}
		}

		if (NameText)
		{
			NameText->SetText(WeaponDefinition ? GetWeaponDisplayName(WeaponDefinition) : FText::FromString(TEXT("Empty")));
		}
	}
}

void UYogHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyExampleHudLayout();
	EnsureWeaponLoadoutPanel();
	RebindWeaponPanelPlayer();
	RefreshWeaponPanel(true);
	ApplyWidgetReflectorDebugVisibility();
}

void UYogHUDRootWidget::NativeDestruct()
{
	if (APlayerCharacterBase* Player = BoundWeaponPanelPlayer.Get())
	{
		Player->OnWeaponSwitched.RemoveDynamic(this, &UYogHUDRootWidget::HandleWeaponSwitched);
	}
	BoundWeaponPanelPlayer.Reset();

	Super::NativeDestruct();
}

void UYogHUDRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RebindWeaponPanelPlayer();

	ComboListRefreshAccumulator += InDeltaTime;
	if (ComboListRefreshAccumulator >= 0.1f)
	{
		ComboListRefreshAccumulator = 0.f;
		RefreshWeaponPanel();
	}
}

bool UYogHUDRootWidget::GetActiveWeaponSlotScreenCenter(FVector2D& OutScreenCenter) const
{
	const UWidget* CenterWidget = ActiveWeaponSlot ? ActiveWeaponSlot.Get() : Cast<UWidget>(WeaponGlassIcon.Get());
	if (!CenterWidget)
	{
		return false;
	}

	const FGeometry& Geometry = CenterWidget->GetCachedGeometry();
	const FVector2D LocalSize = Geometry.GetLocalSize();
	if (LocalSize.IsNearlyZero())
	{
		return false;
	}

	OutScreenCenter = Geometry.LocalToAbsolute(LocalSize * 0.5f);
	return true;
}

void UYogHUDRootWidget::ApplyExampleHudLayout()
{
	if (TopLeftPlayerInfoRegion)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TopLeftPlayerInfoRegion->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
			CanvasSlot->SetPosition(FVector2D(24.f, 24.f));
			CanvasSlot->SetSize(FVector2D(560.f, 220.f));
			CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
		}
		TopLeftPlayerInfoRegion->SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
	}

	if (TopRightPlayerInfoRegion)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TopRightPlayerInfoRegion->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(1.f, 0.f));
			CanvasSlot->SetPosition(FVector2D(-24.f, 24.f));
			CanvasSlot->SetSize(FVector2D(420.f, 160.f));
			CanvasSlot->SetAlignment(FVector2D(1.f, 0.f));
		}
		TopRightPlayerInfoRegion->SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
	}

	if (BottomRightPlayerInfoRegion)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BottomRightPlayerInfoRegion->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(1.f, 1.f));
			CanvasSlot->SetPosition(FVector2D(-24.f, -24.f));
			CanvasSlot->SetSize(FVector2D(360.f, 112.f));
			CanvasSlot->SetAlignment(FVector2D(1.f, 1.f));
		}
		BottomRightPlayerInfoRegion->SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
	}

	if (PlayerCommonInfoHud)
	{
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(PlayerCommonInfoHud->Slot))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Right);
			OverlaySlot->SetVerticalAlignment(VAlign_Bottom);
			OverlaySlot->SetPadding(FMargin(0.f, 0.f, 8.f, 8.f));
		}
	}
}

void UYogHUDRootWidget::EnsureWeaponLoadoutPanel()
{
	if (WeaponLoadoutPanel || !TopLeftPlayerInfoRegion || !WidgetTree)
	{
		return;
	}

	UHorizontalBox* RootRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RuntimeWeaponLoadoutPanel"));
	if (!RootRow)
	{
		return;
	}

	WeaponLoadoutPanel = RootRow;

	auto BuildWeaponSlot = [this](const FName SlotName, const FName IconName, const FName TextName, bool bActive) -> UWidget*
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), SlotName);
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("%sFrame"), *SlotName.ToString()));
		UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("%sStack"), *SlotName.ToString()));
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("%sIconBox"), *SlotName.ToString()));
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);

		if (!SizeBox || !Frame || !Stack || !IconBox || !Icon || !NameText)
		{
			return nullptr;
		}

		SizeBox->SetWidthOverride(bActive ? 116.f : 74.f);
		SizeBox->SetHeightOverride(bActive ? 142.f : 104.f);
		SizeBox->SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));

		Frame->SetBrushColor(bActive
			? FLinearColor(0.065f, 0.055f, 0.050f, 0.88f)
			: FLinearColor(0.035f, 0.038f, 0.045f, 0.74f));
		Frame->SetPadding(bActive ? FMargin(8.f, 8.f, 8.f, 6.f) : FMargin(6.f, 6.f, 6.f, 4.f));
		Frame->SetContent(Stack);
		SizeBox->AddChild(Frame);

		IconBox->SetWidthOverride(bActive ? 86.f : 54.f);
		IconBox->SetHeightOverride(bActive ? 86.f : 54.f);
		ConfigureWeaponIcon(Icon);
		IconBox->AddChild(Icon);
		if (UVerticalBoxSlot* IconSlot = Stack->AddChildToVerticalBox(IconBox))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));
		}

		ConfigureWeaponSlotText(NameText, bActive ? 12 : 10, bActive
			? FLinearColor(0.95f, 0.87f, 0.70f, 1.f)
			: FLinearColor(0.68f, 0.72f, 0.80f, 1.f));
		if (UVerticalBoxSlot* TextSlot = Stack->AddChildToVerticalBox(NameText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		if (bActive)
		{
			ActiveWeaponSlot = SizeBox;
			ActiveWeaponIcon = Icon;
			ActiveWeaponNameText = NameText;
		}
		else
		{
			InactiveWeaponSlot = SizeBox;
			InactiveWeaponIcon = Icon;
			InactiveWeaponNameText = NameText;
		}

		return SizeBox;
	};

	if (UWidget* ActiveSlot = BuildWeaponSlot(TEXT("RuntimeActiveWeaponSlot"), TEXT("RuntimeActiveWeaponIcon"), TEXT("RuntimeActiveWeaponName"), true))
	{
		if (UHorizontalBoxSlot* BoxSlot = RootRow->AddChildToHorizontalBox(ActiveSlot))
		{
			BoxSlot->SetVerticalAlignment(VAlign_Top);
			BoxSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}
	}

	if (UWidget* InactiveSlot = BuildWeaponSlot(TEXT("RuntimeInactiveWeaponSlot"), TEXT("RuntimeInactiveWeaponIcon"), TEXT("RuntimeInactiveWeaponName"), false))
	{
		if (UHorizontalBoxSlot* BoxSlot = RootRow->AddChildToHorizontalBox(InactiveSlot))
		{
			BoxSlot->SetVerticalAlignment(VAlign_Top);
			BoxSlot->SetPadding(FMargin(0.f, 32.f, 12.f, 0.f));
		}
	}

	if (WeaponGlassIcon)
	{
		WeaponGlassIcon->RemoveFromParent();
		WeaponGlassIcon->SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
		if (UHorizontalBoxSlot* BoxSlot = RootRow->AddChildToHorizontalBox(WeaponGlassIcon.Get()))
		{
			BoxSlot->SetVerticalAlignment(VAlign_Top);
			BoxSlot->SetPadding(FMargin(0.f, 40.f, 10.f, 0.f));
		}
	}

	if (WeaponComboListPanel)
	{
		WeaponComboListPanel->RemoveFromParent();
		if (USizeBox* ComboSizeBox = Cast<USizeBox>(WeaponComboListPanel))
		{
			ComboSizeBox->SetWidthOverride(320.f);
		}
		WeaponComboListPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (UHorizontalBoxSlot* BoxSlot = RootRow->AddChildToHorizontalBox(WeaponComboListPanel))
		{
			BoxSlot->SetVerticalAlignment(VAlign_Top);
			BoxSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		}
	}

	if (UOverlaySlot* OverlaySlot = TopLeftPlayerInfoRegion->AddChildToOverlay(RootRow))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Left);
		OverlaySlot->SetVerticalAlignment(VAlign_Top);
	}
}

void UYogHUDRootWidget::ApplyWidgetReflectorDebugVisibility()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		return;
	}

	YogWidgetReflectorDebug::ApplyToWidgetTree(WidgetTree->RootWidget);
}

void UYogHUDRootWidget::RebindWeaponPanelPlayer()
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
	if (BoundWeaponPanelPlayer.Get() == Player)
	{
		return;
	}

	if (APlayerCharacterBase* OldPlayer = BoundWeaponPanelPlayer.Get())
	{
		OldPlayer->OnWeaponSwitched.RemoveDynamic(this, &UYogHUDRootWidget::HandleWeaponSwitched);
	}

	BoundWeaponPanelPlayer = Player;

	if (Player)
	{
		Player->OnWeaponSwitched.RemoveDynamic(this, &UYogHUDRootWidget::HandleWeaponSwitched);
		Player->OnWeaponSwitched.AddDynamic(this, &UYogHUDRootWidget::HandleWeaponSwitched);
	}

	RefreshWeaponPanel(true);
}

void UYogHUDRootWidget::HandleWeaponSwitched()
{
	RefreshWeaponPanel(true);
}

void UYogHUDRootWidget::RefreshWeaponPanel(bool bForce)
{
	UWeaponDefinition* CurrentWeapon = nullptr;
	UWeaponDefinition* InactiveWeapon = nullptr;
	if (const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
	{
		CurrentWeapon = Player->EquippedWeaponDef.Get();
		InactiveWeapon = Player->InactiveWeaponDef.Get();
	}

	if (!bForce
		&& CachedComboWeaponDefinition.Get() == CurrentWeapon
		&& CachedInactiveWeaponDefinition.Get() == InactiveWeapon)
	{
		return;
	}

	CachedComboWeaponDefinition = CurrentWeapon;
	CachedInactiveWeaponDefinition = InactiveWeapon;

	UpdateWeaponSlot(ActiveWeaponSlot, ActiveWeaponIcon, ActiveWeaponNameText, CurrentWeapon, true);
	UpdateWeaponSlot(InactiveWeaponSlot, InactiveWeaponIcon, InactiveWeaponNameText, InactiveWeapon, false);

	if (!WeaponComboListPanel || !WeaponComboListText)
	{
		return;
	}

	if (!CurrentWeapon)
	{
		WeaponComboListPanel->SetVisibility(ESlateVisibility::Collapsed);
		WeaponComboListText->SetText(FText::GetEmpty());
		return;
	}

	WeaponComboListPanel->SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
	WeaponComboListText->SetText(WeaponComboTextUtils::BuildComboHintText(CurrentWeapon, 0, true));
	ApplyWidgetReflectorDebugVisibility();
}
