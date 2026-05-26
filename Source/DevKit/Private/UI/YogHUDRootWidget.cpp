#include "UI/YogHUDRootWidget.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/Widget.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "UI/WeaponComboTextUtils.h"
#include "UI/YogCommonRichTextBlock.h"

void UYogHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshWeaponComboList(true);
}

void UYogHUDRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ComboListRefreshAccumulator += InDeltaTime;
	if (ComboListRefreshAccumulator >= 0.1f)
	{
		ComboListRefreshAccumulator = 0.f;
		RefreshWeaponComboList();
	}
}

void UYogHUDRootWidget::RefreshWeaponComboList(bool bForce)
{
	UWeaponDefinition* CurrentWeapon = nullptr;
	if (const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
	{
		CurrentWeapon = Player->EquippedWeaponDef.Get();
	}

	if (!bForce && CachedComboWeaponDefinition.Get() == CurrentWeapon)
	{
		return;
	}

	CachedComboWeaponDefinition = CurrentWeapon;

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

	WeaponComboListPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	WeaponComboListText->SetText(WeaponComboTextUtils::BuildComboHintText(CurrentWeapon, 0, true));
}
