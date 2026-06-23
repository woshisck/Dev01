#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Components/ActorComponent.h"
#include "Data/RuneDataAsset.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
class UAbilitySystemComponent;
struct FCombatDeckActionContext;

// Deprecated compatibility shell. Player combat now uses direct action abilities and CombatDeckComponent action slots.
UCLASS(ClassGroup=(Deprecated), meta=(DisplayName = "Deprecated Combo Runtime Component"))
class DEVKIT_API UComboRuntimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UComboRuntimeComponent();

	UFUNCTION(BlueprintPure, Category = "Deprecated Combo Runtime")
	bool HasComboSource() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Deprecated Combo Runtime")
	bool HasWeaponComboSource() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Deprecated Combo Runtime")
	bool HasSkillComboSource() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Deprecated Combo Runtime", meta = (DeprecatedFunction, DeprecationMessage = "SpecialAttack combo runtime is deprecated. Use active skills."))
	bool HasSpecialAttackComboSource() const { return HasSkillComboSource(); }

	UFUNCTION(BlueprintPure, Category = "Deprecated Combo Runtime")
	bool HasWeaponSkillComboSource() const { return false; }

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime", meta = (DeprecatedFunction, DeprecationMessage = "ComboRuntime is deprecated for player combat."))
	bool TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime", meta = (DeprecatedFunction, DeprecationMessage = "ComboRuntime is deprecated for player combat. Use the broad Attack action."))
	bool TryActivateAttack(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime", meta = (DeprecatedFunction, DeprecationMessage = "ComboRuntime is deprecated for player combat. Use the broad WeaponSkill action."))
	bool TryActivateWeaponSkill(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime", meta = (DeprecatedFunction, DeprecationMessage = "ComboRuntime is deprecated for player combat. Use the broad Dash action."))
	bool TryActivateDash(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime", meta = (DeprecatedFunction, DeprecationMessage = "ComboRuntime is deprecated for player combat. Use PlayerActiveSkillComponent."))
	bool TryActivateSkill(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime", meta = (DeprecatedFunction, DeprecationMessage = "Special input is deprecated. Use Skill / PlayerActiveSkillComponent."))
	bool TryActivateSpecial(APlayerCharacterBase* PlayerOwner);

	bool TryActivateSkillCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner);
	bool TryActivateSpecialCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner);
	bool TryActivateSpecialAttackCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner)
	{
		return TryActivateSpecialCombo(AbilityClass, PlayerOwner);
	}

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime")
	void ResetCombo();

	UFUNCTION(BlueprintCallable, Category = "Deprecated Combo Runtime")
	void ClearRuntimeCombatLooseTags();

	UFUNCTION(BlueprintPure, Category = "Deprecated Combo Runtime")
	FName GetActiveNodeId() const { return NAME_None; }

	UFUNCTION(BlueprintPure, Category = "Deprecated Combo Runtime")
	TSubclassOf<UYogGameplayAbility> GetWeaponSkillAbility() const { return WeaponSkillAbility; }

	void SetWeaponSkillAbility(TSubclassOf<UYogGameplayAbility> InAbility);
	TSubclassOf<UYogGameplayAbility> GetComboSpecialActionAbility() const { return GetWeaponSkillAbility(); }
	void SetComboSpecialActionAbility(TSubclassOf<UYogGameplayAbility> InAbility) { SetWeaponSkillAbility(InAbility); }

	void EnsureWeaponComboAbilitiesGranted(APlayerCharacterBase* PlayerOwner);

	void RegisterActiveAttackAbility(const FGuid& AttackGuid);
	bool HandleAttackAbilityEnded(const FGuid& EndedAttackGuid);
	FCombatDeckActionContext BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const;

private:
	UPROPERTY()
	TSubclassOf<UYogGameplayAbility> WeaponSkillAbility;
};
