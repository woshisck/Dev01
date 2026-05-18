#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "MetaTypes.generated.h"

// ============================================================
//  局外成长系统 — 公共类型定义
//  DataTable 行类型、枚举、货币成本结构体均在此定义。
//  修改货币显示名称：只需修改 DT_MetaCurrencyRules 中对应行的
//  DisplayName 字段，无需改代码。
// ============================================================

// ── 成长树所属侧 ─────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMetaSide : uint8
{
	Mystic  UMETA(DisplayName = "神秘侧"),
	Flesh   UMETA(DisplayName = "血肉侧"),
};

// ── 升级节点效果类型 ──────────────────────────────────────────
UENUM(BlueprintType)
enum class EMetaUpgradeEffectType : uint8
{
	StatBoost     UMETA(DisplayName = "数值加成"),
	FeatureUnlock UMETA(DisplayName = "功能解锁"),
	SlotUnlock    UMETA(DisplayName = "槽位解锁"),
};

// ── 单种货币花费（Tag 驱动，新增货币不改此结构体）──────────────
USTRUCT(BlueprintType)
struct DEVKIT_API FMetaCurrencyCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MetaProgression")
	FGameplayTag CurrencyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MetaProgression", meta = (ClampMin = "0"))
	int32 Amount = 0;
};

// ============================================================
//  DT_MetaUpgradeNodes 行类型
//  RowName = 节点唯一标识（建议格式：MetaNode.Flesh.Attack.Basic）
// ============================================================
USTRUCT(BlueprintType)
struct DEVKIT_API FMetaUpgradeNodeRow : public FTableRowBase
{
	GENERATED_BODY()

	// ── 基础信息 ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node")
	EMetaSide Side = EMetaSide::Flesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node", meta = (ClampMin = "1"))
	int32 MaxLevel = 1;

	// ── 解锁条件 ──────────────────────────────────────────────
	// 神秘侧等级门槛（血肉侧节点填 0）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock", meta = (ClampMin = "0"))
	int32 MysticLevelRequired = 0;

	// 前置节点 RowName 列表（所有前置均满级才可购买此节点）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
	TArray<FName> Prerequisites;

	// ── 花费（每级相同花费，Tag 驱动可扩展）─────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	TArray<FMetaCurrencyCost> CostsPerLevel;

	// ── 效果 ──────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	EMetaUpgradeEffectType EffectType = EMetaUpgradeEffectType::StatBoost;

	// 数值类：修改哪个 GAS Attribute，每级加多少
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect",
		meta = (EditCondition = "EffectType == EMetaUpgradeEffectType::StatBoost", EditConditionHides))
	FGameplayAttribute StatAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect",
		meta = (EditCondition = "EffectType == EMetaUpgradeEffectType::StatBoost", EditConditionHides))
	float StatValuePerLevel = 0.f;

	// 功能解锁类：解锁哪个功能 Tag（故事引擎 / 教程系统查询）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect",
		meta = (EditCondition = "EffectType == EMetaUpgradeEffectType::FeatureUnlock", EditConditionHides))
	FGameplayTag FeatureTag;
};

// ============================================================
//  DT_MetaCurrencyRules 行类型
//  RowName = 货币 Tag 叶节点名（如 Common.A）
//  改名只需改 DisplayName，不动代码
// ============================================================
USTRUCT(BlueprintType)
struct DEVKIT_API FMetaCurrencyRow : public FTableRowBase
{
	GENERATED_BODY()

	// 货币的 GameplayTag（与 FMetaCurrencyCost::CurrencyTag 对应）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency")
	FGameplayTag CurrencyTag;

	// ← 改名只改这里
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency")
	FText DisplayName;

	// UI 上显示的简称（如"氧"）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency")
	FText ShortName;

	// 持有上限（0 = 无上限）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency", meta = (ClampMin = "0"))
	int32 MaxCapacity = 0;
};
