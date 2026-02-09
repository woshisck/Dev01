// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTaskGameplayAbilityEnd.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAsyncTaskGameplayAbilityEndedEv);

class UYogGameplayAbility;
struct FGameplayTagContainer;
struct FGameplayTag;
/**
 * 
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class DEVKIT_API UAsyncTaskGameplayAbilityEnd : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintAssignable)
	FAsyncTaskGameplayAbilityEndedEv OnEnded;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncTaskGameplayAbilityEnd* ListenForGameplayAbilityEnd(UAbilitySystemComponent* abilitySystemComponent, TSubclassOf<UGameplayAbility> abilityClass);

	// You must call this function manually when you want the AsyncTask to end.
	// For UMG Widgets, you would call it in the Widget's Destruct event.
	//UFUNCTION(BlueprintCallable)
	//void EndTask();

	virtual void Activate() override;

protected:


	TObjectPtr<UYogGameplayAbility> AbilityListeningTo;

	//UFUNCTION()
	//virtual void OnCallback(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	virtual void OnCallback();
};
