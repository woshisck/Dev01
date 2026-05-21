#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/WeaponComboConfigDA.h"
#include "GameplayAbilitySpec.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
class UGameplayAbilityComboGraph;
struct FCombatDeckActionContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UComboRuntimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UComboRuntimeComponent();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadComboConfig(UWeaponComboConfigDA* InComboConfig);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasComboSource() const { return ComboConfig != nullptr || ComboGraph != nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner);

	bool TryActivateCombo(ECombatGraphInputAction InputAction, APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasDashInputNode() const;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateDash(APlayerCharacterBase* PlayerOwner);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ResetCombo();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool SaveCurrentNodeForDash(EComboDashSaveMode SaveMode = EComboDashSaveMode::PreserveIfSourceAllows, float ExpireSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearSavedDashNode();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void NotifyDashEnded(bool bWasCancelled);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearRuntimeCombatLooseTags();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetCurrentNodeId() const { return CurrentNodeId; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveNodeId() const { return ActiveNode.NodeId; }

	const FWeaponComboNodeConfig* GetActiveNode() const;
	FGuid GetActiveAttackGuid() const { return ActiveAttackGuid; }
	void RegisterActiveAttackAbility(const FGuid& AttackGuid, const FGameplayAbilitySpecHandle& AbilityHandle);
	bool HandleAttackAbilityEnded(const FGuid& EndedAttackGuid);
	int32 GetComboIndex() const { return ComboIndex; }
	const FGameplayTagContainer& GetComboTags() const { return ComboTags; }
	bool DidComboContinue() const { return bComboContinued; }
	bool DidExitComboState() const { return bExitedComboState; }
	bool ConsumeActivationFromDashSave();
	FCombatDeckActionContext BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const;

private:
	UPROPERTY()
	TObjectPtr<UWeaponComboConfigDA> ComboConfig = nullptr;

	UPROPERTY()
	TObjectPtr<UGameplayAbilityComboGraph> ComboGraph = nullptr;

	UPROPERTY()
	FName CurrentNodeId = NAME_None;

	UPROPERTY()
	FName SavedDashNodeId = NAME_None;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveNode;

	UPROPERTY()
	FWeaponComboNodeConfig ActiveDashNode;

	UPROPERTY()
	FGuid ActiveAttackGuid;

	FGameplayAbilitySpecHandle ActiveAbilitySpecHandle;

	FTimerHandle DashSaveExpireTimerHandle;

	UPROPERTY()
	int32 ComboIndex = 0;

	UPROPERTY()
	FGameplayTagContainer ComboTags;

	bool bActiveNodeValid = false;
	bool bActiveDashNodeValid = false;
	bool bActivationFromDashSave = false;
	bool bComboContinued = true;
	bool bExitedComboState = false;

	UPROPERTY()
	FGameplayTagContainer RuntimeCombatLooseTags;

	void TrackRuntimeCombatLooseTag(const FGameplayTag& Tag);
	void ExpireSavedDashNode();
};
