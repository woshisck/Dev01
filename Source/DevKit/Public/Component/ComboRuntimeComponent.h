#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Components/YogComboGraphRuntimeComponent.h"
#include "Data/WeaponComboNodeConfig.h"
#include "GameplayAbilitySpec.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
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

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasComboSource() const { return HasComboGraph(); }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner);

	virtual void ResetCombo() override;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearRuntimeCombatLooseTags();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveNodeId() const { return ActiveNode.NodeId; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	TSubclassOf<UYogGameplayAbility> GetComboSpecialActionAbility() const { return ComboSpecialActionAbility; }

	void SetComboSpecialActionAbility(TSubclassOf<UYogGameplayAbility> InAbility);

	const FWeaponComboNodeConfig* GetActiveNode() const;
	void RegisterActiveAttackAbility(const FGuid& AttackGuid, const FGameplayAbilitySpecHandle& AbilityHandle);
	bool HandleAttackAbilityEnded(const FGuid& EndedAttackGuid);
	FCombatDeckActionContext BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const;

private:
	UPROPERTY()
	TSubclassOf<UYogGameplayAbility> ComboSpecialActionAbility;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveNode;

	FGameplayAbilitySpecHandle ActiveAbilitySpecHandle;

	UPROPERTY()
	FGameplayTagContainer RuntimeCombatLooseTags;

	bool IsActiveComboAbilityRunning(UAbilitySystemComponent* ASC) const;
	void ClearStaleActiveComboState(UAbilitySystemComponent* ASC, const TCHAR* Reason);
	void TrackRuntimeCombatLooseTag(const FGameplayTag& Tag);
};
