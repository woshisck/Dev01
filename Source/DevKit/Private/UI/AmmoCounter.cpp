#include "UI/AmmoCounter.h"

#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateBrush.h"

void UAmmoCounter::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	BindToASC();
}

void UAmmoCounter::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}

void UAmmoCounter::BindToASC()
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
		.AddUObject(this, &UAmmoCounter::OnCurrentAmmoChanged);

	MaxAmmoHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UPlayerAttributeSet::GetMaxAmmoAttribute())
		.AddUObject(this, &UAmmoCounter::OnMaxAmmoChanged);

	const FGameplayTag RangedWeaponTag = GetRangedWeaponTag();
	if (RangedWeaponTag.IsValid())
	{
		bHasRangedWeaponTag = BoundASC->HasMatchingGameplayTag(RangedWeaponTag);
		RangedWeaponTagHandle = BoundASC->RegisterGameplayTagEvent(
			RangedWeaponTag,
			EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UAmmoCounter::OnRangedWeaponTagChanged);
	}
	else
	{
		bHasRangedWeaponTag = false;
	}

	RebuildIcons(CachedMax);
	RefreshIconColors(CachedCurrent, CachedMax);
	RefreshVisibility();
}

void UAmmoCounter::UnbindFromASC()
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

void UAmmoCounter::OnCurrentAmmoChanged(const FOnAttributeChangeData& Data)
{
	CachedCurrent = FMath::RoundToInt(Data.NewValue);
	RefreshIconColors(CachedCurrent, CachedMax);
	RefreshVisibility();
}

void UAmmoCounter::OnMaxAmmoChanged(const FOnAttributeChangeData& Data)
{
	CachedMax = FMath::RoundToInt(Data.NewValue);
	RebuildIcons(CachedMax);
	RefreshIconColors(CachedCurrent, CachedMax);
	RefreshVisibility();
}

void UAmmoCounter::OnRangedWeaponTagChanged(const FGameplayTag /*CallbackTag*/, int32 NewCount)
{
	bHasRangedWeaponTag = NewCount > 0;
	RefreshVisibility();
}

void UAmmoCounter::RebuildIcons(int32 Max)
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

void UAmmoCounter::RefreshIconColors(int32 Current, int32 Max)
{
	for (int32 i = 0; i < BulletIcons.Num(); ++i)
	{
		if (BulletIcons[i])
		{
			BulletIcons[i]->SetColorAndOpacity(i < Current ? FilledColor : EmptyColor);
		}
	}
}

void UAmmoCounter::RefreshVisibility()
{
	const bool bHasAmmoCapacity = CachedMax > 0;
	const bool bShouldShow = bHasAmmoCapacity && (!bOnlyShowWithRangedWeaponTag || bHasRangedWeaponTag);
	SetVisibility(bShouldShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

FGameplayTag UAmmoCounter::GetRangedWeaponTag() const
{
	return FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
}
