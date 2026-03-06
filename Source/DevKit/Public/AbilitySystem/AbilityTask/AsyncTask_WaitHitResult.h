// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AsyncTask_WaitHitResult.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWaitReceiveHitResultDelegate, class UYogAbilitySystemComponent*, SourceASC, bool, Hitsuccess);


/**
 * 
 */
UCLASS()
class DEVKIT_API UAsyncTask_WaitHitResult : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FWaitReceiveHitResultDelegate OnHitResult;

	virtual void Activate() override;
	

	UFUNCTION()
	void OnHitResultReceived(class UYogAbilitySystemComponent* SourceASC, bool HitResult);

	// Wait until the ability owner receives damage.
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAsyncTask_WaitHitResult* WaitReceiveHitResult(UGameplayAbility* OwningAbility, bool TriggerOnce);

protected:
	bool TriggerOnce;

	virtual void OnDestroy(bool AbilityIsEnding) override;

};
