// Fill out your copyright notice in the Description page of Project Settings.


#include "GCN_VisualBase.h"

void AGCN_VisualBase::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	Super::HandleGameplayCue(MyTarget, EventType,Parameters);
	UObject* obj = Parameters.EffectContext.GetSourceObject();
	

}

AActor* AGCN_VisualBase::GetInstigatorFromGameplayCueParameters(const FGameplayCueParameters& Parameters)
{
	return Parameters.EffectContext.GetInstigator();
}

FVector AGCN_VisualBase::GetOriginFromGameplayCueParameters(const FGameplayCueParameters& Parameters)
{
	return Parameters.EffectContext.GetOrigin();
}

AActor* AGCN_VisualBase::GetSourceObjectFromGameplayCueParameters(const FGameplayCueParameters& Parameters)
{
	return Cast<AActor>(Parameters.EffectContext.GetSourceObject());
}
