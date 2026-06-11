#pragma once

#include "CoreMinimal.h"
#include "Components/YogComboGraphRuntimeComponent.h"
#include "Data/WeaponComboNodeConfig.h"
#include "GameplayAbilitySpec.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
class UGameplayAbility;
class UGameplayAbilityComboGraph;
class UAbilitySystemComponent;
struct FCombatDeckActionContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UComboRuntimeComponent : public UYogComboGraphRuntimeComponent
{
	GENERATED_BODY()

public:
	UComboRuntimeComponent();

	virtual void LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph) override;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadWeaponComboGraph(UGameplayAbilityComboGraph* InComboGraph);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadSpecialAttackComboGraph(UGameplayAbilityComboGraph* InComboGraph);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasComboSource() const { return HasWeaponComboSource(); }

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasWeaponComboSource() const { return WeaponComboGraph != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasSpecialAttackComboSource() const { return SpecialAttackComboGraph != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	UGameplayAbilityComboGraph* GetWeaponComboGraph() const { return WeaponComboGraph; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	UGameplayAbilityComboGraph* GetSpecialAttackComboGraph() const { return SpecialAttackComboGraph; }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateAttack(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateDash(APlayerCharacterBase* PlayerOwner);

	virtual void ResetCombo() override;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearRuntimeCombatLooseTags();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveNodeId() const { return ActiveNode.NodeId; }

	const FWeaponComboNodeConfig* GetActiveNode() const;
	void RegisterActiveAttackAbility(const FGuid& AttackGuid, const FGameplayAbilitySpecHandle& AbilityHandle);
	bool HandleAttackAbilityEnded(const FGuid& EndedAttackGuid);
	FCombatDeckActionContext BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const;

private:
	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraph> WeaponComboGraph = nullptr;

	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraph> SpecialAttackComboGraph = nullptr;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveNode;

	FGameplayAbilitySpecHandle ActiveAbilitySpecHandle;

	UPROPERTY()
	FGameplayTagContainer RuntimeCombatLooseTags;

	bool IsActiveComboAbilityRunning(UAbilitySystemComponent* ASC) const;
	void ClearStaleActiveComboState(UAbilitySystemComponent* ASC, const TCHAR* Reason);
	void TrackRuntimeCombatLooseTag(const FGameplayTag& Tag);
	void SetActiveComboGraph(UGameplayAbilityComboGraph* InComboGraph);
	void EnsureAbilityGranted(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass);
	bool TryActivateComboFromGraph(
		UGameplayAbilityComboGraph* SourceGraph,
		EYogComboGraphInputAction GraphInput,
		ECardRequiredAction RuntimeInputAction,
		ECombatDeckActionSlot ActionSlot,
		ECombatDeckFlowRole FlowRole,
		APlayerCharacterBase* PlayerOwner);
};
