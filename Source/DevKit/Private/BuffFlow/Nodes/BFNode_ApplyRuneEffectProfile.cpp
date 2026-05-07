#include "BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Data/EffectDataAsset.h"
#include "Data/RuneCardEffectProfileDA.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"

UBFNode_ApplyRuneEffectProfile::UBFNode_ApplyRuneEffectProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Profile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyRuneEffectProfile::Cleanup()
{
	if (Profile && Profile->Effect.bRemoveEffectOnCleanup && GrantedASC.IsValid() && GrantedHandle.IsValid())
	{
		GrantedASC->RemoveActiveGameplayEffect(GrantedHandle);
	}
	GrantedASC.Reset();
	GrantedHandle = FActiveGameplayEffectHandle();
	Super::Cleanup();
}

void UBFNode_ApplyRuneEffectProfile::ExecuteInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!Profile)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, nullptr, nullptr, EBuffFlowTraceResult::Failed, TEXT("Profile is null"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* SourceASC = GetOwnerASC();
	AActor* TargetActor = ResolveTarget(Target);
	UAbilitySystemComponent* TargetASC = TargetActor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor) : nullptr;
	if (!SourceASC || !TargetASC)
	{
		if (BFC)
		{
			BFC->RecordTrace(
				this,
				Profile,
				TargetActor,
				EBuffFlowTraceResult::Failed,
				TEXT("Missing source or target ASC"),
				FString::Printf(TEXT("SourceASC=%s TargetASC=%s"), *GetNameSafe(SourceASC), *GetNameSafe(TargetASC)));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	if (AActor* OwnerActor = BFC ? BFC->GetOwner() : nullptr)
	{
		Context.AddSourceObject(OwnerActor);
		Context.AddInstigator(OwnerActor, OwnerActor);
	}

	FGameplayEffectSpecHandle ClassSpecHandle;
	TUniquePtr<FGameplayEffectSpec> DataAssetSpec;
	UGameplayEffect* DynamicEffect = nullptr;
	FGameplayEffectSpec* Spec = nullptr;
	if (Profile->Effect.GameplayEffectClass)
	{
		ClassSpecHandle = SourceASC->MakeOutgoingSpec(Profile->Effect.GameplayEffectClass, 1.0f, Context);
		Spec = ClassSpecHandle.IsValid() ? ClassSpecHandle.Data.Get() : nullptr;
	}
	else if (Profile->Effect.EffectDataAsset)
	{
		DynamicEffect = Profile->Effect.EffectDataAsset->CreateGameplayEffect(GetTransientPackage());
		if (DynamicEffect)
		{
			DataAssetSpec = MakeUnique<FGameplayEffectSpec>(DynamicEffect, Context, 1.0f);
			Spec = DataAssetSpec.Get();
		}
	}

	if (!Spec)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, TargetActor, EBuffFlowTraceResult::Failed, TEXT("No valid GameplayEffect source"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	TArray<FString> ValueStrings;
	for (const FRuneCardProfileSetByCaller& SetByCaller : Profile->Effect.SetByCallerValues)
	{
		if (!SetByCaller.Tag.IsValid())
		{
			continue;
		}

		float Value = SetByCaller.Value;
		if (BFC && SetByCaller.TuningKey != NAME_None)
		{
			Value = BFC->GetRuneTuningValueForFlow(GetFlowAsset(), SetByCaller.TuningKey, Value);
		}
		if (SetByCaller.bUseCombatCardEffectMultiplier && BFC && BFC->HasCombatCardEffectContext())
		{
			Value *= BFC->GetLastCombatCardEffectContext().EffectMultiplier;
		}
		Spec->SetSetByCallerMagnitude(SetByCaller.Tag, Value);
		ValueStrings.Add(FString::Printf(TEXT("%s=%.2f"), *SetByCaller.Tag.ToString(), Value));
	}

	const int32 ApplicationCount = FMath::Clamp(
		ApplicationCountOverride > 0 ? ApplicationCountOverride : Profile->Effect.ApplicationCount,
		1,
		50);

	FActiveGameplayEffectHandle LastHandle;
	for (int32 Index = 0; Index < ApplicationCount; ++Index)
	{
		LastHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec);
	}

	if (!GrantedHandle.IsValid() && LastHandle.IsValid())
	{
		GrantedHandle = LastHandle;
		GrantedASC = TargetASC;
	}

	if (BFC)
	{
		BFC->RecordTrace(
			this,
			Profile,
			TargetActor,
			EBuffFlowTraceResult::Success,
			TEXT("Applied effect profile"),
			FString::Printf(
				TEXT("EffectClass=%s EffectDA=%s Count=%d HandleValid=%d %s"),
				*GetNameSafe(Profile->Effect.GameplayEffectClass.Get()),
				*GetNameSafe(Profile->Effect.EffectDataAsset),
				ApplicationCount,
				LastHandle.IsValid() ? 1 : 0,
				*FString::Join(ValueStrings, TEXT(","))));
	}

	TriggerOutput(TEXT("Out"), false);
}
