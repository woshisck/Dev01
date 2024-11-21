#pragma once

#include "CoreMinimal.h"
#include "BaseAttributeSet.h"
#include "YogCombatSet.generated.h"

UCLASS(BlueprintType)
class DEVKIT_API UYogCombatSet : public UBaseAttributeSet
{
	GENERATED_BODY()

public:
	// Sets default values for this empty's properties
	UYogCombatSet();




	ATTRIBUTE_ACCESSORS(UYogCombatSet, BaseDMG);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, WeaponDMG)
	ATTRIBUTE_ACCESSORS(UYogCombatSet, DMGAmplify)
	ATTRIBUTE_ACCESSORS(UYogCombatSet, OwnerSpeed)
	ATTRIBUTE_ACCESSORS(UYogCombatSet, DMGCorrect)
	ATTRIBUTE_ACCESSORS(UYogCombatSet, DMGAbsorb)
	ATTRIBUTE_ACCESSORS(UYogCombatSet, HitRate)
	ATTRIBUTE_ACCESSORS(UYogCombatSet, Evade)


private:

	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseDMG;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData WeaponDMG;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData DMGAmplify;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData OwnerSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData DMGCorrect;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData DMGAbsorb;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData HitRate;

	UPROPERTY(BlueprintReadOnly, Category = "Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Evade;


};
