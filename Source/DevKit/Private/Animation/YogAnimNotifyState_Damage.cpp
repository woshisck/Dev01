// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/YogAnimNotifyState_Damage.h"

#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/GameplayEffect/GE_MeleeAttackFrame.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/GameplayCue/HitCueData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/RuneDataAsset.h"

UYogAnimNotifyState_Damage::UYogAnimNotifyState_Damage()
{
	EventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack"));
}

void UYogAnimNotifyState_Damage::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	FDamageWindowRuntime& Runtime = RuntimeByMesh.FindOrAdd(TObjectKey<USkeletalMeshComponent>(MeshComp));
	Runtime.NextSampleId = 0;
	Runtime.LastFilteredSampleId = INDEX_NONE;
	Runtime.HitActorKeys.Reset();
	Runtime.LastFilteredActors.Reset();
	RemoveAttackFrameGE(Runtime);

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	UYogAbilitySystemComponent* ASC = Character ? Character->GetASC() : nullptr;
	if (Character && ASC)
	{
		ApplyAttackFrameGE(Character, ASC, ResolveActionData(Character), Runtime);
	}

	if (bEvaluateOnBegin)
	{
		DispatchDamageSample(MeshComp);
	}

	SendSwingEvent(Character);
}

void UYogAnimNotifyState_Damage::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (FDamageWindowRuntime* Runtime = RuntimeByMesh.Find(TObjectKey<USkeletalMeshComponent>(MeshComp)))
	{
		RemoveAttackFrameGE(*Runtime);
	}
	RuntimeByMesh.Remove(TObjectKey<USkeletalMeshComponent>(MeshComp));
}

void UYogAnimNotifyState_Damage::Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const
{
	Super::Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UYogAnimNotifyState_Damage::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (bEvaluateEveryTick)
	{
		DispatchDamageSample(MeshComp);
	}
}

FString UYogAnimNotifyState_Damage::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Melee Dmg Window | %.0f / R:%.0f"), ActDamage, ActRange);
}

FActionData UYogAnimNotifyState_Damage::BuildActionData() const
{
	if (AttackDataOverride)
	{
		return AttackDataOverride->BuildActionData();
	}

	FActionData Out;
	Out.ActDamage = ActDamage;
	Out.ActRange = ActRange;
	Out.ActResilience = ActResilience;
	Out.ActDmgReduce = ActDmgReduce;
	Out.hitboxTypes = HitboxTypes;
	return Out;
}

FActionData UYogAnimNotifyState_Damage::ResolveActionData(AYogCharacterBase* Character) const
{
	if (Character)
	{
		if (UYogAbilitySystemComponent* ASC = Character->GetASC())
		{
			if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
			{
				if (const FComboAttackConfig* NodeAttackConfig = MeleeGA->GetConfiguredAttackConfig())
				{
					return NodeAttackConfig->BuildActionData();
				}
				if (MeleeGA->HasConfiguredAttackData())
				{
					return MeleeGA->GetAbilityActionData();
				}
			}
		}
	}

	return BuildActionData();
}

void UYogAnimNotifyState_Damage::FilterHitActorsForEvent(const AYogCharacterBase* TargetingCharacter,
	const FGameplayEventData& EventData, TArray<AActor*>& InOutActors) const
{
	if (!bHitEachTargetOnce || !TargetingCharacter || !IsDamageWindowEvent(EventData))
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = TargetingCharacter->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	FDamageWindowRuntime* Runtime = RuntimeByMesh.Find(TObjectKey<USkeletalMeshComponent>(MeshComp));
	if (!Runtime)
	{
		return;
	}

	const int32 SampleId = static_cast<int32>(EventData.EventMagnitude);
	if (Runtime->LastFilteredSampleId == SampleId)
	{
		InOutActors.Reset();
		for (const TWeakObjectPtr<AActor>& WeakActor : Runtime->LastFilteredActors)
		{
			if (AActor* Actor = WeakActor.Get())
			{
				InOutActors.Add(Actor);
			}
		}
		return;
	}

	Runtime->LastFilteredSampleId = SampleId;
	Runtime->LastFilteredActors.Reset();

	for (int32 Index = InOutActors.Num() - 1; Index >= 0; --Index)
	{
		AActor* Actor = InOutActors[Index];
		if (!IsValid(Actor))
		{
			InOutActors.RemoveAtSwap(Index);
			continue;
		}

		const TObjectKey<AActor> ActorKey(Actor);
		if (Runtime->HitActorKeys.Contains(ActorKey))
		{
			InOutActors.RemoveAtSwap(Index);
			continue;
		}

		Runtime->HitActorKeys.Add(ActorKey);
		Runtime->LastFilteredActors.Add(Actor);
	}
}

bool UYogAnimNotifyState_Damage::IsDamageWindowEvent(const FGameplayEventData& EventData) const
{
	return EventData.OptionalObject == this;
}

void UYogAnimNotifyState_Damage::DispatchDamageSample(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return;
	}

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	FDamageWindowRuntime& Runtime = RuntimeByMesh.FindOrAdd(TObjectKey<USkeletalMeshComponent>(MeshComp));
	PopulatePendingHitData(Character);

	const FGameplayTag EffectiveEventTag = ResolveEventTag(Character);
	FGameplayEventData EventData;
	EventData.Instigator = Character;
	EventData.EventTag = EffectiveEventTag;
	EventData.OptionalObject = this;
	EventData.EventMagnitude = static_cast<float>(++Runtime.NextSampleId);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, EffectiveEventTag, EventData);
}

FGameplayTag UYogAnimNotifyState_Damage::ResolveEventTag(AYogCharacterBase* Character) const
{
	if (Character)
	{
		if (UYogAbilitySystemComponent* ASC = Character->GetASC())
		{
			if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
			{
				if (const FComboAttackConfig* NodeAttackConfig = MeleeGA->GetConfiguredAttackConfig())
				{
					if (NodeAttackConfig->EventTag.IsValid())
					{
						return NodeAttackConfig->EventTag;
					}
				}
				else if (const UMontageAttackDataAsset* ConfiguredAttackData = MeleeGA->GetConfiguredAttackData())
				{
					if (ConfiguredAttackData->EventTag.IsValid())
					{
						return ConfiguredAttackData->EventTag;
					}
				}
			}
		}
	}

	if (AttackDataOverride && AttackDataOverride->EventTag.IsValid())
	{
		return AttackDataOverride->EventTag;
	}

	return EventTag;
}

void UYogAnimNotifyState_Damage::SendSwingEvent(AYogCharacterBase* Character) const
{
	if (!Character)
	{
		return;
	}

	static const FGameplayTag TAG_Swing = FGameplayTag::RequestGameplayTag(FName("Action.Attack.Swing"));
	FGameplayEventData SwingEventData;
	SwingEventData.Instigator = Character;
	SwingEventData.EventTag = TAG_Swing;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, TAG_Swing, SwingEventData);
}

void UYogAnimNotifyState_Damage::PopulatePendingHitData(AYogCharacterBase* Character) const
{
	if (!Character)
	{
		return;
	}

	const UMontageAttackDataAsset* EffectiveAttackData = AttackDataOverride;
	const FComboAttackConfig* EffectiveNodeAttackConfig = nullptr;
	if (UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
		{
			if (const FComboAttackConfig* NodeAttackConfig = MeleeGA->GetConfiguredAttackConfig())
			{
				EffectiveNodeAttackConfig = NodeAttackConfig;
			}
			else if (MeleeGA->HasConfiguredAttackData())
			{
				EffectiveAttackData = MeleeGA->GetConfiguredAttackData();
			}
		}
	}

	const TArray<TObjectPtr<URuneDataAsset>>& EffectiveRuneEffects = EffectiveNodeAttackConfig
		? EffectiveNodeAttackConfig->AdditionalRuneEffects
		: EffectiveAttackData
		? EffectiveAttackData->AdditionalRuneEffects
		: AdditionalRuneEffects;
	if (EffectiveRuneEffects.Num() > 0)
	{
		Character->PendingAdditionalHitRunes = EffectiveRuneEffects;
	}

	const EHitStopMode EffectiveHitStopMode = EffectiveNodeAttackConfig
		? EffectiveNodeAttackConfig->HitStopMode
		: EffectiveAttackData
		? EffectiveAttackData->HitStopMode
		: HitStopMode;
	if (EffectiveHitStopMode != EHitStopMode::None)
	{
		AYogCharacterBase::FPendingHitStopOverride& Override = Character->PendingHitStopOverride;
		Override.bActive = true;
		Override.Mode = EffectiveHitStopMode;
		Override.FrozenDuration = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopFrozenDuration : EffectiveAttackData ? EffectiveAttackData->HitStopFrozenDuration : HitStopFrozenDuration;
		Override.SlowDuration = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopSlowDuration : EffectiveAttackData ? EffectiveAttackData->HitStopSlowDuration : HitStopSlowDuration;
		Override.SlowRate = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopSlowRate : EffectiveAttackData ? EffectiveAttackData->HitStopSlowRate : HitStopSlowRate;
		Override.CatchUpRate = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopCatchUpRate : EffectiveAttackData ? EffectiveAttackData->HitStopCatchUpRate : HitStopCatchUpRate;
	}

	static const TArray<FGameplayTag> EmptyOnHitEventTags;
	const TArray<FGameplayTag>& EffectiveOnHitEventTags = EffectiveNodeAttackConfig
		? EffectiveNodeAttackConfig->OnHitEventTags
		: EffectiveAttackData
		? EffectiveAttackData->OnHitEventTags
		: EmptyOnHitEventTags;
	if (EffectiveOnHitEventTags.Num() > 0)
	{
		Character->PendingOnHitEventTags = EffectiveOnHitEventTags;
	}

	if (HitImpactCueTag.IsValid())
	{
		Character->PendingHitImpactCueTag = HitImpactCueTag;
		Character->PendingHitImpactCueData = HitImpactCueData;
	}
}

void UYogAnimNotifyState_Damage::ApplyAttackFrameGE(AYogCharacterBase* Character, UAbilitySystemComponent* ASC,
	const FActionData& ActionData, FDamageWindowRuntime& Runtime) const
{
	if (!Character || !ASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Character);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UGE_MeleeAttackFrame::StaticClass(), 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	static const FGameplayTag TAG_ActDamage = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
	static const FGameplayTag TAG_ActRange = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
	static const FGameplayTag TAG_ActRes = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
	static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDamage, ActionData.ActDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRange, ActionData.ActRange);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRes, ActionData.ActResilience);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, ActionData.ActDmgReduce);

	Runtime.AttackFrameASC = ASC;
	Runtime.AttackFrameGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void UYogAnimNotifyState_Damage::RemoveAttackFrameGE(FDamageWindowRuntime& Runtime) const
{
	if (Runtime.AttackFrameGEHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = Runtime.AttackFrameASC.Get())
		{
			ASC->RemoveActiveGameplayEffect(Runtime.AttackFrameGEHandle);
		}
	}

	Runtime.AttackFrameASC = nullptr;
	Runtime.AttackFrameGEHandle = FActiveGameplayEffectHandle();
}
