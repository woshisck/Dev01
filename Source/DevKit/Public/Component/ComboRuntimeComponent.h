#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Components/ActorComponent.h"
#include "Data/RuneDataAsset.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
class UAbilitySystemComponent;
struct FCombatDeckActionContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UComboRuntimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UComboRuntimeComponent();

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasComboSource() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasWeaponComboSource() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasSpecialAttackComboSource() const { return false; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasWeaponSkillComboSource() const { return false; }

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

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ResetCombo();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearRuntimeCombatLooseTags();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveNodeId() const { return NAME_None; }

	UFUNCTION(BlueprintPure, Category = "Combo")
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
