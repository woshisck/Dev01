#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_WeaponSkill.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/DataAsset.h"
#include "WeaponSkillDataAsset.generated.h"

class UTexture2D;
class UWeaponSkillAbilityMontageData;

/**
 * 可装备战技定义。
 *
 * 每种战技对应一个独立 C++ GA 类；此 DA 负责选择实现类，并提供该战技独立的
 * 动作数据和显示信息。
 */
UCLASS(BlueprintType, Blueprintable, Const)
class DEVKIT_API UWeaponSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 战技的稳定身份标签；旧资产未填写时使用原生 GA 的默认标签兼容。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Skill|Identity", meta = (Categories = "Weapon.Skill", DisplayName = "战技标签"))
	FGameplayTag SkillTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Skill|Display", meta = (DisplayName = "中文名称"))
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Skill|Display", meta = (MultiLine = true, DisplayName = "中文简介"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Skill|Display", meta = (DisplayName = "战技图标"))
	TObjectPtr<UTexture2D> Icon;

	/** 此战技唯一对应的原生 Gameplay Ability 实现。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Skill|Runtime", meta = (DisplayName = "战技 GA 类"))
	TSubclassOf<UGA_WeaponSkill> AbilityClass;

	/** 仅在装备此战技时使用的蒙太奇和动作行。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Skill|Runtime", meta = (DisplayName = "战技动作数据"))
	TObjectPtr<UWeaponSkillAbilityMontageData> AbilityData;

	UFUNCTION(BlueprintPure, Category = "Weapon Skill", meta = (DisplayName = "获取有效战技标签"))
	FGameplayTag GetResolvedSkillTag() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("WeaponSkill")), GetFName());
	}
};

/** 双手剑多段连击战技的专属 C++ 数据类型。 */
UCLASS(BlueprintType)
class DEVKIT_API UWeaponSkill_THSwordComboDataAsset : public UWeaponSkillDataAsset
{
	GENERATED_BODY()
};

/** 格挡战技专属字段的数据类型。 */
UCLASS(BlueprintType)
class DEVKIT_API UWeaponSkill_BlockDataAsset : public UWeaponSkillDataAsset
{
	GENERATED_BODY()
};

/** 突刺战技专属字段的数据类型。 */
UCLASS(BlueprintType)
class DEVKIT_API UWeaponSkill_ThrustDataAsset : public UWeaponSkillDataAsset
{
	GENERATED_BODY()
};
