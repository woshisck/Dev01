#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Components/YogComboGraphRuntimeComponent.h"
#include "Data/WeaponComboNodeConfig.h"
#include "GameplayAbilitySpec.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
class UGameplayAbility;
class UGameplayAbilityComboGraph;
class UAbilitySystemComponent;
class UGameplayAbilityComboGraphNode;
struct FCombatDeckActionContext;

// Snapshot of one combo graph's cursor so multiple graphs (weapon vs weapon skill)
// can be swapped in and out of the single shared base-component state without losing
// where each was left off. Saved on switch-away, restored on switch-back.
USTRUCT()
struct FComboGraphContext
{
	GENERATED_BODY()

	UPROPERTY()
	FName CurrentNodeId = NAME_None;

	UPROPERTY()
	FName ActiveNodeId = NAME_None;

	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraphNode> ActiveGraphNode = nullptr;

	UPROPERTY()
	FGuid ActiveAttackGuid;

	UPROPERTY()
	int32 ComboIndex = 0;

	UPROPERTY()
	FGameplayTagContainer ComboTags;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveNode;

	bool bActiveNodeValid = false;
	bool bComboContinued = true;
	bool bExitedComboState = false;

	FGameplayAbilitySpecHandle ActiveAbilitySpecHandle;
};

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

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadWeaponSkillComboGraph(UGameplayAbilityComboGraph* InComboGraph);

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
	bool TryActivateWeaponSkill(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateDash(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateSpecial(APlayerCharacterBase* PlayerOwner);

	bool TryActivateSpecialCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner);
	bool TryActivateSpecialAttackCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner) { return TryActivateSpecialCombo(AbilityClass, PlayerOwner); }

	virtual void ResetCombo() override;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearRuntimeCombatLooseTags();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveNodeId() const { return ActiveNode.NodeId; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	TSubclassOf<UYogGameplayAbility> GetWeaponSkillAbility() const { return WeaponSkillAbility; }

	void SetWeaponSkillAbility(TSubclassOf<UYogGameplayAbility> InAbility);
	TSubclassOf<UYogGameplayAbility> GetComboSpecialActionAbility() const { return GetWeaponSkillAbility(); }
	void SetComboSpecialActionAbility(TSubclassOf<UYogGameplayAbility> InAbility) { SetWeaponSkillAbility(InAbility); }

	void EnsureWeaponComboAbilitiesGranted(APlayerCharacterBase* PlayerOwner);

	const FWeaponComboNodeConfig* GetActiveNode() const;
	bool ConsumePendingAbilityNode(FWeaponComboNodeConfig& OutNode);
	void RegisterActiveAttackAbility(const FGuid& AttackGuid, const FGameplayAbilitySpecHandle& AbilityHandle);
	bool HandleAttackAbilityEnded(const FGuid& EndedAttackGuid);
	FCombatDeckActionContext BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const;

private:
	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraph> WeaponComboGraph = nullptr;

	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraph> SpecialAttackComboGraph = nullptr;

	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraph> WeaponSkillComboGraph = nullptr;

	UPROPERTY()
	TMap<TObjectPtr<UGameplayAbilityComboGraph>, FComboGraphContext> GraphContexts;

	UPROPERTY()
	TSubclassOf<UYogGameplayAbility> WeaponSkillAbility;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveNode;

	UPROPERTY()
	FWeaponComboNodeConfig PendingAbilityNode;

	bool bPendingAbilityNodeValid = false;

	// Set when a switch-back restores a non-empty cursor, so the next combo input does
	// not treat the restored (but idle) cursor as stale and wipe it. Consumed once.
	bool bSuppressStaleClearOnce = false;

	FGameplayAbilitySpecHandle ActiveAbilitySpecHandle;

	UPROPERTY()
	FGameplayTagContainer RuntimeCombatLooseTags;

	bool IsActiveComboAbilityRunning(UAbilitySystemComponent* ASC) const;
	void ClearStaleActiveComboState(UAbilitySystemComponent* ASC, const TCHAR* Reason);
	void TrackRuntimeCombatLooseTag(const FGameplayTag& Tag);
	void SetActiveComboGraph(UGameplayAbilityComboGraph* InComboGraph);
	void SaveActiveContext();
	void RestoreContextFor(UGameplayAbilityComboGraph* Graph);
	void EnsureAbilityGranted(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass);
	bool TryActivateComboFromGraph(
		UGameplayAbilityComboGraph* SourceGraph,
		EYogComboGraphInputAction GraphInput,
		ECardRequiredAction RuntimeInputAction,
		ECombatDeckActionSlot ActionSlot,
		ECombatDeckFlowRole FlowRole,
		APlayerCharacterBase* PlayerOwner,
		TSubclassOf<UGameplayAbility> AbilityOverride = nullptr);
};
