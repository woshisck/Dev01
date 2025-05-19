// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "YogAbilityTypes.h"
#include "Engine/DataTable.h"



#include "YogGameplayAbility.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;


class AActor;
class AController;
class FText;
class UAnimMontage;
class UGameplayEffect;


class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEffectSpec;
struct FGameplayEventData;

struct FGameplayEventData;

USTRUCT(BlueprintType)
struct FYogAbilityData : public FTableRowBase
{
    GENERATED_BODY()

public:
	FYogAbilityData()
		: Damage(0.0f), DMGAmplify(0.0f), MontagePlayRate(0.0f), AbilityPower(1.0f)
	{
	}
    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DMGAmplify;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MontagePlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityPower;
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityEndedSignature);


UCLASS()
class DEVKIT_API UYogGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UYogGameplayAbility();


	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);

	/** Called when this ability is granted to the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	void K2_OnAbilityAdded();

	/** Called when this ability is removed from the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
	void K2_OnAbilityRemoved();


	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FAbilityEndedSignature EventOn_AbilityEnded;


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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Data")
    TObjectPtr<UDataTable> YogAbilityDataTable;

public:

	UFUNCTION(BlueprintCallable, Category = "Ability Data")
	void GetAbilityTableData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Data")
    FName DataRowName;


	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	AYogCharacterBase* GetOwnerCharacterInfo();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Data")
	FString AbilityTableRowName;

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled);

};
