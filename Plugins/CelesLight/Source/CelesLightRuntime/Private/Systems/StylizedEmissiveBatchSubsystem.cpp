#include "Systems/StylizedEmissiveBatchSubsystem.h"

#include "Actors/StylizedEmissiveLight.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"

namespace
{
	TAutoConsoleVariable<int32> CVarCelesEmissiveAutoBatch(
		TEXT("r.CelesLight.Emissive.AutoBatch"),
		1,
		TEXT("Automatically batch compatible Stylized Emissive Source meshes per loaded level.\n")
		TEXT("0: disabled, 1: enabled (default)"),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarCelesEmissiveMinBatchSize(
		TEXT("r.CelesLight.Emissive.MinBatchSize"),
		2,
		TEXT("Minimum number of compatible sources required to create one HISM batch."),
		ECVF_Default);

	struct FStylizedEmissiveBatchKey
	{
		UStaticMesh* Mesh = nullptr;
		UMaterialInterface* Material = nullptr;
		bool bUseLumenGI = false;

		bool operator==(const FStylizedEmissiveBatchKey& Other) const
		{
			return Mesh == Other.Mesh
				&& Material == Other.Material
				&& bUseLumenGI == Other.bUseLumenGI;
		}

		friend uint32 GetTypeHash(const FStylizedEmissiveBatchKey& Key)
		{
			uint32 Hash = HashCombine(GetTypeHash(Key.Mesh), GetTypeHash(Key.Material));
			return HashCombine(Hash, GetTypeHash(Key.bUseLumenGI));
		}
	};

	struct FStylizedEmissiveBatchItem
	{
		TObjectPtr<AStylizedEmissiveLight> Source = nullptr;
		FTransform WorldTransform = FTransform::Identity;
		FStylizedEmissivePerInstanceData PerInstanceData;
	};
}

void UStylizedEmissiveBatchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(
		this, &UStylizedEmissiveBatchSubsystem::HandleLevelAddedToWorld);
	LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(
		this, &UStylizedEmissiveBatchSubsystem::HandleLevelRemovedFromWorld);
}

void UStylizedEmissiveBatchSubsystem::Deinitialize()
{
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	FWorldDelegates::LevelRemovedFromWorld.Remove(LevelRemovedHandle);
	LevelAddedHandle.Reset();
	LevelRemovedHandle.Reset();

	ClearAllBatches();
	Super::Deinitialize();
}

void UStylizedEmissiveBatchSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	RebuildAllBatches();
}

bool UStylizedEmissiveBatchSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::GamePreview;
}

void UStylizedEmissiveBatchSubsystem::RebuildAllBatches()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ClearAllBatches();
	if (CVarCelesEmissiveAutoBatch.GetValueOnGameThread() == 0)
	{
		return;
	}

	for (ULevel* Level : World->GetLevels())
	{
		RebuildLevelBatch(Level);
	}
}

void UStylizedEmissiveBatchSubsystem::ClearAllBatches()
{
	UWorld* World = GetWorld();
	if (World)
	{
		for (ULevel* Level : World->GetLevels())
		{
			ClearLevelBatch(Level, true);
		}
	}

	for (AActor* BatchActor : BatchActors)
	{
		if (IsValid(BatchActor))
		{
			BatchActor->Destroy();
		}
	}
	BatchActors.Reset();
	BatchedSourceCount = 0;
	BatchComponentCount = 0;
}

void UStylizedEmissiveBatchSubsystem::RebuildLevelBatch(ULevel* Level)
{
	UWorld* World = GetWorld();
	if (!Level || !World || Level->GetWorld() != World)
	{
		return;
	}

	ClearLevelBatch(Level, true);
	if (CVarCelesEmissiveAutoBatch.GetValueOnGameThread() == 0)
	{
		return;
	}

	TMap<FStylizedEmissiveBatchKey, TArray<FStylizedEmissiveBatchItem>> Groups;
	for (AActor* LevelActor : Level->Actors)
	{
		AStylizedEmissiveLight* Source = Cast<AStylizedEmissiveLight>(LevelActor);
		if (!IsValid(Source))
		{
			continue;
		}

		UStaticMesh* Mesh = nullptr;
		UMaterialInterface* Material = nullptr;
		FTransform WorldTransform = FTransform::Identity;
		bool bUseLumenGI = false;
		if (!Source->GetAutomaticBatchRenderData(Mesh, Material, WorldTransform, bUseLumenGI))
		{
			continue;
		}

		FStylizedEmissiveBatchKey Key;
		Key.Mesh = Mesh;
		Key.Material = Material;
		Key.bUseLumenGI = bUseLumenGI;

		FStylizedEmissiveBatchItem Item;
		Item.Source = Source;
		Item.WorldTransform = WorldTransform;
		Item.PerInstanceData = Source->GetPerInstanceMaterialData();
		Groups.FindOrAdd(Key).Add(MoveTemp(Item));
	}

	const int32 MinBatchSize = FMath::Max(
		2, CVarCelesEmissiveMinBatchSize.GetValueOnGameThread());
	bool bNeedsBatchActor = false;
	int32 NewBatchComponentCount = 0;
	for (const TPair<FStylizedEmissiveBatchKey, TArray<FStylizedEmissiveBatchItem>>& Group : Groups)
	{
		if (Group.Value.Num() >= MinBatchSize)
		{
			bNeedsBatchActor = true;
			break;
		}
	}

	if (!bNeedsBatchActor)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = MakeUniqueObjectName(
		Level, AActor::StaticClass(), TEXT("StylizedEmissiveRuntimeBatch"));
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.OverrideLevel = Level;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* BatchActor = World->SpawnActor<AActor>(
		AActor::StaticClass(), FTransform::Identity, SpawnParameters);
	if (!BatchActor)
	{
		return;
	}
	BatchActor->SetActorEnableCollision(false);
	BatchActor->SetReplicates(false);
	BatchActors.Add(BatchActor);

	USceneComponent* Root = NewObject<USceneComponent>(
		BatchActor, TEXT("BatchRoot"), RF_Transient);
	BatchActor->SetRootComponent(Root);
	BatchActor->AddInstanceComponent(Root);
	Root->SetMobility(EComponentMobility::Static);
	Root->RegisterComponent();

	for (const TPair<FStylizedEmissiveBatchKey, TArray<FStylizedEmissiveBatchItem>>& Group : Groups)
	{
		const FStylizedEmissiveBatchKey& Key = Group.Key;
		const TArray<FStylizedEmissiveBatchItem>& Items = Group.Value;
		if (Items.Num() < MinBatchSize)
		{
			continue;
		}

		UHierarchicalInstancedStaticMeshComponent* BatchComponent =
			NewObject<UHierarchicalInstancedStaticMeshComponent>(
				BatchActor, NAME_None, RF_Transient);
		BatchComponent->SetupAttachment(Root);
		BatchActor->AddInstanceComponent(BatchComponent);
		BatchComponent->SetMobility(EComponentMobility::Static);
		BatchComponent->SetStaticMesh(Key.Mesh);
		BatchComponent->SetMaterial(0, Key.Material);
		BatchComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BatchComponent->SetGenerateOverlapEvents(false);
		BatchComponent->SetCastShadow(false);
		BatchComponent->SetReceivesDecals(false);
		BatchComponent->SetRenderInMainPass(true);
		BatchComponent->SetAffectDynamicIndirectLighting(Key.bUseLumenGI);
		BatchComponent->SetAffectIndirectLightingWhileHidden(false);
		BatchComponent->SetEmissiveLightSource(Key.bUseLumenGI);
		BatchComponent->SetVisibleInRayTracing(Key.bUseLumenGI);
		BatchComponent->SetAffectDistanceFieldLighting(Key.bUseLumenGI);
		BatchComponent->SetNumCustomDataFloats(
			FStylizedEmissivePerInstanceData::NumCustomDataFloats);
		BatchComponent->RegisterComponent();
		BatchComponent->PreAllocateInstancesMemory(Items.Num());

		TArray<AStylizedEmissiveLight*> SuccessfullyBatchedSources;
		SuccessfullyBatchedSources.Reserve(Items.Num());
		for (const FStylizedEmissiveBatchItem& Item : Items)
		{
			const int32 InstanceIndex = BatchComponent->AddInstance(
				Item.WorldTransform, true);
			if (InstanceIndex == INDEX_NONE)
			{
				continue;
			}

			const FLinearColor& Color = Item.PerInstanceData.EmissiveColor;
			BatchComponent->SetCustomDataValue(
				InstanceIndex, FStylizedEmissivePerInstanceData::ColorDataIndex + 0, Color.R);
			BatchComponent->SetCustomDataValue(
				InstanceIndex, FStylizedEmissivePerInstanceData::ColorDataIndex + 1, Color.G);
			BatchComponent->SetCustomDataValue(
				InstanceIndex, FStylizedEmissivePerInstanceData::ColorDataIndex + 2, Color.B);
			BatchComponent->SetCustomDataValue(
				InstanceIndex,
				FStylizedEmissivePerInstanceData::IntensityDataIndex,
				Item.PerInstanceData.EmissiveIntensity);
			SuccessfullyBatchedSources.Add(Item.Source);
		}
		BatchComponent->MarkRenderStateDirty();

		if (SuccessfullyBatchedSources.Num() == 0)
		{
			BatchComponent->DestroyComponent();
			continue;
		}

		for (AStylizedEmissiveLight* Source : SuccessfullyBatchedSources)
		{
			if (IsValid(Source))
			{
				Source->SetRuntimeBatched(true);
				++BatchedSourceCount;
			}
		}
		++BatchComponentCount;
		++NewBatchComponentCount;
	}

	if (NewBatchComponentCount == 0)
	{
		BatchActor->Destroy();
		BatchActors.Remove(BatchActor);
	}
}

void UStylizedEmissiveBatchSubsystem::ClearLevelBatch(
	ULevel* Level, const bool bRestoreSources)
{
	if (!Level)
	{
		return;
	}

	if (bRestoreSources)
	{
		for (AActor* LevelActor : Level->Actors)
		{
			if (AStylizedEmissiveLight* Source = Cast<AStylizedEmissiveLight>(LevelActor))
			{
				if (Source->IsRuntimeBatched())
				{
					Source->SetRuntimeBatched(false);
					BatchedSourceCount = FMath::Max(0, BatchedSourceCount - 1);
				}
			}
		}
	}

	for (int32 Index = BatchActors.Num() - 1; Index >= 0; --Index)
	{
		AActor* BatchActor = BatchActors[Index];
		if (!IsValid(BatchActor))
		{
			BatchActors.RemoveAtSwap(Index);
			continue;
		}

		if (BatchActor->GetLevel() == Level)
		{
			TArray<UHierarchicalInstancedStaticMeshComponent*> Components;
			BatchActor->GetComponents(Components);
			BatchComponentCount = FMath::Max(
				0,
				BatchComponentCount - Components.Num());
			BatchActor->Destroy();
			BatchActors.RemoveAtSwap(Index);
		}
	}
}

void UStylizedEmissiveBatchSubsystem::HandleLevelAddedToWorld(
	ULevel* Level, UWorld* World)
{
	if (World == GetWorld() && HasCalledBeginPlay())
	{
		RebuildLevelBatch(Level);
	}
}

void UStylizedEmissiveBatchSubsystem::HandleLevelRemovedFromWorld(
	ULevel* Level, UWorld* World)
{
	if (World == GetWorld())
	{
		ClearLevelBatch(Level, false);
	}
}
