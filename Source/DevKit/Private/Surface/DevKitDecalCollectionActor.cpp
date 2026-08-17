#include "Surface/DevKitDecalCollectionActor.h"

#include "Components/SceneComponent.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#if WITH_EDITOR
#include "Engine/Engine.h"
#include "EngineUtils.h"
#endif
#include "HAL/IConsoleManager.h"
#include "Logging/MessageLog.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "RVT/DevKitRVTSurfaceInstanceActor.h"

namespace
{
	/** One derived ISM batch is identified by the source asset and an optional
	 * per-instance material override.  This keeps a preview override isolated
	 * to one instance instead of changing every instance in the source batch. */
	struct FDevKitDerivedBatchKey
	{
		UDevKitDecalAsset* Asset = nullptr;
		UMaterialInterface* MaterialOverride = nullptr;

		bool operator==(const FDevKitDerivedBatchKey& Other) const
		{
			return Asset == Other.Asset && MaterialOverride == Other.MaterialOverride;
		}
	};

	FORCEINLINE uint32 GetTypeHash(const FDevKitDerivedBatchKey& Key)
	{
		return HashCombine(GetTypeHash(Key.Asset), GetTypeHash(Key.MaterialOverride));
	}

	/**
	 * Editor-only preview switch for collection rendering.  It deliberately
	 * lives on the derived component rather than the authored asset: RVT writer
	 * materials can have no useful main-pass output in the editor/Game View.
	 * Runtime worlds never consult this switch.
	 */
	static TAutoConsoleVariable<int32> CVarDecalCollectionDebugGeometryMaterial(
		TEXT("r.Yog.DecalCollection.DebugGeometryMaterial"),
		0,
		TEXT("Use the engine default surface material for RVT visible-mesh collection editor preview. Disabled by default so artists inspect the authored material; runtime always keeps the authored RVT material."),
		ECVF_Default);

#if WITH_EDITOR
	static void RefreshCollectionsAfterDiagnosticCVarChange()
	{
		static int32 LastValue = INDEX_NONE;
		const int32 CurrentValue = CVarDecalCollectionDebugGeometryMaterial.GetValueOnAnyThread();
		if (CurrentValue == LastValue)
		{
			return;
		}
		LastValue = CurrentValue;
		if (!GEngine)
		{
			return;
		}
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType != EWorldType::Editor || !WorldContext.World())
			{
				continue;
			}
			for (TActorIterator<ADevKitDecalCollectionActor> It(WorldContext.World()); It; ++It)
			{
				if (ADevKitDecalCollectionActor* CollectionActor = *It)
				{
					CollectionActor->RebuildDerivedRendering();
				}
			}
		}
	}

	static FAutoConsoleVariableSink DiagnosticCVarSink(
		FConsoleCommandDelegate::CreateStatic(&RefreshCollectionsAfterDiagnosticCVarChange));
#endif
}

UDevKitDecalCollectionComponent::UDevKitDecalCollectionComponent()
{
	SetIsReplicatedByDefault(false);
	SetFlags(RF_Transactional);
}

FGuid UDevKitDecalCollectionComponent::AddRecord(UDevKitDecalAsset* Asset, const FTransform& WorldTransform)
{
	if (!Asset || !Asset->IsValidDefinition())
	{
		return FGuid();
	}
	Modify();
	PaletteAssets.AddUnique(Asset);
	FDevKitDecalPlacementRecord& Record = Records.AddDefaulted_GetRef();
	Record.InstanceGuid = FGuid::NewGuid();
	Record.Asset = Asset;
	Record.Transform = WorldTransform;
	Record.DecalSize = Asset->DefaultDecalSize;
	return Record.InstanceGuid;
}

bool UDevKitDecalCollectionComponent::AddPaletteAsset(UDevKitDecalAsset* Asset)
{
	// Palette definitions may be created before their mesh/material are chosen.
	// Placement still calls IsValidDefinition(), so an incomplete card cannot
	// accidentally create an invalid ISM instance.
	if (!Asset || !Asset->DefinitionGuid.IsValid() || Asset->Backend == EDevKitDecalBackend::Unsupported)
	{
		return false;
	}

	if (PaletteAssets.Contains(Asset))
	{
		return true;
	}

	Modify();
	PaletteAssets.Add(Asset);
	return true;
}

FDevKitDecalPlacementRecord* UDevKitDecalCollectionComponent::FindRecord(const FGuid& InstanceGuid)
{
	return Records.FindByPredicate([&InstanceGuid](const FDevKitDecalPlacementRecord& Record)
	{
		return Record.InstanceGuid == InstanceGuid;
	});
}

const FDevKitDecalPlacementRecord* UDevKitDecalCollectionComponent::FindRecord(const FGuid& InstanceGuid) const
{
	return Records.FindByPredicate([&InstanceGuid](const FDevKitDecalPlacementRecord& Record)
	{
		return Record.InstanceGuid == InstanceGuid;
	});
}

bool UDevKitDecalCollectionComponent::RemoveRecord(const FGuid& InstanceGuid)
{
	const int32 Index = Records.IndexOfByPredicate([&InstanceGuid](const FDevKitDecalPlacementRecord& Record)
	{
		return Record.InstanceGuid == InstanceGuid;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Modify();
	Records.RemoveAt(Index);
	return true;
}

bool UDevKitDecalCollectionComponent::UpdateRecordTransform(const FGuid& InstanceGuid, const FTransform& WorldTransform)
{
	if (FDevKitDecalPlacementRecord* Record = FindRecord(InstanceGuid))
	{
		Modify();
		Record->Transform = WorldTransform;
		return true;
	}
	return false;
}

ADevKitDecalCollectionActor::ADevKitDecalCollectionActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetFlags(RF_Transactional);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);
	Collection = CreateDefaultSubobject<UDevKitDecalCollectionComponent>(TEXT("Collection"));
	CollectionGuid = FGuid::NewGuid();
}

void ADevKitDecalCollectionActor::RebuildDerivedRendering()
{
	if (Collection)
	{
		Collection->Modify();
	}
	ClearDerivedRendering();
	BuildDerivedRendering();
}

bool ADevKitDecalCollectionActor::ValidateCollection()
{
	if (!Collection || !CollectionGuid.IsValid())
	{
		return false;
	}
	for (const FDevKitDecalPlacementRecord& Record : Collection->Records)
	{
		if (!Record.IsValid())
		{
			return false;
		}
	}
	return true;
}

void ADevKitDecalCollectionActor::BeginEditSession()
{
	bEditSessionActive = true;
}

void ADevKitDecalCollectionActor::EndEditSession(bool /*bApply*/)
{
	// The session flag is transient. The editor mode owns snapshot/restore and the
	// normal transaction buffer; entering/leaving a mode must not dirty the map.
	bEditSessionActive = false;
}

void ADevKitDecalCollectionActor::SetDerivedInstanceEditingEnabled(bool bEnabled)
{
	for (UInstancedStaticMeshComponent* Component : DerivedRVTComponents)
	{
		if (!Component)
		{
			continue;
		}
		Component->bSelectable = bEnabled;
		// Keep the hit-proxy table resident even when the mode is not active.
		// The mode flag below still gates CanEdit/CanMove, while this avoids a
		// stale actor-level proxy after a rebuild or a mode re-entry.
		Component->bHasPerInstanceHitProxies = true;
		Component->bWantsEditorEffects = false;
		// bHasPerInstanceHitProxies is consumed while the component builds its
		// hit-proxy array. Re-register after changing the mode selection policy so
		// a component cannot retain an actor-level proxy from the previous state.
		Component->ReregisterComponent();
		UE_LOG(LogTemp, Display, TEXT("DecalCollection '%s' instance editing policy: enabled=%d selectable=%d per_instance_hit_proxies=%d registered=%d instances=%d editable_inherited=%d"),
			*GetName(), bEnabled ? 1 : 0, Component->bSelectable ? 1 : 0,
			Component->bHasPerInstanceHitProxies ? 1 : 0, Component->IsRegistered() ? 1 : 0,
			Component->GetInstanceCount(), Component->bEditableWhenInherited ? 1 : 0);
	}
}

bool ADevKitDecalCollectionActor::UpdateDerivedInstanceTransform(UInstancedStaticMeshComponent* Component, int32 InstanceIndex, const FTransform& WorldTransform)
{
	if (!Collection || !Component || InstanceIndex < 0)
	{
		return false;
	}

	const int32 ComponentIndex = DerivedRVTComponents.IndexOfByKey(Component);
	if (!DerivedInstanceGuids.IsValidIndex(ComponentIndex)
		|| !DerivedInstanceGuids[ComponentIndex].IsValidIndex(InstanceIndex))
	{
		return false;
	}

	const FGuid& RecordGuid = DerivedInstanceGuids[ComponentIndex][InstanceIndex];
	return Collection->UpdateRecordTransform(RecordGuid, WorldTransform);
}

bool ADevKitDecalCollectionActor::FindRecordForDerivedInstance(
	UInstancedStaticMeshComponent* Component,
	int32 InstanceIndex,
	FGuid& OutRecordGuid) const
{
	OutRecordGuid.Invalidate();
	const int32 ComponentIndex = DerivedRVTComponents.IndexOfByKey(Component);
	if (!DerivedInstanceGuids.IsValidIndex(ComponentIndex)
		|| !DerivedInstanceGuids[ComponentIndex].IsValidIndex(InstanceIndex))
	{
		return false;
	}

	OutRecordGuid = DerivedInstanceGuids[ComponentIndex][InstanceIndex];
	return OutRecordGuid.IsValid();
}

bool ADevKitDecalCollectionActor::FindRecordForDerivedDeferred(UDecalComponent* Component, FGuid& OutRecordGuid) const
{
	OutRecordGuid.Invalidate();
	const int32 ComponentIndex = DerivedDeferredComponents.IndexOfByKey(Component);
	if (!DerivedDeferredGuids.IsValidIndex(ComponentIndex))
	{
		return false;
	}

	OutRecordGuid = DerivedDeferredGuids[ComponentIndex];
	return OutRecordGuid.IsValid();
}

bool ADevKitDecalCollectionActor::UpdateDerivedDeferredTransform(UDecalComponent* Component, const FTransform& WorldTransform)
{
	FGuid RecordGuid;
	if (!Collection || !Component || !FindRecordForDerivedDeferred(Component, RecordGuid)
		|| !Collection->UpdateRecordTransform(RecordGuid, WorldTransform))
	{
		return false;
	}

	Component->SetWorldTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	return true;
}

bool ADevKitDecalCollectionActor::SetRecordMaterialOverride(const FGuid& InstanceGuid, UMaterialInterface* Material)
{
	if (!Collection)
	{
		return false;
	}
	if (FDevKitDecalPlacementRecord* Record = Collection->FindRecord(InstanceGuid))
	{
		Collection->Modify();
		Record->MaterialOverride = Material;
		return true;
	}
	return false;
}

bool ADevKitDecalCollectionActor::RebindRecordAsset(const FGuid& InstanceGuid, UDevKitDecalAsset* NewAsset)
{
	if (!Collection || !NewAsset || !NewAsset->IsValidDefinition())
	{
		return false;
	}
	if (FDevKitDecalPlacementRecord* Record = Collection->FindRecord(InstanceGuid))
	{
		Collection->Modify();
		Record->Asset = NewAsset;
		Record->MaterialOverride = nullptr;
		Collection->PaletteAssets.AddUnique(NewAsset);
		return true;
	}
	return false;
}

bool ADevKitDecalCollectionActor::AdoptDeferredDecal(UDecalComponent* SourceComponent, UDevKitDecalAsset* DeferredAsset, FGuid& OutRecordGuid)
{
	OutRecordGuid.Invalidate();
	if (!Collection || !SourceComponent || !DeferredAsset
		|| DeferredAsset->Backend != EDevKitDecalBackend::DeferredProjection
		|| !DeferredAsset->IsValidDefinition())
	{
		return false;
	}

	AActor* SourceActor = SourceComponent->GetOwner();
	if (!SourceActor || SourceActor == this || SourceActor->GetLevel() != GetLevel())
	{
		return false;
	}

	Modify();
	Collection->Modify();
	SourceActor->Modify();
	SourceComponent->Modify();
	OutRecordGuid = Collection->AddRecord(DeferredAsset, SourceComponent->GetComponentTransform());
	FDevKitDecalPlacementRecord* Record = Collection->FindRecord(OutRecordGuid);
	if (!Record)
	{
		OutRecordGuid.Invalidate();
		return false;
	}

	Record->DecalSize = SourceComponent->DecalSize;
	Record->MaterialOverride = SourceComponent->GetDecalMaterial();
	Record->SourceActorPath = SourceActor->GetPathName();
	Record->SourceComponentName = SourceComponent->GetFName();
	Record->bSourceHiddenForAdoption = true;
	Collection->AddPaletteAsset(DeferredAsset);
	SourceComponent->SetVisibility(false, true);
	SourceComponent->SetHiddenInGame(true, true);
	RebuildDerivedRendering();
	MarkPackageDirty();
	if (ULevel* OwnerLevel = GetLevel())
	{
		OwnerLevel->MarkPackageDirty();
	}
	return true;
}

bool ADevKitDecalCollectionActor::AdoptInstancedMesh(UInstancedStaticMeshComponent* SourceComponent, UDevKitDecalAsset* MeshAsset, int32& OutAdoptedCount)
{
	OutAdoptedCount = 0;
	if (!Collection || !SourceComponent || !MeshAsset || !MeshAsset->IsValidDefinition()
		|| MeshAsset->Backend == EDevKitDecalBackend::DeferredProjection
		|| MeshAsset->Backend == EDevKitDecalBackend::Unsupported)
	{
		return false;
	}

	AActor* SourceActor = SourceComponent->GetOwner();
	UStaticMesh* SourceMesh = SourceComponent->GetStaticMesh();
	UStaticMesh* AssetMesh = MeshAsset->Mesh ? MeshAsset->Mesh.Get() : (MeshAsset->LegacyRVTAsset ? MeshAsset->LegacyRVTAsset->Mesh.Get() : nullptr);
	const bool bAssetRequiresCollision = MeshAsset->bEnableCollision || (MeshAsset->LegacyRVTAsset && MeshAsset->LegacyRVTAsset->bEnableCollision);
	const bool bAssetCastsShadow = MeshAsset->bCastShadow || (MeshAsset->LegacyRVTAsset && MeshAsset->LegacyRVTAsset->bCastShadow);
	if (!SourceActor || SourceActor == this || SourceActor->GetLevel() != GetLevel() || !SourceMesh || SourceMesh != AssetMesh
		|| SourceComponent->GetInstanceCount() <= 0 || SourceComponent->NumCustomDataFloats != 0
		|| SourceComponent->GetCollisionProfileName() != UCollisionProfile::NoCollision_ProfileName
		|| SourceComponent->CanEverAffectNavigation() || bAssetRequiresCollision
		|| SourceComponent->CastShadow != bAssetCastsShadow)
	{
		return false;
	}

	// A single authored material record is currently the batch contract. Reject
	// multi-slot overrides rather than silently flattening a structurally richer mesh.
	const int32 MaterialSlotCount = SourceComponent->GetNumMaterials();
	if (MaterialSlotCount > 1)
	{
		return false;
	}
	TArray<FTransform> SourceTransforms;
	SourceTransforms.Reserve(SourceComponent->GetInstanceCount());
	for (int32 InstanceIndex = 0; InstanceIndex < SourceComponent->GetInstanceCount(); ++InstanceIndex)
	{
		FTransform WorldTransform;
		if (!SourceComponent->GetInstanceTransform(InstanceIndex, WorldTransform, true))
		{
			return false;
		}
		SourceTransforms.Add(WorldTransform);
	}

	Modify();
	Collection->Modify();
	SourceActor->Modify();
	SourceComponent->Modify();
	UMaterialInterface* SourceMaterial = SourceComponent->GetMaterial(0);
	for (const FTransform& WorldTransform : SourceTransforms)
	{
		const FGuid RecordGuid = Collection->AddRecord(MeshAsset, WorldTransform);
		FDevKitDecalPlacementRecord* Record = Collection->FindRecord(RecordGuid);
		if (!Record)
		{
			return false;
		}
		Record->MaterialOverride = SourceMaterial;
		Record->SourceActorPath = SourceActor->GetPathName();
		Record->SourceComponentName = SourceComponent->GetFName();
		Record->bSourceHiddenForAdoption = true;
		++OutAdoptedCount;
	}

	Collection->AddPaletteAsset(MeshAsset);
	SourceComponent->SetVisibility(false, true);
	SourceComponent->SetHiddenInGame(true, true);
	RebuildDerivedRendering();
	MarkPackageDirty();
	if (ULevel* OwnerLevel = GetLevel())
	{
		OwnerLevel->MarkPackageDirty();
	}
	return true;
}

bool ADevKitDecalCollectionActor::RestoreAdoptedSource(const FGuid& InstanceGuid)
{
	if (!Collection)
	{
		return false;
	}
	FDevKitDecalPlacementRecord* Record = Collection->FindRecord(InstanceGuid);
	if (!Record || !Record->bSourceHiddenForAdoption || Record->SourceActorPath.IsEmpty() || Record->SourceComponentName.IsNone())
	{
		return false;
	}

	UWorld* World = GetWorld();
	AActor* SourceActor = nullptr;
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (*It && (*It)->GetPathName() == Record->SourceActorPath)
			{
				SourceActor = *It;
				break;
			}
		}
	}
	UActorComponent* SourceComponent = nullptr;
	if (SourceActor)
	{
		TInlineComponentArray<UActorComponent*> Components(SourceActor);
		if (UActorComponent* const* FoundComponent = Components.FindByPredicate([Record](const UActorComponent* Component)
		{
			return Component && Component->GetFName() == Record->SourceComponentName;
		}))
		{
			SourceComponent = *FoundComponent;
		}
	}
	if (!SourceComponent)
	{
		return false;
	}

	Modify();
	Collection->Modify();
	SourceActor->Modify();
	SourceComponent->Modify();
	if (USceneComponent* SourceSceneComponent = Cast<USceneComponent>(SourceComponent))
	{
		SourceSceneComponent->SetVisibility(true, true);
		SourceSceneComponent->SetHiddenInGame(false, true);
	}
	else
	{
		return false;
	}
	for (FDevKitDecalPlacementRecord& Candidate : Collection->Records)
	{
		if (Candidate.bSourceHiddenForAdoption
			&& Candidate.SourceActorPath == Record->SourceActorPath
			&& Candidate.SourceComponentName == Record->SourceComponentName)
		{
			Candidate.bEnabled = false;
			Candidate.bSourceHiddenForAdoption = false;
		}
	}
	RebuildDerivedRendering();
	MarkPackageDirty();
	if (ULevel* OwnerLevel = GetLevel())
	{
		OwnerLevel->MarkPackageDirty();
	}
	return true;
}

void ADevKitDecalCollectionActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!CollectionGuid.IsValid())
	{
		CollectionGuid = FGuid::NewGuid();
	}
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		ClearDerivedRendering();
		BuildDerivedRendering();
	}
}

void ADevKitDecalCollectionActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Derived ISM components are intentionally transient and therefore are not
	// serialized into the map.  On a cold editor load (and on a PIE duplicate)
	// construction is not a sufficient guarantee that the render batch exists.
	// Rebuild after all serialized actor/components have been initialized so the
	// collection is visible in both the normal viewport and Game View (G).
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		ClearDerivedRendering();
		BuildDerivedRendering();
	}
}

void ADevKitDecalCollectionActor::BeginPlay()
{
	Super::BeginPlay();

	// PIE/game worlds can duplicate an editor actor with transient components
	// authored under editor-preview policy.  Re-evaluate the effective runtime
	// quality policy once more in the actual game world.
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		ClearDerivedRendering();
		BuildDerivedRendering();
	}
}

void ADevKitDecalCollectionActor::ClearDerivedRendering()
{
	TArray<UActorComponent*> ComponentsToRemove;
	for (UActorComponent* Component : GetComponents())
	{
		if (Component && Component->ComponentTags.Contains(TEXT("DevKit.DecalCollection.Derived")))
		{
			ComponentsToRemove.Add(Component);
		}
	}
	for (UActorComponent* Component : ComponentsToRemove)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	DerivedRVTComponents.Reset();
	DerivedInstanceGuids.Reset();
	DerivedDeferredComponents.Reset();
	DerivedDeferredGuids.Reset();
}

void ADevKitDecalCollectionActor::BuildDerivedRendering()
{
	if (!Collection)
	{
		return;
	}

	TMap<FDevKitDerivedBatchKey, TArray<const FDevKitDecalPlacementRecord*>> Groups;
	for (const FDevKitDecalPlacementRecord& Record : Collection->Records)
	{
		if (!Record.bEnabled || !Record.IsValid() || !Record.Asset)
		{
			continue;
		}
		if (Record.Asset->Backend == EDevKitDecalBackend::RVTPlane
			|| Record.Asset->Backend == EDevKitDecalBackend::RVTVisibleMesh
			|| Record.Asset->Backend == EDevKitDecalBackend::MeshDecal
			|| Record.Asset->Backend == EDevKitDecalBackend::StaticMeshOverlay)
		{
			Groups.FindOrAdd(FDevKitDerivedBatchKey{Record.Asset, Record.MaterialOverride}).Add(&Record);
		}
	}

	for (const TPair<FDevKitDerivedBatchKey, TArray<const FDevKitDecalPlacementRecord*>>& Pair : Groups)
	{
		UDevKitDecalAsset* Asset = Pair.Key.Asset;
		if (!Asset)
		{
			continue;
		}
		UStaticMesh* Mesh = Asset->Mesh ? Asset->Mesh : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Mesh : nullptr);
		UMaterialInterface* Material = Pair.Key.MaterialOverride;
		if (!Material)
		{
			Material = Asset->Material.Get();
			if (!Material && Asset->LegacyRVTAsset)
			{
				Material = Asset->LegacyRVTAsset->Material.Get();
			}
		}
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("Decal collection '%s' skipped mesh asset '%s': no mesh"), *GetName(), *Asset->GetName());
			continue;
		}

		const FString MaterialSuffix = Pair.Key.MaterialOverride
			? FString::Printf(TEXT("_%s"), *Pair.Key.MaterialOverride->GetName())
			: FString();
		const bool bRVTBackend = Asset->Backend == EDevKitDecalBackend::RVTPlane
			|| Asset->Backend == EDevKitDecalBackend::RVTVisibleMesh;
		const FName ComponentName = MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(),
			*FString::Printf(TEXT("Derived_%s_%s%s"), bRVTBackend ? TEXT("RVT") : TEXT("Mesh"), *Asset->GetName(), *MaterialSuffix));
		UDevKitDecalCollectionISMComponent* ISM = NewObject<UDevKitDecalCollectionISMComponent>(this, ComponentName, RF_Transient);
		ISM->CreationMethod = EComponentCreationMethod::Instance;
		ISM->ComponentTags.Add(TEXT("DevKit.DecalCollection.Derived"));
		ISM->SetMobility(EComponentMobility::Static);
		// Collection records can cover an entire level while the collection actor
		// itself stays at the origin.  Do not let the inherited mesh/actor cull
		// policy remove the whole derived batch when the camera moves away from
		// that origin (this is the same failure mode as the legacy decal BP).
		ISM->SetCullDistances(0, 0);
		ISM->MinDrawDistance = 0.f;
		ISM->LDMaxDrawDistance = 0.f;
		ISM->bNeverDistanceCull = true;
		ISM->bAllowCullDistanceVolume = false;
		// Build the per-instance hit-proxy table up front.  These components are
		// transient and are re-created by the actor, so toggling this flag only
		// after registration can leave the viewport with the actor-level proxy from
		// the previous render state.  The collection currently contains a bounded
		// authoring set (191 instances in the test level); keeping the table alive
		// makes entering the mode deterministic while CanEditSMInstance still gates
		// actual W/E/R mutation to the active edit session.
		ISM->bEditableWhenInherited = true;
		ISM->bSelectable = false;
		ISM->bHasPerInstanceHitProxies = true;
		ISM->bWantsEditorEffects = false;
		ISM->SetupAttachment(SceneRoot);
		ISM->SetStaticMesh(Mesh);
		const bool bEditorPreview = !GetWorld() || !GetWorld()->IsGameWorld();
		// RVT writer materials are optimized for the virtual-texture pass and may
		// have no useful main-pass output while inspecting the level or Game View
		// (G).  Keep the authored writer at runtime, but give visible-mesh records
		// a neutral editor preview material so the real mesh silhouette remains
		// visible and editable.
		const bool bUseDebugMaterial = bEditorPreview
			&& Asset->Backend == EDevKitDecalBackend::RVTVisibleMesh
			&& Pair.Key.MaterialOverride == nullptr
			&& CVarDecalCollectionDebugGeometryMaterial.GetValueOnGameThread() != 0;
		if (bUseDebugMaterial)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		if (Material)
		{
			const int32 MaterialSlotCount = FMath::Max(1, Mesh->GetStaticMaterials().Num());
			for (int32 Slot = 0; Slot < MaterialSlotCount; ++Slot)
			{
				ISM->SetMaterial(Slot, Material);
			}
		}
		ISM->RuntimeVirtualTextures.Reset();
		if (bRVTBackend && Asset->SurfaceRVT)
		{
			ISM->RuntimeVirtualTextures.Add(Asset->SurfaceRVT);
		}
		if (bRVTBackend && Asset->HeightRVT)
		{
			ISM->RuntimeVirtualTextures.AddUnique(Asset->HeightRVT);
		}
		bool bPreferBatchedGeometryProxy = false;
		bool bForceProjectionOnly = false;
		if (const IConsoleVariable* ProxyCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Yog.BatchProxyPreference")))
		{
			bPreferBatchedGeometryProxy = ProxyCVar->GetInt() != 0;
		}
		if (const IConsoleVariable* ForceCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Yog.RVTSurface.ForceProjectionOnly")))
		{
			bForceProjectionOnly = ForceCVar->GetInt() != 0;
		}
		const EDevKitRVTSurfaceAssetType LegacyAssetType = Asset->LegacyRVTAsset
			? Asset->LegacyRVTAsset->AssetType
			: (Asset->Backend == EDevKitDecalBackend::RVTVisibleMesh
				? EDevKitRVTSurfaceAssetType::VisibleObject
				: EDevKitRVTSurfaceAssetType::PlaneDecal);
		const EDevKitRVTSurfaceGeometryPolicy Policy = Asset->GeometryPolicy != EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault
			? Asset->GeometryPolicy
			: (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->GeometryPolicy : EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault);
		const bool bRequiresGameplayCollision = Asset->bEnableCollision
			|| (Asset->LegacyRVTAsset && Asset->LegacyRVTAsset->bEnableCollision);
		bool bKeepSourceGeometry = !bRVTBackend || ADevKitRVTSurfaceInstanceActor::ShouldRenderSourceGeometry(
			LegacyAssetType, Policy, bPreferBatchedGeometryProxy, bForceProjectionOnly, bRequiresGameplayCollision);
		// The collection's derived components are also the editor preview.  Do not
		// let a runtime QualityScaled/RVTOnly decision hide visible ground meshes
		// while the user is in Preview/Game View (G); the game-world policy still
		// applies when the level is actually played.
		if (bEditorPreview && LegacyAssetType == EDevKitRVTSurfaceAssetType::VisibleObject)
		{
			bKeepSourceGeometry = true;
		}
		ISM->VirtualTextureRenderPassType = bKeepSourceGeometry
			? ERuntimeVirtualTextureMainPassType::Always
			: ERuntimeVirtualTextureMainPassType::Exclusive;
		ISM->SetVisibility(true, false);
		ISM->SetHiddenInGame(false, false);
		ISM->bRenderInMainPass = true;
		const bool bEnableCollision = (Asset->Backend == EDevKitDecalBackend::RVTVisibleMesh || Asset->Backend == EDevKitDecalBackend::StaticMeshOverlay)
			&& bKeepSourceGeometry && bRequiresGameplayCollision;
		const bool bCastShadow = bKeepSourceGeometry && (Asset->bCastShadow
			|| (Asset->LegacyRVTAsset && Asset->LegacyRVTAsset->bCastShadow));
		ISM->SetCollisionProfileName(bEnableCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		ISM->SetCanEverAffectNavigation(bEnableCollision);
		ISM->SetCastShadow(bCastShadow);
		ISM->SetVisibleInRayTracing(bKeepSourceGeometry);
		AddInstanceComponent(ISM);
		// These components are transient and are created after the actor's
		// serialized component registration pass.  Register explicitly against
		// the owning world; RegisterComponent() alone can be a no-op during a
		// cold editor load/PIE duplication when the actor has not yet become the
		// component registration context.
		if (UWorld* OwnerWorld = GetWorld())
		{
			ISM->RegisterComponentWithWorld(OwnerWorld);
		}
		else
		{
			ISM->RegisterComponent();
		}
		if (!ISM->IsRegistered())
		{
			ISM->RegisterComponent();
		}
		UE_LOG(LogTemp, Display, TEXT("DecalCollection '%s' built '%s': registered=%d instances=%d mesh=%s material=%s mesh_slots=%d debug_material=%d"),
			*GetName(), *ISM->GetName(), ISM->IsRegistered() ? 1 : 0, Pair.Value.Num(),
			Mesh ? *Mesh->GetPathName() : TEXT("None"),
			Material ? *Material->GetPathName() : TEXT("None"),
			Mesh->GetStaticMaterials().Num(), bUseDebugMaterial ? 1 : 0);
		TArray<FGuid> RecordGuids;
		RecordGuids.Reserve(Pair.Value.Num());
		for (const FDevKitDecalPlacementRecord* Record : Pair.Value)
		{
			if (Record)
			{
				const int32 InstanceIndex = ISM->AddInstance(Record->Transform, true);
				RecordGuids.Add(Record->InstanceGuid);
				for (int32 DataIndex = 0; DataIndex < Record->CustomData.Num(); ++DataIndex)
				{
					ISM->SetCustomDataValue(InstanceIndex, DataIndex, Record->CustomData[DataIndex], false);
				}
			}
		}
		ISM->UpdateBounds();
		// The component is registered before instances are appended.  Recompute
		// both the render bounds and the scene proxy after the instance buffer has
		// been populated; otherwise a transient ISM can retain an empty/one-mesh
		// bounds and disappear in the normal viewport or Game View (G).
		ISM->UpdateBounds();
		ISM->MarkRenderStateDirty();
		// ISM scene proxies cache the instance render data at registration time.
		// This component is deliberately registered before adding instances so it
		// can resolve the owning editor world; reregister once after population to
		// force a fresh proxy/bounds in both the editor preview and a PIE duplicate.
		if (ISM->IsRegistered())
		{
			ISM->ReregisterComponent();
		}
		const FBoxSphereBounds& RenderBounds = ISM->Bounds;
		UE_LOG(LogTemp, Display, TEXT("DecalCollection '%s' render state '%s': registered=%d instances=%d bounds_origin=(%.1f,%.1f,%.1f) bounds_extent=(%.1f,%.1f,%.1f) actor_hidden=%d temp_hidden=%d editor_only=%d visible=%d hidden_game=%d main_pass=%d pass=%d keep_source=%d editor_preview=%d"),
			*GetName(), *ISM->GetName(), ISM->IsRegistered() ? 1 : 0, ISM->GetInstanceCount(),
			RenderBounds.Origin.X, RenderBounds.Origin.Y, RenderBounds.Origin.Z,
			RenderBounds.BoxExtent.X, RenderBounds.BoxExtent.Y, RenderBounds.BoxExtent.Z,
			IsHiddenEd() ? 1 : 0, IsTemporarilyHiddenInEditor() ? 1 : 0, bIsEditorOnlyActor ? 1 : 0,
			ISM->IsVisible() ? 1 : 0, ISM->bHiddenInGame ? 1 : 0, ISM->bRenderInMainPass ? 1 : 0,
			static_cast<int32>(ISM->VirtualTextureRenderPassType), bKeepSourceGeometry ? 1 : 0, bEditorPreview ? 1 : 0);
		DerivedRVTComponents.Add(ISM);
		DerivedInstanceGuids.Add(MoveTemp(RecordGuids));
	}

	// Deferred decals deliberately remain one component/proxy per record.  The
	// Collection owns their lifecycle and records, but it must not claim a
	// mesh-ISM batching win that the deferred decal renderer does not provide.
	for (const FDevKitDecalPlacementRecord& Record : Collection->Records)
	{
		if (!Record.bEnabled || !Record.IsValid() || !Record.Asset
			|| Record.Asset->Backend != EDevKitDecalBackend::DeferredProjection)
		{
			continue;
		}

		UMaterialInterface* Material = Record.MaterialOverride ? Record.MaterialOverride.Get() : Record.Asset->Material.Get();
		if (!Material)
		{
			UE_LOG(LogTemp, Warning, TEXT("Decal collection '%s' skipped deferred asset '%s': no material"), *GetName(), *Record.Asset->GetName());
			continue;
		}

		const FName ComponentName = MakeUniqueObjectName(
			this,
			UDecalComponent::StaticClass(),
			*FString::Printf(TEXT("Derived_Deferred_%s"), *Record.InstanceGuid.ToString(EGuidFormats::Digits)));
		UDecalComponent* Decal = NewObject<UDecalComponent>(this, ComponentName, RF_Transient);
		Decal->CreationMethod = EComponentCreationMethod::Instance;
		Decal->ComponentTags.Add(TEXT("DevKit.DecalCollection.Derived"));
		Decal->SetMobility(EComponentMobility::Static);
		Decal->SetupAttachment(SceneRoot);
		Decal->SetDecalMaterial(Material);
		Decal->DecalSize = Record.DecalSize.IsNearlyZero() ? Record.Asset->DefaultDecalSize : Record.DecalSize;
		Decal->SortOrder = Record.Asset->DeferredSortOrder;
		// UDecalComponent is a scene component rather than a collision/shadow
		// primitive. Its decal proxy has no gameplay collision or mesh shadow path.
		Decal->SetWorldTransform(Record.Transform, false, nullptr, ETeleportType::TeleportPhysics);
		AddInstanceComponent(Decal);
		if (UWorld* OwnerWorld = GetWorld())
		{
			Decal->RegisterComponentWithWorld(OwnerWorld);
		}
		else
		{
			Decal->RegisterComponent();
		}
		DerivedDeferredComponents.Add(Decal);
		DerivedDeferredGuids.Add(Record.InstanceGuid);
	}

	// Rebuilds can happen while the mode is still active (for example after a
	// placement or an undo). Preserve the current selection policy for the newly
	// created transient components instead of making the user re-enter the mode.
	SetDerivedInstanceEditingEnabled(bEditSessionActive);
}

namespace
{
	void SyncMovedInstance(UDevKitDecalCollectionISMComponent* Component, const FSMInstanceId& InstanceId)
	{
		if (!Component || InstanceId.ISMComponent != Component || !Component->GetOwner())
		{
			return;
		}
		ADevKitDecalCollectionActor* Owner = Cast<ADevKitDecalCollectionActor>(Component->GetOwner());
		if (!Owner)
		{
			return;
		}
		FTransform WorldTransform;
		if (Component->GetInstanceTransform(InstanceId.InstanceIndex, WorldTransform, true))
		{
			Owner->UpdateDerivedInstanceTransform(Component, InstanceId.InstanceIndex, WorldTransform);
		}
	}
}

bool UDevKitDecalCollectionISMComponent::CanEditSMInstance(const FSMInstanceId& InstanceId) const
{
	if (!Super::CanEditSMInstance(InstanceId))
	{
		return false;
	}

	const ADevKitDecalCollectionActor* Owner = Cast<ADevKitDecalCollectionActor>(GetOwner());
	return Owner && Owner->bEditSessionActive;
}

bool UDevKitDecalCollectionISMComponent::CanMoveSMInstance(const FSMInstanceId& InstanceId, const ETypedElementWorldType WorldType) const
{
	if (!Super::CanMoveSMInstance(InstanceId, WorldType))
	{
		return false;
	}

	const ADevKitDecalCollectionActor* Owner = Cast<ADevKitDecalCollectionActor>(GetOwner());
	return Owner && Owner->bEditSessionActive;
}

bool UDevKitDecalCollectionISMComponent::SetSMInstanceTransform(const FSMInstanceId& InstanceId, const FTransform& InstanceTransform, bool bWorldSpace, bool bMarkRenderStateDirty, bool bTeleport)
{
	const bool bResult = Super::SetSMInstanceTransform(InstanceId, InstanceTransform, bWorldSpace, bMarkRenderStateDirty, bTeleport);
	if (bResult)
	{
		SyncMovedInstance(this, InstanceId);
	}
	return bResult;
}

void UDevKitDecalCollectionISMComponent::NotifySMInstanceMovementStarted(const FSMInstanceId& InstanceId)
{
	Super::NotifySMInstanceMovementStarted(InstanceId);
	if (ADevKitDecalCollectionActor* Owner = Cast<ADevKitDecalCollectionActor>(GetOwner()))
	{
		Owner->Modify();
	}
}

void UDevKitDecalCollectionISMComponent::NotifySMInstanceMovementOngoing(const FSMInstanceId& InstanceId)
{
	Super::NotifySMInstanceMovementOngoing(InstanceId);
	SyncMovedInstance(this, InstanceId);
}

void UDevKitDecalCollectionISMComponent::NotifySMInstanceMovementEnded(const FSMInstanceId& InstanceId)
{
	Super::NotifySMInstanceMovementEnded(InstanceId);
	SyncMovedInstance(this, InstanceId);
}
