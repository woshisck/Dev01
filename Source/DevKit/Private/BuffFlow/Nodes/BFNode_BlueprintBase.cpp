#include "BuffFlow/Nodes/BFNode_BlueprintBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Data/RuneDataAsset.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"

UBFNode_BlueprintBase::UBFNode_BlueprintBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Blueprint");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

UBuffFlowComponent* UBFNode_BlueprintBase::BP_GetBuffFlowComponent() const
{
	return GetBuffFlowComponent();
}

AYogCharacterBase* UBFNode_BlueprintBase::BP_GetBuffOwner() const
{
	return GetBuffOwner();
}

UYogAbilitySystemComponent* UBFNode_BlueprintBase::BP_GetOwnerASC() const
{
	return GetOwnerASC();
}

AActor* UBFNode_BlueprintBase::BP_ResolveTarget(const EBFTargetSelector Selector) const
{
	return ResolveTarget(Selector);
}

UAbilitySystemComponent* UBFNode_BlueprintBase::BP_GetASCFromActor(AActor* Actor) const
{
	return Actor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor) : nullptr;
}

URuneDataAsset* UBFNode_BlueprintBase::BP_GetSourceRune() const
{
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		return BFC->GetActiveSourceRuneData(GetFlowAsset());
	}
	return nullptr;
}

float UBFNode_BlueprintBase::BP_GetRuneTuningValue(const FName Key, const float DefaultValue) const
{
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		return BFC->GetRuneTuningValueForFlow(GetFlowAsset(), Key, DefaultValue);
	}
	return DefaultValue;
}

bool UBFNode_BlueprintBase::BP_HasCombatCardContext() const
{
	if (const UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		return BFC->HasCombatCardEffectContext();
	}
	return false;
}

bool UBFNode_BlueprintBase::BP_GetCombatCardContext(FCombatCardEffectContext& OutContext) const
{
	if (const UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		if (BFC->HasCombatCardEffectContext())
		{
			OutContext = BFC->GetLastCombatCardEffectContext();
			return true;
		}
	}

	OutContext = FCombatCardEffectContext();
	return false;
}

float UBFNode_BlueprintBase::BP_GetCombatCardEffectMultiplier(const float DefaultValue) const
{
	FCombatCardEffectContext Context;
	return BP_GetCombatCardContext(Context) ? Context.EffectMultiplier : DefaultValue;
}

bool UBFNode_BlueprintBase::BP_ApplyGameplayEffectToTarget(
	const TSubclassOf<UGameplayEffect> EffectClass,
	const EBFTargetSelector Target,
	const float Level,
	const int32 ApplicationCount,
	const TArray<FBFBlueprintSetByCallerMagnitude>& SetByCallerMagnitudes,
	FActiveGameplayEffectHandle& OutHandle)
{
	OutHandle = FActiveGameplayEffectHandle();

	if (!EffectClass)
	{
		BP_RecordTrace(EBuffFlowTraceResult::Failed, nullptr, TEXT("GameplayEffect class is null"), TEXT(""));
		return false;
	}

	UAbilitySystemComponent* SourceASC = GetOwnerASC();
	AActor* TargetActor = ResolveTarget(Target);
	UAbilitySystemComponent* TargetASC = BP_GetASCFromActor(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		BP_RecordTrace(
			EBuffFlowTraceResult::Failed,
			TargetActor,
			TEXT("Missing source or target ASC"),
			FString::Printf(TEXT("SourceASC=%s TargetASC=%s"), *GetNameSafe(SourceASC), *GetNameSafe(TargetASC)));
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	if (AActor* OwnerActor = GetBuffOwner())
	{
		EffectContext.AddInstigator(OwnerActor, OwnerActor);
		EffectContext.AddSourceObject(OwnerActor);
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		BP_RecordTrace(EBuffFlowTraceResult::Failed, TargetActor, TEXT("Failed to create GameplayEffect spec"), TEXT(""));
		return false;
	}

	const float CombatCardMultiplier = BP_GetCombatCardEffectMultiplier(1.f);
	TArray<FString> ValueStrings;
	for (const FBFBlueprintSetByCallerMagnitude& SetByCaller : SetByCallerMagnitudes)
	{
		if (!SetByCaller.Tag.IsValid())
		{
			continue;
		}

		const float Value = SetByCaller.bUseCombatCardEffectMultiplier
			? SetByCaller.Value * CombatCardMultiplier
			: SetByCaller.Value;
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCaller.Tag, Value);
		ValueStrings.Add(FString::Printf(TEXT("%s=%.2f"), *SetByCaller.Tag.ToString(), Value));
	}

	const int32 ResolvedApplicationCount = FMath::Clamp(ApplicationCount, 1, 50);
	for (int32 Index = 0; Index < ResolvedApplicationCount; ++Index)
	{
		OutHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	const bool bApplied = OutHandle.IsValid();
	BP_RecordTrace(
		bApplied ? EBuffFlowTraceResult::Success : EBuffFlowTraceResult::Skipped,
		TargetActor,
		bApplied ? TEXT("Applied GameplayEffect from Blueprint node") : TEXT("GameplayEffect applied with no active handle"),
		FString::Printf(
			TEXT("Effect=%s Count=%d Level=%.2f %s"),
			*GetNameSafe(EffectClass.Get()),
			ResolvedApplicationCount,
			Level,
			*FString::Join(ValueStrings, TEXT(","))));
	return bApplied;
}

bool UBFNode_BlueprintBase::BP_SendGameplayEventToTarget(
	const FGameplayTag EventTag,
	const EBFTargetSelector EventReceiver,
	const EBFTargetSelector PayloadTarget,
	const EBFTargetSelector Instigator,
	const float EventMagnitude)
{
	if (!EventTag.IsValid())
	{
		BP_RecordTrace(EBuffFlowTraceResult::Failed, nullptr, TEXT("GameplayEvent tag is invalid"), TEXT(""));
		return false;
	}

	AActor* ReceiverActor = ResolveTarget(EventReceiver);
	UAbilitySystemComponent* ReceiverASC = BP_GetASCFromActor(ReceiverActor);
	if (!ReceiverASC)
	{
		BP_RecordTrace(EBuffFlowTraceResult::Failed, ReceiverActor, TEXT("GameplayEvent receiver has no ASC"), TEXT(""));
		return false;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Target = ResolveTarget(PayloadTarget);
	EventData.Instigator = ResolveTarget(Instigator);
	EventData.EventMagnitude = EventMagnitude;

	ReceiverASC->HandleGameplayEvent(EventTag, &EventData);
	BP_RecordTrace(
		EBuffFlowTraceResult::Success,
		ReceiverActor,
		TEXT("Sent GameplayEvent from Blueprint node"),
		FString::Printf(TEXT("Tag=%s Magnitude=%.2f"), *EventTag.ToString(), EventMagnitude));
	return true;
}

void UBFNode_BlueprintBase::BP_RecordTrace(
	const EBuffFlowTraceResult Result,
	AActor* TargetActor,
	const FString& Message,
	const FString& Values)
{
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		BFC->RecordTrace(this, nullptr, TargetActor, Result, Message, Values);
	}
}

void UBFNode_BlueprintBase::BP_TriggerOut(const bool bFinish)
{
	TriggerOutput(TEXT("Out"), bFinish);
}

void UBFNode_BlueprintBase::BP_TriggerFailed(const bool bFinish)
{
	TriggerOutput(TEXT("Failed"), bFinish);
}
