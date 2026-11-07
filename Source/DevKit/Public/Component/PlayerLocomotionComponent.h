// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/PlayerLocomotionMath.h"
#include "PlayerLocomotionComponent.generated.h"

class APlayerCharacterBase;

UENUM(BlueprintType)
enum class EAimTargetMode : uint8
{
	// Nearest living hostile, re-evaluated at a low rate with stickiness.
	Auto,
	// Player-chosen target, kept until released, dead or out of range.
	Focus
};

/**
 * Player locomotion decisions that the animation blueprint reads but must not own:
 * aim target (Auto / Focus), locomotion facing, and the Free / Locked reference frame.
 * Pivot / stop / start are derived by UYogAnimInstance from CharacterMovement alone; nothing here gates input.
 *
 * Disabled by default (bEnableLocomotion). While disabled it never touches the character,
 * so existing movement behaviour is unchanged.
 *
 * See Docs/05_完成记录与任务看板/PlayerLocomotion_Design.md.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UPlayerLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerLocomotionComponent();

	// --- Master switch ---------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
	bool bEnableLocomotion = false;

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetLocomotionEnabled(bool bEnabled);

	// --- Aim target ------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Aim")
	EAimTargetMode AimMode = EAimTargetMode::Auto;

	/** Auto mode picks targets inside this radius (cm). Also the candidate pool for Focus lock / cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Aim", meta = (ClampMin = "50.0"))
	float AimAcquireRadius = 600.f;

	/** A held target is dropped beyond this radius (cm). Keep it larger than the acquire radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Aim", meta = (ClampMin = "50.0"))
	float AimReleaseRadius = 800.f;

	/** Auto mode only switches to a different target when it is at least this much closer (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Aim", meta = (ClampMin = "0.0"))
	float AimRetargetMargin = 100.f;

	/** Seconds between Auto target evaluations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Aim", meta = (ClampMin = "0.05"))
	float AimUpdateInterval = 0.15f;

	/** Focus mode: when the locked target is lost, fall back to Auto picking instead of dropping to no target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Aim")
	bool bFocusFallsBackToAuto = false;

	/** Degrees per second the body turns toward the aim target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Aim", meta = (ClampMin = "1.0"))
	float AimTurnRateYaw = 720.f;

	UFUNCTION(BlueprintCallable, Category = "Locomotion|Aim")
	void SetAimMode(EAimTargetMode NewMode);

	/** Switches to Focus mode and locks the candidate closest to the facing direction. False if none in range. */
	UFUNCTION(BlueprintCallable, Category = "Locomotion|Aim")
	bool LockFocusTarget();

	/** Clears the aim target. In Focus mode the player stays unlocked until the next lock. */
	UFUNCTION(BlueprintCallable, Category = "Locomotion|Aim")
	void ReleaseFocusTarget();

	/** Switches to Focus mode and moves the lock to the next candidate, clockwise around the player. */
	UFUNCTION(BlueprintCallable, Category = "Locomotion|Aim")
	bool CycleFocusTarget();

	UFUNCTION(BlueprintPure, Category = "Locomotion|Aim")
	AActor* GetAimTarget() const { return AimTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Locomotion|Aim")
	bool HasAimTarget() const { return AimTarget.IsValid(); }

	/** Signed yaw (degrees, positive = right) from the current facing to the aim target. 0 with no target. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Aim")
	float GetAimYawToTarget() const;

	// --- Mode ------------------------------------------------------------------------------

	/**
	 * World yaw of "screen up" when no player camera manager is available yet (first frames, tests).
	 * Matches the -45 degree input rotation in AYogPlayerControllerBase::Move.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion|Mode")
	float FallbackViewYaw = -45.f;

	/** Free without an aim target, Locked with one. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Mode")
	ELocoMode GetLocoMode() const;

	/** World yaw of "screen up" from the active player camera. The Free mode reference frame. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Mode")
	float GetViewYaw() const;

	/** World yaw that directional values are measured against: the view in Free mode, the body in Locked mode. */
	UFUNCTION(BlueprintPure, Category = "Locomotion|Mode")
	float GetLocoFrameYaw() const;

	// --- State read by the animation instance ----------------------------------------------

	/** True while an ability, stun, death, a hit reaction or a UI block owns movement and facing. */
	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool IsLocomotionSuspended() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool IsAbilityOwningFacing() const;
	bool IsTargetStillValid(const AActor& Target) const;
	void UpdateAimTarget(float NowSeconds);
	void SetFacingOverride(bool bActive);
	void TickAimFacing(float DeltaTime, const AActor& Target);
	void ResetRuntimeState();
	AActor* PickCandidateClosestToFacing(const TArray<AActor*>& Candidates) const;
	float WorldTimeSeconds() const;

	TWeakObjectPtr<APlayerCharacterBase> OwnerPlayer;
	TWeakObjectPtr<AActor> AimTarget;

	float NextAimUpdateTime = 0.f;

	// True once the player locked a Focus target, so losing it can be told apart from never having locked one.
	bool bFocusLockHeld = false;

	bool bFacingOverrideActive = false;
	bool bCachedOrientToMovement = true;
};
