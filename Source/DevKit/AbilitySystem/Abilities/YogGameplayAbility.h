// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "YogAbilityTypes.h"
#include "Engine/DataTable.h"
#include "../../Component/HitBoxBufferComponent.h"
//#include <DevKit/Data/AbilityData.h>


#include "YogGameplayAbility.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;
struct FActionData;

class AActor;
class AController;
class FText;
class UAnimMontage;
class UGameplayEffect;
class UAbilityData;

struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEffectSpec;
struct FGameplayEventData;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityStartSignature);


UCLASS()
class DEVKIT_API UYogGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UYogGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	/** Called when this ability is granted to the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	void K2_OnAbilityAdded();

	/** Called when this ability is removed from the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
	void K2_OnAbilityRemoved();



	UPROPERTY(BlueprintAssignable)
	FAbilityEndedSignature EventOn_AbilityEnded;

	UPROPERTY(BlueprintAssignable)
	FAbilityStartSignature EventOn_AbilityStart;


	///** Map of gameplay tags to gameplay effect containers */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage Effect)
	//TMap<FGameplayTag, FYogGameplayEffectContainer> EffectContainerMap;

	/** Make gameplay effect container spec to be applied later, using the passed in container */
	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	virtual FYogGameplayEffectContainerSpec MakeEffectContainerSpecFromContainer(const FYogGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	virtual FYogGameplayEffectContainerSpec MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);


	/** Applies a gameplay effect container spec that was previously created */
	UFUNCTION(BlueprintCallable, Category = Ability)
	virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainerSpec(const FYogGameplayEffectContainerSpec& ContainerSpec);

	/** Applies a gameplay effect container, by creating and then applying the spec */
	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);


    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Data")
    //TObjectPtr<UDataTable> YogAbilityDataTable;


	UPROPERTY()
	FAbilityType AbilityTypeData;

public:

	//TODO: SET bRetriggerInstancedAbility for RetriggerAbility in notify

	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DamageBox")
	//TArray<FHitBoxData> array_Hitbox;

	UPROPERTY(BlueprintReadOnly)
	FActionData AbilityActData;

	UFUNCTION(BlueprintCallable, Category = "Setting")
	void UpdateRetrigger(bool retriggerable);

	UFUNCTION(BlueprintCallable, Category = "Ability Data")
	void GetAbilityTableData();

	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	AYogCharacterBase* GetOwnerCharacterInfo();


	UFUNCTION(BlueprintCallable, Category = "GameplayEffect")
	int GetCurrentGameplayEffect(FGameplayTag EffectTag);



public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "ActionData"))
	FDataTableRowHandle ActionRow;

	UFUNCTION(BlueprintCallable)
	FActionData GetRowData();

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<UYogGameplayAbility> Ability;


protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled);
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

};
