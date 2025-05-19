// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTaskGameplayAbilityEnd.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAsyncTaskGameplayAbilityEnded);

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

public:
	UAsyncTaskGameplayAbilityEnd(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FOnAsyncTaskGameplayAbilityEnded OnEnded;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UAsyncTaskGameplayAbilityEnd* ListenForGameplayAbilityEnd(UObject* InWorldContextObject, UAbilitySystemComponent* abilitySystemComponent, TSubclassOf<UYogGameplayAbility> abilityClass);

	// You must call this function manually when you want the AsyncTask to end.
	// For UMG Widgets, you would call it in the Widget's Destruct event.
	//UFUNCTION(BlueprintCallable)
	//void EndTask();

	virtual void Activate() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> TargetASC;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UYogGameplayAbility> AbilityClass;

	TObjectPtr<UYogGameplayAbility> AbilityListeningTo;


	//UFUNCTION()
	//virtual void OnCallback(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void OnCallback();


	TWeakObjectPtr<UWorld> WorldPtr;
};
