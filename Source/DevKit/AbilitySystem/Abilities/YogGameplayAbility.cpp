// Fill out your copyright notice in the Description page of Project Settings.
#include "YogGameplayAbility.h"





UYogGameplayAbility::UYogGameplayAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}



TArray<FActiveGameplayEffectHandle> UYogGameplayAbility::ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	return TArray<FActiveGameplayEffectHandle>();
}
