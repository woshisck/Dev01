// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GSCTypes.h"
#include "Abilities/GSCAbilitySet.h"
#include "GSCAbilitySystemComponent.generated.h"

class UGSCAbilityInputBindingComponent;
class UGSCComboManagerComponent;
class UInputAction;

USTRUCT(BlueprintType)
struct FGSCAbilityInputMapping
{
	GENERATED_BODY()

	/** Type of ability to grant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	TSubclassOf<UGameplayAbility> Ability;
	
	/** Level to grant the ability at */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	int32 Level = 1;

	/** Input action to bind the ability to, if any (can be left unset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	TObjectPtr<UInputAction> InputAction = nullptr;

	/**
	 * The EnhancedInput Trigger Event type to use for the ability activation on pressed handle.
	 *
	 * ---
	 *
	 * The most common trigger types are likely to be Started for actions that happen once, immediately upon pressing a button,
	 * and Triggered for continuous actions that happen every frame while holding an input
	 *
	 * Warning: The Triggered value should only be used for Input Actions that you know only trigger once. If your action
	 * triggered event happens on every tick, this might lead to issues with ability activation (since you'll be
	 * trying to activate abilities every frame). When in doubt, use the default Started value.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability, meta=(EditCondition = "InputAction != nullptr", EditConditionHides))
	EGSCAbilityTriggerEvent TriggerEvent = EGSCAbilityTriggerEvent::Started;
};

USTRUCT(BlueprintType)
struct FGSCAttributeSetDefinition
{
	GENERATED_BODY()

	/** Attribute Set to grant */
	UPROPERTY(EditAnywhere, Category=Attributes)
	TSubclassOf<UAttributeSet> AttributeSet;

	/** Data table reference to initialize the attributes with, if any (can be left unset) */
	UPROPERTY(EditAnywhere, Category=Attributes, meta = (RequiredAssetDataTags = "RowStructure=/Script/GameplayAbilities.AttributeMetaData"))
	TObjectPtr<UDataTable> InitializationData = nullptr;
};

USTRUCT()
struct FGSCMappedAbility
{
	GENERATED_BODY()

	FGameplayAbilitySpecHandle Handle;
	FGameplayAbilitySpec Spec;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InputAction;

	FGSCMappedAbility(): InputAction(nullptr)
	{
	}

	FGSCMappedAbility(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilitySpec& Spec, UInputAction* const InputAction)
		: Handle(Handle),
		  Spec(Spec),
		  InputAction(InputAction)
	{
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGSCOnGiveAbility, FGameplayAbilitySpec&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSCOnInitAbilityActorInfo);

/**
 * Revamped Ability System Component for 3.0.0
 *
 * This one is meant to be attached in Blueprint (or via a GameFeature), although 4.27 still requires ASC and IAbilitySystemInterface to be implemented in cpp.
 */
UCLASS(ClassGroup="GASCompanion", meta=(BlueprintSpawnableComponent))
class GASCOMPANION_API UGSCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** List of Gameplay Abilities to grant when the Ability System Component is initialized */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	TArray<FGSCAbilityInputMapping> GrantedAbilities;

	/** List of Attribute Sets to grant when the Ability System Component is initialized, with optional initialization data */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	TArray<FGSCAttributeSetDefinition> GrantedAttributes;

	/** List of GameplayEffects to apply when the Ability System Component is initialized (typically on begin play) */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;

	/** List of Gameplay Ability Sets to grant when the Ability System Component is initialized */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	TArray<TSoftObjectPtr<UGSCAbilitySet>> GrantedAbilitySets;

	/**
	 * Event called just after InitAbilityActorInfo, once abilities and attributes have been granted.
	 *
	 * This will happen multiple times for both client / server:
	 *
	 * - Once for Server after component initialization
	 * - Once for Server after replication of owning actor (Possessed by for Player State)
	 * - Once for Client after component initialization
	 * - Once for Client after replication of owning actor (Once more for Player State OnRep_PlayerState)
	 *
	 * Also depends on whether ASC lives on Pawns or Player States.
	 */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Abilities")
	FGSCOnInitAbilityActorInfo OnInitAbilityActorInfo;

	/**
	 * Specifically set abilities to persist across deaths / respawns or possessions.
	 *
	 * When this is set to false, abilities will only be granted the first time InitAbilityActor is called. This is the default
	 * behavior for ASC living on Player States (GSCModularPlayerState specifically).
	 *
	 * Do not set it true for ASC living on Player States if you're using ability input binding. Only ASC living on Pawns supports this.
	 * 
	 * (Default is true)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	bool bResetAbilitiesOnSpawn = true;

	/**
	 * Specifically set attributes to persist across deaths / respawns or possessions.
	 *
	 * When this is set to false, attributes are only granted the first time InitAbilityActor is called. This is the default
	 * behavior for ASC living on Player States (GSCModularPlayerState specifically).
	 *
	 * Set it (or leave it) to true if you want attribute values to be re-initialized to their default values.
	 * 
	 * (Default is true)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	bool bResetAttributesOnSpawn = true;

	/** Delegate invoked OnGiveAbility (when an ability is granted and available) */
	FGSCOnGiveAbility OnGiveAbilityDelegate;

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	//~ End UActorComponent interface

	//~ Begin UObject interface
	virtual void BeginDestroy() override;
	//~ End UObject interface

	//~ Begin UAbilitySystemComponent interface
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	/**
	 * Overrides InputPressed to conditionally ActivateComboAbility or regular TryActivateAbility based on AbilitySpec Ability CDO
	 * (if child of GSCMeleeAbility, will activate combo via combo component)
	 */
	virtual void AbilityLocalInputPressed(int32 InputID) override;
	//~ End UAbilitySystemComponent interface

	/**
	 * DEPRECATED: Please use regular ASC->GiveAbility() method instead.
	 *
	 * Helper to grant an Ability to the ASC. Since 5.0 introduced a BlueprintCallable for this, you should use ASC->GiveAbility() instead.
	 */
	UE_DEPRECATED(5.2, "Please use regular ASC->GiveAbility() method instead.")
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GAS Companion|Abilities", meta=(DeprecatedFunction, DeprecationMessage="Please use GiveAbility() method of engine's UAbilitySystemComponent instead. Or the newly added GiveAbilityWithInput method of UGSCAbilityInputBindingComponent."))
	FGameplayAbilitySpecHandle GrantAbility(TSubclassOf<UGameplayAbility> Ability, bool bRemoveAfterActivation);
	
	/**
	 * Grants a given Ability Set to the ASC, adding defined Abilities, Attributes, Effects and Owned Tags.
	 *
	 * This method is meant to run on both Authority (must be called from server), and on Client if you'd like to setup binding as well (Important to call on client too for Owned Tags)
	 *
	 * During Pawn initialization, if you'd like to grant a list of Ability Sets manually with this method, the typical place to do so is:
	 *
	 * - OnInitAbilityActorInfo (event exposed by both UGSCAbilitySystemComponent and UGSCCoreComponent)
	 * - OnBeginPlay but only if ASC is on the Pawn (not using GSCModularPlayerState to hold the ASC)
	 * 
	 * Both Player State pawns and non Player State pawns can use OnInitAbilityActorInfo, while only non Player State pawns can use OnBeginPlay to grant the ability.
	 * 
	 * Also, for input binding to work and register properly, the avatar actor for this ASC must have UGSCAbilityInputBindingComponent actor component.
	 * 
	 * @param InAbilitySet The Ability Set to grant to the ASC
	 * @param OutHandle Handle that can be used to remove the set later on
	 * 
	 * @return True if the ability set was granted successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Ability Sets")
	bool GiveAbilitySet(const UGSCAbilitySet* InAbilitySet, FGSCAbilitySetHandle& OutHandle);
	
	/**
	 * Removes the AbilitySet represented by InAbilitySetHandle from the passed in ASC. Clears out any previously granted Abilities,
	 * Attributes, Effects and Owned Tags from the set.
	 *
	 * Like granting, it is advised to call this method on both Server and Client for multiplayer games.
	 * 
	 * @param InAbilitySetHandle Handle of the Ability Set to remove
	 * 
	 * @return True if the ability set was removed successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Ability Sets")
	bool ClearAbilitySet(UPARAM(ref) FGSCAbilitySetHandle& InAbilitySetHandle);

	//~ Those are Delegate Callbacks register with this ASC to trigger corresponding events on the Owning Character (mainly for ability queuing)
	virtual void OnAbilityActivatedCallback(UGameplayAbility* Ability);
	virtual void OnAbilityFailedCallback(const UGameplayAbility* Ability, const FGameplayTagContainer& Tags);
	virtual void OnAbilityEndedCallback(UGameplayAbility* Ability);

	/** Called when Ability System Component is initialized from InitAbilityActorInfo */
	virtual void GrantDefaultAbilitiesAndAttributes(AActor* InOwnerActor, AActor* InAvatarActor);
	
	/** Called when Ability System Component is initialized from InitAbilityActorInfo */
	virtual void GrantDefaultAbilitySets(AActor* InOwnerActor, AActor* InAvatarActor);

	/** Called from GrantDefaultAbilitiesAndAttributes. Determine if ability should be granted, prevents re-adding an ability previously granted in case bResetAbilitiesOnSpawn is set to false */
	virtual bool ShouldGrantAbility(TSubclassOf<UGameplayAbility> InAbility, const int32 InLevel = 1);
	
	/** Called from GrantDefaultAbilitySets. Determine if ability set should be granted, prevents re-granting a set previously added */
	virtual bool ShouldGrantAbilitySet(const UGSCAbilitySet* InAbilitySet) const;

	/** Returns true whether the current owner actor is of type PlayerState */
	bool IsPlayerStateOwner() const;

protected:
	// Cached granted Ability Handles
	UPROPERTY(transient)
	TArray<FGSCMappedAbility> AddedAbilityHandles;

	// Cached granted AttributeSets
	UPROPERTY(transient)
	TArray<TObjectPtr<UAttributeSet>> AddedAttributes;

	// Cached applied Startup Effects
	UPROPERTY(transient)
	TArray<FActiveGameplayEffectHandle> AddedEffects;
	
	// Cached granted Ability Sets
	UPROPERTY(transient)
	TArray<FGSCAbilitySetHandle> AddedAbilitySets;

	// Keep track of OnGiveAbility handles bound to handle input binding on clients
	TArray<FDelegateHandle> InputBindingDelegateHandles;

	// Cached ComboComponent on Character (if it has any)
	UPROPERTY()
	TObjectPtr<UGSCComboManagerComponent> ComboComponent;

	//~ Begin UAbilitySystemComponent interface
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	//~ End UAbilitySystemComponent interface

	/** Called when Ability System Component is initialized */
	void GrantStartupEffects();

	/** Reinit the cached ability actor info (specifically the player controller) */
	UFUNCTION()
	void OnPawnControllerChanged(APawn* Pawn, AController* NewController);

	/** Handler for AbilitySystem OnGiveAbility delegate. Sets up input binding for clients (not authority) when ability is granted and available for binding. */
	virtual void HandleOnGiveAbility(FGameplayAbilitySpec& AbilitySpec, UGSCAbilityInputBindingComponent* InputComponent, UInputAction* InputAction, EGSCAbilityTriggerEvent TriggerEvent, FGameplayAbilitySpec NewAbilitySpec);
};
