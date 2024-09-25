// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GameFeatures/Actions/GSCGameFeatureAction_AddAnimLayers.h"

#include "GSCLog.h"
#include "GameFeaturesSubsystemSettings.h"
#include "Components/GSCLinkAnimLayersComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "GASCompanion"

void UGSCGameFeatureAction_AddAnimLayers::OnGameFeatureActivating()
{
	if (!ensureAlways(ComponentRequestHandles.IsEmpty()))
	{
		Reset();
	}

	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UGSCGameFeatureAction_AddAnimLayers::HandleGameInstanceStart);

	check(ComponentRequestHandles.Num() == 0);

	// Add to any worlds with associated game instances that have already been initialized
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		AddToWorld(WorldContext);
	}
}

void UGSCGameFeatureAction_AddAnimLayers::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);

	// Releasing the handles will also remove the components from any registered actors too
	ComponentRequestHandles.Empty();

	Reset();
}

#if WITH_EDITORONLY_DATA
void UGSCGameFeatureAction_AddAnimLayers::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if (!UAssetManager::IsInitialized())
	{
		return;
	}
#else
	if (!UAssetManager::IsValid())
	{
		return;
	}
#endif

	for (const FGSCAnimLayerEntry& Entry : AnimLayerEntries)
	{
		for (TSoftClassPtr<UAnimInstance> AnimLayer : Entry.AnimLayers)
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, AnimLayer.ToSoftObjectPath().GetAssetPath());
			AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, AnimLayer.ToSoftObjectPath().GetAssetPath());
#else
			AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, AnimLayer.ToSoftObjectPath());
			AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, AnimLayer.ToSoftObjectPath());
#endif
		}
	}
}
#endif

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
EDataValidationResult UGSCGameFeatureAction_AddAnimLayers::IsDataValid(FDataValidationContext& Context) const
#else
EDataValidationResult UGSCGameFeatureAction_AddAnimLayers::IsDataValid(FDataValidationContext& Context)
#endif
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	int32 EntryIndex = 0;
	for (const FGSCAnimLayerEntry& Entry : AnimLayerEntries)
	{
		if (Entry.ActorClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("AnimLayerEntryHasNullActor", "Null ActorClass at index {0} in AnimLayerEntries"), FText::AsNumber(EntryIndex)));
		}

		if (Entry.AnimLayers.IsEmpty())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("EntryHasNoAnimLayers", "Empty AnimLayers at index {0} in AnimLayerEntries"), FText::AsNumber(EntryIndex)));
		}

		int32 AnimLayerIndex = 0;
		for (const TSoftClassPtr<UAnimInstance>& AnimLayer : Entry.AnimLayers)
		{
			if (AnimLayer.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAnimLayer", "Null AnimLayer at index {0} in AnimLayerEntries[{1}].AnimLayers"), FText::AsNumber(AnimLayerIndex), FText::AsNumber(EntryIndex)));
			}

			++AnimLayerIndex;
		}

		++EntryIndex;
	}

	return Result;
}
#endif

// ReSharper disable once CppParameterMayBeConstPtrOrRef
UActorComponent* UGSCGameFeatureAction_AddAnimLayers::FindOrAddComponentForActor(UClass* ComponentType, AActor* Actor, const FGSCAnimLayerEntry& AnimLayerEntry)
{
	UActorComponent* Component = Actor->FindComponentByClass(ComponentType);

	bool bMakeComponentRequest = (Component == nullptr);
	if (Component)
	{
		// Check to see if this component was created from a different `UGameFrameworkComponentManager` request.
		// `Native` is what `CreationMethod` defaults to for dynamically added components.
		if (Component->CreationMethod == EComponentCreationMethod::Native)
		{
			// Attempt to tell the difference between a true native component and one created by the GameFrameworkComponent system.
			// If it is from the UGameFrameworkComponentManager, then we need to make another request (requests are ref counted).
			const UObject* ComponentArchetype = Component->GetArchetype();
			bMakeComponentRequest = ComponentArchetype->HasAnyFlags(RF_ClassDefaultObject);
		}
	}

	if (bMakeComponentRequest)
	{
		const UWorld* World = Actor->GetWorld();
		const UGameInstance* GameInstance = World->GetGameInstance();

		if (UGameFrameworkComponentManager* ComponentMan = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance))
		{
			const TSharedPtr<FComponentRequestHandle> RequestHandle = ComponentMan->AddComponentRequest(AnimLayerEntry.ActorClass, ComponentType);
			ComponentRequestHandles.Add(RequestHandle);
		}

		if (!Component)
		{
			Component = Actor->FindComponentByClass(ComponentType);
			ensureAlways(Component);
		}
	}

	return Component;
}

void UGSCGameFeatureAction_AddAnimLayers::Reset()
{
	// TODO: Handle reset of anim layers
}

void UGSCGameFeatureAction_AddAnimLayers::HandleActorExtension(AActor* Actor, const FName EventName, const int32 EntryIndex)
{
	if (AnimLayerEntries.IsValidIndex(EntryIndex))
	{
		const FGSCAnimLayerEntry& Entry = AnimLayerEntries[EntryIndex];
		if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
		{
			RemoveAnimLayers(Actor);
		}
		else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
		{
			AddAnimLayers(Actor, Entry);
		}
	}
}

void UGSCGameFeatureAction_AddAnimLayers::AddAnimLayers(AActor* Actor, const FGSCAnimLayerEntry& Entry)
{
	GSC_LOG(Verbose, TEXT("AddAnimLayers to '%s'."), *Actor->GetPathName());

	UGSCLinkAnimLayersComponent* LinkAnimLayersComponent = FindOrAddComponentForActor<UGSCLinkAnimLayersComponent>(Actor, Entry);
	if (!LinkAnimLayersComponent)
	{
		GSC_LOG(Error, TEXT("Failed to find/add a LinkAnimLayersComponent to '%s'. Anim Layers will not be linked."), *Actor->GetPathName());
		return;
	}

	FActorExtensions AddedExtensions;
	AddedExtensions.AnimLayers.Reserve(Entry.AnimLayers.Num());

	for (const TSoftClassPtr<UAnimInstance>& AnimLayerType : Entry.AnimLayers)
	{
		UClass* AnimInstanceType = AnimLayerType.LoadSynchronous();
		LinkAnimLayersComponent->LinkAnimLayer(AnimInstanceType);
		AddedExtensions.AnimLayers.Add(AnimInstanceType);
	}

	ActiveExtensions.Add(Actor, AddedExtensions);
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UGSCGameFeatureAction_AddAnimLayers::RemoveAnimLayers(AActor* Actor)
{
	GSC_LOG(Verbose, TEXT("RemoveAnimLayers from '%s'."), *Actor->GetPathName());
	if (FActorExtensions* ActorExtensions = ActiveExtensions.Find(Actor))
	{
		if (UGSCLinkAnimLayersComponent* LinkAnimLayersComponent = Actor->FindComponentByClass<UGSCLinkAnimLayersComponent>())
		{
			for (const TSubclassOf<UAnimInstance>& AnimLayer : ActorExtensions->AnimLayers)
			{
				LinkAnimLayersComponent->UnlinkAnimLayer(AnimLayer);
			}
		}

		ActiveExtensions.Remove(Actor);
	}
}

void UGSCGameFeatureAction_AddAnimLayers::AddToWorld(const FWorldContext& WorldContext)
{
	const UWorld* World = WorldContext.World();
	const UGameInstance* GameInstance = WorldContext.OwningGameInstance;

	if ((GameInstance != nullptr) && (World != nullptr) && World->IsGameWorld())
	{

		UGameFrameworkComponentManager* GFCM = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);

		if (GFCM)
		{
			int32 EntryIndex = 0;
			for (const FGSCAnimLayerEntry& Entry : AnimLayerEntries)
			{
				if (!Entry.ActorClass.IsNull())
				{
					const UGameFrameworkComponentManager::FExtensionHandlerDelegate AddAnimLayersDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
						this,
						&UGSCGameFeatureAction_AddAnimLayers::HandleActorExtension,
						EntryIndex
					);
					TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle = GFCM->AddExtensionHandler(Entry.ActorClass, AddAnimLayersDelegate);

					ComponentRequestHandles.Add(ExtensionRequestHandle);
					
					EntryIndex++;
				}
			}
		}
	}
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UGSCGameFeatureAction_AddAnimLayers::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	if (const FWorldContext* WorldContext = GameInstance->GetWorldContext())
	{
		AddToWorld(*WorldContext);
	}
}

#undef LOCTEXT_NAMESPACE
