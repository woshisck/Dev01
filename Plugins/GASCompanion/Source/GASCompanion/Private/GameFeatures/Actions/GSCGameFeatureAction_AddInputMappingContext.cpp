// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GameFeatures/Actions/GSCGameFeatureAction_AddInputMappingContext.h"

#include "EnhancedInputSubsystems.h"
#include "GSCLog.h"
#include "GameFeaturesSubsystemSettings.h"
#include "InputMappingContext.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "GASCompanion"

void UGSCGameFeatureAction_AddInputMappingContext::OnGameFeatureActivating()
{
	if (!ensure(ExtensionRequestHandles.IsEmpty()) || !ensure(ControllersAddedTo.IsEmpty()))
	{
		Reset();
	}

	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UGSCGameFeatureAction_AddInputMappingContext::HandleGameInstanceStart);

	// Add to any worlds with associated game instances that have already been initialized
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		AddToWorld(WorldContext);
	}

	Super::OnGameFeatureActivating();
}

void UGSCGameFeatureAction_AddInputMappingContext::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);

	Reset();
}

#if WITH_EDITORONLY_DATA
void UGSCGameFeatureAction_AddInputMappingContext::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
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

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, InputMapping.ToSoftObjectPath().GetAssetPath());
	AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, InputMapping.ToSoftObjectPath().GetAssetPath());
#else
	AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, InputMapping.ToSoftObjectPath());
	AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, InputMapping.ToSoftObjectPath());
#endif
}
#endif

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
EDataValidationResult UGSCGameFeatureAction_AddInputMappingContext::IsDataValid(FDataValidationContext& Context) const
#else
EDataValidationResult UGSCGameFeatureAction_AddInputMappingContext::IsDataValid(FDataValidationContext& Context)
#endif
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (InputMapping.IsNull())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(LOCTEXT("NullInputMapping", "Null InputMapping."));
	}

	return Result;
}
#endif

void UGSCGameFeatureAction_AddInputMappingContext::AddToWorld(const FWorldContext& WorldContext)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;

	if ((GameInstance != nullptr) && (World != nullptr) && World->IsGameWorld())
	{
		UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);

		if (ComponentManager)
		{
			if (!InputMapping.IsNull())
			{
				const UGameFrameworkComponentManager::FExtensionHandlerDelegate AddMappingContextDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
					this,
					&UGSCGameFeatureAction_AddInputMappingContext::HandleControllerExtension
				);
				const TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle = ComponentManager->AddExtensionHandler(APlayerController::StaticClass(), AddMappingContextDelegate);

				ExtensionRequestHandles.Add(ExtensionRequestHandle);
			}
		}
	}
}

void UGSCGameFeatureAction_AddInputMappingContext::Reset()
{
	ExtensionRequestHandles.Empty();

	while (!ControllersAddedTo.IsEmpty())
	{
		TWeakObjectPtr<APlayerController> ControllerPtr = ControllersAddedTo.Top();
		if (ControllerPtr.IsValid())
		{
			RemoveInputMapping(ControllerPtr.Get());
		}
		else
		{
			ControllersAddedTo.Pop();
		}
	}
}

void UGSCGameFeatureAction_AddInputMappingContext::HandleControllerExtension(AActor* Actor, const FName EventName)
{
	APlayerController* PC = CastChecked<APlayerController>(Actor);

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveInputMapping(PC);
	}
	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		AddInputMappingForPlayer(PC->GetLocalPlayer());
	}
}

void UGSCGameFeatureAction_AddInputMappingContext::AddInputMappingForPlayer(UPlayer* Player)
{
	if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), Priority);
			}
		}
		else
		{
			GSC_LOG(Error, TEXT("Failed to find `UEnhancedInputLocalPlayerSubsystem` for local player. Input mappings will not be added. Make sure you're set to use the EnhancedInput system via config file."));
		}
	}
}

void UGSCGameFeatureAction_AddInputMappingContext::RemoveInputMapping(APlayerController* PlayerController)
{
	if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSystem->RemoveMappingContext(InputMapping.Get());
		}
	}

	ControllersAddedTo.Remove(PlayerController);
}

void UGSCGameFeatureAction_AddInputMappingContext::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	if (const FWorldContext* WorldContext = GameInstance->GetWorldContext())
	{
		AddToWorld(*WorldContext);
	}
}

#undef LOCTEXT_NAMESPACE
