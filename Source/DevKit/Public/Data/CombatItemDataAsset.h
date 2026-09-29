#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CombatItemDataAsset.generated.h"

class UAnimMontage;
class UTexture2D;

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatItemConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Handed to the triggered ability as FGameplayEventData::OptionalObject; the ability owns playback.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// Sent as a gameplay event on use. Author a GA with a matching GameplayEvent AbilityTrigger.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (Categories = "GameplayEvent"))
	FGameplayTag ActivationTag;
};

UCLASS(BlueprintType)
class DEVKIT_API UCombatItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Item")
	FCombatItemConfig Config;
};
