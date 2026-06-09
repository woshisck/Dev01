#pragma once

#include "CoreMinimal.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "GameplayTagContainer.h"
#include "WeaponComboNodeConfig.generated.h"

class UAnimMontage;
class UGameplayAbilityComboGraphNode;
class UMontageConfigDA;

USTRUCT(BlueprintType)
struct DEVKIT_API FWeaponComboNodeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FName ParentNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	ECardRequiredAction InputAction = ECardRequiredAction::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card")
	ECombatDeckActionSlot CombatDeckActionSlot = ECombatDeckActionSlot::Attack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card")
	ECombatDeckFlowRole CombatDeckFlowRole = ECombatDeckFlowRole::Starter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	EYogComboGraphAttackType AttackType = EYogComboGraphAttackType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UMontageConfigDA> MontageConfig = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UMontageAttackDataAsset> AttackDataOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FComboAttackConfig NodeAttackConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bIsComboFinisher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window")
	bool bOverrideComboWindow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bOverrideComboWindow", ClampMin = "0"))
	int32 ComboWindowStartFrame = 18;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bOverrideComboWindow", ClampMin = "0"))
	int32 ComboWindowEndFrame = 27;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bOverrideComboWindow", ClampMin = "1"))
	int32 ComboWindowTotalFrames = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card")
	ECombatCardTriggerTiming CardTriggerTiming = ECombatCardTriggerTiming::OnCommit;

	static FWeaponComboNodeConfig FromComboGraphNode(
		const UGameplayAbilityComboGraphNode* Node,
		ECardRequiredAction InputAction,
		ECombatDeckActionSlot DefaultActionSlot = ECombatDeckActionSlot::Attack,
		ECombatDeckFlowRole DefaultFlowRole = ECombatDeckFlowRole::Starter);
};
