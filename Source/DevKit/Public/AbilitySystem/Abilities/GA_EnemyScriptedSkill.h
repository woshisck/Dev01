#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Engine/TimerHandle.h"
#include "GA_EnemyScriptedSkill.generated.h"

class AActor;
class AEnemyCharacterBase;
class APlayerCharacterBase;
class UAbilityTask_ApplyRootMotionMoveToForce;
class UAnimInstance;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UEnemySkill;
class UGameplayEffect;
class UPrimitiveComponent;

/**
 * Enemy skill whose behavior lives on a UEnemySkill asset (C++ subclass or Lua module) instead of in the GA.
 * AbilityTags: Enemy.Skill.Skill1~4, one thin subclass per slot so TryActivateAbilityByExactTag finds it.
 * Granted by AEnemyCharacterBase from UEnemyData::Skills, with the asset as the spec's SourceObject.
 *
 * The GA owns everything frame- or physics-bound (montage, root motion, hit detection) and reports what
 * happened as named events to the asset:
 *   MontageEnded        the montage blended out normally
 *   MontageInterrupted  another montage or a cancel cut it off
 *   MoveFinished        a charge move ran its full distance (Other = the pinned player, if any; already released)
 *   HitPlayer           a non-pinning charge reached the player (Other = player); the move is already stopped
 *   GrabPlayer          a pinning charge reached the player (Other = player); the charge keeps going, carrying them
 *   HitWall             a charge ran head-on into a wall, or drove the pinned player into one (Other = the
 *                       pinned player, if any; already released); the move is already stopped
 *   <custom>            any name raised by a UAN_EnemySkillEvent notify, e.g. WindupEnd
 */
UCLASS(Abstract)
class DEVKIT_API UGA_EnemyScriptedSkill : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyScriptedSkill(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** The GA class that answers to SlotTag (Enemy.Skill.Skill1~4); null for any other tag. */
	static TSubclassOf<UGA_EnemyScriptedSkill> GetClassForSlot(const FGameplayTag& SlotTag);

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

	UFUNCTION(BlueprintPure, Category = "Enemy|Skill")
	AEnemyCharacterBase* GetSkillOwner() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Skill")
	UEnemySkill* GetSkillAsset() const { return SkillAsset; }

	/** The AI focus actor captured on activation, else the player pawn. */
	UFUNCTION(BlueprintPure, Category = "Enemy|Skill")
	AActor* GetSkillTarget() const;

	/** Plays the skill asset's montage from Section (None = from the start). Replaces a montage this skill is already playing. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	bool PlaySkillMontage(FName Section);

	/** Jumps inside the montage this skill is playing; a looping section keeps looping until the next jump. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	void JumpToSection(FName Section);

	/** Snaps the owner's yaw to face the skill target. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	void FaceTarget();

	/**
	 * Locks the direction to the target and moves the owner in a straight line past it.
	 * Speed in cm/s, Distance in cm. WallHitDot: a blocking hit counts as a wall only when
	 * Dot(HitNormal, ChargeDir) is below it (-0.7 = roughly head-on). HitPadding (cm) widens the
	 * player-contact sphere beyond the capsule radius. bPinPlayer: on contact the player is grabbed and
	 * carried in front of the owner to the end of the charge, or crushed against the wall that stops it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	bool StartChargeMove(float Speed, float Distance, float WallHitDot, float HitPadding, bool bPinPlayer = false);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	void StopChargeMove();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	FActiveGameplayEffectHandle ApplyEffectToActor(AActor* Target, TSubclassOf<UGameplayEffect> Effect, float Level = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	FActiveGameplayEffectHandle ApplyEffectToSelf(TSubclassOf<UGameplayEffect> Effect, float Level = 1.f);

	/** Forwards an event to the skill asset. Notifies and the skill's own tasks call this. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	void RaiseSkillEvent(FName EventName, AActor* Other);

	/** Raises EventName after Delay seconds unless the skill has ended by then. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	void RaiseSkillEventAfter(FName EventName, float Delay);

	/** Play length of a section of the skill asset's montage at rate 1; 0 when the section does not exist. */
	UFUNCTION(BlueprintPure, Category = "Enemy|Skill")
	float GetSectionLength(FName Section) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Skill")
	void EndSkill();

private:
	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnChargeMoveFinished();

	UFUNCTION()
	void OnOwnerCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnOwnerMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);

	void CheckChargePlayerContact();
	void ResolveChargeHit(FName EventName, AActor* Other);
	void BeginPin(APlayerCharacterBase* Player);
	void ReleasePin();
	void CrushPinnedPlayerIntoWall();
	void DisarmChargeDetection();
	void SetMontageRootMotionSuppressed(bool bSuppressed);
	void ReleaseMontageTask();

	UPROPERTY()
	TObjectPtr<UEnemySkill> SkillAsset;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> ChargeTask;

	TWeakObjectPtr<AActor> SavedFocusActor;
	FTimerHandle ChargeContactTimer;
	TArray<FTimerHandle> DelayedEventTimers;
	FVector ChargeDir = FVector::ZeroVector;
	float ChargeWallHitDot = -0.7f;
	float ChargeHitPadding = 30.f;
	bool bChargeArmed = false;
	bool bChargePinPlayer = false;

	TWeakObjectPtr<APlayerCharacterBase> PinnedPlayer;
	// Owner-to-player center distance along ChargeDir while pinned.
	float PinDistance = 0.f;

	// Montage instance whose root motion is pushed off while the charge move runs; popped on stop.
	TWeakObjectPtr<UAnimInstance> RootMotionDisabledAnimInstance;
	TWeakObjectPtr<UAnimMontage> RootMotionDisabledMontage;
};

UCLASS() class DEVKIT_API UGA_EnemyScriptedSkill1 : public UGA_EnemyScriptedSkill { GENERATED_BODY() public: UGA_EnemyScriptedSkill1(); };
UCLASS() class DEVKIT_API UGA_EnemyScriptedSkill2 : public UGA_EnemyScriptedSkill { GENERATED_BODY() public: UGA_EnemyScriptedSkill2(); };
UCLASS() class DEVKIT_API UGA_EnemyScriptedSkill3 : public UGA_EnemyScriptedSkill { GENERATED_BODY() public: UGA_EnemyScriptedSkill3(); };
UCLASS() class DEVKIT_API UGA_EnemyScriptedSkill4 : public UGA_EnemyScriptedSkill { GENERATED_BODY() public: UGA_EnemyScriptedSkill4(); };
