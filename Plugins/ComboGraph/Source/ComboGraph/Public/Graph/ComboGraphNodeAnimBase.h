// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphNodeBase.h"
#include "ComboGraphRuntimeTypes.h"
#include "GameplayTaskOwnerInterface.h"
#include "Engine/StreamableManager.h"
#include "ComboGraphNodeAnimBase.generated.h"

class UAnimationAsset;
class UAnimNotify;
class UAnimNotifyState;
class UComboGraphAbilityTask_StartGraph;
struct FAnimNotifyEvent;

UENUM(BlueprintType)
enum class EComboGraphAsyncHandlingMethod : uint8
{
	/** Async load of soft pointers in containers are handled upfront, gameplay cues get resolved objects in cue parameters */
	ResolvedObjects UMETA(DisplayName="Pass down resolved objects to cues"),

	/** Pass down just the source object paths to cues, letting the gameplay cue handler do the async loading if need be */
	SoftObjectPaths UMETA(DisplayName="Only pass down soft object paths")
};

UENUM(BlueprintType)
enum class EComboGraphDamageHandlingMethod : uint8
{
	/** Use Gameplay Ability System (Gameplay Effects) */
	AbilitySystem UMETA(DisplayName="Gameplay Ability System"),

	/** Use Unreal's Damage System (for Pawns not using Attributes and Effects from GAS) */
	DamageSystem UMETA(DisplayName="Unreal's Damage System")
};

/**
* Struct defining a list of gameplay tags, and optional source object to pass arbitrary data to GameplayCues (in GameplayCueParameters' SourceObject)
*/
USTRUCT(BlueprintType)
struct COMBOGRAPH_API FComboGraphCueContainerDefinition
{
	GENERATED_BODY()

public:
	/** Tags passed to the gameplay cue handler when this cue container is activated (multiple tags if matching valid gameplay cues will invoke them individually) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer, meta = (Categories="GameplayCue"))
	FGameplayTagContainer GameplayCueTags;

	/**
	 * Specify here which type of SourceObject you'd like to pass down to GameplayCues.
	 *
	 * Allows to pass arbitrary data down to GameplayCues, made available in GameplayCueParameters' Effect Context.
	 *
	 * Example usage: Pass down a Niagara Emitter or Cascade Particle System, or SoundCues.
	 */
	UPROPERTY(EditDefaultsOnly, Category = GameplayCueContainer)
	EComboGraphCueSourceObjectType CueSourceObjectType = EComboGraphCueSourceObjectType::None;

	// Edit Condition Hides may cause crashes in 4.26 (https://issues.unrealengine.com/issue/UE-117661)

	/** A particle system (Niagara) that if defined will be passed down to Gameplay Cues as SourceObject and available in params */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer, meta = (EditCondition = "CueSourceObjectType == EComboGraphCueSourceObjectType::Niagara"))
	// TSoftObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;
	//
	// /** A particle system (Cascade) that if defined will be passed down to Gameplay Cues as SourceObject and available in params */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer, meta = (EditCondition = "CueSourceObjectType == EComboGraphCueSourceObjectType::Cascade"))
	// TSoftObjectPtr<UParticleSystem> CascadeSystem = nullptr;
	//
	// /** The Sound to play (Cue, Wave, ...) that if defined will be passed down to Gameplay Cues as SourceObject and available in params */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer, meta = (EditCondition = "CueSourceObjectType == EComboGraphCueSourceObjectType::Sound"))
	// TSoftObjectPtr<USoundBase> SoundEffect = nullptr;

	/** A particle system (Niagara) that if defined will be passed down to Gameplay Cues as SourceObject and available in params */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer, meta = (EditCondition = "CueSourceObjectType == EComboGraphCueSourceObjectType::Niagara", EditConditionHides))
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	/** A particle system (Cascade) that if defined will be passed down to Gameplay Cues as SourceObject and available in params */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer, meta = (EditCondition = "CueSourceObjectType == EComboGraphCueSourceObjectType::Cascade", EditConditionHides))
	TSoftObjectPtr<UParticleSystem> CascadeSystem = nullptr;

	/** The Sound to play (Cue, Wave, ...) that if defined will be passed down to Gameplay Cues as SourceObject and available in params */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer, meta = (EditCondition = "CueSourceObjectType == EComboGraphCueSourceObjectType::Sound", EditConditionHides))
	TSoftObjectPtr<USoundBase> SoundEffect = nullptr;

	void GetAssetPathsForAsyncLoad(TArray<FSoftObjectPath>& OutSoftObjectPaths) const
	{
		if (CueSourceObjectType == EComboGraphCueSourceObjectType::Niagara)
		{
			OutSoftObjectPaths.Add(NiagaraSystem.ToSoftObjectPath());
		}
		else if (CueSourceObjectType == EComboGraphCueSourceObjectType::Cascade)
		{
			OutSoftObjectPaths.Add(CascadeSystem.ToSoftObjectPath());
		}
		else if (CueSourceObjectType == EComboGraphCueSourceObjectType::Sound)
		{
			OutSoftObjectPaths.Add(SoundEffect.ToSoftObjectPath());
		}
	}

	FString GetDebugString() const
	{
		FString AssetPath;
		if (CueSourceObjectType == EComboGraphCueSourceObjectType::Niagara)
		{
			AssetPath = NiagaraSystem.ToSoftObjectPath().ToString();
		}
		else if (CueSourceObjectType == EComboGraphCueSourceObjectType::Cascade)
		{
			AssetPath = CascadeSystem.ToSoftObjectPath().ToString();
		}
		else if (CueSourceObjectType == EComboGraphCueSourceObjectType::Sound)
		{
			AssetPath = SoundEffect.ToSoftObjectPath().ToString();
		}
		else
		{
			AssetPath = TEXT("NONE");
		}

		return FString::Printf(TEXT("Cue Container Definition: %s (%s)"), *GameplayCueTags.ToString(), *AssetPath);
	}

	/** Returns appropriate SourceObject depending on currently selected CueSourceObjectType (Niagara, Cascade or SoundEffect) */
	UObject* GetSourceObject() const;
};

/**
* Struct holding a list of GameplayCueContainer definitions with gameplay tags, and optional source object to pass arbitrary data to GameplayCues (in GameplayCueParameters' SourceObject)
*/
USTRUCT(BlueprintType)
struct COMBOGRAPH_API FComboGraphCueContainer
{
	GENERATED_BODY()

public:

	/** List of gameplay tags (that should be associated with Gameplay Cues), and optional source object to pass arbitrary data to GameplayCues (in GameplayCueParameters' SourceObject) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayCueContainer)
	TArray<FComboGraphCueContainerDefinition> Definitions;

	/** Returns definitions sharing same GameplayCueTag as one along the list of resolved CueParamsObjects for each to include in Gameplay Cue Params */
	void GetAggregatedDefinitionsAndObjects(TMap<FGameplayTag, FComboGraphCueContainerDefinition>& OutDefinitions, TArray<TWeakObjectPtr<UObject>>& OutSourceObjects)
	{
		for (FComboGraphCueContainerDefinition Definition : Definitions)
		{
			TArray<FGameplayTag> GameplayCueTags;
			Definition.GameplayCueTags.GetGameplayTagArray(GameplayCueTags);

			OutSourceObjects.Add(Definition.GetSourceObject());

			for (FGameplayTag GameplayCueTag : GameplayCueTags)
			{
				OutDefinitions.FindOrAdd(GameplayCueTag, Definition);
			}
		}
	}

	/** Returns definitions sharing same GameplayCueTag as one along the list of Soft Object Paths for each to include in Gameplay Cue Params */
	void GetAggregatedDefinitionsAndPaths(TMap<FGameplayTag, FComboGraphCueContainerDefinition>& OutDefinitions, TArray<FSoftObjectPath>& OutSoftObjectPaths)
	{
		for (FComboGraphCueContainerDefinition Definition : Definitions)
		{
			TArray<FGameplayTag> GameplayCueTags;
			Definition.GameplayCueTags.GetGameplayTagArray(GameplayCueTags);

			Definition.GetAssetPathsForAsyncLoad(OutSoftObjectPaths);

			for (FGameplayTag GameplayCueTag : GameplayCueTags)
			{
				OutDefinitions.FindOrAdd(GameplayCueTag, Definition);
			}
		}
	}
};

/**
 *  Base Class for all Anim related Combo Graph nodes (montage or sequence)
 *
 *  Holds runtime properties for animation and effects / cues containers.
 */
UCLASS()
class COMBOGRAPH_API UComboGraphNodeAnimBase : public UComboGraphNodeBase, public IGameplayTaskOwnerInterface
{
	GENERATED_BODY()

public:
	/** Anim montage play rate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0"))
	float MontagePlayRate = 1.f;

	/** Sets scale to apply to root motion translation on the animating character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0"))
	float RootMotionScale = 1.f;

	/**
	 * If true, this montage will be aborted if the ability ends normally. It is always stopped when the ability is explicitly cancelled.
	 *
	 * This value is passed down to the ability task playing the montage, and corresponds to bStopWhenAbilityEnds parameter value of the PlayMontageAndWait ability task.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	bool bStopAnimationWhenAbilityEnds = true;

	/**
	 * Specify which method to use to handle Effects Containers
	 *
	 * When using Gameplay Abilities, you should use `AbilitySystem` method and go through Gameplay Effects to modify your Attributes.
	 *
	 * Non Gameplay Abilities user can fallback to Unreal's Damage System.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects")
	EComboGraphDamageHandlingMethod DamageHandlingMethod = EComboGraphDamageHandlingMethod::AbilitySystem;

	/** Map of gameplay tags we're listening to gameplay effect containers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects", meta = (EditCondition = "DamageHandlingMethod == EComboGraphDamageHandlingMethod::AbilitySystem", EditConditionHides))
	TMap<FGameplayTag, FComboGraphGameplayEffectContainer> EffectsContainerMap;

	/** Map of gameplay tags we're listening to damage containers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects", meta = (EditCondition = "DamageHandlingMethod == EComboGraphDamageHandlingMethod::DamageSystem", EditConditionHides))
	TMap<FGameplayTag, FComboGraphDamageSystemContainer> DamagesContainerMap;

	/** This GameplayEffect represents the cost (mana, stamina, etc) of the combo node. It will be applied when the node is committed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects", meta = (EditCondition = "DamageHandlingMethod == EComboGraphDamageHandlingMethod::AbilitySystem", EditConditionHides))
	TSubclassOf<UGameplayEffect> CostGameplayEffect;

	/**
	 * This tag container can be used to define Gameplay Tags to listen to with the underlying "Play Montage and Wait For Event" task.
	 *
	 * While the animation is playing, if a gameplay event with one of those tags is sent to the owning actor, the "EventReceived" delegate
	 * for the "StartComboGraph" task will be called with the Gameplay Tag for the event and Gameplay Event Data structure.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Events")
	FGameplayTagContainer EventTags;

	/**
	 * Async handling method for gameplay cue source objects in containers below (not exposed and set to resolved for now)
	 *
	 * Gameplay Cues implemented in Blueprints can do the async loading but actually playing sound or niagara / particles after a few frames
	 * seems to not be supported (at least for Gameplay Cue Statics). For this reason, we set it by default to resolved always.
	 */
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Cues (Hit Impacts)")
	EComboGraphAsyncHandlingMethod AsyncHandlingMethod = EComboGraphAsyncHandlingMethod::ResolvedObjects;

	/** Map of gameplay tags we're listening to to gameplay cues containers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Cues (Hit Impacts)")
	TMap<FGameplayTag, FComboGraphCueContainer> CuesContainerMap;

	/**
	 * Notify states overrides for Auto Setup.
	 *
	 * Before going through play montage task, adds all items from this map to plugin settings map (if any keys are in both, the value from the node map wins).
	 *
	 * If animation asset (montage or sequence) being played is using one of the AnimNotifyState, the auto setup for this notify is ignored (notifies in anim assets win).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Auto Setup")
	TMap<TSoftClassPtr<UAnimNotifyState>, FComboGraphNotifyStateAutoSetup> NotifyStatesOverrides;

	/** Returns Montage property or a dynamically created Montage from Sequence */
	virtual UAnimMontage* GetMontage() const { return nullptr; }

	/** Returns underlying Anim Asset (either Montage or Sequence) */
	virtual UAnimSequenceBase* GetAnimationAsset() const { return nullptr; }

	/** Returns underlying Anim Asset Length (PlayLength) */
	virtual float GetAnimationLength() const { return -1.f; }

	/** Simple getter to return StartSection Name property on montage node (sequences will always return None) */
	virtual FName GetAnimationStartSection() const { return NAME_None; }

	/** Returns play time for a given section on a node montage (sequence will always return -1.0) */
	virtual float GetSectionLength(FName SectionName) const { return -1.f; }

	/** Returns total animation play time and subtract provided section time from the result (sequence will always return -1.0) */
	virtual float GetAnimationLengthMinusSection(FName SectionName) const { return -1.f; }

	/** Returns total animation play time and subtract section defined by StartSection name property if it is defined (sequence will always return sequence length) */
	virtual float GetAnimationLengthMinusStartSection() const { return -1.f; }

	/** Meant to be overriden by child class to update their AnimationAsset reference */
	virtual void SetAnimationAsset(UAnimationAsset* Asset) { check(false); /*Base function called*/ }
	virtual bool SupportsAssetClass(UClass* AssetClass) { return false; }

#if WITH_EDITOR
	// These gets called from the Slate SGraphNode Widget in graphs
	virtual FText GetAnimAssetLabel() const { return FText::FromString(""); };
	virtual FText GetAnimAssetLabelTooltip() const { return FText::FromString(""); };
	virtual FText GetAnimAssetText() const { return FText::FromString(""); };
#endif

	/** BP exposed API */

	/** Returns title for the node as displayed in the editor (either name of the animation asset or if defined the NodeTitle property) */
	UFUNCTION(BlueprintCallable, DisplayName = "GetNodeTitle", Category = "Combo Graph")
	FText K2_GetNodeTitle() const;

	/** Returns the animation asset defined in editor for the node. Can be either a sequence or montage. */
	UFUNCTION(BlueprintCallable, DisplayName = "GetAnimationAsset", Category = "Combo Graph")
	UAnimSequenceBase* K2_GetAnimationAsset() const;

	/**
	 * Returns the type as text of underlying animation asset for the node. Can be either a sequence or montage.
	 *
	 * Effectively just calling AnimationAsset->GetClass();
	 */
	UFUNCTION(BlueprintCallable, DisplayName = "GetAnimationClass", Category = "Combo Graph")
	UClass* K2_GetAnimationClass() const;

	/** Returns the list of children nodes for this node, that is direct descendant in the graph hierarchy. */
	UFUNCTION(BlueprintCallable, DisplayName = "GetChildren", Category = "Combo Graph")
	TArray<UComboGraphNodeAnimBase*> K2_GetChildren() const;

	/** Returns the ComboGraph UObject this node is part of. */
	UFUNCTION(BlueprintCallable, DisplayName = "GetOwningGraph", Category = "Combo Graph")
	UComboGraph* K2_GetOwningGraph() const;

	/** Returns the owning ability task this node currently being played in. Should only return valid instance if the task is currently active / running. */
	UFUNCTION(BlueprintCallable, DisplayName = "GetOwningTask", Category = "Combo Graph")
	UComboGraphAbilityTask_StartGraph* K2_GetOwningTask() const;

	/** Returns the owning gameplay ability this node currently being played in. Should only return valid instance if the ability is currently active / running. */
	UFUNCTION(BlueprintCallable, DisplayName = "GetOwningAbility", Category = "Combo Graph")
	UGameplayAbility* K2_GetOwningAbility() const;

	/** Returns the direct parent node that this node was transitioned from. */
	UFUNCTION(BlueprintCallable, DisplayName = "GetPreviousNode", Category = "Combo Graph")
	UComboGraphNodeAnimBase* K2_GetPreviousNode() const;
	
	/** Returns the direct parent node that this node was transitioned from. */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	AActor* GetOwnerActor() const;
	
	/** Returns the direct parent node that this node was transitioned from. */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	AActor* GetAvatarActor() const;
	
	/** Returns the direct parent node that this node was transitioned from. */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	APawn* GetAvatarAsPawn() const;
	
	/** Returns the direct parent node that this node was transitioned from. */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	ACharacter* GetAvatarAsCharacter() const;

	/**
	 * Used by Combo Graph Task to calculate Damage Container damage to apply or Effect Container set by caller magnitudes before being applied via Effects.
	 *
	 * Can be overridden in Blueprint, and should return the new computed magnitude value (default native implementation simply return the Container Magnitude value)
	 *
	 * @param OriginalMagnitude Original magnitude or base damage value defined in Combo Node for the Effect or Damage Container (depends on DamageHandlingMethod)
	 * @param ElapsedSeconds Time in seconds returned by the Enhanced Input system. Total time the action has been evaluating triggering (Ongoing & Triggered)
	 * @param TriggeredSeconds Time in seconds returned by the Enhanced Input system. Time the action has been actively triggered (Triggered only)
	 * @param AnimationLength Total animation play time with section time defined by StartSection on montage node subtracted from the result. If no StartSection is defined (None), total animation play time is used
	 * @param MontagePlayTime Actual Play time in seconds for the montage at the time the container is applied
	 * @param InputMagnitude Normalized magnitude value ranging from 0 to 1 based on Enhanced Input Elapsed Time and Animation Length (1 means Elapsed Time is at or above the total animation length)
	 * @param ContainerTag Gameplay Tag key for the Effect or Damage container (depends on DamageHandlingMethod). Can be used to get back original container from DamagesContainerMap or EffectsContainerMap
	 * @return New adjusted magnitude value
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Combo Graph")
	float GetMagnitudeForContainer(float OriginalMagnitude, float ElapsedSeconds, float TriggeredSeconds, float AnimationLength, float MontagePlayTime, float InputMagnitude, FGameplayTag ContainerTag) const;

	/** Returns true if this node can be activated right now. Has no side effects. */
	UFUNCTION(BlueprintNativeEvent, DisplayName="CanActivateNode", Category = "Combo Graph")
	bool K2_CanActivateNode() const;

	/** Event triggered when the node is activated and transitioned to in the owning graph, before CanActivateNode checks / cost commit and activation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combo Graph|Events")
	void OnInitialized();

	/**
	 * Event triggered when the node is activated and transitioned to in the owning graph.
	 *
	 * This only triggers if checks (CanActivateNode / Cost commit) have been successful and not preventing execution.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combo Graph|Events")
	void OnActivated();

	/** Event triggered when the node is ending and owning graph is transitioning to the next node (or ending the graph). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combo Graph|Events")
	void OnDeactivated();

	/** Event triggered when the animation asset for this node is played as an anim montage in the owning ability task and gameplay ability. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combo Graph|Events")
	void OnMontagePlay(UAnimMontage* Montage);

	/**
	 * Event triggered when the actor owning the combo graph this node is part of is receiving a gameplay event this node listens to
	 * (either with the EventTags property or if it is defining Gameplay Effects or Cues Containers).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combo Graph|Events")
	void OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);

	//~ Begin IGameplayTaskOwnerInterface
	virtual UGameplayTasksComponent* GetGameplayTasksComponent(const UGameplayTask& Task) const override;
	virtual AActor* GetGameplayTaskOwner(const UGameplayTask* Task) const override;
	virtual AActor* GetGameplayTaskAvatar(const UGameplayTask* Task) const override;
	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;
	//~ End IGameplayTaskOwnerInterface

	// -- Cue Containers --

	/** Gather assets to load for a cue container and request async load from Streamable Manager, invoking DoneDelegate when loading is complete. */
	void AsyncLoadContainer(FComboGraphCueContainer* Container, FStreamableDelegate DoneDelegate = FStreamableDelegate()) const;

	/** Applies a gameplay cue container if one is matching EventTag */
	void ApplyCuesContainerForTag(const FGameplayTag& ContainerTag, const FGameplayEventData& EventData);

	/** Given a Cue Container, will build up the appropriate Gameplay Cue Parameters along custom effect context with optional assets to pass in and execute gameplay cue via Cached Ability's ASC */
	void ApplyCueContainer(FComboGraphCueContainer* Container, const FGameplayEventData& EventData, bool bPassOnlySoftObjectPaths = true);

	/**  Create common gameplay cue parameters with custom effect context to pass down to gameplay cues */
	FGameplayCueParameters CreateGameplayCueParameters(const FGameplayEventData& EventData) const;

	// -- Anim Notify Helpers --

	/** Helper to return the combo notify state event from animation this node is using. Here searching for a notify state class */
	FAnimNotifyEvent* GetAnimNotify(TSubclassOf<UAnimNotifyState> NotifyStateClass) const;
	bool HasAnimNotifyState(TSubclassOf<UAnimNotifyState> NotifyStateClass) const;

	/** Helper to return the combo notify state event from animation this node is using. Here searching for a notify class */
	FAnimNotifyEvent* GetAnimNotify(TSubclassOf<UAnimNotify> NotifyClass) const;
	bool HasAnimNotify(TSubclassOf<UAnimNotify> NotifyClass) const;

	/** Helper to return the combo notify state event from animation this node is using. Here searching by name */
	FAnimNotifyEvent* GetAnimNotify(FName NotifyName) const;
	bool HasAnimNotify(FName NotifyName) const;

	float GetAnimNotifyTriggerTime(TSubclassOf<UAnimNotify> NotifyClass) const;
	float GetAnimNotifyTriggerTime(FName NotifyName) const;

protected:

	/** Cached pointer to owning combo graph ability task, mostly to provide access to its API from within combo nodes BPs */
	TWeakObjectPtr<UGameplayTask> CachedOwningTask;
	
	/** Cached pointer to owning combo graph ability, mostly to provide access to owning ASC and actor info when applying cue containers */
	TWeakObjectPtr<UGameplayAbility> CachedOwningAbility;

private:
	FString GetAbilitySystemGlobalsWarning() const;
};
