// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ComboGraphBTTask_RunComboGraph.generated.h"

class UGameplayTask_StartComboGraph;
class UInputAction;
class UComboGraph;

/**
 * Run indicated ComboGraph Asset on Pawn Controlled by BT
 */
UCLASS(Category = "Combo Graph", meta = (DisplayName = "Run Combo Graph"))
class COMBOGRAPH_API UComboGraphBTTask_RunComboGraph : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UComboGraphBTTask_RunComboGraph(const FObjectInitializer& ObjectInitializer);

	/** Animation asset to play. Note that it needs to match the skeleton of pawn this BT is controlling */
	UPROPERTY(Category = Node, EditAnywhere)
	TObjectPtr<UComboGraph> ComboGraphAsset;

	/** Stack of Input Actions to process and simulate inputs when the underlying gameplay task receives ComboBegin event, and schedule for execution on the next frame */
	UPROPERTY(Category = Node, EditAnywhere)
	TArray<TObjectPtr<UInputAction>> Inputs;

	UPROPERTY(Transient)
	TObjectPtr<UGameplayTask_StartComboGraph> Task;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTreeComponent> CachedOwnerComp;

	UPROPERTY(Transient)
	TObjectPtr<APawn> ControlledPawn;

	TArray<UInputAction*> InputStack;

	//~ Begin UBTTaskNode interface
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif

protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	//~ Begin UBTTaskNode interface

private:
	/** Clears off any delegates bound to gameplay task */
	void CleanUp(UBehaviorTreeComponent& OwnerComp);

	UFUNCTION()
	void HandleGraphEnd(FGameplayTag GameplayTag, FGameplayEventData GameplayEventData);

	UFUNCTION()
	void HandleEventReceived(FGameplayTag GameplayTag, FGameplayEventData GameplayEventData);
	void SimulateNextComboInput();

	UFUNCTION()
	void HandleActivationFailed(FGameplayTag GameplayTag, FGameplayEventData GameplayEventData);

};
