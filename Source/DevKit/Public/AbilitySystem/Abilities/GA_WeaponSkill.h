#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "GA_WeaponSkill.generated.h"

class UWeaponSkillDataAsset;

UCLASS(BlueprintType, Blueprintable, DisplayName = "武器战技 GA")
class DEVKIT_API UGA_WeaponSkill : public UGA_PlayMontage
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Called by the equipped-skill input path immediately before activation.
	 * Concrete skills can advance internal stages or reject an unavailable
	 * follow-up without granting a separate GA for every montage slot.
	 */
	virtual bool PrepareForInputActivation();

	/** Returns the DA that granted this equipped weapon-skill ability. */
	UFUNCTION(BlueprintPure, Category = "战技", meta = (DisplayName = "获取已装备战技数据"))
	const UWeaponSkillDataAsset* GetEquippedWeaponSkillData() const;

	/** Stable authoring identity used by the YogTool weapon manager. */
	UFUNCTION(BlueprintPure, Category = "战技|制作约束", meta = (DisplayName = "获取战技标签"))
	FGameplayTag GetWeaponSkillTag() const { return WeaponSkillTag; }

	UFUNCTION(BlueprintPure, Category = "战技|制作约束", meta = (DisplayName = "获取战技显示名称"))
	FText GetWeaponSkillDisplayName() const { return WeaponSkillDisplayName; }

	UFUNCTION(BlueprintPure, Category = "战技|制作约束", meta = (DisplayName = "获取战技简介"))
	FText GetWeaponSkillDescription() const { return WeaponSkillDescription; }

	const TArray<FGameplayTag>& GetRequiredMontageSlots() const { return RequiredMontageSlots; }
	TSoftClassPtr<UWeaponSkillDataAsset> GetWeaponSkillDataClass() const { return WeaponSkillDataClass; }

protected:
	void ConfigureWeaponSkillComboTag(const TCHAR* ComboTagName);

	/**
	 * Concrete native weapon-skill classes provide their editor identity here.
	 * Classes without a valid tag are implementation helpers and are omitted
	 * from the weapon manager's create/remove catalogue.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战技|制作约束", meta = (Categories = "Weapon.Skill", DisplayName = "战技标签"))
	FGameplayTag WeaponSkillTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战技|制作约束", meta = (DisplayName = "中文名称"))
	FText WeaponSkillDisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战技|制作约束", meta = (MultiLine = true, DisplayName = "中文简介"))
	FText WeaponSkillDescription;

	/** Montage keys that this GA can consume from its dedicated AbilityData. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战技|制作约束", meta = (DisplayName = "必需蒙太奇槽位"))
	TArray<FGameplayTag> RequiredMontageSlots;

	/**
	 * Dedicated C++ DA type created for this GA by the weapon manager.
	 * Skill-specific numeric fields belong on that subclass.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战技|制作约束", meta = (DisplayName = "专属战技 DA 类型"))
	TSoftClassPtr<UWeaponSkillDataAsset> WeaponSkillDataClass;
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo1 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo2 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo3 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo4 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
