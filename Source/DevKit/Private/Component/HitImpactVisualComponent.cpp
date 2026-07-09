#include "Component/HitImpactVisualComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UHitImpactVisualComponent::UHitImpactVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHitImpactVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(false);
}

void UHitImpactVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearVisualOffset();

	Super::EndPlay(EndPlayReason);
}

void UHitImpactVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHitPushActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float SafeOutDuration = FMath::Max(0.001f, PushOutDuration);
	const float SafeReturnDuration = FMath::Max(0.001f, ReturnDuration);

	ElapsedTime += FMath::Max(0.0f, DeltaTime);

	if (ElapsedTime <= SafeOutDuration)
	{
		const float Alpha = FMath::Clamp(ElapsedTime / SafeOutDuration, 0.0f, 1.0f);
		const float EaseAlpha = 1.0f - FMath::Square(1.0f - Alpha);
		ApplyVisualOffset(FMath::Lerp(StartOffset, PeakOffset, EaseAlpha));
		return;
	}

	const float ReturnAlpha = FMath::Clamp((ElapsedTime - SafeOutDuration) / SafeReturnDuration, 0.0f, 1.0f);
	const float EaseAlpha = ReturnAlpha * ReturnAlpha * (3.0f - 2.0f * ReturnAlpha);
	ApplyVisualOffset(FMath::Lerp(PeakOffset, FVector::ZeroVector, EaseAlpha));

	if (ReturnAlpha >= 1.0f)
	{
		ClearVisualOffset();
	}
}

void UHitImpactVisualComponent::PlayHitPush(AActor* SourceActor, float Strength)
{
	if (!SourceActor)
	{
		return;
	}

	PlayHitPushFromLocation(SourceActor->GetActorLocation(), Strength);
}

void UHitImpactVisualComponent::PlayHitPushFromLocation(FVector SourceLocation, float Strength)
{
	AActor* Owner = GetOwner();
	USkeletalMeshComponent* Mesh = ResolveMesh();
	if (!bEnableHitPush || !Owner || !Mesh)
	{
		return;
	}

	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.0f;
	if (bHitPushActive && MinRefreshInterval > 0.0f && Now - LastPushStartTime < MinRefreshInterval)
	{
		return;
	}

	FVector PushDirection = Owner->GetActorLocation() - SourceLocation;
	PushDirection.Z = 0.0f;
	if (!PushDirection.Normalize())
	{
		PushDirection = -Owner->GetActorForwardVector();
		PushDirection.Z = 0.0f;
		if (!PushDirection.Normalize())
		{
			return;
		}
	}

	const float Distance = FMath::Max(0.0f, PushDistance * FMath::Max(0.0f, Strength));
	if (Distance <= KINDA_SMALL_NUMBER && FMath::IsNearlyZero(VerticalLift))
	{
		return;
	}

	const FVector WorldOffset = PushDirection * Distance + FVector::UpVector * VerticalLift;
	const FVector LocalOffset = ConvertWorldOffsetToMeshParentSpace(Mesh, WorldOffset);
	const float MaxDistance = FMath::Max(0.0f, MaxAccumulatedPushDistance);

	StartOffset = AppliedOffset;
	PeakOffset = MaxDistance > 0.0f
		? (AppliedOffset + LocalOffset).GetClampedToMaxSize(MaxDistance)
		: FVector::ZeroVector;

	ElapsedTime = 0.0f;
	LastPushStartTime = Now;
	bHitPushActive = true;
	SetComponentTickEnabled(true);

	if (PushOutDuration <= 0.0f)
	{
		ApplyVisualOffset(PeakOffset);
	}
}

void UHitImpactVisualComponent::StopHitPush()
{
	ClearVisualOffset();
}

USkeletalMeshComponent* UHitImpactVisualComponent::ResolveMesh() const
{
	if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		return CharacterOwner->GetMesh();
	}

	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

FVector UHitImpactVisualComponent::ConvertWorldOffsetToMeshParentSpace(const USkeletalMeshComponent* Mesh, const FVector& WorldOffset) const
{
	if (!Mesh)
	{
		return FVector::ZeroVector;
	}

	if (const USceneComponent* Parent = Mesh->GetAttachParent())
	{
		return Parent->GetComponentTransform().InverseTransformVectorNoScale(WorldOffset);
	}

	return Mesh->GetComponentTransform().InverseTransformVectorNoScale(WorldOffset);
}

void UHitImpactVisualComponent::ApplyVisualOffset(const FVector& NewOffset)
{
	USkeletalMeshComponent* Mesh = ResolveMesh();
	if (!Mesh)
	{
		AppliedOffset = FVector::ZeroVector;
		return;
	}

	const FVector DeltaOffset = NewOffset - AppliedOffset;
	if (!DeltaOffset.IsNearlyZero())
	{
		Mesh->SetRelativeLocation(Mesh->GetRelativeLocation() + DeltaOffset, false, nullptr, ETeleportType::TeleportPhysics);
		AppliedOffset = NewOffset;
	}
}

void UHitImpactVisualComponent::ClearVisualOffset()
{
	ApplyVisualOffset(FVector::ZeroVector);

	AppliedOffset = FVector::ZeroVector;
	StartOffset = FVector::ZeroVector;
	PeakOffset = FVector::ZeroVector;
	ElapsedTime = 0.0f;
	bHitPushActive = false;
	SetComponentTickEnabled(false);
}
