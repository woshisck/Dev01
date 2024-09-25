// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Components/ActorComponent.h"
#include "GSCComboManagerComponent.generated.h"

class UGSCCoreComponent;
class UAbilitySystemComponent;
class UGSCGameplayAbility;
class ACharacter;

UCLASS(BlueprintType, Blueprintable, ClassGroup=("GASCompanion"), meta=(BlueprintSpawnableComponent))
class GASCOMPANION_API UGSCComboManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// Sets default values for this component's properties
	UGSCComboManagerComponent();

	UPROPERTY()
	TObjectPtr<ACharacter> OwningCharacter;

	UPROPERTY()
	TObjectPtr<UGSCCoreComponent> OwnerCoreComponent;

	/** Reference to GA_GSC_Melee_Base */
	TSubclassOf<UGSCGameplayAbility> MeleeBaseAbility;

	/** The combo index for the currently active combo */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "GAS Companion|Combo")
	int32 ComboIndex = 0;

	/** Whether or not the combo window is opened (eg. player can queue next combo within this window) */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "GAS Companion|Combo")
	bool bComboWindowOpened = false;

	/** Should we queue the next combo montage for the currently active combo */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "GAS Companion|Combo")
	bool bShouldTriggerCombo = false;

	/** Should we trigger the next combo montage */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "GAS Companion|Combo")
	bool bRequestTriggerCombo = false;

	/** Should we trigger the next combo montage */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "GAS Companion|Combo")
	bool bNextComboAbilityActivated = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Setup GetOwner to character and sets references for ability system component and the owner itself. */
	void SetupOwner();

	/** Get the currently active combo ability */
	UGameplayAbility* GetCurrentActiveComboAbility() const;

	/** Part of the combo system, will increment the ComboIndex counter, only if the combo window has been opened */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|Combat")
	void IncrementCombo();

	/** Part of the combo system, will reset the ComboIndex counter to 0 */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|Combat")
	void ResetCombo();

	/** Part of the combo system, gate combo ability activation based on if character is already using the ability */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|Combat")
	void ActivateComboAbility(TSubclassOf<UGSCGameplayAbility> AbilityClass, bool bAllowRemoteActivation = true);

	void SetComboIndex(int32 InComboIndex);

	/** Returns true if this component's actor has authority */
	virtual bool IsOwnerActorAuthoritative() const;

protected:
	/** Cached value of rather this is a simulated actor */
	UPROPERTY()
	bool bCachedIsNetSimulated;

	//~Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	//~End UActorComponent interface

	UFUNCTION(Server, Reliable)
	void ServerActivateComboAbility(TSubclassOf<UGSCGameplayAbility> AbilityClass, bool bAllowRemoteActivation = true);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastActivateComboAbility(TSubclassOf<UGSCGameplayAbility> AbilityClass, bool bAllowRemoteActivation = true);

	void ActivateComboAbilityInternal(TSubclassOf<UGSCGameplayAbility> AbilityClass, bool bAllowRemoteActivation = true);

	UFUNCTION(Server, Reliable)
	void ServerSetComboIndex(int32 InComboIndex);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetComboIndex(int32 InComboIndex);

private:
	/** Caches the flags that indicate whether this component has network authority. */
	void CacheIsNetSimulated();
};
