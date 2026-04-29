// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/ComboGraphNodeAnimBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ComboGraphAbilityTypes.h"
#include "ComboGraphLog.h"
#include "NiagaraSystem.h"
#include "Abilities/Tasks/ComboGraphAbilityTask_StartGraph.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Character.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Utils/ComboGraphBlueprintLibrary.h"

UObject* FComboGraphCueContainerDefinition::GetSourceObject() const
{
	UObject* Result = nullptr;
	if (CueSourceObjectType == EComboGraphCueSourceObjectType::Niagara)
	{
		Result = NiagaraSystem.LoadSynchronous();
	}
	else if (CueSourceObjectType == EComboGraphCueSourceObjectType::Cascade)
	{
		Result = CascadeSystem.LoadSynchronous();
	}
	else if (CueSourceObjectType == EComboGraphCueSourceObjectType::Sound)
	{
		Result = SoundEffect.LoadSynchronous();
	}

	return Result;
}

FText UComboGraphNodeAnimBase::K2_GetNodeTitle() const
{
	return GetNodeTitle();
}

UAnimSequenceBase* UComboGraphNodeAnimBase::K2_GetAnimationAsset() const
{
	return GetAnimationAsset();
}

UClass* UComboGraphNodeAnimBase::K2_GetAnimationClass() const
{
	const UAnimSequenceBase* AnimAsset = GetAnimationAsset();
	return AnimAsset ? AnimAsset->GetClass() : nullptr;
}

TArray<UComboGraphNodeAnimBase*> UComboGraphNodeAnimBase::K2_GetChildren() const
{
	TArray<UComboGraphNodeAnimBase*> Children;

	for (UComboGraphNodeBase* ChildrenNode : ChildrenNodes)
	{
		if (UComboGraphNodeAnimBase* Child = Cast<UComboGraphNodeAnimBase>(ChildrenNode))
		{
			Children.Add(Child);
		}
	}

	return Children;
}

UComboGraph* UComboGraphNodeAnimBase::K2_GetOwningGraph() const
{
	return Cast<UComboGraph>(GetOuter());
}

UComboGraphAbilityTask_StartGraph* UComboGraphNodeAnimBase::K2_GetOwningTask() const
{
	if (!CachedOwningTask.IsValid())
	{
		return nullptr;
	}

	return Cast<UComboGraphAbilityTask_StartGraph>(CachedOwningTask.Get());
}

UGameplayAbility* UComboGraphNodeAnimBase::K2_GetOwningAbility() const
{
	return CachedOwningAbility.IsValid() ? CachedOwningAbility.Get() : nullptr;
}

UComboGraphNodeAnimBase* UComboGraphNodeAnimBase::K2_GetPreviousNode() const
{
	if (!CachedOwningTask.IsValid())
	{
		return nullptr;
	}
	
	if (const UComboGraphAbilityTask_StartGraph* Task = Cast<UComboGraphAbilityTask_StartGraph>(CachedOwningTask.Get()))
	{
		return Task->GetPreviousNode();
	}

	return nullptr;
}

AActor* UComboGraphNodeAnimBase::GetOwnerActor() const
{
	if (!CachedOwningTask.IsValid())
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::GetOwnerActor() - Trying to get owner actor while combo node is not initialized (CachedOwningTask null)"));
		return nullptr;
	}

	return CachedOwningTask->GetOwnerActor();
}

AActor* UComboGraphNodeAnimBase::GetAvatarActor() const
{
	if (!CachedOwningTask.IsValid())
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::GetAvatarActor() - Trying to get avatar actor while combo node is not initialized (CachedOwningTask null)"));
		return nullptr;
	}

	return CachedOwningTask->GetAvatarActor();
}

APawn* UComboGraphNodeAnimBase::GetAvatarAsPawn() const
{
	if (!CachedOwningTask.IsValid())
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::GetAvatarAsPawn() - Trying to get avatar actor while combo node is not initialized (CachedOwningTask null)"));
		return nullptr;
	}

	return Cast<APawn>(CachedOwningTask->GetAvatarActor());
}

ACharacter* UComboGraphNodeAnimBase::GetAvatarAsCharacter() const
{
	if (!CachedOwningTask.IsValid())
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::GetAvatarAsCharacter() - Trying to get avatar actor while combo node is not initialized (CachedOwningTask null)"));
		return nullptr;
	}

	return Cast<ACharacter>(CachedOwningTask->GetAvatarActor());
}

float UComboGraphNodeAnimBase::GetMagnitudeForContainer_Implementation(const float OriginalMagnitude, float ElapsedSeconds, float TriggeredSeconds, float AnimationLength, float MontagePlayTime, float InputMagnitude, FGameplayTag ContainerTag) const
{
	return OriginalMagnitude;
}

bool UComboGraphNodeAnimBase::K2_CanActivateNode_Implementation() const
{
	return true;
}

UGameplayTasksComponent* UComboGraphNodeAnimBase::GetGameplayTasksComponent(const UGameplayTask& Task) const
{
	if (const UAbilityTask* AbilityTask = Cast<const UAbilityTask>(&Task))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return AbilityTask->AbilitySystemComponent.Get();
#else
		return AbilityTask->AbilitySystemComponent;
#endif	
	}

	return nullptr;
}

AActor* UComboGraphNodeAnimBase::GetGameplayTaskOwner(const UGameplayTask* Task) const
{
	const UAbilityTask* AbilityTask = Cast<const UAbilityTask>(Task);

	if (AbilityTask)
	{
		return AbilityTask->GetOwnerActor();
	}

	return nullptr;
}

AActor* UComboGraphNodeAnimBase::GetGameplayTaskAvatar(const UGameplayTask* Task) const
{
	if (const UAbilityTask* AbilityTask = Cast<const UAbilityTask>(Task))
	{
		return AbilityTask->GetAvatarActor();
	}
	return nullptr;
}

void UComboGraphNodeAnimBase::OnGameplayTaskInitialized(UGameplayTask& Task)
{
	if (UAbilityTask* AbilityTask = Cast<UAbilityTask>(&Task))
	{
		CachedOwningTask = AbilityTask;
		CachedOwningAbility = AbilityTask->Ability;
	}
	else
	{
		CachedOwningTask = &Task;
	}

	OnInitialized();
}

void UComboGraphNodeAnimBase::OnGameplayTaskActivated(UGameplayTask& Task)
{
	CG_RUNTIME_LOG(Verbose, TEXT("OnGameplayTaskActivated for %s (%s)"), *GetName(), *GetNodeTitle().ToString())

	bDebugActive = true;
	OnActivated();
}

void UComboGraphNodeAnimBase::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	bDebugActive = false;
	OnDeactivated();
}

void UComboGraphNodeAnimBase::AsyncLoadContainer(FComboGraphCueContainer* Container, const FStreamableDelegate DoneDelegate) const
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphNodeAnimBase::AsyncLoadContainer"))

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	CG_RUNTIME_LOG(Verbose, TEXT("  - Streamable Manager %s"), *StreamableManager.GetManagerName())

	TArray<FSoftObjectPath> AssetsToLoad;
	for (FComboGraphCueContainerDefinition Definition : Container->Definitions)
	{
		Definition.GetAssetPathsForAsyncLoad(AssetsToLoad);
	}

	for (FSoftObjectPath SoftObjectPath : AssetsToLoad)
	{
		CG_RUNTIME_LOG(Verbose, TEXT("  - Soft Path: %s"), *SoftObjectPath.ToString())
	}

	if (AssetsToLoad.Num() == 0)
	{
		// No assets to load, invoke Done delegate right away
		DoneDelegate.ExecuteIfBound();
		return;
	}

	StreamableManager.RequestAsyncLoad(AssetsToLoad, DoneDelegate);
}

void UComboGraphNodeAnimBase::ApplyCuesContainerForTag(const FGameplayTag& ContainerTag, const FGameplayEventData& EventData)
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphNodeAnimBase::ApplyCuesContainerForTag - Try to apply container (%s)"), *ContainerTag.ToString())

	FComboGraphCueContainer* FoundContainer = CuesContainerMap.Find(ContainerTag);
	if (!FoundContainer)
	{
		// Gameplay tag does not match container
		return;
	}

	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphGameplayCueManager ApplyCuesContainerForTag %s"), *ContainerTag.ToString())

	if (AsyncHandlingMethod == EComboGraphAsyncHandlingMethod::SoftObjectPaths)
	{
		// Pass down just the source object paths to cues, letting the cue handler do the async
		ApplyCueContainer(FoundContainer, EventData);
	}
	else if (AsyncHandlingMethod == EComboGraphAsyncHandlingMethod::ResolvedObjects)
	{
		// Async load and resolve objects now, passing down full objects to cues
		AsyncLoadContainer(FoundContainer, FStreamableDelegate::CreateLambda([this, FoundContainer, EventData]()
		{
			CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphGameplayCueManager ApplyCuesContainerForTag Asynd Load Done"))
			ApplyCueContainer(FoundContainer, EventData, false);
		}));
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UComboGraphNodeAnimBase::ApplyCueContainer(FComboGraphCueContainer* Container, const FGameplayEventData& EventData, bool bPassOnlySoftObjectPaths)
{
	check(Container);
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphGameplayCueManager ApplyCueContainer %s (Definitions: %d)"), *EventData.EventTag.ToString(), Container->Definitions.Num())

	if (!CachedOwningAbility.IsValid())
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - Cached Owning Ability is null, can't apply container."))
		return;
	}

	UAbilitySystemComponent* ASC = CachedOwningAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - Cached Owning Ability ASC is null, can't apply container."))
		return;
	}

	// Build up Gameplay Cue Parameters (Shared for all definitions in a container)
	FGameplayCueParameters Params = CreateGameplayCueParameters(EventData);
	TMap<FGameplayTag, FComboGraphCueContainerDefinition> Definitions;

	bool bShouldProceed = false;
	if (UComboGraphBlueprintLibrary::IsValidContext(Params.EffectContext.Get()))
	{
		FComboGraphGameplayEffectContext* Context = static_cast<FComboGraphGameplayEffectContext*>(Params.EffectContext.Get());
		if (Context)
		{
			TArray<FSoftObjectPath> SoftObjectPaths;
			Container->GetAggregatedDefinitionsAndPaths(Definitions, SoftObjectPaths);

			if (bPassOnlySoftObjectPaths)
			{
				CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - SetCueParamsObjects with %d object paths"), SoftObjectPaths.Num())
				Context->SetCueParamsObjectsPath(SoftObjectPaths);
				bShouldProceed = true;
			}
			else
			{
				// Resolve objects now, assuming async load already happened
				TArray<TWeakObjectPtr<UObject>> SourceObjects;
				Container->GetAggregatedDefinitionsAndObjects(Definitions, SourceObjects);

				CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - SetCueParamsObjects with %d source objects"), SourceObjects.Num())
				Context->SetCueParamsObjects(SourceObjects);
				Context->SetCueParamsObjectsPath(SoftObjectPaths);
				bShouldProceed = true;
			}
		}
	}
	else
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - Can't cast to custom context.\n%s"), *GetAbilitySystemGlobalsWarning())
	}

	if (!bShouldProceed)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - Something went wrong, most likely invalid effect context and can't pass down cue params as either object or paths"))
		return;
	}

	// Finally, execute gameplay cue for each defined tags
	for (auto Definition : Definitions)
	{
		FGameplayTag GameplayCueTag = Definition.Key;
		FComboGraphCueContainerDefinition CueContainerDefinition = Definition.Value;

		FGameplayCueParameters CueParams = FGameplayCueParameters(Params);
		CueParams.MatchedTagName = GameplayCueTag;
		if (ASC)
		{
			CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - Execute Gameplay Cue for %s"), *GameplayCueTag.ToString())
			ASC->ExecuteGameplayCue(GameplayCueTag, CueParams);

			// Executing cues locally would look like this
			// CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphNodeAnimBase::ApplyCueContainer - Execute Gameplay Cue locally for %s"), *GameplayCueTag.ToString())
			// UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(ASC->GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::Executed, CueParams);
		}
	}
}

FGameplayCueParameters UComboGraphNodeAnimBase::CreateGameplayCueParameters(const FGameplayEventData& EventData) const
{
	check(CachedOwningAbility.IsValid());

	FGameplayCueParameters GameplayCueParameters;
	GameplayCueParameters.AbilityLevel = CachedOwningAbility->GetAbilityLevel();
	GameplayCueParameters.GameplayEffectLevel = EventData.ContextHandle.GetAbilityLevel();
	GameplayCueParameters.NormalizedMagnitude = 1.f;
	GameplayCueParameters.RawMagnitude = 0.f;

	// TODO: Pass down additional infos into custom effect context (such as original event tag for the container applying the cue, original event data / target data ?, ...)

	// Setup cue params magnitude based on matching damage / effect container for event tag
	if (DamageHandlingMethod == EComboGraphDamageHandlingMethod::DamageSystem)
	{
		const FComboGraphDamageSystemContainer* Container = DamagesContainerMap.Find(EventData.EventTag);
		if (Container)
		{
			GameplayCueParameters.RawMagnitude = Container->BaseDamage;
			GameplayCueParameters.NormalizedMagnitude = Container->BaseDamage;
		}
	}
	else if (DamageHandlingMethod == EComboGraphDamageHandlingMethod::AbilitySystem)
	{
		const FComboGraphGameplayEffectContainer* Container = EffectsContainerMap.Find(EventData.EventTag);
		if (Container && Container->bUseSetByCallerMagnitude)
		{
			GameplayCueParameters.RawMagnitude = Container->SetByCallerMagnitude;
			GameplayCueParameters.NormalizedMagnitude = Container->SetByCallerMagnitude;
		}
	}

	// Set source to be this combo node
	GameplayCueParameters.SourceObject = MakeWeakObjectPtr(this);

	// Pass down effect context
	FGameplayEffectContextHandle Context = FGameplayEffectContextHandle(UAbilitySystemGlobals::Get().AllocGameplayEffectContext());
	GameplayCueParameters.EffectContext = Context;

	// Instigator is the owner of the ability
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	AActor* Instigator = EventData.Instigator ? const_cast<AActor*>(EventData.Instigator.Get()) : nullptr;
#else
	AActor* Instigator = EventData.Instigator ? const_cast<AActor*>(EventData.Instigator) : nullptr;
#endif

	// The physical actor that actually did the damage, can be a weapon or projectile and / or be the same as Instigator
	AActor* EffectCauser = CachedOwningAbility->GetAvatarActorFromActorInfo();

	// Update Cue Params for Instigator and Effect Causer
	GameplayCueParameters.Instigator = Instigator;
	GameplayCueParameters.EffectCauser = EffectCauser;

	// Update effect context instigator / effect cause to match gameplay cue params (if not defined already)
	if (!Context.GetInstigator() && !Context.GetEffectCauser())
	{
		Context.AddInstigator(Instigator, EffectCauser);
	}

	// Extract Hit Location from either Effect Context Hit Result or from Target Data
	const FHitResult* ContextHitResult = EventData.ContextHandle.GetHitResult();
	const FHitResult TargetDataHitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData, 0);

	if (ContextHitResult)
	{
		GameplayCueParameters.Location = ContextHitResult->Location;
		GameplayCueParameters.Normal = ContextHitResult->Normal;
		GameplayCueParameters.PhysicalMaterial = ContextHitResult->PhysMaterial;

		// Update Context Hit result passed down to cue params
		Context.AddHitResult(*ContextHitResult);
	}
	else
	{
		GameplayCueParameters.Location = TargetDataHitResult.Location;
		GameplayCueParameters.Normal = TargetDataHitResult.Normal;
		GameplayCueParameters.PhysicalMaterial = TargetDataHitResult.PhysMaterial;

		// Update Context Hit result passed down to cue params
		Context.AddHitResult(TargetDataHitResult);
	}

	return GameplayCueParameters;
}

FAnimNotifyEvent* UComboGraphNodeAnimBase::GetAnimNotify(TSubclassOf<UAnimNotifyState> NotifyStateClass) const
{
	UAnimSequenceBase* AnimationAsset = Cast<UAnimSequenceBase>(GetAnimationAsset());
	if (!AnimationAsset)
	{
		return nullptr;
	}

	return AnimationAsset->Notifies.FindByPredicate([NotifyStateClass](const FAnimNotifyEvent NotifyEvent) -> bool
	{
		return NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA(NotifyStateClass);
	});
}

bool UComboGraphNodeAnimBase::HasAnimNotifyState(TSubclassOf<UAnimNotifyState> NotifyStateClass) const
{
	return GetAnimNotify(NotifyStateClass) != nullptr;
}

FAnimNotifyEvent* UComboGraphNodeAnimBase::GetAnimNotify(TSubclassOf<UAnimNotify> NotifyClass) const
{
	UAnimSequenceBase* AnimationAsset = Cast<UAnimSequenceBase>(GetAnimationAsset());
	if (!AnimationAsset)
	{
		return nullptr;
	}

	return AnimationAsset->Notifies.FindByPredicate([NotifyClass](const FAnimNotifyEvent NotifyEvent) -> bool
	{
		return NotifyEvent.Notify && NotifyEvent.Notify->IsA(NotifyClass);
	});
}

bool UComboGraphNodeAnimBase::HasAnimNotify(const TSubclassOf<UAnimNotify> NotifyClass) const
{
	return GetAnimNotify(NotifyClass) != nullptr;
}

FAnimNotifyEvent* UComboGraphNodeAnimBase::GetAnimNotify(FName NotifyName) const
{
	UAnimSequenceBase* AnimationAsset = Cast<UAnimSequenceBase>(GetAnimationAsset());
	if (!AnimationAsset)
	{
		return nullptr;
	}

	return AnimationAsset->Notifies.FindByPredicate([NotifyName](const FAnimNotifyEvent NotifyEvent) -> bool
	{
		return !NotifyEvent.NotifyName.IsNone() && NotifyEvent.NotifyName == NotifyName;
	});
}

bool UComboGraphNodeAnimBase::HasAnimNotify(const FName NotifyName) const
{
	return GetAnimNotify(NotifyName) != nullptr;
}

float UComboGraphNodeAnimBase::GetAnimNotifyTriggerTime(const TSubclassOf<UAnimNotify> NotifyClass) const
{
	FAnimNotifyEvent* AnimNotifyEvent = GetAnimNotify(NotifyClass);
	return AnimNotifyEvent ? AnimNotifyEvent->GetTriggerTime() : -1.f;
}

float UComboGraphNodeAnimBase::GetAnimNotifyTriggerTime(const FName NotifyName) const
{
	FAnimNotifyEvent* AnimNotifyEvent = GetAnimNotify(NotifyName);
	return AnimNotifyEvent ? AnimNotifyEvent->GetTriggerTime() : -1.f;
}

FString UComboGraphNodeAnimBase::GetAbilitySystemGlobalsWarning() const
{
	const TArray<FString> Lines = {
		"\n ",
		"Please, make sure you have setup AbilitySystemGlobals to be either ComboGraphAbilitySystemGlobals or a child of it. It needs to implement `AllocGameplayEffectContext` and return FComboGraphGameplayEffectContext.",
		"You can do so with the following configuration in DefaultGame.ini:",
		"\n ",
		"[/Script/GameplayAbilities.AbilitySystemGlobals]\n"
		"AbilitySystemGlobalsClassName=\"/Script/ComboGraph.ComboGraphAbilitySystemGlobals\""
		"\n "
	};

	return FString::Join(Lines, LINE_TERMINATOR);
}
