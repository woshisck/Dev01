#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameModes/LevelFlowTypes.h"
#include "YogLootSubsystem.generated.h"

class APlayerCharacterBase;
class URewardLootTableDataAsset;
class URuneDataAsset;

DECLARE_MULTICAST_DELEGATE(FOnLootSelected);

/**
 * Generates reward options and grants loot. Lives per world, so the "already offered" set resets with
 * every level. Replaces the loot code that used to sit on AYogGameMode.
 */
UCLASS()
class DEVKIT_API UYogLootSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Fires after the player takes any loot through GrantLoot. Used by LENode_WaitForLootSelected. */
	FOnLootSelected OnLootSelected;

	/**
	 * Rolls rune cards from Table (null falls back to UYogSettings::DefaultRewardLootTable).
	 * With bAvoidRepeats, runes already offered in this world are skipped, so sibling pickups differ.
	 */
	TArray<FLootOption> GenerateRuneOptions(const URewardLootTableDataAsset* Table, bool bAvoidRepeats = true);

	/** Grants one option to Player. Returns true when something was actually granted. */
	bool GrantLoot(APlayerCharacterBase* Player, const FLootOption& Option);

	/** Material currency used when an option leaves its tag empty. */
	static FGameplayTag ResolveMaterialCurrencyTag(FGameplayTag ConfiguredTag);

private:
	int32 ResolveOptionCount(const URewardLootTableDataAsset& Table) const;

	TSet<TWeakObjectPtr<URuneDataAsset>> OfferedRunes;
};
