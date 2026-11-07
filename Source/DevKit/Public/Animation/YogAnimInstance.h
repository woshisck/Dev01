// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "Animation/PlayerLocomotionMath.h"


#include "YogAnimInstance.generated.h"


class UAbilitySystemComponent;

/**
 * 
 */
UCLASS(Config = Game)
class DEVKIT_API UYogAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UYogAnimInstance(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

protected:


	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	// Gameplay tags that can be mapped to blueprint variables. The variables will automatically update as the tags are added or removed.
	// These should be used instead of manually querying for the gameplay tags.

	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float GroundDistance = -1.0f;

	// --- Player locomotion (see PlayerLocomotion_Design.md) ---------------------------------
	// All Loco-prefixed so they cannot collide with the Blueprint variables inherited by
	// ABP_YogCharacter_Base (Speed, HasVelocity, LocalVelocityDirectionAngle, ...).
	// Written only in NativeThreadSafeUpdateAnimation. Stay at defaults for non-player owners.

	/**
	 * Idle / Starting / Moving / Pivoting / Stopping, derived only from CharacterMovement velocity + acceleration.
	 * Pivoting: acceleration opposes velocity at speed, until velocity reaches zero or flips -> Starting.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	ELocoMotionState LocoMotionState = ELocoMotionState::Idle;

	/** Starting, Moving or Pivoting (input is held). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bLocoIsMoving = false;

	/** LocoMotionState == Stopping. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bLocoIsStopping = false;

	/**
	 * Free: no aim target, every directional value below is relative to the camera view (Front = screen up).
	 * Locked: aim target held, values are relative to the body, which faces the target.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	ELocoMode LocoMode = ELocoMode::Free;

	/** Velocity direction relative to the LocoMode frame, -180..180. Holds its last value at rest. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LocoMoveAngle = 0.f;

	/** Live cardinal with dead-zone hysteresis. Used by Loop. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	ELocoCardinal LocoMoveCardinal = ELocoCardinal::Front;

	/** Acceleration angle captured on entering Starting / Moving from Idle, Stopping or Pivoting, so the Start clip is chosen once. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LocoStartAngle = 0.f;

	/** Cardinal captured when input is released, so the Stop clip is chosen once. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	ELocoCardinal LocoStopCardinal = ELocoCardinal::Front;

	/** Aim lock active: use the strafe set instead of plain run. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bLocoHasAimTarget = false;

	/** Signed yaw from facing to the aim target, for an optional upper-body / head aim layer. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LocoAimYawToTarget = 0.f;

	/** LocoMotionState == Pivoting. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bLocoIsPivoting = false;

	/** Pivot clip family, chosen once on entering Pivoting from the pre-reversal cardinal. Holds its last value. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	ELocoPivotType LocoPivotType = ELocoPivotType::None;

	/** An ability, stun or death owns movement and facing. The ABP stays out of Start / Stop / Pivot. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bLocoInLock = false;

	/** Hysteresis band added to each cardinal edge (degrees). */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0", ClampMax = "44.0"))
	float LocoCardinalDeadZone = 15.f;

	/** Horizontal speed (cm/s) below which the character counts as not moving. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0"))
	float LocoMinMoveSpeed = 3.f;

	/** Acceleration within this many degrees of directly opposite the velocity starts a pivot. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Pivot", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float LocoPivotAngleTolerance = 45.f;

	/** A pivot only starts above this fraction of max speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Pivot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LocoPivotMinSpeedRatio = 0.5f;

	/** Starting becomes Moving at this fraction of max speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LocoStartToLoopSpeedRatio = 0.9f;

private:
	// Plain-data copy of everything the locomotion math reads from actors and components.
	// Filled on the game thread, consumed on the worker thread.
	struct FLocoInputSnapshot
	{
		bool bValid = false;
		bool bLocomotionOn = false;
		bool bSuspended = false;
		bool bHasAimTarget = false;
		bool bMovingOnGround = false;
		ELocoMode Mode = ELocoMode::Free;
		float AimYawToTarget = 0.f;
		float MaxSpeed = 0.f;
		FVector Velocity = FVector::ZeroVector;
		FVector Acceleration = FVector::ZeroVector;
		// Yaw-only frame the directional values are measured in (view in Free mode, body in Locked mode).
		FRotator FrameRotation = FRotator::ZeroRotator;
	};

	void GatherLocomotionInputs(const class APlayerCharacterBase& Player);
	void UpdateLocomotionState_ThreadSafe();

	FLocoInputSnapshot LocoInput;

protected:


	UFUNCTION()
	void DebugAnimationLayers();

};
