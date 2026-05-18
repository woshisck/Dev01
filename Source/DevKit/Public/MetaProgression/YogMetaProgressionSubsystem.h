#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MetaProgression/MetaTypes.h"
#include "SaveGame/YogSaveGame.h"
#include "YogMetaProgressionSubsystem.generated.h"

class UYogSaveSubsystem;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMetaCurrencyChanged, FGameplayTag, CurrencyTag, int32, NewAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaNodePurchased, FName, NodeRowName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaFeatureUnlocked, FGameplayTag, FeatureTag);

// ============================================================
//  UYogMetaProgressionSubsystem
//
//  管理局外成长数据的读写：货币、升级节点、功能解锁。
//  数据持久化委托给 UYogSaveSubsystem；本系统只做业务逻辑。
//
//  配置方式：在 BP 子类中将 MetaUpgradeNodeTable / MetaCurrencyRuleTable
//  赋值为对应 DataTable 资产。
// ============================================================
UCLASS()
class DEVKIT_API UYogMetaProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ── DataTable 资产引用（在 GameInstance BP Init 事件中赋值）──────
	UPROPERTY(BlueprintReadWrite, Category = "MetaProgression|Config")
	TObjectPtr<UDataTable> MetaUpgradeNodeTable;

	UPROPERTY(BlueprintReadWrite, Category = "MetaProgression|Config")
	TObjectPtr<UDataTable> MetaCurrencyRuleTable;

	// =========================================================
	// 货币操作
	// =========================================================

	UFUNCTION(BlueprintPure, Category = "MetaProgression|Currency")
	int32 GetCurrencyAmount(FGameplayTag CurrencyTag) const;

	// Amount 可为负（当作扣除），但结果 ≥ 0 且 ≤ MaxCapacity
	UFUNCTION(BlueprintCallable, Category = "MetaProgression|Currency")
	void AddCurrency(FGameplayTag CurrencyTag, int32 Amount);

	// 返回 false 表示余额不足，不扣除
	UFUNCTION(BlueprintCallable, Category = "MetaProgression|Currency")
	bool SpendCurrency(FGameplayTag CurrencyTag, int32 Amount);

	// =========================================================
	// 升级节点
	// =========================================================

	UFUNCTION(BlueprintPure, Category = "MetaProgression|Node")
	int32 GetNodeLevel(FName NodeRowName) const;

	// 检查：前置已满 + 神秘等级满足 + 货币充足 + 未达 MaxLevel
	UFUNCTION(BlueprintPure, Category = "MetaProgression|Node")
	bool CanPurchaseNode(FName NodeRowName) const;

	// 购买一级；成功返回 true 并触发存档
	UFUNCTION(BlueprintCallable, Category = "MetaProgression|Node")
	bool TryPurchaseNode(FName NodeRowName);

	// =========================================================
	// 功能解锁（故事引擎 / 教程系统查询）
	// =========================================================

	UFUNCTION(BlueprintPure, Category = "MetaProgression|Feature")
	bool IsFeatureUnlocked(FGameplayTag FeatureTag) const;

	// 直接解锁（剧情触发，不花费货币）
	UFUNCTION(BlueprintCallable, Category = "MetaProgression|Feature")
	void UnlockFeature(FGameplayTag FeatureTag);

	// =========================================================
	// 神秘侧等级
	// =========================================================

	UFUNCTION(BlueprintPure, Category = "MetaProgression|Mystic")
	int32 GetMysticSideLevel() const;

	UFUNCTION(BlueprintPure, Category = "MetaProgression|Mystic")
	int32 GetAvailableMysticPoints() const;

	UFUNCTION(BlueprintCallable, Category = "MetaProgression|Mystic")
	void AddMysticPoints(int32 Amount);

	// =========================================================
	// 打造存档（局内 BeginPlay 时 AsyncLoad 后 Grant）
	// =========================================================

	UFUNCTION(BlueprintCallable, Category = "MetaProgression|Crafting")
	void AddCraftedStarterRune(const FPrimaryAssetId& AssetId);

	UFUNCTION(BlueprintCallable, Category = "MetaProgression|Crafting")
	void AddCraftedWeaponFinisher(const FPrimaryAssetId& AssetId);

	UFUNCTION(BlueprintPure, Category = "MetaProgression|Crafting")
	const TArray<FPrimaryAssetId>& GetCraftedStarterRunes() const;

	UFUNCTION(BlueprintPure, Category = "MetaProgression|Crafting")
	const TArray<FPrimaryAssetId>& GetCraftedWeaponFinishers() const;

	// =========================================================
	// 事件广播
	// =========================================================

	UPROPERTY(BlueprintAssignable)
	FOnMetaCurrencyChanged OnCurrencyChanged;

	UPROPERTY(BlueprintAssignable)
	FOnMetaNodePurchased OnNodePurchased;

	UPROPERTY(BlueprintAssignable)
	FOnMetaFeatureUnlocked OnFeatureUnlocked;

private:

	const FMetaUpgradeNodeRow* GetNodeRow(FName NodeRowName) const;
	const FMetaCurrencyRow*    GetCurrencyRow(FGameplayTag CurrencyTag) const;

	UDataTable* GetUpgradeTable() const;
	UDataTable* GetCurrencyTable() const;

	FMetaProgressionData&       GetMeta();
	const FMetaProgressionData& GetMeta() const;

	UYogSaveSubsystem* GetSaveSys() const;
	void               CommitSave();
};
