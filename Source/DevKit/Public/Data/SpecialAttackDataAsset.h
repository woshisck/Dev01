#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Engine/DataAsset.h"
#include "SpecialAttackDataAsset.generated.h"

class UAnimMontage;
class UTexture2D;

USTRUCT(BlueprintType)
struct DEVKIT_API FSpecialAttackConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Attack")
	FName SpecialAttackId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Attack")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Attack")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Attack")
	TSubclassOf<UYogGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Attack")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// Deprecated ComboGraph reference. Special runtime now uses AbilityData/montage tags.
	UPROPERTY(Transient)
	TObjectPtr<UObject> ComboGraph = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Attack", meta = (ClampMin = "0.0"))
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Attack", meta = (ClampMin = "0.0"))
	float PlayRate = 1.0f;
};

UCLASS(BlueprintType)
class DEVKIT_API USpecialAttackDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special Attack")
	FSpecialAttackConfig Config;
};
