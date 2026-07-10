#include "Animation/ANS_AttackMotionWarpRedirect.h"

#include "Character/PlayerCharacterBase.h"
#include "Character/YogPlayerControllerBase.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_SkewWarp.h"

UANS_AttackMotionWarpRedirect::UANS_AttackMotionWarpRedirect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (URootMotionModifier_Warp* WarpModifier = Cast<URootMotionModifier_Warp>(RootMotionModifier))
	{
		WarpModifier->WarpTargetName = WarpTargetName;
		WarpModifier->bWarpTranslation = true;
	}
}

URootMotionModifier* UANS_AttackMotionWarpRedirect::AddRootMotionModifier_Implementation(
	UMotionWarpingComponent* MotionWarpingComp,
	const UAnimSequenceBase* Animation,
	float StartTime,
	float EndTime) const
{
	RefreshRedirectTarget(MotionWarpingComp);
	URootMotionModifier* Modifier = Super::AddRootMotionModifier_Implementation(MotionWarpingComp, Animation, StartTime, EndTime);
	if (URootMotionModifier_Warp* WarpModifier = Cast<URootMotionModifier_Warp>(Modifier))
	{
		WarpModifier->WarpTargetName = WarpTargetName;
	}
	return Modifier;
}

FString UANS_AttackMotionWarpRedirect::GetNotifyName_Implementation() const
{
	return TEXT("Attack Motion Warp Redirect");
}

bool UANS_AttackMotionWarpRedirect::RefreshRedirectTarget(UMotionWarpingComponent* MotionWarpingComp) const
{
	if (!MotionWarpingComp || WarpTargetName.IsNone())
	{
		return false;
	}

	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(MotionWarpingComp->GetOwner());
	if (!PlayerCharacter)
	{
		return false;
	}

	FVector AttackDirection = FVector::ZeroVector;
	if (!ResolveAttackDirection(PlayerCharacter, AttackDirection))
	{
		return false;
	}

	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	const FRotator TargetRotation(0.f, AttackDirection.Rotation().Yaw, 0.f);
	FVector TargetLocation = PlayerLocation;

	const AYogPlayerControllerBase* PlayerController = Cast<AYogPlayerControllerBase>(PlayerCharacter->GetController());
	FVector MoveDirection = FVector::ZeroVector;
	const bool bShouldRedirect =
		bEnableRedirect
		&& RedirectDistance > 0.f
		&& PlayerController
		&& PlayerController->TryGetAttackMotionWarpRedirectMoveDirection(MoveDirection, InputGraceTime)
		&& DirectionsMatch(MoveDirection, AttackDirection, MaxAngleDegrees);

	if (bShouldRedirect)
	{
		TargetLocation += AttackDirection * RedirectDistance;
	}

	MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(WarpTargetName, FTransform(TargetRotation, TargetLocation));
	return true;
}

bool UANS_AttackMotionWarpRedirect::ResolveAttackDirection(const APlayerCharacterBase* PlayerCharacter, FVector& OutDirection) const
{
	OutDirection = FVector::ZeroVector;
	if (!PlayerCharacter)
	{
		return false;
	}

	const UMotionWarpingComponent* MotionWarpingComp = PlayerCharacter->MotionWarpingComponent;
	if (MotionWarpingComp)
	{
		if (const FMotionWarpingTarget* ExistingTarget = MotionWarpingComp->FindWarpTarget(WarpTargetName))
		{
			OutDirection = ExistingTarget->Rotator().Vector();
		}
	}

	if (OutDirection.IsNearlyZero())
	{
		OutDirection = PlayerCharacter->LastInputDirection;
	}

	if (OutDirection.IsNearlyZero())
	{
		OutDirection = PlayerCharacter->GetActorForwardVector();
	}

	OutDirection.Z = 0.f;
	if (OutDirection.IsNearlyZero())
	{
		return false;
	}

	OutDirection.Normalize();
	return true;
}

bool UANS_AttackMotionWarpRedirect::DirectionsMatch(const FVector& MoveDirection, const FVector& AttackDirection, const float MaxAngle)
{
	FVector MoveDirection2D = MoveDirection;
	MoveDirection2D.Z = 0.f;
	if (MoveDirection2D.IsNearlyZero())
	{
		return false;
	}
	MoveDirection2D.Normalize();

	FVector AttackDirection2D = AttackDirection;
	AttackDirection2D.Z = 0.f;
	if (AttackDirection2D.IsNearlyZero())
	{
		return false;
	}
	AttackDirection2D.Normalize();

	const float DirectionDot = FVector::DotProduct(MoveDirection2D, AttackDirection2D);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(MaxAngle, 0.f, 180.f)));
	return DirectionDot >= MinDot;
}
