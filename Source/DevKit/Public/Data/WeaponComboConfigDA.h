#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/RuneDataAsset.h"
#include "WeaponComboConfigDA.generated.h"

class UAnimMontage;
class UMontageAttackDataAsset;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UMontageConfigDA> MontageConfig = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UMontageAttackDataAsset> AttackDataOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bIsComboFinisher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bAllowDashSave = true;

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
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UWeaponComboConfigDA : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<FName> RootNodes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<FWeaponComboNodeConfig> Nodes;

	UFUNCTION(BlueprintPure, Category = "Combo")
	FWeaponComboNodeConfig FindNodeChecked(FName NodeId) const;

	const FWeaponComboNodeConfig* FindNode(FName NodeId) const;
	const FWeaponComboNodeConfig* FindRootNode(ECardRequiredAction InputAction) const;
	const FWeaponComboNodeConfig* FindChildNode(FName ParentNodeId, ECardRequiredAction InputAction) const;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ValidateConfig(TArray<FText>& OutWarnings) const;
};
