#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShopDataAsset.generated.h"

class URuneDataAsset;

USTRUCT(BlueprintType)
struct DEVKIT_API FShopRuneEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<URuneDataAsset> RuneAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "-1"))
	int32 OverrideGoldCost = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	bool bSoldOutAfterPurchase = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText Note;
};

UCLASS(BlueprintType)
class DEVKIT_API UShopDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FShopRuneEntry> StockPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "1", ClampMax = "6"))
	int32 MaxVisibleItems = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	bool bShuffleStockOnOpen = false;
};
