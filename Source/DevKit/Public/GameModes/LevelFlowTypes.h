// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/RuneDataAsset.h"
#include "GameplayTagContainer.h"
#include "LevelFlowTypes.generated.h"

class UTexture2D;
class UWeaponSkillDataAsset;

/** 战利品类型（预留扩展） */
UENUM(BlueprintType)
enum class ELootType : uint8
{
	Rune			UMETA(DisplayName = "Rune"),
	Gold			UMETA(DisplayName = "Gold"),
	Material		UMETA(DisplayName = "Material"),

	// Only supported on the immediate-grant path
	// (AEnemyCharacterBase::SpawnKillRewardPickups -> ARewardPickup::GrantImmediateLoot).
	// The rune card widget cannot show it.
	WeaponSkill		UMETA(DisplayName = "WeaponSkill"),
};

/** 单个战利品选项，由 GameMode 生成后广播给 UI */
USTRUCT(BlueprintType)
struct DEVKIT_API FLootOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ELootType LootType = ELootType::Rune;

	// LootType == Rune 时有效
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<URuneDataAsset> RuneAsset = nullptr;

	// LootType == WeaponSkill 时有效
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UWeaponSkillDataAsset> WeaponSkillAsset = nullptr;

	// LootType == WeaponSkill 时有效：可用次数，<=0 表示永久
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 WeaponSkillCharges = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FGameplayTag MetaCurrencyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

/** One button in the reward pickup's "choose your reward type" popup. */
USTRUCT(BlueprintType)
struct DEVKIT_API FLootTypeChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ELootType LootType = ELootType::Rune;

	// Gold / Material amount. Ignored for Rune and WeaponSkill.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "LootType == ELootType::Gold || LootType == ELootType::Material", EditConditionHides))
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "LootType == ELootType::Material", EditConditionHides))
	FGameplayTag MetaCurrencyTag;

	// Only used when LootType == WeaponSkill.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "LootType == ELootType::WeaponSkill", EditConditionHides))
	TObjectPtr<UWeaponSkillDataAsset> WeaponSkillAsset = nullptr;

	// Only used when LootType == WeaponSkill. <= 0 means permanent.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "LootType == ELootType::WeaponSkill", EditConditionHides))
	int32 WeaponSkillCharges = INDEX_NONE;

	// Button label. Empty falls back to a default derived from LootType and Amount.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

