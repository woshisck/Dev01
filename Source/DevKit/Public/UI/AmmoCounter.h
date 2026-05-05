#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "AmmoCounter.generated.h"

class UAbilitySystemComponent;
class UHorizontalBox;
class UImage;

/**
 * Musket ammo HUD.
 *
 * Kept for older WBP parents. Newer assets should prefer UWBP_AmmoCounter,
 * but both classes use the same Weapon.Type.Ranged visibility gate.
 */
UCLASS()
class DEVKIT_API UAmmoCounter : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> BulletIconBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
	FVector2D IconSize = FVector2D(22.f, 22.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
	float IconPadding = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
	FLinearColor FilledColor = FLinearColor(1.f, 0.8f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
	FLinearColor EmptyColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
	bool bOnlyShowWithRangedWeaponTag = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BindToASC();
	void UnbindFromASC();

	void OnCurrentAmmoChanged(const FOnAttributeChangeData& Data);
	void OnMaxAmmoChanged(const FOnAttributeChangeData& Data);
	void OnRangedWeaponTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void RebuildIcons(int32 Max);
	void RefreshIconColors(int32 Current, int32 Max);
	void RefreshVisibility();
	FGameplayTag GetRangedWeaponTag() const;

	FDelegateHandle CurrentAmmoHandle;
	FDelegateHandle MaxAmmoHandle;
	FDelegateHandle RangedWeaponTagHandle;

	int32 CachedMax = 0;
	int32 CachedCurrent = 0;
	bool bHasRangedWeaponTag = false;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	UPROPERTY()
	TArray<TObjectPtr<UImage>> BulletIcons;
};
