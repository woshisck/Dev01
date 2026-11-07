// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/YogAnimInstance.h"
#include "Character/YogCharacterBase.h"
#include "Character/YogCharacterMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/PlayerLocomotionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UYogAnimInstance::UYogAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UYogAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	GameplayTagPropertyMap.Initialize(this, ASC);

}


void UYogAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void UYogAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	LocoInput.bValid = false;

	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetOwningActor());
	if (!Character)
	{
		return;
	}
	//[Sunchuankai] Update Ground info from movementComponent;
	//UYogCharacterMovementComponent* CharMoveComp = CastChecked<UYogCharacterMovementComponent>(Character->GetCharacterMovement());
	//const FLyraCharacterGroundInfo& GroundInfo = CharMoveComp->GetGroundInfo();
	//GroundDistance = GroundInfo.GroundDistance;

	if (const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Character))
	{
		GatherLocomotionInputs(*Player);
	}
}

void UYogAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	UpdateLocomotionState_ThreadSafe();
}

void UYogAnimInstance::GatherLocomotionInputs(const APlayerCharacterBase& Player)
{
	const UCharacterMovementComponent* Movement = Player.GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	const UPlayerLocomotionComponent* Locomotion = Player.FindComponentByClass<UPlayerLocomotionComponent>();
	const bool bLocomotionOn = Locomotion && Locomotion->bEnableLocomotion;

	LocoInput.bValid = true;
	LocoInput.bLocomotionOn = bLocomotionOn;
	LocoInput.bSuspended = bLocomotionOn && Locomotion->IsLocomotionSuspended();
	LocoInput.bHasAimTarget = bLocomotionOn && Locomotion->HasAimTarget();
	LocoInput.AimYawToTarget = LocoInput.bHasAimTarget ? Locomotion->GetAimYawToTarget() : 0.f;
	LocoInput.Mode = bLocomotionOn ? Locomotion->GetLocoMode() : ELocoMode::Locked;
	LocoInput.Velocity = Movement->Velocity;
	LocoInput.Acceleration = Movement->GetCurrentAcceleration();
	LocoInput.MaxSpeed = Movement->GetMaxSpeed();
	LocoInput.bMovingOnGround = Movement->IsMovingOnGround();
	LocoInput.FrameRotation = bLocomotionOn
		? FRotator(0.f, Locomotion->GetLocoFrameYaw(), 0.f)
		: Player.GetActorRotation();
}

void UYogAnimInstance::UpdateLocomotionState_ThreadSafe()
{
	if (!LocoInput.bValid)
	{
		return;
	}

	const FVector Velocity2D = LocoInput.Velocity * FVector(1.f, 1.f, 0.f);
	const float Speed2D = Velocity2D.Size();

	bLocoInLock = LocoInput.bSuspended;
	bLocoHasAimTarget = LocoInput.bHasAimTarget;
	LocoAimYawToTarget = LocoInput.AimYawToTarget;
	LocoMode = LocoInput.Mode;

	// The cardinal is read before the state step so a pivot is labelled with the pre-reversal direction.
	// It holds at rest so the Stop clip can still be picked from the last valid direction.
	if (Speed2D > LocoMinMoveSpeed)
	{
		LocoMoveAngle = YogLocomotion::CalculateMoveAngle(Velocity2D, LocoInput.FrameRotation);
		LocoMoveCardinal = YogLocomotion::AngleToCardinal(LocoMoveAngle, LocoMoveCardinal, LocoCardinalDeadZone);
	}

	FLocoMotionTuning Tuning;
	Tuning.MinMoveSpeed = LocoMinMoveSpeed;
	Tuning.PivotAngleToleranceDeg = LocoPivotAngleTolerance;
	Tuning.PivotMinSpeedRatio = LocoPivotMinSpeedRatio;
	Tuning.StartToLoopSpeedRatio = LocoStartToLoopSpeedRatio;

	const bool bAllowPivot = LocoInput.bMovingOnGround && !LocoInput.bSuspended;
	const ELocoMotionState Previous = LocoMotionState;
	LocoMotionState = YogLocomotion::NextMotionState(Previous, Velocity2D, LocoInput.Acceleration,
	                                                 LocoInput.MaxSpeed, bAllowPivot, Tuning);

	if (LocoMotionState != Previous)
	{
		switch (LocoMotionState)
		{
		case ELocoMotionState::Pivoting:
			LocoPivotType = YogLocomotion::ChoosePivotType(LocoMoveCardinal);
			break;
		case ELocoMotionState::Starting:
		case ELocoMotionState::Moving:
			// Leaving Idle, Stopping or Pivoting: velocity may still be near zero or pointing the old way,
			// so read the new direction from acceleration.
			if (Previous == ELocoMotionState::Idle || Previous == ELocoMotionState::Stopping || Previous == ELocoMotionState::Pivoting)
			{
				LocoStartAngle = YogLocomotion::CalculateMoveAngle(LocoInput.Acceleration, LocoInput.FrameRotation);
			}
			break;
		case ELocoMotionState::Stopping:
			LocoStopCardinal = LocoMoveCardinal;
			break;
		case ELocoMotionState::Idle:
		default:
			break;
		}
	}

	bLocoIsMoving = LocoMotionState == ELocoMotionState::Starting
		|| LocoMotionState == ELocoMotionState::Moving
		|| LocoMotionState == ELocoMotionState::Pivoting;
	bLocoIsStopping = LocoMotionState == ELocoMotionState::Stopping;
	bLocoIsPivoting = LocoMotionState == ELocoMotionState::Pivoting;
}

void UYogAnimInstance::DebugAnimationLayers()
{

}
