// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTask/AsyncTask_WaitHitResult.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

void UAsyncTask_WaitHitResult::Activate()
{
	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(AbilitySystemComponent);

	if (ASC)
	{
		ASC->ReceiveHitResult.AddDynamic(this, &UAsyncTask_WaitHitResult::OnHitResultReceived);
		//ASC->ReceiveHitResult.AddDynamic(this, &UAsyncTask_WaitHitResult::OnHitResultReceived);
	}
}

void UAsyncTask_WaitHitResult::OnHitResultReceived(UYogAbilitySystemComponent* SourceASC, bool HitResult)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnHitResult.Broadcast(SourceASC, true);
	}

	if (TriggerOnce)
	{
		EndTask();
	}
}

UAsyncTask_WaitHitResult* UAsyncTask_WaitHitResult::WaitReceiveHitResult(UGameplayAbility* OwningAbility, bool TriggerOnce)
{
	UAsyncTask_WaitHitResult* MyObj = NewAbilityTask<UAsyncTask_WaitHitResult>(OwningAbility);
	MyObj->TriggerOnce = TriggerOnce;
	return MyObj;
}

void UAsyncTask_WaitHitResult::OnDestroy(bool AbilityIsEnding)
{
	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(AbilitySystemComponent);

	//if (ASC)
	//{
	//	ASC->ReceivedDamage.RemoveDynamic(this, &UGDAT_WaitReceiveDamage::OnDamageReceived);
	//}

	Super::OnDestroy(AbilityIsEnding);
}
