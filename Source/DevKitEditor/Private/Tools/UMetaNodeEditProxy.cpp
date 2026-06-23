#include "Tools/UMetaNodeEditProxy.h"

#include "Engine/DataTable.h"
#include "SaveGame/YogSaveGame.h"
#include "ScopedTransaction.h"

void UMetaNodeEditProxy::LoadFromRow(UDataTable* Table, FName RowName)
{
	SourceTable = Table;
	SourceRowName = RowName;

	if (Table)
	{
		if (const FMetaUpgradeNodeRow* Row = Table->FindRow<FMetaUpgradeNodeRow>(RowName, TEXT("LoadFromRow"), false))
		{
			Data = *Row;
		}
	}
}

#if WITH_EDITOR
void UMetaNodeEditProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!SourceTable || !SourceRowName.IsValid()) return;

	FScopedTransaction Transaction(NSLOCTEXT("MetaProgressionWorkbench", "EditNode", "Edit Meta Node"));
	SourceTable->Modify();
	SourceTable->AddRow(SourceRowName, Data);
	SourceTable->MarkPackageDirty();
}
#endif

void UMetaCurrencyEditProxy::LoadFromRow(UDataTable* Table, FName RowName)
{
	SourceTable = Table;
	SourceRowName = RowName;

	if (Table)
	{
		if (const FMetaCurrencyRow* Row = Table->FindRow<FMetaCurrencyRow>(RowName, TEXT("LoadCurrencyFromRow"), false))
		{
			Data = *Row;
		}
	}
}

#if WITH_EDITOR
void UMetaCurrencyEditProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!SourceTable || !SourceRowName.IsValid()) return;

	FScopedTransaction Transaction(NSLOCTEXT("MetaProgressionWorkbench", "EditCurrency", "Edit Meta Currency"));
	SourceTable->Modify();
	SourceTable->AddRow(SourceRowName, Data);
	SourceTable->MarkPackageDirty();
}
#endif

void UYogSaveSlotEditProxy::LoadFromSave(UYogSaveGame* Save, int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	bHasData = Save != nullptr;

	if (!Save)
	{
		bHasPendingRun = false;
		CheckpointFloor = 0;
		CurrentHP = 0.f;
		CurrentGold = 0;
		CurrentPhase = 0;
		CurrentHeat = 0.f;
		CompletedCombatBattleCount = 0;
		CombatDeckShuffleCooldownDuration = 0.f;
		CombatDeckMaxActiveSequenceSize = 0;
		CombatDeckCards.Reset();
		CombatDeckCardOrientations.Reset();
		EquippedWeaponDef.Reset();
		ActiveSacrificeGrace.Reset();
		PlacedRunes.Reset();
		PendingRunes.Reset();
		HiddenPassiveRuneInstances.Reset();
		SacrificeOfferingCosts.Reset();
		PlayerStateData = FPlayerGASData{};
		WeaponInstanceItems.Reset();
		MapStateData = FYogMapStateData{};
		SavedCharacter.Reset();
		TutorialState = ETutorialState::NeedWeaponTutorial;
		ShownPopupKeys.Reset();
		TotalRuns = 0;
		TotalKills = 0;
		HighestFloor = 0;
		TotalPlayTimeSeconds = 0;
		TotalDeaths = 0;
		TotalGoldEarned = 0;
		MetaCurrencies.Reset();
		NodeLevels.Reset();
		UnlockedFeatures.Reset();
		MysticSideLevel = 0;
		CraftedStarterRunes.Reset();
		CraftedWeaponFinisherCards.Reset();
		return;
	}

	bHasPendingRun = Save->RunCheckpoint.bIsValid;
	CheckpointFloor = Save->RunCheckpoint.CheckpointFloor;
	CurrentHP = Save->RunCheckpoint.CurrentHP;
	CurrentGold = Save->RunCheckpoint.CurrentGold;
	CurrentPhase = Save->RunCheckpoint.CurrentPhase;
	CurrentHeat = Save->RunCheckpoint.CurrentHeat;
	CompletedCombatBattleCount = Save->RunCheckpoint.CompletedCombatBattleCount;
	CombatDeckShuffleCooldownDuration = Save->RunCheckpoint.CombatDeckShuffleCooldownDuration;
	CombatDeckMaxActiveSequenceSize = Save->RunCheckpoint.CombatDeckMaxActiveSequenceSize;
	CombatDeckCards = Save->RunCheckpoint.CombatDeckCards;
	CombatDeckCardOrientations = Save->RunCheckpoint.CombatDeckCardOrientations;
	EquippedWeaponDef = Save->RunCheckpoint.EquippedWeaponDef;
	ActiveSacrificeGrace = Save->RunCheckpoint.ActiveSacrificeGrace;
	PlacedRunes = Save->RunCheckpoint.PlacedRunes;
	PendingRunes = Save->RunCheckpoint.PendingRunes;
	HiddenPassiveRuneInstances = Save->RunCheckpoint.HiddenPassiveRuneInstances;
	SacrificeOfferingCosts = Save->RunCheckpoint.SacrificeOfferingCosts;

	PlayerStateData = Save->PlayerStateData;
	WeaponInstanceItems = Save->WeaponInstanceItems;
	MapStateData = Save->MapStateData;
	SavedCharacter = Save->SavedCharacter;
	TutorialState = Save->TutorialState;
	ShownPopupKeys = Save->ShownPopupKeys;

	TotalRuns = Save->Statistics.TotalRuns;
	TotalKills = Save->Statistics.TotalKills;
	HighestFloor = Save->Statistics.HighestFloor;
	TotalPlayTimeSeconds = Save->Statistics.TotalPlayTimeSeconds;
	TotalDeaths = Save->Statistics.TotalDeaths;
	TotalGoldEarned = Save->Statistics.TotalGoldEarned;

	MetaCurrencies = Save->MetaProgression.MetaCurrencies;
	NodeLevels = Save->MetaProgression.NodeLevels;
	UnlockedFeatures = Save->MetaProgression.UnlockedFeatures;
	MysticSideLevel = Save->MetaProgression.MysticSideLevel;
	CraftedStarterRunes = Save->MetaProgression.CraftedStarterRunes;
	CraftedWeaponFinisherCards = Save->MetaProgression.CraftedWeaponFinisherCards;
}

void UYogSaveSlotEditProxy::ApplyToSave(UYogSaveGame* Save) const
{
	if (!Save) return;

	Save->RunCheckpoint.bIsValid = bHasPendingRun;
	Save->RunCheckpoint.CheckpointFloor = CheckpointFloor;
	Save->RunCheckpoint.CurrentHP = CurrentHP;
	Save->RunCheckpoint.CurrentGold = CurrentGold;
	Save->RunCheckpoint.CurrentPhase = CurrentPhase;
	Save->RunCheckpoint.CurrentHeat = CurrentHeat;
	Save->RunCheckpoint.CompletedCombatBattleCount = FMath::Max(0, CompletedCombatBattleCount);
	Save->RunCheckpoint.CombatDeckShuffleCooldownDuration = FMath::Max(0.f, CombatDeckShuffleCooldownDuration);
	Save->RunCheckpoint.CombatDeckMaxActiveSequenceSize = FMath::Max(0, CombatDeckMaxActiveSequenceSize);
	Save->RunCheckpoint.CombatDeckCards = CombatDeckCards;
	Save->RunCheckpoint.CombatDeckCardOrientations = CombatDeckCardOrientations;
	Save->RunCheckpoint.EquippedWeaponDef = EquippedWeaponDef;
	Save->RunCheckpoint.ActiveSacrificeGrace = ActiveSacrificeGrace;
	Save->RunCheckpoint.PlacedRunes = PlacedRunes;
	Save->RunCheckpoint.PendingRunes = PendingRunes;
	Save->RunCheckpoint.HiddenPassiveRuneInstances = HiddenPassiveRuneInstances;
	Save->RunCheckpoint.SacrificeOfferingCosts = SacrificeOfferingCosts;

	Save->PlayerStateData = PlayerStateData;
	Save->WeaponInstanceItems = WeaponInstanceItems;
	Save->MapStateData = MapStateData;
	Save->SavedCharacter = SavedCharacter;
	Save->TutorialState = TutorialState;
	Save->ShownPopupKeys = ShownPopupKeys;

	Save->Statistics.TotalRuns = FMath::Max(0, TotalRuns);
	Save->Statistics.TotalKills = FMath::Max(0, TotalKills);
	Save->Statistics.HighestFloor = FMath::Max(0, HighestFloor);
	Save->Statistics.TotalPlayTimeSeconds = FMath::Max(0, TotalPlayTimeSeconds);
	Save->Statistics.TotalDeaths = FMath::Max(0, TotalDeaths);
	Save->Statistics.TotalGoldEarned = FMath::Max(0, TotalGoldEarned);

	Save->MetaProgression.MetaCurrencies = MetaCurrencies;
	Save->MetaProgression.NodeLevels = NodeLevels;
	Save->MetaProgression.UnlockedFeatures = UnlockedFeatures;
	Save->MetaProgression.MysticSideLevel = MysticSideLevel;
	Save->MetaProgression.CraftedStarterRunes = CraftedStarterRunes;
	Save->MetaProgression.CraftedWeaponFinisherCards = CraftedWeaponFinisherCards;
	Save->SlotLastPlayTime = FDateTime::Now();
}

FString UYogSaveSlotEditProxy::BuildExportText() const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("SlotIndex,%d"), SlotIndex));
	Lines.Add(FString::Printf(TEXT("HasData,%s"), bHasData ? TEXT("true") : TEXT("false")));
	Lines.Add(FString::Printf(TEXT("HasPendingRun,%s"), bHasPendingRun ? TEXT("true") : TEXT("false")));
	Lines.Add(FString::Printf(TEXT("CheckpointFloor,%d"), CheckpointFloor));
	Lines.Add(FString::Printf(TEXT("CurrentHP,%.2f"), CurrentHP));
	Lines.Add(FString::Printf(TEXT("CurrentGold,%d"), CurrentGold));
	Lines.Add(FString::Printf(TEXT("CurrentPhase,%d"), CurrentPhase));
	Lines.Add(FString::Printf(TEXT("CurrentHeat,%.2f"), CurrentHeat));
	Lines.Add(FString::Printf(TEXT("CompletedCombatBattleCount,%d"), CompletedCombatBattleCount));
	Lines.Add(FString::Printf(TEXT("CombatDeckShuffleCooldownDuration,%.2f"), CombatDeckShuffleCooldownDuration));
	Lines.Add(FString::Printf(TEXT("CombatDeckMaxActiveSequenceSize,%d"), CombatDeckMaxActiveSequenceSize));
	Lines.Add(FString::Printf(TEXT("EquippedWeaponDef,%s"), *EquippedWeaponDef.ToSoftObjectPath().ToString()));
	Lines.Add(FString::Printf(TEXT("ActiveSacrificeGrace,%s"), *ActiveSacrificeGrace.ToSoftObjectPath().ToString()));
	Lines.Add(FString::Printf(TEXT("TotalRuns,%d"), TotalRuns));
	Lines.Add(FString::Printf(TEXT("TotalKills,%d"), TotalKills));
	Lines.Add(FString::Printf(TEXT("HighestFloor,%d"), HighestFloor));
	Lines.Add(FString::Printf(TEXT("TotalPlayTimeSeconds,%d"), TotalPlayTimeSeconds));
	Lines.Add(FString::Printf(TEXT("TotalDeaths,%d"), TotalDeaths));
	Lines.Add(FString::Printf(TEXT("TotalGoldEarned,%d"), TotalGoldEarned));
	Lines.Add(FString::Printf(TEXT("TutorialState,%d"), static_cast<int32>(TutorialState)));
	Lines.Add(FString::Printf(TEXT("MapStateLevelName,%s"), *MapStateData.LevelName.ToString()));

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[CombatDeckCards]"));
	for (int32 Index = 0; Index < CombatDeckCards.Num(); ++Index)
	{
		const FString CardPath = CombatDeckCards[Index].ToSoftObjectPath().ToString();
		const FString Orientation = CombatDeckCardOrientations.IsValidIndex(Index)
			? StaticEnum<ECombatCardLinkOrientation>()->GetNameStringByValue(static_cast<int64>(CombatDeckCardOrientations[Index]))
			: TEXT("MissingOrientation");
		Lines.Add(FString::Printf(TEXT("%d,%s,%s"), Index, *CardPath, *Orientation));
	}

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[RunCheckpointCounts]"));
	Lines.Add(FString::Printf(TEXT("PlacedRunes,%d"), PlacedRunes.Num()));
	Lines.Add(FString::Printf(TEXT("PendingRunes,%d"), PendingRunes.Num()));
	Lines.Add(FString::Printf(TEXT("HiddenPassiveRuneInstances,%d"), HiddenPassiveRuneInstances.Num()));
	Lines.Add(FString::Printf(TEXT("SacrificeOfferingCosts,%d"), SacrificeOfferingCosts.Num()));
	Lines.Add(FString::Printf(TEXT("WeaponInstanceItems,%d"), WeaponInstanceItems.Num()));
	Lines.Add(FString::Printf(TEXT("SavedCharacter,%d"), SavedCharacter.Num()));
	Lines.Add(FString::Printf(TEXT("ShownPopupKeys,%d"), ShownPopupKeys.Num()));

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[LegacyPlayerAttributes]"));
	Lines.Add(FString::Printf(TEXT("Attack,%.2f"), PlayerStateData.PlayerAttributeData.Attack));
	Lines.Add(FString::Printf(TEXT("AttackPower,%.2f"), PlayerStateData.PlayerAttributeData.AttackPower));
	Lines.Add(FString::Printf(TEXT("Health,%.2f"), PlayerStateData.PlayerAttributeData.Health));
	Lines.Add(FString::Printf(TEXT("MaxHealth,%.2f"), PlayerStateData.PlayerAttributeData.MaxHealth));
	Lines.Add(FString::Printf(TEXT("AttackSpeed,%.2f"), PlayerStateData.PlayerAttributeData.AttackSpeed));
	Lines.Add(FString::Printf(TEXT("AttackRange,%.2f"), PlayerStateData.PlayerAttributeData.AttackRange));
	Lines.Add(FString::Printf(TEXT("Sanity,%.2f"), PlayerStateData.PlayerAttributeData.Sanity));
	Lines.Add(FString::Printf(TEXT("MoveSpeed,%.2f"), PlayerStateData.PlayerAttributeData.MoveSpeed));
	Lines.Add(FString::Printf(TEXT("Dodge,%.2f"), PlayerStateData.PlayerAttributeData.Dodge));
	Lines.Add(FString::Printf(TEXT("Resilience,%.2f"), PlayerStateData.PlayerAttributeData.Resilience));
	Lines.Add(FString::Printf(TEXT("Resist,%.2f"), PlayerStateData.PlayerAttributeData.Resist));
	Lines.Add(FString::Printf(TEXT("DmgTaken,%.2f"), PlayerStateData.PlayerAttributeData.DmgTaken));
	Lines.Add(FString::Printf(TEXT("Crit_Rate,%.2f"), PlayerStateData.PlayerAttributeData.Crit_Rate));
	Lines.Add(FString::Printf(TEXT("Crit_Damage,%.2f"), PlayerStateData.PlayerAttributeData.Crit_Damage));

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[LegacyPlayerOwnedTags]"));
	for (const TPair<FGameplayTag, int32>& Pair : PlayerStateData.PlayerOwnedTags)
	{
		Lines.Add(FString::Printf(TEXT("%s,%d"), *Pair.Key.ToString(), Pair.Value));
	}

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[ShownPopupKeys]"));
	for (const FGameplayTag& Tag : ShownPopupKeys)
	{
		Lines.Add(Tag.ToString());
	}

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[MetaCurrencies]"));
	for (const TPair<FGameplayTag, int32>& Pair : MetaCurrencies)
	{
		Lines.Add(FString::Printf(TEXT("%s,%d"), *Pair.Key.ToString(), Pair.Value));
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[NodeLevels]"));
	for (const TPair<FName, int32>& Pair : NodeLevels)
	{
		Lines.Add(FString::Printf(TEXT("%s,%d"), *Pair.Key.ToString(), Pair.Value));
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[UnlockedFeatures]"));
	for (const FGameplayTag& Tag : UnlockedFeatures)
	{
		Lines.Add(Tag.ToString());
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[CraftedStarterRunes]"));
	for (const FPrimaryAssetId& AssetId : CraftedStarterRunes)
	{
		Lines.Add(AssetId.ToString());
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("[CraftedWeaponFinisherCards]"));
	for (const FPrimaryAssetId& AssetId : CraftedWeaponFinisherCards)
	{
		Lines.Add(AssetId.ToString());
	}
	return FString::Join(Lines, LINE_TERMINATOR);
}
