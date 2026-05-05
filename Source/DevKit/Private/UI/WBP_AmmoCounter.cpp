#include "UI/WBP_AmmoCounter.h"

#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateBrush.h"

void UWBP_AmmoCounter::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	BindToASC();
}

void UWBP_AmmoCounter::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}

void UWBP_AmmoCounter::BindToASC()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		RefreshVisibility();
		return;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		RefreshVisibility();
		return;
	}

	BoundASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!BoundASC)
	{
		RefreshVisibility();
		return;
	}

	bool bFoundCurrent = false;
	bool bFoundMax = false;
	CachedCurrent = FMath::RoundToInt(
		BoundASC->GetGameplayAttributeValue(UPlayerAttributeSet::GetCurrentAmmoAttribute(), bFoundCurrent));
	CachedMax = FMath::RoundToInt(
		BoundASC->GetGameplayAttributeValue(UPlayerAttributeSet::GetMaxAmmoAttribute(), bFoundMax));

	CurrentAmmoHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UPlayerAttributeSet::GetCurrentAmmoAttribute())
		.AddUObject(this, &UWBP_AmmoCounter::OnCurrentAmmoChanged);

	MaxAmmoHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UPlayerAttributeSet::GetMaxAmmoAttribute())
		.AddUObject(this, &UWBP_AmmoCounter::OnMaxAmmoChanged);

	const FGameplayTag RangedWeaponTag = GetRangedWeaponTag();
	if (RangedWeaponTag.IsValid())
	{
		bHasRangedWeaponTag = BoundASC->HasMatchingGameplayTag(RangedWeaponTag);
		RangedWeaponTagHandle = BoundASC->RegisterGameplayTagEvent(
			RangedWeaponTag,
			EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UWBP_AmmoCounter::OnRangedWeaponTagChanged);
	}
	else
	{
		bHasRangedWeaponTag = false;
	}

	RebuildIcons(CachedMax);
	RefreshIconColors(CachedCurrent, CachedMax);
	RefreshVisibility();
}

void UWBP_AmmoCounter::UnbindFromASC()
{
	if (!BoundASC)
	{
		return;
	}

	if (CurrentAmmoHandle.IsValid())
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(
			UPlayerAttributeSet::GetCurrentAmmoAttribute()).Remove(CurrentAmmoHandle);
		CurrentAmmoHandle.Reset();
	}

	if (MaxAmmoHandle.IsValid())
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(
			UPlayerAttributeSet::GetMaxAmmoAttribute()).Remove(MaxAmmoHandle);
		MaxAmmoHandle.Reset();
	}

	const FGameplayTag RangedWeaponTag = GetRangedWeaponTag();
	if (RangedWeaponTagHandle.IsValid() && RangedWeaponTag.IsValid())
	{
		BoundASC->RegisterGameplayTagEvent(
			RangedWeaponTag,
			EGameplayTagEventType::NewOrRemoved).Remove(RangedWeaponTagHandle);
		RangedWeaponTagHandle.Reset();
	}

	BoundASC = nullptr;
	bHasRangedWeaponTag = false;
}

void UWBP_AmmoCounter::OnCurrentAmmoChanged(const FOnAttributeChangeData& Data)
{
	CachedCurrent = FMath::RoundToInt(Data.NewValue);
	RefreshIconColors(CachedCurrent, CachedMax);
	RefreshVisibility();
}

void UWBP_AmmoCounter::OnMaxAmmoChanged(const FOnAttributeChangeData& Data)
{
	CachedMax = FMath::RoundToInt(Data.NewValue);
	RebuildIcons(CachedMax);
	RefreshIconColors(CachedCurrent, CachedMax);
	RefreshVisibility();
}

void UWBP_AmmoCounter::OnRangedWeaponTagChanged(const FGameplayTag /*CallbackTag*/, int32 NewCount)
{
	bHasRangedWeaponTag = NewCount > 0;
	RefreshVisibility();
}

void UWBP_AmmoCounter::RebuildIcons(int32 Max)
{
	if (!BulletIconBox)
	{
		return;
	}

	BulletIconBox->ClearChildren();
	BulletIcons.Reset();

	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	Brush.OutlineSettings.CornerRadii = FVector4(3.f, 3.f, 3.f, 3.f);

	for (int32 i = 0; i < Max; ++i)
	{
		UImage* Icon = NewObject<UImage>(this);
		Icon->SetBrush(Brush);
		Icon->SetColorAndOpacity(FilledColor);
		Icon->SetDesiredSizeOverride(IconSize);

		if (UHorizontalBoxSlot* IconSlot = BulletIconBox->AddChildToHorizontalBox(Icon))
		{
			IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			IconSlot->SetPadding(FMargin(i == 0 ? 0.f : IconPadding, 0.f, 0.f, 0.f));
		}

		BulletIcons.Add(Icon);
	}
}

void UWBP_AmmoCounter::RefreshIconColors(int32 Current, int32 Max)
{
	for (int32 i = 0; i < BulletIcons.Num(); ++i)
	{
		if (BulletIcons[i])
		{
			BulletIcons[i]->SetColorAndOpacity(i < Current ? FilledColor : EmptyColor);
		}
	}
}

void UWBP_AmmoCounter::RefreshVisibility()
{
	const bool bHasAmmoCapacity = CachedMax > 0;
	const bool bShouldShow = bHasAmmoCapacity && (!bOnlyShowWithRangedWeaponTag || bHasRangedWeaponTag);
	SetVisibility(bShouldShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

FGameplayTag UWBP_AmmoCounter::GetRangedWeaponTag() const
{
	return FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
}
