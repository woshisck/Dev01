#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Component/CombatDeckComponent.h"
#include "GA_PlayerDash.generated.h"

class AActor;
class ACharacter;
class APlayerCharacterBase;
class UAnimMontage;
class UPrimitiveComponent;
struct FCollisionQueryParams;
struct FCollisionShape;

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_PlayerDash : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PlayerDash();

	UAnimMontage* ResolveDashMontage(APlayerCharacterBase* Player, const FGameplayTag& AbilityTag) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	bool ShouldResolveCombatDeck() const { return bResolveCombatDeck; }

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECombatDeckActionSlot GetCombatDeckActionSlot() const { return CombatDeckActionSlot; }

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECombatDeckFlowRole GetCombatDeckFlowRole() const { return CombatDeckFlowRole; }

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECombatCardTriggerTiming GetCombatDeckTriggerTiming() const { return CombatDeckTriggerTiming; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
	float DashMaxDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Tag")
	FGameplayTagContainer DashCancelProtectedTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "10.0", ClampMax = "100.0"))
	float DashCapsuleRadius = 35.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "1.0"))
	float DashMontageRootMotionLength = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Deck")
	bool bResolveCombatDeck = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Deck", meta = (EditCondition = "bResolveCombatDeck"))
	ECombatDeckActionSlot CombatDeckActionSlot = ECombatDeckActionSlot::Dash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Deck", meta = (EditCondition = "bResolveCombatDeck"))
	ECombatDeckFlowRole CombatDeckFlowRole = ECombatDeckFlowRole::Catalyst;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Deck", meta = (EditCondition = "bResolveCombatDeck"))
	ECombatCardTriggerTiming CombatDeckTriggerTiming = ECombatCardTriggerTiming::OnCommit;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void PreActivate(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate,
		const FGameplayEventData* TriggerEventData = nullptr) override;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

private:
	float GetFurthestValidDashDistance(const FVector& Start, const FVector& End);

	bool FindFirstDashBlockingHit(
		const FVector& SweepStart,
		const FVector& SweepEnd,
		const FCollisionShape& Shape,
		const FCollisionQueryParams& Params,
		FHitResult& OutHit) const;

	float FindAirWallExitDistance(
		const FVector& SweepStart,
		const FVector& DashDirection,
		const UPrimitiveComponent* AirWallComponent) const;

	float GetDashStopDistance(float HitDistance) const;

	void SetDashCollision(ACharacter* Character, ECollisionResponse Response) const;

	void ApplyDashMoveIgnores(ACharacter* Character);
	void ClearDashMoveIgnores(ACharacter* Character);
	bool HasEnemyOverlapAt(ACharacter* Character, const FVector& Location) const;
	void ResolveEnemyOverlapAfterDash(ACharacter* Character) const;

	void PrintDashDebugInfo();

	FTimerHandle DebugPrintTimer;
	FVector DashDebugStartLocation;
	float DashAnimScale = 1.f;
	FVector LastDashDirection = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AActor>> DashIgnoredActors;

	mutable FGameplayTagContainer PendingSaveComboTags;

	UPROPERTY()
	FCombatCardResolveResult ActiveCombatCardResult;

	FGuid ActiveCombatDeckGuid;

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
