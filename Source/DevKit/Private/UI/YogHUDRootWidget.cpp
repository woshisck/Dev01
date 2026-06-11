#include "UI/YogHUDRootWidget.h"

#include "Components/Widget.h"
#include "UI/PlayerBuffBarWidget.h"

void UYogHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayerBuffBar)
	{
		PlayerBuffBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	RefreshWeaponComboList(true);
}

void UYogHUDRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UYogHUDRootWidget::RefreshWeaponComboList(bool bForce)
{
	if (WeaponComboListPanel)
	{
		WeaponComboListPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}
