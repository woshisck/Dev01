// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "Character/YogInteractable.h"
#include "AbilitySystem/Attribute/EnemyAttributeSet.h"

#include "Data/EnemyData.h"
#include "EnemyCharacterBase.generated.h"

class ARewardPickup;
class UBuffFlowComponent;
class UCapsuleComponent;
class UEnemyHealthDisplayComponent;
class UEnemyWeaponDefinition;
class UInteractPromptComponent;
class UPrimitiveComponent;
class UMontageVFXBindingComponent;
class UYogAbilitySystemComponent;

/**
 *
 */
UCLASS()
class DEVKIT_API AEnemyCharacterBase : public AYogCharacterBase, public IYogInteractable
{
	GENERATED_BODY()
public:

	AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 关卡 Buff（RuneDA FlowAsset）直接在此组件上激活，无需背包
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuffFlow")
	TObjectPtr<UBuffFlowComponent> BuffFlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Health Display")
	TObjectPtr<UEnemyHealthDisplayComponent> HealthDisplayComponent;

	// Tutorial/practice enemies can opt out of room clear, wave, and kill-count progression.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Level")
	bool bCountsForLevelClear = true;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die() override;
	virtual void FinishDying() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool IsAlive() const override;
	virtual float GetDeathDisappearDelayAfterAnimation(bool bHasDissolveCue) const override;
	virtual bool IsLootableCorpse() const override { return bLootableCorpse; }

	/** True while the corpse is in the world, can be hit and can be looted. */
	bool IsCorpseInteractable() const { return bCorpseActive && !bCorpseConsumed; }

	/**
	 * Counts one player swing against this corpse. SwingId dedupes the several damage events a single
	 * swing fires; an invalid id falls back to a short time window. Bursts the corpse on the last hit.
	 */
	bool RegisterCorpseHit(const FGuid& SwingId);

	// ~ IYogInteractable
	virtual void TryInteract(APlayerCharacterBase* Player) override;
	virtual int32 GetInteractPriority() const override { return YogInteractPriority::Corpse; }
	virtual bool CanInteract(const APlayerCharacterBase* Player) const override { return IsCorpseInteractable(); }

	void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Weapon")
	void SetPendingEnemyWeaponDefinition(UEnemyWeaponDefinition* WeaponDefinition);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Weapon")
	void ApplyEnemyWeaponDefinition(UEnemyWeaponDefinition* WeaponDefinition);

	UFUNCTION(BlueprintPure, Category = "Enemy|Weapon")
	UEnemyWeaponDefinition* GetEquippedEnemyWeaponDefinition() const { return EquippedEnemyWeaponDefinition.Get(); }

	UFUNCTION(BlueprintPure, Category = "Enemy|Identity")
	UEnemyData* GetEnemyData() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Identity")
	EEnemyCombatTier GetCombatTier() const;

	/** Falls back to the actor name when the data asset leaves DisplayName empty. */
	UFUNCTION(BlueprintPure, Category = "Enemy|Identity")
	FText GetEnemyDisplayName() const;

	virtual const UWeaponDefinitionBase* GetEffectiveWeaponDefinition() const override;
	virtual AWeaponInstance* GetEquippedWeaponActor() const override;

	void SetAIAttackRuntimeContext(
		bool bIsReposition,
		bool bRequestRepositionOnResolve,
		AActor* TargetActor,
		float DistanceToTarget);
	bool ConsumeAIAttackRuntimeContext(FEnemyAIAttackRuntimeContext& OutContext);
	void ClearAIAttackRuntimeContext();

protected:
	UFUNCTION()
	void OnHealthChangedForDeath(float NewHealth);

	UFUNCTION()
	void OnReceivedDamageForAI(UYogAbilitySystemComponent* SourceASC, float Damage);

	void DestroySpawnedEnemyWeaponActors();

	/** Rolls UEnemyData::KillRewards into the pending loot arrays and decides whether the body stays as a lootable corpse. */
	void RollKillRewards();

	/**
	 * Spawns the pending loot as pickups around the body and clears it.
	 * ScatterRadius > 0 randomizes each pickup inside that radius instead of lining them up.
	 */
	void SpawnKillRewardPickups(bool bPlayFocusCue, float ScatterRadius, TArray<ARewardPickup*>* OutPickups = nullptr);

	void EnterLootableCorpseState();
	void ExplodeCorpse();

	UFUNCTION()
	void OnCorpseInteractBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnCorpseInteractEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UEnemyAttributeSet> EnemyAttributeSet;

	UPROPERTY(Transient)
	FEnemyAIAttackRuntimeContext PendingAIAttackContext;

	UPROPERTY(Transient)
	TObjectPtr<UEnemyWeaponDefinition> PendingEnemyWeaponDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UEnemyWeaponDefinition> EquippedEnemyWeaponDefinition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedEnemyWeaponActors;

	// Bundled gold/material/rune loot shares one pickup; each weapon skill needs its own, see SpawnKillRewardPickups.
	UPROPERTY(Transient)
	TArray<FLootOption> PendingCorpseBundledLoot;

	UPROPERTY(Transient)
	TArray<FLootOption> PendingCorpseSkillLoot;

	// Created when the corpse state starts so ordinary enemies carry no extra components or widgets.
	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> CorpseInteractVolume;

	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> CorpseHitVolume;

	UPROPERTY(Transient)
	TObjectPtr<UInteractPromptComponent> CorpsePromptComp;

	TWeakObjectPtr<APlayerCharacterBase> CorpseNearbyPlayer;

	FGuid LastCorpseHitSwingId;
	float LastCorpseHitTime = -1.f;
	int32 CorpseHitCount = 0;

	bool bLootableCorpse = false;
	bool bCorpseActive = false;
	bool bCorpseConsumed = false;


	friend UEnemyAttributeSet;
};
