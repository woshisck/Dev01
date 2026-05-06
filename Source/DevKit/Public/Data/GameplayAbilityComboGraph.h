#pragma once

#include "CoreMinimal.h"
#include "GenericGraph.h"
#include "GenericGraphEdge.h"
#include "GenericGraphNode.h"
#include "Data/WeaponComboConfigDA.h"
#include "GameplayAbilityComboGraph.generated.h"

class UAnimMontage;
class UMontageAttackDataAsset;

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGameplayAbilityComboGraphNode : public UGenericGraphNode
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraphNode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	ECardRequiredAction RootInputAction = ECardRequiredAction::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (ClampMin = "1"))
	int32 TotalFrames = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UMontageAttackDataAsset> AttackDataOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bIsComboFinisher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bAllowDashSave = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window")
	bool bUseNodeComboWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bUseNodeComboWindow", ClampMin = "0"))
	int32 ComboWindowStartFrame = 18;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bUseNodeComboWindow", ClampMin = "0"))
	int32 ComboWindowEndFrame = 27;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card", meta = (AdvancedDisplay))
	ECombatCardTriggerTiming CardTriggerTiming = ECombatCardTriggerTiming::OnCommit;

	FWeaponComboNodeConfig BuildRuntimeConfig(ECardRequiredAction InputAction) const;
	virtual FText GetDescription_Implementation() const override;

#if WITH_EDITORONLY_DATA
	bool bDebugActive = false;
#endif

#if WITH_EDITOR
	virtual FLinearColor GetBackgroundColor() const override;
	virtual FText GetNodeTitle() const override;
	virtual bool CanCreateConnectionTo(UGenericGraphNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage) override;
#endif
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGameplayAbilityComboGraphEdge : public UGenericGraphEdge
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraphEdge();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	ECardRequiredAction InputAction = ECardRequiredAction::Any;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
#endif
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGameplayAbilityComboGraph : public UGenericGraph
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraph();

	const UGameplayAbilityComboGraphNode* FindRootComboNode(ECardRequiredAction InputAction) const;
	const UGameplayAbilityComboGraphNode* FindChildComboNode(FName ParentNodeId, ECardRequiredAction InputAction) const;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ValidateComboGraph(TArray<FText>& OutWarnings) const;
};
