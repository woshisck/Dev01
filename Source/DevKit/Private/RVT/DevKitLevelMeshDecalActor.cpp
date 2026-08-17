#include "RVT/DevKitLevelMeshDecalActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogDevKitLevelMeshDecalActor, Log, All);

ADevKitLevelMeshDecalActor::ADevKitLevelMeshDecalActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bEnableAutoLODGeneration = false;
}

void ADevKitLevelMeshDecalActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCullingPolicy();
}

void ADevKitLevelMeshDecalActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyCullingPolicy();
}

#if WITH_EDITOR
void ADevKitLevelMeshDecalActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyCullingPolicy();
}
#endif

void ADevKitLevelMeshDecalActor::ApplyCullingPolicy()
{
	UBoxComponent* BoundsProxy = FindBoundsProxy();
	const FVector EffectiveExtent(
		FMath::Max(1.0, FMath::Abs(DesiredBoundsExtent.X)),
		FMath::Max(1.0, FMath::Abs(DesiredBoundsExtent.Y)),
		FMath::Max(1.0, FMath::Abs(DesiredBoundsExtent.Z)));

	if (BoundsProxy)
	{
		bLoggedMissingBoundsProxy = false;
		BoundsProxy->SetBoxExtent(EffectiveExtent, false);
		BoundsProxy->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		BoundsProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoundsProxy->SetGenerateOverlapEvents(false);
		BoundsProxy->SetCanEverAffectNavigation(false);
		BoundsProxy->SetCastShadow(false);
		BoundsProxy->SetVisibility(false);
		BoundsProxy->SetHiddenInGame(true);
		BoundsProxy->bUseAttachParentBound = false;
		BoundsProxy->UpdateBounds();
	}
	else if (!bLoggedMissingBoundsProxy)
	{
		UE_LOG(
			LogDevKitLevelMeshDecalActor,
			Warning,
			TEXT("Level mesh decal actor '%s' has no UBoxComponent named '%s' or tagged '%s'. "
				"Add a Blueprint BoundsProxy or enable the BoundsScale fallback."),
			*GetPathName(),
			*BoundsProxyName.ToString(),
			*BoundsProxyTag.ToString());
		bLoggedMissingBoundsProxy = true;
	}

	TInlineComponentArray<UInstancedStaticMeshComponent*> OwnedInstanceMeshes(this);
	for (UInstancedStaticMeshComponent* InstanceComponent : OwnedInstanceMeshes)
	{
		ApplyPolicyToInstanceComponent(InstanceComponent, BoundsProxy);
	}
}

void ADevKitLevelMeshDecalActor::ApplyPolicyToInstanceComponent(
	UInstancedStaticMeshComponent* InstanceComponent,
	UBoxComponent* BoundsProxy)
{
	if (!InstanceComponent)
	{
		return;
	}

	const bool bUsesSharedBounds = BoundsProxy && InstanceComponent->IsAttachedTo(BoundsProxy);
	if (!bUsesSharedBounds && !bUseBoundsScaleFallback)
	{
		return;
	}

	InstanceComponent->SetCullDistances(0, 0);
	InstanceComponent->InstanceMinDrawDistance = 0;
	InstanceComponent->MinDrawDistance = 0.0f;
	InstanceComponent->LDMaxDrawDistance = 0.0f;
	InstanceComponent->bNeverDistanceCull = true;
	InstanceComponent->bAllowCullDistanceVolume = false;
	InstanceComponent->bEnableAutoLODGeneration = false;
	InstanceComponent->SetCullDistance(0.0f);
	InstanceComponent->SetCachedMaxDrawDistance(0.0f);

	if (bUsesSharedBounds)
	{
		// Every intermediate scene component must forward its parent bound; setting the leaf alone
		// would stop at the first wrapper introduced by an artist-authored Blueprint hierarchy.
		RouteComponentBoundsThroughProxy(InstanceComponent, BoundsProxy);
	}
	else
	{
		InstanceComponent->bUseAttachParentBound = false;
		InstanceComponent->SetBoundsScale(FMath::Max(1.0f, FallbackBoundsScale));
	}
	InstanceComponent->UpdateBounds();
	InstanceComponent->MarkRenderStateDirty();
}

UBoxComponent* ADevKitLevelMeshDecalActor::FindBoundsProxy() const
{
	TInlineComponentArray<UBoxComponent*> BoxComponents(this);
	for (UBoxComponent* BoxComponent : BoxComponents)
	{
		if (!BoxComponent)
		{
			continue;
		}

		const FString ComponentName = BoxComponent->GetName();
		const FString DesiredName = BoundsProxyName.ToString();
		if (BoxComponent->GetFName() == BoundsProxyName
			|| (!DesiredName.IsEmpty()
				&& ComponentName.StartsWith(DesiredName + TEXT("_GEN_VARIABLE"))))
		{
			return BoxComponent;
		}
	}

	if (!BoundsProxyTag.IsNone())
	{
		for (UBoxComponent* BoxComponent : BoxComponents)
		{
			if (BoxComponent && BoxComponent->ComponentHasTag(BoundsProxyTag))
			{
				return BoxComponent;
			}
		}
	}

	return nullptr;
}

void ADevKitLevelMeshDecalActor::RouteComponentBoundsThroughProxy(
	UInstancedStaticMeshComponent* InstanceComponent,
	UBoxComponent* BoundsProxy)
{
	TArray<USceneComponent*, TInlineAllocator<8>> BoundRoute;
	USceneComponent* CurrentComponent = InstanceComponent;
	while (CurrentComponent && CurrentComponent != BoundsProxy)
	{
		CurrentComponent->bUseAttachParentBound = true;
		BoundRoute.Add(CurrentComponent);
		CurrentComponent = CurrentComponent->GetAttachParent();
	}

	// Parent bounds must be current before a child copies them. Walk from the proxy's direct child
	// back down to the ISM leaf so arbitrary artist-authored wrapper depth remains deterministic.
	BoundsProxy->UpdateBounds();
	for (int32 Index = BoundRoute.Num() - 1; Index >= 0; --Index)
	{
		BoundRoute[Index]->UpdateBounds();
	}
}
