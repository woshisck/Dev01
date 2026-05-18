#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/WeaponComboConfigDA.h"
#include "Interfaces/YogComboGraphActiveInstance.h"
#include "ComboRuntimeComponent.generated.h"

class APlayerCharacterBase;
class UGameplayAbilityComboGraph;
struct FCombatDeckActionContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UComboRuntimeComponent : public UActorComponent, public IYogComboGraphActiveInstance
{
	GENERATED_BODY()

public:
	UComboRuntimeComponent();

	// IYogComboGraphActiveInstance — exposes current ComboGraph + active node id to the plugin's PIE debugger.
	virtual const UGameplayAbilityComboGraph* GetActiveComboGraph() const override { return ComboGraph; }
	virtual FName GetActiveComboNodeId() const override { return CurrentNodeId; }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadComboConfig(UWeaponComboConfigDA* InComboConfig);

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph);

	UFUNCTION(BlueprintPure, Category = "Combo")
	bool HasComboSource() const { return ComboConfig != nullptr || ComboGraph != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	UGameplayAbilityComboGraph* GetComboGraph() const { return ComboGraph; }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner);

	/** Call when the active combo node's montage begins. Spawns OnMontageStartFx. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void NotifyMontageStarted();

	/** Call when a hit lands during the active combo node's montage. Spawns OnHitSuccessFx and applies HitSuccessDilation. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void NotifyHitLanded();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ResetCombo();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void SaveCurrentNodeForDash();

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetCurrentNodeId() const { return CurrentNodeId; }

	UFUNCTION(BlueprintPure, Category = "Combo")
	FName GetActiveNodeId() const { return ActiveNode.NodeId; }

	const FWeaponComboNodeConfig* GetActiveNode() const;
	FGuid GetActiveAttackGuid() const { return ActiveAttackGuid; }
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
	FGuid ActiveAttackGuid;

	UPROPERTY()
	int32 ComboIndex = 0;

	UPROPERTY()
	FGameplayTagContainer ComboTags;

	bool bActiveNodeValid = false;
	bool bActivationFromDashSave = false;
	bool bComboContinued = true;
	bool bExitedComboState = false;

	// Hit-dilation state — tracked so the restore timer knows which scope to undo.
	FTimerHandle DilationRestoreHandle;
	EComboHitDilationScope ActiveDilationScope = EComboHitDilationScope::None;

	void PlayFxBinding(const FComboNodeFxBinding& Binding);
	void ApplyHitDilation(const FComboHitDilationSettings& Settings);
	void RestoreTimeDilation();
};
