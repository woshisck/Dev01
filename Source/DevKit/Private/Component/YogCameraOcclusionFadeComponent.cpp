// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/YogCameraOcclusionFadeComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

UYogCameraOcclusionFadeComponent::UYogCameraOcclusionFadeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UYogCameraOcclusionFadeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFadeInterpolation(DeltaTime);

	if (!bEnableOcclusionFade)
	{
		TimeSinceLastTrace = 0.0f;
		MarkAllTargetsVisible();
		return;
	}

	TimeSinceLastTrace += DeltaTime;
	if (TimeSinceLastTrace >= FMath::Max(0.0f, TraceInterval))
	{
		TimeSinceLastTrace = 0.0f;
		RunOcclusionTrace();
	}
}

void UYogCameraOcclusionFadeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreAllMaterials();
	Super::EndPlay(EndPlayReason);
}

void UYogCameraOcclusionFadeComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UYogCameraOcclusionFadeComponent* This = CastChecked<UYogCameraOcclusionFadeComponent>(InThis);
	for (TPair<TObjectKey<UPrimitiveComponent>, FYogOcclusionFadeTarget>& Pair : This->FadeTargets)
	{
		FYogOcclusionFadeTarget& Target = Pair.Value;
		Collector.AddReferencedObjects(Target.OriginalMaterials);
		Collector.AddReferencedObjects(Target.DynamicMaterials);
	}

	Super::AddReferencedObjects(InThis, Collector);
}

bool UYogCameraOcclusionFadeComponent::IsFadeAllowedForComponent(const UPrimitiveComponent* Component) const
{
	if (!Component || Component->bHiddenInGame)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	AActor* ComponentOwner = Component->GetOwner();
	if (!ComponentOwner || ComponentOwner == Owner)
	{
		return false;
	}

	const bool bComponentHasTag = !OccluderTag.IsNone() && Component->ComponentHasTag(OccluderTag);
	const bool bActorHasTag = !OccluderTag.IsNone() && ComponentOwner->ActorHasTag(OccluderTag);
	const bool bHasOccluderTag = bComponentHasTag || bActorHasTag;

	if (Owner && ComponentOwner->IsAttachedTo(Owner) && !bHasOccluderTag)
	{
		return false;
	}

	if (bOnlyFadeTaggedOccluders && !bHasOccluderTag)
	{
		return false;
	}

	return Component->GetNumMaterials() > 0;
}

void UYogCameraOcclusionFadeComponent::RunOcclusionTrace()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	APlayerCameraManager* CameraManager = GetOwnerCameraManager();
	if (!World || !Owner || !CameraManager)
	{
		MarkAllTargetsVisible();
		return;
	}

	MarkAllTargetsVisible();

	const FVector Start = CameraManager->GetCameraLocation();
	const FVector End = Owner->GetActorLocation() + PlayerTargetOffset;
	const float SafeTraceRadius = FMath::Max(0.0f, TraceRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(YogCameraOcclusionFade), false, Owner);
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(Owner);

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(SafeTraceRadius),
		QueryParams);

	bool bHitFadeAllowedOccluder = false;
	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (IsFadeAllowedForComponent(HitComponent))
		{
			bHitFadeAllowedOccluder = true;
			AddOrUpdateFadeTarget(HitComponent);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugOcclusionTrace)
	{
		const FColor TraceColor = bHitFadeAllowedOccluder ? FColor::Orange : FColor::Green;
		const FColor BlockedHitColor = FColor::Red;
		const FColor IgnoredHitColor = FColor::Silver;
		const float Duration = FMath::Max(0.0f, DebugDrawDuration);

		DrawDebugLine(World, Start, End, TraceColor, false, Duration, 0, 1.5f);
		DrawDebugSphere(World, Start, SafeTraceRadius, 12, FColor::Blue, false, Duration);
		DrawDebugSphere(World, End, SafeTraceRadius, 12, TraceColor, false, Duration);

		for (const FHitResult& Hit : Hits)
		{
			const UPrimitiveComponent* HitComponent = Hit.GetComponent();
			const bool bFadeAllowed = IsFadeAllowedForComponent(HitComponent);
			DrawDebugSphere(
				World,
				Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint,
				FMath::Max(8.0f, SafeTraceRadius * 0.5f),
				8,
				bFadeAllowed ? BlockedHitColor : IgnoredHitColor,
				false,
				Duration);
		}
	}
#endif
}

void UYogCameraOcclusionFadeComponent::UpdateFadeInterpolation(float DeltaTime)
{
	TArray<TObjectKey<UPrimitiveComponent>> TargetsToRemove;

	for (TPair<TObjectKey<UPrimitiveComponent>, FYogOcclusionFadeTarget>& Pair : FadeTargets)
	{
		FYogOcclusionFadeTarget& Target = Pair.Value;
		UPrimitiveComponent* Component = Target.Component.Get();
		if (!Component)
		{
			TargetsToRemove.Add(Pair.Key);
			continue;
		}

		const float Speed = Target.TargetAlpha < Target.CurrentAlpha ? FadeOutSpeed : FadeInSpeed;
		Target.CurrentAlpha = FMath::FInterpTo(Target.CurrentAlpha, Target.TargetAlpha, DeltaTime, FMath::Max(0.01f, Speed));
		ApplyAlphaToTarget(Target);

		if (FMath::IsNearlyEqual(Target.TargetAlpha, 1.0f, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Target.CurrentAlpha, 1.0f, 0.01f))
		{
			RestoreTargetMaterials(Target);
			TargetsToRemove.Add(Pair.Key);
		}
	}

	for (const TObjectKey<UPrimitiveComponent>& TargetKey : TargetsToRemove)
	{
		FadeTargets.Remove(TargetKey);
	}
}

void UYogCameraOcclusionFadeComponent::MarkAllTargetsVisible()
{
	for (TPair<TObjectKey<UPrimitiveComponent>, FYogOcclusionFadeTarget>& Pair : FadeTargets)
	{
		Pair.Value.TargetAlpha = 1.0f;
		Pair.Value.bHitThisTrace = false;
	}
}

void UYogCameraOcclusionFadeComponent::AddOrUpdateFadeTarget(UPrimitiveComponent* Component)
{
	if (!Component)
	{
		return;
	}

	FYogOcclusionFadeTarget& Target = FadeTargets.FindOrAdd(TObjectKey<UPrimitiveComponent>(Component));
	if (!Target.Component.IsValid())
	{
		Target.Component = Component;
		Target.CurrentAlpha = 1.0f;
		CacheMaterialsForTarget(Target, Component);
	}

	Target.TargetAlpha = FMath::Clamp(MinVisibleAlpha, 0.0f, 1.0f);
	Target.bHitThisTrace = true;
	ApplyAlphaToTarget(Target);
}

void UYogCameraOcclusionFadeComponent::CacheMaterialsForTarget(FYogOcclusionFadeTarget& Target, UPrimitiveComponent* Component)
{
	if (!Component)
	{
		return;
	}

	const int32 MaterialCount = Component->GetNumMaterials();
	Target.OriginalMaterials.Reset(MaterialCount);
	Target.DynamicMaterials.Reset(MaterialCount);

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		Target.OriginalMaterials.Add(Component->GetMaterial(MaterialIndex));

		UMaterialInterface* SourceMaterial = OcclusionFadeMaterial ? OcclusionFadeMaterial.Get() : nullptr;
		UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(MaterialIndex, SourceMaterial);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(FadeScalarParameterName, Target.CurrentAlpha);
		}
		Target.DynamicMaterials.Add(DynamicMaterial);
	}
}

void UYogCameraOcclusionFadeComponent::ApplyAlphaToTarget(FYogOcclusionFadeTarget& Target)
{
	for (UMaterialInstanceDynamic* DynamicMaterial : Target.DynamicMaterials)
	{
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(FadeScalarParameterName, Target.CurrentAlpha);
		}
	}
}

void UYogCameraOcclusionFadeComponent::RestoreTargetMaterials(FYogOcclusionFadeTarget& Target)
{
	UPrimitiveComponent* Component = Target.Component.Get();
	if (!Component)
	{
		return;
	}

	for (int32 MaterialIndex = 0; MaterialIndex < Target.OriginalMaterials.Num(); ++MaterialIndex)
	{
		Component->SetMaterial(MaterialIndex, Target.OriginalMaterials[MaterialIndex]);
	}
}

void UYogCameraOcclusionFadeComponent::RestoreAllMaterials()
{
	for (TPair<TObjectKey<UPrimitiveComponent>, FYogOcclusionFadeTarget>& Pair : FadeTargets)
	{
		RestoreTargetMaterials(Pair.Value);
	}

	FadeTargets.Reset();
}

APlayerCameraManager* UYogCameraOcclusionFadeComponent::GetOwnerCameraManager() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			return PlayerController->PlayerCameraManager;
		}
	}

	return UGameplayStatics::GetPlayerCameraManager(this, 0);
}
