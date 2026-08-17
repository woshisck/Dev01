#include "RVT/DevKitRVTSurfaceInstanceActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "RVT/DevKitRVTSurfaceAsset.h"
#include "VT/RuntimeVirtualTexture.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

ADevKitRVTSurfaceInstanceActor::ADevKitRVTSurfaceInstanceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);

	InstanceComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SurfaceInstances"));
	InstanceComponent->SetupAttachment(SceneRoot);
	InstanceComponent->SetMobility(EComponentMobility::Static);
	InstanceComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	InstanceComponent->SetCastShadow(false);
	InstanceComponent->bEditableWhenInherited = true;
	InstanceComponent->bSelectable = true;
	InstanceComponent->bHasPerInstanceHitProxies = true;
	InstanceComponent->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;
}

void ADevKitRVTSurfaceInstanceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySurfaceAsset();
}

void ADevKitRVTSurfaceInstanceActor::BeginPlay()
{
	Super::BeginPlay();
	// Serialized component state may have been authored under another quality tier. Resolve it
	// again after DeviceProfiles and saved graphics settings have been applied.
	ApplySurfaceAsset();
}

#if WITH_EDITOR
void ADevKitRVTSurfaceInstanceActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplySurfaceAsset();
}
#endif

void ADevKitRVTSurfaceInstanceActor::ApplySurfaceAsset()
{
	if (!InstanceComponent)
	{
		return;
	}

	// ApplySurfaceAsset is also called during construction/load. Only record the component when an
	// editor transaction is actively authoring a change; otherwise construction would dirty or add
	// transaction state to every loaded controller.
#if WITH_EDITOR
	if (GUndo
		&& !GIsTransacting
		&& !InstanceComponent->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		InstanceComponent->Modify();
	}
#endif

	InstanceComponent->bSelectable = true;
	InstanceComponent->bHasPerInstanceHitProxies = true;

	if (!SurfaceAsset)
	{
		// Clearing the asset in Details must not leave a stale mesh/RVT writer or collision proxy in
		// the level. Keep the component selectable so a new asset can be assigned normally.
		InstanceComponent->SetStaticMesh(nullptr);
		InstanceComponent->EmptyOverrideMaterials();
		InstanceComponent->RuntimeVirtualTextures.Reset();
		InstanceComponent->VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;
		InstanceComponent->VirtualTextureCullMips = 0;
		InstanceComponent->TranslucencySortPriority = 0;
		InstanceComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		InstanceComponent->SetGenerateOverlapEvents(false);
		InstanceComponent->SetCanEverAffectNavigation(false);
		InstanceComponent->SetCastShadow(false);
		InstanceComponent->bCastDynamicShadow = false;
		InstanceComponent->bCastStaticShadow = false;
		InstanceComponent->bCastContactShadow = false;
		InstanceComponent->SetVisibleInRayTracing(false);
		InstanceComponent->MarkRenderStateDirty();
		return;
	}

	InstanceComponent->SetStaticMesh(SurfaceAsset->Mesh);
	InstanceComponent->EmptyOverrideMaterials();

	if (SurfaceAsset->Mesh && SurfaceAsset->Material)
	{
		const int32 MaterialSlotCount = FMath::Max(1, SurfaceAsset->Mesh->GetStaticMaterials().Num());
		for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
		{
			InstanceComponent->SetMaterial(SlotIndex, SurfaceAsset->Material);
		}
	}

	InstanceComponent->RuntimeVirtualTextures.Reset();
	if (SurfaceRVT)
	{
		InstanceComponent->RuntimeVirtualTextures.Add(SurfaceRVT);
	}
	if (SurfaceAsset->bBindWorldHeight && HeightRVT)
	{
		InstanceComponent->RuntimeVirtualTextures.AddUnique(HeightRVT);
	}

	const bool bVisibleObject = SurfaceAsset->AssetType == EDevKitRVTSurfaceAssetType::VisibleObject;
	const UWorld* World = GetWorld();
	const bool bEvaluateRuntimeQuality = World && World->IsGameWorld();
	const IConsoleVariable* BatchProxyPreference =
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.Yog.BatchProxyPreference"));
	const IConsoleVariable* ForceProjectionOnly =
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.Yog.RVTSurface.ForceProjectionOnly"));

	bool bRenderSourceGeometry = ShouldRenderSourceGeometry(
		SurfaceAsset->AssetType,
		SurfaceAsset->GeometryPolicy,
		bEvaluateRuntimeQuality && BatchProxyPreference && BatchProxyPreference->GetInt() != 0,
		bEvaluateRuntimeQuality && ForceProjectionOnly && ForceProjectionOnly->GetInt() != 0,
		SurfaceAsset->bEnableCollision);

	// Quality-scaled objects remain directly editable in the editor. Explicit RVTOnly objects and
	// plane decals still preview their projection there.
	if (!bEvaluateRuntimeQuality
		&& bVisibleObject
		&& SurfaceAsset->GeometryPolicy == EDevKitRVTSurfaceGeometryPolicy::QualityScaled)
	{
		bRenderSourceGeometry = true;
	}

	// A visible object must never disappear because a streamed level or incomplete setup omitted
	// its Surface RVT binding. Keep the source mesh as a safe fallback in that case.
	if (bVisibleObject && !bRenderSourceGeometry && !SurfaceRVT)
	{
		bRenderSourceGeometry = true;
	}

	InstanceComponent->VirtualTextureRenderPassType = bRenderSourceGeometry
		? ERuntimeVirtualTextureMainPassType::Always
		: ERuntimeVirtualTextureMainPassType::Exclusive;
	InstanceComponent->VirtualTextureCullMips = 0;
	InstanceComponent->TranslucencySortPriority = SurfaceAsset->Priority;

	// Gameplay collision must never depend on a client's visual-quality setting. The policy resolver
	// keeps collision-bearing visible objects in the source-geometry path on every platform/tier.
	const bool bEnableCollision = bVisibleObject && SurfaceAsset->bEnableCollision;
	InstanceComponent->SetCollisionProfileName(
		bEnableCollision
			? UCollisionProfile::BlockAll_ProfileName
			: UCollisionProfile::NoCollision_ProfileName);
	InstanceComponent->SetGenerateOverlapEvents(false);
	InstanceComponent->SetCanEverAffectNavigation(bEnableCollision);

	const bool bCastShadow = bVisibleObject && bRenderSourceGeometry && SurfaceAsset->bCastShadow;
	InstanceComponent->SetCastShadow(bCastShadow);
	InstanceComponent->bCastDynamicShadow = bCastShadow;
	InstanceComponent->bCastStaticShadow = bCastShadow;
	InstanceComponent->bCastContactShadow = bCastShadow;
	InstanceComponent->SetVisibleInRayTracing(bVisibleObject && bRenderSourceGeometry);
	InstanceComponent->MarkRenderStateDirty();
}

bool ADevKitRVTSurfaceInstanceActor::InitializeSurfaceAsset(UDevKitRVTSurfaceAsset* InSurfaceAsset)
{
	if (!IsValid(InSurfaceAsset) || !InstanceComponent)
	{
		return false;
	}

	if (SurfaceAsset != InSurfaceAsset && InstanceComponent->GetInstanceCount() > 0)
	{
		return false;
	}

#if WITH_EDITOR
	Modify();
#endif
	SurfaceAsset = InSurfaceAsset;
	ApplySurfaceAsset();
#if WITH_EDITOR
	MarkPackageDirty();
#endif
	return true;
}

bool ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
	const EDevKitRVTSurfaceAssetType AssetType,
	const EDevKitRVTSurfaceGeometryPolicy GeometryPolicy,
	const bool bPreferBatchedGeometryProxy,
	const bool bForceProjectionOnly,
	const bool bRequiresGameplayCollision)
{
	if (AssetType == EDevKitRVTSurfaceAssetType::PlaneDecal)
	{
		return false;
	}
	if (bRequiresGameplayCollision)
	{
		return true;
	}

	switch (GeometryPolicy)
	{
	case EDevKitRVTSurfaceGeometryPolicy::RVTOnly:
		return false;
	case EDevKitRVTSurfaceGeometryPolicy::QualityScaled:
		return !bForceProjectionOnly && !bPreferBatchedGeometryProxy;
	case EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible:
	case EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault:
	default:
		return true;
	}
}

int32 ADevKitRVTSurfaceInstanceActor::AddSurfaceInstance(const FTransform& WorldTransform)
{
	if (!InstanceComponent)
	{
		return INDEX_NONE;
	}

	InstanceComponent->Modify();
	return InstanceComponent->AddInstance(WorldTransform, true);
}

void ADevKitRVTSurfaceInstanceActor::ClearSurfaceInstances()
{
	if (!InstanceComponent)
	{
		return;
	}

	InstanceComponent->Modify();
	InstanceComponent->ClearInstances();
}

int32 ADevKitRVTSurfaceInstanceActor::GetSurfaceInstanceCount() const
{
	return InstanceComponent ? InstanceComponent->GetInstanceCount() : 0;
}
