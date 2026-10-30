#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RewardLootTableDataAsset.generated.h"

class URuneDataAsset;

/**
 * Rune pool and card count used when a reward pickup offers a rune selection.
 * A pickup may point at its own table; otherwise UYogSettings::DefaultRewardLootTable is used.
 */
UCLASS(BlueprintType)
class DEVKIT_API URewardLootTableDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<TObjectPtr<URuneDataAsset>> RunePool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", ClampMax = "3"))
	int32 DefaultOptionCount = 3;

	// While the player carries this tag the card count drops to SingleChoiceOptionCount.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FGameplayTag SingleChoiceTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1", ClampMax = "3"))
	int32 SingleChoiceOptionCount = 1;
};
