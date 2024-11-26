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

	ATTRIBUTE_ACCESSORS(UYogCombatSet, baseDMG);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, WeaponDMG);

	ATTRIBUTE_ACCESSORS(UYogCombatSet, BaseHeal);


	ATTRIBUTE_ACCESSORS(UYogCombatSet, BuffATKAmplify);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, BuffATK);



	ATTRIBUTE_ACCESSORS(UYogCombatSet, OwnerSpeed);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, DMGCorrect);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, DMGAbsorb);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, HitRate);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, Evade);
	ATTRIBUTE_ACCESSORS(UYogCombatSet, DMGDealResult);

	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData baseDMG;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData BaseHeal;




	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData WeaponDMG;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData 	DMGDealResult;


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OwnerSpeed, Category = "Combat")
	FGameplayAttributeData OwnerSpeed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DMGCorrect, Category = "Combat")
	FGameplayAttributeData DMGCorrect;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DMGAbsorb, Category = "Combat")
	FGameplayAttributeData DMGAbsorb;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HitRate, Category = "Combat")
	FGameplayAttributeData HitRate;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Evade, Category = "Combat")
	FGameplayAttributeData Evade;



	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BuffATKAmplify, Category = "Combat")
	FGameplayAttributeData BuffATKAmplify;


	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BuffATK, Category = "Combat")
	FGameplayAttributeData BuffATK;




protected:


	UFUNCTION()
	virtual void OnRep_OwnerSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_DMGCorrect(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_DMGAbsorb(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_HitRate(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	virtual void OnRep_Evade(const FGameplayAttributeData& OldValue);


	UFUNCTION()
	virtual void OnRep_BuffATKAmplify(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_BuffATK(const FGameplayAttributeData& OldValue);

};
