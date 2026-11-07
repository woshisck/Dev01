// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/YogCharacterMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/YogRootMotionSource_CurveMotion.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"


//UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_MovementStopped, "Gameplay.MovementStopped");

UYogCharacterMovementComponent::UYogCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FRotator UYogCharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	//if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	//{
	//	if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
	//	{
	//		return FRotator(0, 0, 0);
	//	}
	//}

	return Super::GetDeltaRotation(DeltaTime);
}

float UYogCharacterMovementComponent::GetMaxSpeed() const
{
	//if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	//{
	//	if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
	//	{
	//		return 0;
	//	}
	//}
	return Super::GetMaxSpeed();
}

static const FYogRootMotionSource_CurveMotion* YogMovement_AsCurveMotion(const TSharedPtr<FRootMotionSource>& Source)
{
	if (!Source.IsValid() || Source->GetScriptStruct() != FYogRootMotionSource_CurveMotion::StaticStruct())
	{
		return nullptr;
	}

	if (Source->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval)
		|| Source->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		return nullptr;
	}

	return static_cast<const FYogRootMotionSource_CurveMotion*>(Source.Get());
}

bool UYogCharacterMovementComponent::HasCurveMotion(bool bRotationOnly) const
{
	// Pending sources count: a notify that fired this frame owns facing before its first movement tick.
	for (const TArray<TSharedPtr<FRootMotionSource>>* List : { &CurrentRootMotion.RootMotionSources, &CurrentRootMotion.PendingAddRootMotionSources })
	{
		for (const TSharedPtr<FRootMotionSource>& Source : *List)
		{
			const FYogRootMotionSource_CurveMotion* CurveSource = YogMovement_AsCurveMotion(Source);
			if (CurveSource && (!bRotationOnly || CurveSource->DrivesRotation()))
			{
				return true;
			}
		}
	}
	return false;
}

void UYogCharacterMovementComponent::CancelSequenceCurveMotion()
{
	for (const TArray<TSharedPtr<FRootMotionSource>>* List : { &CurrentRootMotion.RootMotionSources, &CurrentRootMotion.PendingAddRootMotionSources })
	{
		for (const TSharedPtr<FRootMotionSource>& Source : *List)
		{
			const FYogRootMotionSource_CurveMotion* CurveSource = YogMovement_AsCurveMotion(Source);
			if (CurveSource && CurveSource->bFromSequence)
			{
				Source->Status.SetFlag(ERootMotionSourceStatusFlags::MarkedForRemoval);
			}
		}
	}
}

void UYogCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	// Runs inside PerformMovement, so curve yaw is recorded in saved moves and replayed on correction
	// exactly like translation. See FYogRootMotionSource_CurveMotion for why it is not routed through
	// the engine's override-rotation path.
	bool bHasCurveRotation = false;
	float DeltaYaw = 0.f;
	for (const TSharedPtr<FRootMotionSource>& Source : CurrentRootMotion.RootMotionSources)
	{
		const FYogRootMotionSource_CurveMotion* CurveSource = YogMovement_AsCurveMotion(Source);
		if (!CurveSource || !CurveSource->DrivesRotation())
		{
			continue;
		}

		bHasCurveRotation = true;
		DeltaYaw += CurveSource->PreparedDeltaYaw;
	}

	// While a curve owns yaw, orient-to-movement / controller rotation would fight it.
	if (!bHasCurveRotation)
	{
		Super::PhysicsRotation(DeltaTime);
		return;
	}

	if (FMath::IsNearlyZero(DeltaYaw) || !HasValidData())
	{
		return;
	}

	const FQuat DeltaQuat(FVector::UpVector, FMath::DegreesToRadians(DeltaYaw));
	MoveUpdatedComponent(FVector::ZeroVector, DeltaQuat * UpdatedComponent->GetComponentQuat(), false);
}

const FYogGroundInfo& UYogCharacterMovementComponent::GetGroundInfo()
{
	
	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	return CachedGroundInfo;

	// TODO: insert return statement here
}


void UYogCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}
