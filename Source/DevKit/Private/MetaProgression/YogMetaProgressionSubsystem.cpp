#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "MetaProgression/MetaProgressionSettings.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "Engine/DataTable.h"

namespace
{
FGameplayTag GetMysticPointTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("Currency.Meta.MysticPoint"), false);
}
}

// =========================================================
// 内部辅助
// =========================================================

void UYogMetaProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UMetaProgressionSettings* Settings = GetDefault<UMetaProgressionSettings>();
	MetaUpgradeNodeTable = Settings->MetaUpgradeNodeTable.LoadSynchronous();
	MetaCurrencyRuleTable = Settings->MetaCurrencyRuleTable.LoadSynchronous();

	if (!MetaUpgradeNodeTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaProgression] MetaUpgradeNodeTable is not configured or failed to load."));
	}

	if (!MetaCurrencyRuleTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaProgression] MetaCurrencyRuleTable is not configured or failed to load."));
	}
}

UYogSaveSubsystem* UYogMetaProgressionSubsystem::GetSaveSys() const
{
	return GetGameInstance()->GetSubsystem<UYogSaveSubsystem>();
}

FMetaProgressionData& UYogMetaProgressionSubsystem::GetMeta()
{
	return GetSaveSys()->GetCurrentSave()->MetaProgression;
}

const FMetaProgressionData& UYogMetaProgressionSubsystem::GetMeta() const
{
	return GetSaveSys()->GetCurrentSave()->MetaProgression;
}

UDataTable* UYogMetaProgressionSubsystem::GetUpgradeTable() const
{
	return MetaUpgradeNodeTable;
}

UDataTable* UYogMetaProgressionSubsystem::GetCurrencyTable() const
{
	return MetaCurrencyRuleTable;
}

const FMetaUpgradeNodeRow* UYogMetaProgressionSubsystem::GetNodeRow(FName NodeRowName) const
{
	UDataTable* Table = GetUpgradeTable();
	if (!Table) return nullptr;
	return Table->FindRow<FMetaUpgradeNodeRow>(NodeRowName, TEXT("GetNodeRow"), false);
}

const FMetaCurrencyRow* UYogMetaProgressionSubsystem::GetCurrencyRow(FGameplayTag CurrencyTag) const
{
	UDataTable* Table = GetCurrencyTable();
	if (!Table) return nullptr;

	// 遍历所有行，按 CurrencyTag 字段查找（RowName 可为任意字符串）
	for (auto& Pair : Table->GetRowMap())
	{
		const FMetaCurrencyRow* Row = reinterpret_cast<const FMetaCurrencyRow*>(Pair.Value);
		if (Row && Row->CurrencyTag == CurrencyTag)
		{
			return Row;
		}
	}
	return nullptr;
}

void UYogMetaProgressionSubsystem::CommitSave()
{
	if (UYogSaveSubsystem* SaveSys = GetSaveSys())
	{
		SaveSys->DoAsyncSave();
	}
}

// =========================================================
// 货币操作
// =========================================================

int32 UYogMetaProgressionSubsystem::GetCurrencyAmount(FGameplayTag CurrencyTag) const
{
	const FMetaProgressionData& Meta = GetMeta();
	const int32* Found = Meta.MetaCurrencies.Find(CurrencyTag);
	return Found ? *Found : 0;
}

void UYogMetaProgressionSubsystem::AddCurrency(FGameplayTag CurrencyTag, int32 Amount)
{
	if (Amount == 0) return;
	if (GetCurrencyTable() && !GetCurrencyRow(CurrencyTag))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaProgression] Ignored unknown currency tag: %s"), *CurrencyTag.ToString());
		return;
	}

	FMetaProgressionData& Meta = GetMeta();
	int32& Current = Meta.MetaCurrencies.FindOrAdd(CurrencyTag, 0);
	Current = FMath::Max(0, Current + Amount);

	// 上限裁剪
	if (const FMetaCurrencyRow* Row = GetCurrencyRow(CurrencyTag))
	{
		if (Row->MaxCapacity > 0)
		{
			Current = FMath::Min(Current, Row->MaxCapacity);
		}
	}

	// 正数入账时累计到本局毛收入（不受花费影响）
	if (Amount > 0)
	{
		RunCurrencyGained.FindOrAdd(CurrencyTag, 0) += Amount;
	}

	OnCurrencyChanged.Broadcast(CurrencyTag, Current);
	CommitSave();
}

bool UYogMetaProgressionSubsystem::SpendCurrency(FGameplayTag CurrencyTag, int32 Amount)
{
	if (Amount <= 0) return true;
	if (GetCurrencyTable() && !GetCurrencyRow(CurrencyTag))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaProgression] Ignored spend for unknown currency tag: %s"), *CurrencyTag.ToString());
		return false;
	}

	FMetaProgressionData& Meta = GetMeta();
	int32& Current = Meta.MetaCurrencies.FindOrAdd(CurrencyTag, 0);
	if (Current < Amount)
	{
		return false;
	}

	Current -= Amount;
	OnCurrencyChanged.Broadcast(CurrencyTag, Current);
	CommitSave();
	return true;
}

// =========================================================
// 升级节点
// =========================================================

int32 UYogMetaProgressionSubsystem::GetNodeLevel(FName NodeRowName) const
{
	const int32* Found = GetMeta().NodeLevels.Find(NodeRowName);
	return Found ? *Found : 0;
}

bool UYogMetaProgressionSubsystem::CanPurchaseNode(FName NodeRowName) const
{
	const FMetaUpgradeNodeRow* Row = GetNodeRow(NodeRowName);
	if (!Row) return false;

	const FMetaProgressionData& Meta = GetMeta();

	// 等级上限
	const int32 CurrentLevel = GetNodeLevel(NodeRowName);
	if (CurrentLevel >= Row->MaxLevel) return false;

	// 神秘侧等级门槛
	if (Row->MysticLevelRequired > 0 && Meta.MysticSideLevel < Row->MysticLevelRequired)
	{
		return false;
	}

	// 前置节点：所有前置均需满级
	for (const FName& Prereq : Row->Prerequisites)
	{
		const FMetaUpgradeNodeRow* PrereqRow = GetNodeRow(Prereq);
		const int32 Required = PrereqRow ? PrereqRow->MaxLevel : 1;
		if (GetNodeLevel(Prereq) < Required) return false;
	}

	// 货币检查
	for (const FMetaCurrencyCost& Cost : Row->CostsPerLevel)
	{
		if (Cost.Amount < 0) return false;
		if (GetCurrencyTable() && !GetCurrencyRow(Cost.CurrencyTag)) return false;
		if (GetCurrencyAmount(Cost.CurrencyTag) < Cost.Amount) return false;
	}

	return true;
}

bool UYogMetaProgressionSubsystem::TryPurchaseNode(FName NodeRowName)
{
	if (!CanPurchaseNode(NodeRowName)) return false;

	const FMetaUpgradeNodeRow* Row = GetNodeRow(NodeRowName);

	// 扣除货币（不触发单独存档，购买结束后统一 CommitSave）
	FMetaProgressionData& Meta = GetMeta();
	for (const FMetaCurrencyCost& Cost : Row->CostsPerLevel)
	{
		if (Cost.Amount <= 0)
		{
			continue;
		}
		int32& Current = Meta.MetaCurrencies.FindOrAdd(Cost.CurrencyTag, 0);
		Current -= Cost.Amount;
		OnCurrencyChanged.Broadcast(Cost.CurrencyTag, Current);
	}

	// 升级
	int32& Level = Meta.NodeLevels.FindOrAdd(NodeRowName, 0);
	++Level;

	// 神秘侧节点：购买一级即提升神秘等级
	if (Row->Side == EMetaSide::Mystic)
	{
		++Meta.MysticSideLevel;
	}

	// 功能解锁
	if (Row->EffectType == EMetaUpgradeEffectType::FeatureUnlock && Row->FeatureTag.IsValid())
	{
		if (!Meta.UnlockedFeatures.Contains(Row->FeatureTag))
		{
			Meta.UnlockedFeatures.Add(Row->FeatureTag);
			OnFeatureUnlocked.Broadcast(Row->FeatureTag);
		}
	}

	// 起始符文授予（AddUnique 防重复；多级节点每级各授予一次相同 DA 即叠加）
	if (Row->EffectType == EMetaUpgradeEffectType::RuneGrant && Row->StarterRuneToGrant.IsValid())
	{
		Meta.CraftedStarterRunes.AddUnique(Row->StarterRuneToGrant);
	}

	OnNodePurchased.Broadcast(NodeRowName);
	CommitSave();
	return true;
}

// =========================================================
// 功能解锁
// =========================================================

bool UYogMetaProgressionSubsystem::IsFeatureUnlocked(FGameplayTag FeatureTag) const
{
	return GetMeta().UnlockedFeatures.Contains(FeatureTag);
}

void UYogMetaProgressionSubsystem::UnlockFeature(FGameplayTag FeatureTag)
{
	if (!FeatureTag.IsValid()) return;

	FMetaProgressionData& Meta = GetMeta();
	if (Meta.UnlockedFeatures.Contains(FeatureTag)) return;

	Meta.UnlockedFeatures.Add(FeatureTag);
	OnFeatureUnlocked.Broadcast(FeatureTag);
	CommitSave();
}

// =========================================================
// 神秘侧等级
// =========================================================

int32 UYogMetaProgressionSubsystem::GetMysticSideLevel() const
{
	return GetMeta().MysticSideLevel;
}

int32 UYogMetaProgressionSubsystem::GetAvailableMysticPoints() const
{
	const FGameplayTag MysticPointTag = GetMysticPointTag();
	return MysticPointTag.IsValid() ? GetCurrencyAmount(MysticPointTag) : 0;
}

void UYogMetaProgressionSubsystem::AddMysticPoints(int32 Amount)
{
	const FGameplayTag MysticPointTag = GetMysticPointTag();
	if (!MysticPointTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaProgression] Currency.Meta.MysticPoint tag is missing."));
		return;
	}
	AddCurrency(MysticPointTag, Amount);
}

// =========================================================
// 打造存档
// =========================================================

void UYogMetaProgressionSubsystem::AddCraftedStarterRune(const FPrimaryAssetId& AssetId)
{
	if (!AssetId.IsValid()) return;
	FMetaProgressionData& Meta = GetMeta();
	Meta.CraftedStarterRunes.AddUnique(AssetId);
	CommitSave();
}

void UYogMetaProgressionSubsystem::AddCraftedWeaponFinisher(const FPrimaryAssetId& AssetId)
{
	if (!AssetId.IsValid()) return;
	FMetaProgressionData& Meta = GetMeta();
	Meta.CraftedWeaponFinisherCards.AddUnique(AssetId);
	CommitSave();
}

const TArray<FPrimaryAssetId>& UYogMetaProgressionSubsystem::GetCraftedStarterRunes() const
{
	return GetMeta().CraftedStarterRunes;
}

const TArray<FPrimaryAssetId>& UYogMetaProgressionSubsystem::GetCraftedWeaponFinishers() const
{
	return GetMeta().CraftedWeaponFinisherCards;
}

// =========================================================
// 跑局货币累计
// =========================================================

void UYogMetaProgressionSubsystem::ClearRunCurrencyAccumulator()
{
	RunCurrencyGained.Reset();
}

void UYogMetaProgressionSubsystem::GetAllNodeNames(TArray<FName>& OutNames) const
{
	UDataTable* Table = GetUpgradeTable();
	if (!Table) return;
	OutNames = Table->GetRowNames();
}

void UYogMetaProgressionSubsystem::BroadcastRunEnded(int32 FloorReached, int32 EnemiesKilled)
{
	FRunSummaryData Summary;
	Summary.FloorReached    = FloorReached;
	Summary.EnemiesKilled   = EnemiesKilled;
	Summary.MetaCurrencyGained = RunCurrencyGained;

	// 收集当前可购买的节点名（结算页"下一步"提示用）
	TArray<FName> AllNodes;
	GetAllNodeNames(AllNodes);
	for (const FName& NodeName : AllNodes)
	{
		if (CanPurchaseNode(NodeName))
		{
			Summary.PurchasableNodeNames.Add(NodeName);
		}
	}

	OnRunEnded.Broadcast(Summary);
}
