// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Component/CombatDeckComponent.h"
#include "Engine/EngineTypes.h"
#include "GA_PlayMontage.generated.h"

class UAN_MeleeDamage;
class AActor;
class UAnimMontage;
class UYogTask_PlayMontageAbility;

UCLASS()
class DEVKIT_API UGA_PlayMontage : public UYogGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_PlayMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	bool ShouldResolveCombatDeck() const { return bResolveCombatDeck; }

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECombatDeckActionSlot GetCombatDeckActionSlot() const { return CombatDeckActionSlot; }

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECombatDeckFlowRole GetCombatDeckFlowRole() const { return CombatDeckFlowRole; }

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECombatCardTriggerTiming GetCombatDeckCommitTiming() const { return CombatDeckCommitTiming; }

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, const FGameplayEventData& EventData);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<UYogGameplayEffect> DynamicEffectClass;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

	UPROPERTY()
	TObjectPtr<UYogTask_PlayMontageAbility> ActivePlayMontageTask;

protected:
	virtual void HandleMontageEnded(bool bWasCancelled);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Combo")
	bool bListenForComboWindow = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold")
	bool bHoldMontageUntilInputRelease = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FGameplayTag HoldReleaseEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FName HoldStartSection = TEXT("BlockStart");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FName HoldLoopSection = TEXT("BlockIdle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	FName HoldEndSection = TEXT("BlockEnd");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Hold", meta = (EditCondition = "bHoldMontageUntilInputRelease"))
	bool bJumpToHoldEndSectionOnRelease = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Montage|Block", meta = (EditCondition = "bHoldMontageUntilInputRelease", ClampMin = "0.0"))
	float JustBlockRewardDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Deck")
	bool bResolveCombatDeck = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Deck", meta = (EditCondition = "bResolveCombatDeck"))
	ECombatDeckActionSlot CombatDeckActionSlot = ECombatDeckActionSlot::Any;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Deck", meta = (EditCondition = "bResolveCombatDeck"))
	ECombatDeckFlowRole CombatDeckFlowRole = ECombatDeckFlowRole::Any;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Deck", meta = (EditCondition = "bResolveCombatDeck"))
	ECombatCardTriggerTiming CombatDeckCommitTiming = ECombatCardTriggerTiming::OnCommit;

	// Compatibility field name: this means immediate deck resolution on commit, not removing a card from the deck.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Deck", meta = (EditCondition = "bResolveCombatDeck", DisplayName = "Resolve Combat Deck On Commit"))
	bool bConsumeCombatDeckOnCommit = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Shared Cooldown")
	bool bStartsSharedSkillCooldown = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Shared Cooldown", meta = (EditCondition = "bStartsSharedSkillCooldown", ClampMin = "0.0"))
	float SharedSkillCooldownFallbackDuration = 0.0f;

private:
	void OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount);
	bool IsHoldMontageConfigured(const UAnimMontage* Montage) const;
	bool HasMontageSection(const UAnimMontage* Montage, FName SectionName) const;
	void ConfigureHoldMontageSections();
	void HandleHoldInputReleased();
	void ClearBlockStateTags();
	void SetBlockStateTag(const FGameplayTag& Tag, int32 Count);
	void ScheduleBlockStartWindow();
	void FinishBlockStartWindow();
	void ApplyJustBlockReward();
	FCombatCardResolveResult ResolveCombatDeck(ECombatCardTriggerTiming TriggerTiming, bool bResolveOnCommit);
	FCombatDeckActionContext BuildCombatDeckContext(ECombatCardTriggerTiming TriggerTiming, bool bResolveOnCommit) const;
	void PrimeCombatDeckHitContext(class AYogCharacterBase* Owner, const TArray<AActor*>& HitActors) const;
	void StopActiveCombatDeckFlows();
	FGameplayTag GetPrimaryAbilityTag() const;
	void StartSharedSkillCooldownIfConfigured() const;

	FDelegateHandle CanComboTagHandle;
	FDelegateHandle LegacyCanComboTagHandle;
	float AbilityActivationTime = 0.0f;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveMontage;

	FTimerHandle ComboWindowOpenHandle;
	FTimerHandle ComboWindowCloseHandle;
	FTimerHandle BlockStartWindowHandle;

	bool bIsHandlingMeleeEvent = false;
	bool bActiveHoldMontage = false;
	bool bHoldReleaseReceived = false;
	bool bCombatDeckCardResolvedThisActivation = false;

	UPROPERTY()
	FCombatCardResolveResult ActiveCombatCardResult;

	FGuid ActiveCombatDeckGuid;
};
