#pragma once

#include "CoreMinimal.h"
#include "Components/YogComboGraphRuntimeComponent.h"
#include "Data/WeaponComboConfigDA.h"
#include "GameplayAbilitySpec.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
class UGameplayAbilityComboGraph;
class UAnimMontage;
class UAbilitySystemComponent;
struct FCombatDeckActionContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UComboRuntimeComponent : public UYogComboGraphRuntimeComponent
{
	GENERATED_BODY()

public:
	UComboRuntimeComponent();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadComboConfig(UWeaponComboConfigDA* InComboConfig);

	virtual void LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph) override;

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasComboSource() const { return ComboConfig != nullptr || HasComboGraph(); }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner);

	bool TryActivateCombo(ECombatGraphInputAction InputAction, APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasDashInputNode() const;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateDash(APlayerCharacterBase* PlayerOwner);

	virtual void ResetCombo() override;
	virtual bool SaveCurrentNodeForDash() override;

	bool SaveCurrentNodeForDashWithPolicy(EComboDashSaveMode SaveMode = EComboDashSaveMode::PreserveIfSourceAllows, float ExpireSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearSavedDashNode();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void NotifyDashEnded(bool bWasCancelled);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearRuntimeCombatLooseTags();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveNodeId() const { return ActiveNode.NodeId; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	UAnimMontage* GetActiveDashMontageOverride() const { return ActiveDashMontageOverride; }

	const FWeaponComboNodeConfig* GetActiveNode() const;
	void RegisterActiveAttackAbility(const FGuid& AttackGuid, const FGameplayAbilitySpecHandle& AbilityHandle);
	bool HandleAttackAbilityEnded(const FGuid& EndedAttackGuid);
	FCombatDeckActionContext BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const;

#if WITH_DEV_AUTOMATION_TESTS
	void SetActiveDashMontageOverrideForTest(UAnimMontage* Montage) { ActiveDashMontageOverride = Montage; }
#endif

private:
	UPROPERTY()
	TObjectPtr<UWeaponComboConfigDA> ComboConfig = nullptr;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveNode;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveDashMontageOverride = nullptr;

	FGameplayAbilitySpecHandle ActiveAbilitySpecHandle;
	FTimerHandle DashSaveExpireTimerHandle;

	UPROPERTY()
	FGameplayTagContainer RuntimeCombatLooseTags;

	bool IsActiveComboAbilityRunning(UAbilitySystemComponent* ASC) const;
	void ClearStaleActiveComboState(UAbilitySystemComponent* ASC, const TCHAR* Reason);
	void TrackRuntimeCombatLooseTag(const FGameplayTag& Tag);
	void ExpireSavedDashNode();
};
