#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MetaProgression/MetaTypes.h"
#include "SaveGame/YogSaveGame.h"
#include "UMetaNodeEditProxy.generated.h"

class UDataTable;

UCLASS(Transient)
class DEVKITEDITOR_API UMetaNodeEditProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Node")
	FMetaUpgradeNodeRow Data;

	FName SourceRowName;

	UPROPERTY()
	TObjectPtr<UDataTable> SourceTable;

	void LoadFromRow(UDataTable* Table, FName RowName);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

UCLASS(Transient)
class DEVKITEDITOR_API UMetaCurrencyEditProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Currency")
	FMetaCurrencyRow Data;

	FName SourceRowName;

	UPROPERTY()
	TObjectPtr<UDataTable> SourceTable;

	void LoadFromRow(UDataTable* Table, FName RowName);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

UCLASS(Transient)
class DEVKITEDITOR_API UYogSaveSlotEditProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Slot")
	int32 SlotIndex = 0;

	UPROPERTY(VisibleAnywhere, Category = "Slot")
	bool bHasData = false;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint")
	bool bHasPendingRun = false;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint", meta = (ClampMin = "0"))
	int32 CheckpointFloor = 0;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint")
	float CurrentHP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint")
	int32 CurrentGold = 0;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint")
	int32 CurrentPhase = 0;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint")
	float CurrentHeat = 0.f;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Combat")
	int32 CompletedCombatBattleCount = 0;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Combat Deck")
	float CombatDeckShuffleCooldownDuration = 0.f;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Combat Deck")
	int32 CombatDeckMaxActiveSequenceSize = 0;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Combat Deck")
	TArray<TSoftObjectPtr<URuneDataAsset>> CombatDeckCards;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Combat Deck")
	TArray<ECombatCardLinkOrientation> CombatDeckCardOrientations;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Equipment")
	TSoftObjectPtr<UWeaponDefinition> EquippedWeaponDef;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Sacrifice")
	TSoftObjectPtr<USacrificeGraceDA> ActiveSacrificeGrace;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Runes")
	TArray<FPlacedRune> PlacedRunes;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Runes")
	TArray<FRuneInstance> PendingRunes;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Runes")
	TArray<FRuneInstance> HiddenPassiveRuneInstances;

	UPROPERTY(EditAnywhere, Category = "Run Checkpoint|Sacrifice")
	TArray<FSacrificeOfferingCostState> SacrificeOfferingCosts;

	UPROPERTY(EditAnywhere, Category = "Legacy Player Save")
	FPlayerGASData PlayerStateData;

	UPROPERTY(EditAnywhere, Category = "Legacy Player Save")
	TArray<FWeaponInstanceData> WeaponInstanceItems;

	UPROPERTY(EditAnywhere, Category = "Legacy Player Save")
	FYogMapStateData MapStateData;

	UPROPERTY(EditAnywhere, Category = "Legacy Player Save")
	TArray<FCharacterSaveData> SavedCharacter;

	UPROPERTY(EditAnywhere, Category = "Tutorial")
	ETutorialState TutorialState = ETutorialState::NeedWeaponTutorial;

	UPROPERTY(EditAnywhere, Category = "Tutorial")
	TSet<FGameplayTag> ShownPopupKeys;

	UPROPERTY(EditAnywhere, Category = "Statistics", meta = (ClampMin = "0"))
	int32 TotalRuns = 0;

	UPROPERTY(EditAnywhere, Category = "Statistics", meta = (ClampMin = "0"))
	int32 TotalKills = 0;

	UPROPERTY(EditAnywhere, Category = "Statistics", meta = (ClampMin = "0"))
	int32 HighestFloor = 0;

	UPROPERTY(EditAnywhere, Category = "Statistics", meta = (ClampMin = "0"))
	int32 TotalPlayTimeSeconds = 0;

	UPROPERTY(EditAnywhere, Category = "Statistics", meta = (ClampMin = "0"))
	int32 TotalDeaths = 0;

	UPROPERTY(EditAnywhere, Category = "Statistics", meta = (ClampMin = "0"))
	int32 TotalGoldEarned = 0;

	UPROPERTY(EditAnywhere, Category = "Meta Progression")
	TMap<FGameplayTag, int32> MetaCurrencies;

	UPROPERTY(EditAnywhere, Category = "Meta Progression")
	TMap<FName, int32> NodeLevels;

	UPROPERTY(EditAnywhere, Category = "Meta Progression")
	TSet<FGameplayTag> UnlockedFeatures;

	UPROPERTY(EditAnywhere, Category = "Meta Progression")
	int32 MysticSideLevel = 0;

	UPROPERTY(EditAnywhere, Category = "Meta Progression")
	TArray<FPrimaryAssetId> CraftedStarterRunes;

	UPROPERTY(EditAnywhere, Category = "Meta Progression")
	TArray<FPrimaryAssetId> CraftedWeaponFinisherCards;

	void LoadFromSave(class UYogSaveGame* Save, int32 InSlotIndex);
	void ApplyToSave(class UYogSaveGame* Save) const;
	FString BuildExportText() const;
};
