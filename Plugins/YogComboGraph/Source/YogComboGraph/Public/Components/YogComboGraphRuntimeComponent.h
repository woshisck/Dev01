#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Interfaces/YogComboGraphActiveInstance.h"
#include "TimerManager.h"
#include "YogComboGraphRuntimeComponent.generated.h"

class UGameplayAbilityComboGraph;
class UGameplayAbilityComboGraphNode;

struct YOGCOMBOGRAPH_API FYogComboGraphNodeSelection
{
	const UGameplayAbilityComboGraphNode* Node = nullptr;
	bool bFoundChildNode = false;
};

/**
 * Generic runtime state holder for UGameplayAbilityComboGraph assets.
 *
 * Projects can subclass this to map their own input/action types to the graph input
 * enum and trigger their own abilities, while the plugin owns graph traversal, active
 * node state and debug exposure.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YOGCOMBOGRAPH_API UYogComboGraphRuntimeComponent : public UActorComponent, public IYogComboGraphActiveInstance
{
	GENERATED_BODY()

public:
	UYogComboGraphRuntimeComponent();

	virtual const UGameplayAbilityComboGraph* GetActiveComboGraph() const override { return ComboGraph; }
	virtual FName GetActiveComboNodeId() const override { return CurrentNodeId; }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	virtual void LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasComboGraph() const { return ComboGraph != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	UGameplayAbilityComboGraph* GetComboGraph() const { return ComboGraph; }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateComboGraphNode(EYogComboGraphInputAction InputAction, const FGameplayTagContainer& OwnedTags);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	virtual void ResetCombo();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetCurrentNodeId() const { return CurrentNodeId; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveGraphNodeId() const { return ActiveNodeId; }

	const UGameplayAbilityComboGraphNode* GetActiveGraphNode() const { return ActiveGraphNode; }
	FGuid GetActiveAttackGuid() const { return ActiveAttackGuid; }
	int32 GetComboIndex() const { return ComboIndex; }
	const FGameplayTagContainer& GetComboTags() const { return ComboTags; }
	bool DidComboContinue() const { return bComboContinued; }
	bool DidExitComboState() const { return bExitedComboState; }

	bool FindNextComboGraphNode(EYogComboGraphInputAction InputAction, const FGameplayTagContainer* OwnedTags, FYogComboGraphNodeSelection& OutSelection) const;

protected:
	FName GetComboStartNodeId() const { return CurrentNodeId; }

	void PrepareComboGraphNodeActivation(const FYogComboGraphNodeSelection& Selection);
	void PrepareComboNodeActivation(FName NodeId, bool bFoundChildNode);
	void CommitPreparedComboActivation();
	void ClearPreparedComboActivation();
	void MarkComboActivationMiss();

	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraph> ComboGraph = nullptr;

	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraphNode> ActiveGraphNode = nullptr;

	UPROPERTY()
	FName CurrentNodeId = NAME_None;

	UPROPERTY()
	FName ActiveNodeId = NAME_None;

	UPROPERTY()
	FGuid ActiveAttackGuid;

	UPROPERTY()
	int32 ComboIndex = 0;

	UPROPERTY()
	FGameplayTagContainer ComboTags;

	bool bActiveNodeValid = false;
	bool bComboContinued = true;
	bool bExitedComboState = false;

private:
	bool bPreparedFoundChildNode = false;
};
