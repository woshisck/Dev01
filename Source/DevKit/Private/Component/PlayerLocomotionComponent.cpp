// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/PlayerLocomotionComponent.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Animation/ANS_AutoTarget.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Character/YogPlayerControllerBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

static bool PlayerLocomotion_HasMovementOwningTag(const UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return false;
	}

	// Attacks and skills own Character.State.Skill, dash owns Character.State.Movement.Dash. The StateConflict
	// table does not map these to Block.Movement, so the tags are the only reliable signal.
	static const FGameplayTag TAG_Skill = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill"), false);
	static const FGameplayTag TAG_Dash = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash"), false);

	return (TAG_Skill.IsValid() && ASC->HasMatchingGameplayTag(TAG_Skill))
		|| (TAG_Dash.IsValid() && ASC->HasMatchingGameplayTag(TAG_Dash));
}

UPlayerLocomotionComponent::UPlayerLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerPlayer = Cast<APlayerCharacterBase>(GetOwner());
}

void UPlayerLocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetRuntimeState();
	Super::EndPlay(EndPlayReason);
}

float UPlayerLocomotionComponent::WorldTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

// --- Master switch -------------------------------------------------------------------------

void UPlayerLocomotionComponent::SetLocomotionEnabled(bool bEnabled)
{
	bEnableLocomotion = bEnabled;
	if (!bEnabled)
	{
		ResetRuntimeState();
	}
}

void UPlayerLocomotionComponent::ResetRuntimeState()
{
	SetFacingOverride(false);
	AimTarget.Reset();
	bFocusLockHeld = false;
	NextAimUpdateTime = 0.f;
}

// --- Mode ----------------------------------------------------------------------------------

ELocoMode UPlayerLocomotionComponent::GetLocoMode() const
{
	return AimTarget.IsValid() ? ELocoMode::Locked : ELocoMode::Free;
}

float UPlayerLocomotionComponent::GetViewYaw() const
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	const APlayerController* PC = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
	if (PC && PC->PlayerCameraManager)
	{
		return YogLocomotion::ViewYawFromCameraRotation(PC->PlayerCameraManager->GetCameraRotation());
	}
	return FallbackViewYaw;
}

float UPlayerLocomotionComponent::GetLocoFrameYaw() const
{
	if (GetLocoMode() == ELocoMode::Free)
	{
		return GetViewYaw();
	}
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	return Owner ? Owner->GetActorRotation().Yaw : 0.f;
}

// --- Aim target ----------------------------------------------------------------------------

void UPlayerLocomotionComponent::SetAimMode(EAimTargetMode NewMode)
{
	AimMode = NewMode;
	NextAimUpdateTime = 0.f;
	if (NewMode == EAimTargetMode::Auto)
	{
		bFocusLockHeld = false;
	}
}

bool UPlayerLocomotionComponent::IsTargetStillValid(const AActor& Target) const
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	if (!Owner || !IsValid(&Target))
	{
		return false;
	}

	if (const AYogCharacterBase* TargetCharacter = Cast<AYogCharacterBase>(&Target))
	{
		if (!TargetCharacter->IsAlive())
		{
			return false;
		}
	}

	return FVector::DistSquared(Owner->GetActorLocation(), Target.GetActorLocation()) <= FMath::Square(AimReleaseRadius);
}

AActor* UPlayerLocomotionComponent::PickCandidateClosestToFacing(const TArray<AActor*>& Candidates) const
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	if (!Owner)
	{
		return nullptr;
	}

	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();

	AActor* Best = nullptr;
	float BestDot = -2.f;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* Candidate : Candidates)
	{
		const FVector ToCandidate = Candidate->GetActorLocation() - Origin;
		const float Dot = FVector::DotProduct(Forward, ToCandidate.GetSafeNormal2D());
		const float DistSq = ToCandidate.SizeSquared();

		const bool bBetterAngle = Dot > BestDot + KINDA_SMALL_NUMBER;
		const bool bSameAngleCloser = FMath::IsNearlyEqual(Dot, BestDot, KINDA_SMALL_NUMBER) && DistSq < BestDistSq;
		if (bBetterAngle || bSameAngleCloser)
		{
			Best = Candidate;
			BestDot = Dot;
			BestDistSq = DistSq;
		}
	}

	return Best;
}

void UPlayerLocomotionComponent::UpdateAimTarget(float NowSeconds)
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	if (!Owner)
	{
		return;
	}

	AActor* Current = AimTarget.Get();
	if (Current && !IsTargetStillValid(*Current))
	{
		AimTarget.Reset();
		Current = nullptr;
	}

	if (AimMode == EAimTargetMode::Focus)
	{
		if (Current)
		{
			return;
		}

		// A lock that existed and is now gone. Without a lock yet (never pressed) nothing is "lost".
		if (bFocusLockHeld)
		{
			bFocusLockHeld = false;
			if (bFocusFallsBackToAuto)
			{
				AimMode = EAimTargetMode::Auto;
			}
		}

		if (AimMode == EAimTargetMode::Focus)
		{
			return;
		}
	}

	if (NowSeconds < NextAimUpdateTime)
	{
		return;
	}
	NextAimUpdateTime = NowSeconds + AimUpdateInterval;

	TArray<AActor*> Candidates;
	UANS_AutoTarget::GatherHostileCandidates(Owner, AimAcquireRadius, Candidates);

	const FVector Origin = Owner->GetActorLocation();
	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (AActor* Candidate : Candidates)
	{
		const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}

	if (!Nearest)
	{
		return;
	}

	if (!Current)
	{
		AimTarget = Nearest;
		return;
	}

	if (Nearest == Current)
	{
		return;
	}

	// Stickiness: a different target only wins when it is clearly closer.
	const float CurrentDist = FVector::Dist(Origin, Current->GetActorLocation());
	const float NearestDist = FMath::Sqrt(NearestDistSq);
	if (NearestDist + AimRetargetMargin < CurrentDist)
	{
		AimTarget = Nearest;
	}
}

bool UPlayerLocomotionComponent::LockFocusTarget()
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	if (!Owner)
	{
		return false;
	}

	TArray<AActor*> Candidates;
	UANS_AutoTarget::GatherHostileCandidates(Owner, AimAcquireRadius, Candidates);

	AActor* Target = PickCandidateClosestToFacing(Candidates);
	if (!Target)
	{
		return false;
	}

	AimMode = EAimTargetMode::Focus;
	AimTarget = Target;
	bFocusLockHeld = true;
	return true;
}

void UPlayerLocomotionComponent::ReleaseFocusTarget()
{
	AimTarget.Reset();
	bFocusLockHeld = false;
}

bool UPlayerLocomotionComponent::CycleFocusTarget()
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	if (!Owner)
	{
		return false;
	}

	AActor* Current = AimTarget.Get();
	if (!Current)
	{
		return LockFocusTarget();
	}

	TArray<AActor*> Candidates;
	UANS_AutoTarget::GatherHostileCandidates(Owner, AimAcquireRadius, Candidates);
	if (Candidates.IsEmpty())
	{
		return false;
	}

	const FVector Origin = Owner->GetActorLocation();
	Candidates.Sort([Origin](const AActor& A, const AActor& B)
	{
		const FVector DirA = A.GetActorLocation() - Origin;
		const FVector DirB = B.GetActorLocation() - Origin;
		return FMath::Atan2(DirA.Y, DirA.X) < FMath::Atan2(DirB.Y, DirB.X);
	});

	// The current target may sit between the acquire and release radius and be missing from the list.
	const int32 CurrentIndex = Candidates.IndexOfByKey(Current);
	const int32 NextIndex = CurrentIndex == INDEX_NONE ? 0 : (CurrentIndex + 1) % Candidates.Num();

	AimMode = EAimTargetMode::Focus;
	AimTarget = Candidates[NextIndex];
	bFocusLockHeld = true;
	return true;
}

float UPlayerLocomotionComponent::GetAimYawToTarget() const
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	const AActor* Target = AimTarget.Get();
	if (!Owner || !Target)
	{
		return 0.f;
	}

	const FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	if (ToTarget.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	return FMath::UnwindDegrees(ToTarget.Rotation().Yaw - Owner->GetActorRotation().Yaw);
}

// --- Suspend rules -------------------------------------------------------------------------

bool UPlayerLocomotionComponent::IsAbilityOwningFacing() const
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	return Owner && PlayerLocomotion_HasMovementOwningTag(Owner->GetASC());
}

bool UPlayerLocomotionComponent::IsLocomotionSuspended() const
{
	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	if (!Owner)
	{
		return true;
	}

	if (Owner->bIsDead || Owner->IsStunned() || Owner->IsWaitingForDeathReviveChoice() || !Owner->bMovable)
	{
		return true;
	}

	if (const UCharacterMovementComponent* Movement = Owner->GetCharacterMovement())
	{
		// Knockback, stun and death disable movement outright.
		if (Movement->MovementMode == MOVE_None)
		{
			return true;
		}
	}

	// Covers conversation mode and open menus.
	if (const AYogPlayerControllerBase* Controller = Cast<AYogPlayerControllerBase>(Owner->GetController()))
	{
		if (Controller->IsGameplayInputBlocked())
		{
			return true;
		}
	}

	return IsAbilityOwningFacing();
}

// --- Facing --------------------------------------------------------------------------------

void UPlayerLocomotionComponent::SetFacingOverride(bool bActive)
{
	if (bActive == bFacingOverrideActive)
	{
		return;
	}

	const APlayerCharacterBase* Owner = OwnerPlayer.Get();
	UCharacterMovementComponent* Movement = Owner ? Owner->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		bFacingOverrideActive = false;
		return;
	}

	if (bActive)
	{
		bCachedOrientToMovement = Movement->bOrientRotationToMovement;
		Movement->bOrientRotationToMovement = false;
	}
	else
	{
		Movement->bOrientRotationToMovement = bCachedOrientToMovement;
	}
	bFacingOverrideActive = bActive;
}

void UPlayerLocomotionComponent::TickAimFacing(float DeltaTime, const AActor& Target)
{
	APlayerCharacterBase* Owner = OwnerPlayer.Get();
	if (!Owner)
	{
		return;
	}

	const FVector ToTarget = Target.GetActorLocation() - Owner->GetActorLocation();
	if (ToTarget.SizeSquared2D() <= 1.f)
	{
		return;
	}

	const float DesiredYaw = ToTarget.Rotation().Yaw;
	const float NewYaw = FMath::FixedTurn(Owner->GetActorRotation().Yaw, DesiredYaw, AimTurnRateYaw * DeltaTime);
	Owner->SetActorRotation(FRotator(0.f, NewYaw, 0.f));
}

// --- Tick ----------------------------------------------------------------------------------

void UPlayerLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerPlayer.IsValid())
	{
		return;
	}

	if (!bEnableLocomotion)
	{
		if (bFacingOverrideActive || AimTarget.IsValid() || bFocusLockHeld)
		{
			ResetRuntimeState();
		}
		return;
	}

	UpdateAimTarget(WorldTimeSeconds());

	AActor* Target = AimTarget.Get();
	const bool bWantFacing = Target && !IsLocomotionSuspended();
	SetFacingOverride(bWantFacing);
	if (bWantFacing)
	{
		TickAimFacing(DeltaTime, *Target);
	}
}

// --- Debug console commands ----------------------------------------------------------------
// Focus-mode input mapping is deferred, so these drive the component for testing.

#if !UE_BUILD_SHIPPING
static UPlayerLocomotionComponent* PlayerLocomotion_GetDebugComponent(UWorld* World)
{
	const APlayerCharacterBase* Player = World ? Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	return Player ? Player->LocomotionComponent.Get() : nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs GYogLocoEnableCommand(
	TEXT("Yog.Loco.Enable"),
	TEXT("Yog.Loco.Enable [0|1] - turns the player locomotion component on or off (default 1)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UPlayerLocomotionComponent* Locomotion = PlayerLocomotion_GetDebugComponent(World))
		{
			Locomotion->SetLocomotionEnabled(Args.IsEmpty() || Args[0] != TEXT("0"));
		}
	}));

static FAutoConsoleCommandWithWorld GYogLocoLockCommand(
	TEXT("Yog.Loco.Lock"),
	TEXT("Locks the Focus target closest to the player's facing."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UPlayerLocomotionComponent* Locomotion = PlayerLocomotion_GetDebugComponent(World))
		{
			Locomotion->LockFocusTarget();
		}
	}));

static FAutoConsoleCommandWithWorld GYogLocoCycleCommand(
	TEXT("Yog.Loco.Cycle"),
	TEXT("Moves the Focus lock to the next candidate."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UPlayerLocomotionComponent* Locomotion = PlayerLocomotion_GetDebugComponent(World))
		{
			Locomotion->CycleFocusTarget();
		}
	}));

static FAutoConsoleCommandWithWorld GYogLocoReleaseCommand(
	TEXT("Yog.Loco.Release"),
	TEXT("Releases the Focus target."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UPlayerLocomotionComponent* Locomotion = PlayerLocomotion_GetDebugComponent(World))
		{
			Locomotion->ReleaseFocusTarget();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GYogLocoAutoCommand(
	TEXT("Yog.Loco.Auto"),
	TEXT("Switches to Auto aim mode."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		if (UPlayerLocomotionComponent* Locomotion = PlayerLocomotion_GetDebugComponent(World))
		{
			Locomotion->SetAimMode(EAimTargetMode::Auto);
		}
	}));
#endif
