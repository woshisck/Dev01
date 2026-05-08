// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/AbilityTask/YogTask_PlayMontageAbility.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/BufferComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/HitStopManager.h"
#include "Character/EnemyCharacterBase.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageAttackDataAsset.h"
#include "Engine/World.h"

namespace
{
	AYogCharacterBase* ResolveAttackOwner(const UGameplayAbility* Ability, const FGameplayEventData& EventData)
	{
		if (const AActor* EventInstigatorActor = EventData.Instigator.Get())
		{
			if (AYogCharacterBase* EventInstigator = Cast<AYogCharacterBase>(const_cast<AActor*>(EventInstigatorActor)))
			{
				return EventInstigator;
			}
		}

		if (Ability)
		{
			if (AYogCharacterBase* AvatarOwner = Cast<AYogCharacterBase>(Ability->GetAvatarActorFromActorInfo()))
			{
				return AvatarOwner;
			}

			return Cast<AYogCharacterBase>(Ability->GetOwningActorFromActorInfo());
		}

		return nullptr;
	}

	void CollectHitActors(const FYogGameplayEffectContainerSpec& ContainerSpec, TArray<AActor*>& OutHitActors)
	{
		for (const TSharedPtr<FGameplayAbilityTargetData>& Data : ContainerSpec.TargetData.Data)
		{
			if (Data.IsValid())
			{
				for (TWeakObjectPtr<AActor> WeakActor : Data->GetActors())
				{
					if (AActor* Actor = WeakActor.Get())
					{
						OutHitActors.AddUnique(Actor);
					}
				}
			}
		}
	}

	bool HasEnemyHit(const TArray<AActor*>& HitActors)
	{
		for (AActor* HitActor : HitActors)
		{
			if (Cast<AEnemyCharacterBase>(HitActor))
			{
				return true;
			}
		}

		return false;
	}

	void ConsumePendingHitStopOnEnemyHit(AYogCharacterBase* Owner, const TArray<AActor*>& HitActors)
	{
		if (!Owner)
		{
			return;
		}

		auto& Override = Owner->PendingHitStopOverride;
		if (!Override.bActive)
		{
			return;
		}

		if (HasEnemyHit(HitActors))
		{
			UAnimInstance* AnimInst = Owner->GetMesh() ? Owner->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimInst)
			{
				float UseFrozenDuration = 0.f;
				float UseSlowDuration = 0.f;
				float UseSlowRate = Override.SlowRate;
				float UseCatchUpRate = Override.CatchUpRate;

				if (Override.Mode == EHitStopMode::Freeze)
				{
					UseFrozenDuration = Override.FrozenDuration;
				}
				else if (Override.Mode == EHitStopMode::Slow)
				{
					UseSlowDuration = Override.SlowDuration;
				}

				if ((UseFrozenDuration > 0.f || UseSlowDuration > 0.f) && Owner->GetWorld())
				{
					if (UHitStopManager* HitStopManager = Owner->GetWorld()->GetSubsystem<UHitStopManager>())
					{
						HitStopManager->RequestMontageHitStop(
							AnimInst,
							UseFrozenDuration,
							UseSlowDuration,
							UseSlowRate,
							UseCatchUpRate);
					}
				}
			}
		}

		Override = AYogCharacterBase::FPendingHitStopOverride();
	}
}

UGA_PlayMontage::UGA_PlayMontage(const FObjectInitializer& ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    bRetriggerInstancedAbility = true;
    //NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_PlayMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // On retrigger: silently kill the in-flight task before committing.
    // EndTask() marks it as garbage so its pending delegate callbacks become no-ops.
    if (ActivePlayMontageTask)
    {
        ActivePlayMontageTask->EndTask();
        ActivePlayMontageTask = nullptr;
    }
	ActiveComboAttackData = nullptr;

    // Clear any node-driven combo window timers from a previous activation.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ComboWindowOpenHandle);
        World->GetTimerManager().ClearTimer(ComboWindowCloseHandle);
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        return;
    }

    UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);

    AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());

    // Resolve montage strictly from the active GameplayAbilityComboGraph node.
    // No fallback to AbilityData / MontageConfig DA — the combo graph is the single source of truth.
    UAnimMontage* MontageToPlay = nullptr;
    const APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner);
    const UGameplayAbilityComboGraph* ComboGraph = nullptr;
    if (PlayerOwner && PlayerOwner->ComboRuntimeComponent)
    {
        ComboGraph = PlayerOwner->ComboRuntimeComponent->GetComboGraph();
        if (ComboGraph)
        {
            if (const FWeaponComboNodeConfig* ActiveComboNode = PlayerOwner->ComboRuntimeComponent->GetActiveNode())
            {
                MontageToPlay = ActiveComboNode->Montage;
				ActiveComboAttackData = ActiveComboNode->AttackDataOverride;
            }
        }
    }

    if (!ComboGraph)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_PlayMontage] No UGameplayAbilityComboGraph on owner=%s — aborting activation."),
            *GetNameSafe(Owner));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_PlayMontage] Active combo node has no Montage on graph=%s owner=%s — aborting activation."),
            *GetNameSafe(ComboGraph), *GetNameSafe(Owner));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
	// 记录激活时刻，连击缓存只接受此时间之后的输入
	AbilityActivationTime = GetWorld()->GetTimeSeconds();

	// 每次攻击激活时先强制清零上一招残留的 CanCombo tag
	// 用 SetLooseGameplayTagCount(0) 而非 RemoveLooseGameplayTag(1)，
	// 原因：AnimNotifyState 可能多次 Add（count>1），单次 Remove 不足以清干净
	{
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		const int32 CurrentCount = ASC->GetTagCount(CanComboTag);
		if (CurrentCount > 0)
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}
	}

	// 注册 CanCombo tag 监听前先注销旧 handle，防止重入时产生孤儿监听器
	if (CanComboTagHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(
			CanComboTagHandle,
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")),
			EGameplayTagEventType::NewOrRemoved
		);
	}
	CanComboTagHandle = ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")),
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(this, &UGA_PlayMontage::OnCanComboTagChanged);

    {
        // 从蒙太奇第一个 AN_MeleeDamage 读取攻击参数（原来从 AbilityData 读）
        UAN_MeleeDamage* DmgNotify = nullptr;
        for (FAnimNotifyEvent& NotifyEvent : MontageToPlay->Notifies)
        {
            if (UAN_MeleeDamage* N = Cast<UAN_MeleeDamage>(NotifyEvent.Notify))
            {
                DmgNotify = N;
                break;
            }
        }

        FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
        FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DynamicEffectClass, GetAbilityLevel(), Context);

        if (SpecHandle.IsValid() && DmgNotify)
        {
            FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActDamage")),    DmgNotify->ActDamage);
            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActRange")),     DmgNotify->ActRange);
            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActResilience")),DmgNotify->ActResilience);
            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActDmgReduce")), DmgNotify->ActDmgReduce);

            // Apply to self
            ActiveEffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*Spec));
        }

        UYogTask_PlayMontageAbility* PlayMontageTask = UYogTask_PlayMontageAbility::YogPlayMontageAbility(this, NAME_None, MontageToPlay, FGameplayTagContainer(), 1.0f, NAME_None);
        if (PlayMontageTask)
        {
            PlayMontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayMontage::OnMontageBlendOut);
            PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_PlayMontage::OnMontageCompleted);
            PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayMontage::OnMontageInterrupted);
            PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_PlayMontage::OnMontageCancelled);
            PlayMontageTask->OnEventReceived.AddDynamic(this, &UGA_PlayMontage::OnEventReceived);

            ActivePlayMontageTask = PlayMontageTask;
            PlayMontageTask->ReadyForActivation();

            // Node-driven combo window: schedule CanCombo tag add/remove via timers.
            // Only when the node explicitly overrides the window (bOverrideComboWindow = true).
            const FWeaponComboNodeConfig* ActiveComboNode = PlayerOwner && PlayerOwner->ComboRuntimeComponent
                ? PlayerOwner->ComboRuntimeComponent->GetActiveNode()
                : nullptr;
            if (ActiveComboNode && ActiveComboNode->bOverrideComboWindow && ActiveComboNode->ComboWindowTotalFrames > 0)
            {
                const float MontageDuration = MontageToPlay->GetPlayLength();
                const float TotalFrames = static_cast<float>(ActiveComboNode->ComboWindowTotalFrames);
                const float StartNormalized = FMath::Clamp(static_cast<float>(ActiveComboNode->ComboWindowStartFrame) / TotalFrames, 0.f, 1.f);
                const float EndNormalized   = FMath::Clamp(static_cast<float>(ActiveComboNode->ComboWindowEndFrame)   / TotalFrames, 0.f, 1.f);
                const float StartTime = StartNormalized * MontageDuration;
                const float EndTime   = EndNormalized   * MontageDuration;

                if (UWorld* World = GetWorld())
                {
                    World->GetTimerManager().SetTimer(ComboWindowOpenHandle,  this, &UGA_PlayMontage::OnComboWindowOpen,  FMath::Max(StartTime, 0.01f), false);
                    World->GetTimerManager().SetTimer(ComboWindowCloseHandle, this, &UGA_PlayMontage::OnComboWindowClose, FMath::Max(EndTime,   0.01f), false);
                }
            }
        }
    }


//    if (action_data)
//    {
//        // create Dynamic GameplayEffect
//        UGameplayEffect* ActionEffect = NewObject<UGameplayEffect>(GetTransientPackage(),FName(TEXT("ActionEffect")));
//        ActionEffect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
//        ActionEffect->DurationMagnitude = FScalableFloat(0.0f); // forever longer

//        // modifier
//        FGameplayModifierInfo ModifierInfo;

//        // ATTACK
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActDamage);
//        ModifierInfo.ModifierOp = EGameplayModOp::Additive;
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetAttackAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // ATTACK RAGE
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActRange);
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetAttackRangeAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // RESILIENCE
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActResilience);
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetResilienceAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // DMG TAKEN
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActDmgReduce);
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetDmgTakenAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // APPLY GameplayEffect
//        FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
//        EffectContext.AddSourceObject(this);

//        FActiveGameplayEffectHandle ActiveEffectHandle =
//            ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
//                ActionEffect, 1.0f, EffectContext);
//        UE_LOG(LogTemp, Warning, TEXT("ApplyGameplayEffectToSelf"));

//        // SAVE ActiveEffectHandle FOR LATER REMOVE
//        ActiveEffectHandles.Add(ActiveEffectHandle);

//        // DATA CACHE from player DEPRECATED
//        cache_action_data = MakeShared<FActionData>(*action_data);
//    }

}


void UGA_PlayMontage::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    ActivePlayMontageTask = nullptr;
	ActiveComboAttackData = nullptr;

    // Clear node-driven combo window timers.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ComboWindowOpenHandle);
        World->GetTimerManager().ClearTimer(ComboWindowCloseHandle);
    }

    // remove related Gameplay Effects and gameplaytags(hardcode)
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        for (const FActiveGameplayEffectHandle& effect_Handle : ActiveEffectHandles)
        {
            if (effect_Handle.IsValid())
            {
                ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(effect_Handle);
            }
        }
    }
    //ActiveEffectHandles.Empty();
	// 注销 CanCombo tag 监听
	if (UAbilitySystemComponent* ASCLocal = GetAbilitySystemComponentFromActorInfo())
	{
		ASCLocal->UnregisterGameplayTagEvent(
			CanComboTagHandle,
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")),
			EGameplayTagEventType::NewOrRemoved
		);
		const FGameplayTag CanComboTag2 = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (ASCLocal->GetTagCount(CanComboTag2) > 0)
		{
			ASCLocal->SetLooseGameplayTagCount(CanComboTag2, 0);
		}
	}

	// NOTE: Do NOT reset combo state here. EndAbility is also invoked by UE during a
	// retrigger of bRetriggerInstancedAbility, *between* the next combo node being set
	// on ComboRuntimeComponent and the new ActivateAbility running. Resetting here would
	// wipe the just-set ActiveNode and cause the next combo segment to lose its montage.
	// Combo reset on natural montage end is handled in OnMontageCompleted/OnMontageBlendOut.

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

void UGA_PlayMontage::OnMontageCompleted()
{
    ResetComboToRoot();
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageBlendOut()
{
    ResetComboToRoot();
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageInterrupted()
{
    ResetComboToRoot();
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageCancelled()
{
    ResetComboToRoot();
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::ResetComboToRoot()
{
    // Called only on real montage end (these callbacks become no-ops if EndTask was called
    // by a retrigger). Safe to wipe combo state here without clobbering a chained activation.
    if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
    {
        if (PlayerOwner->ComboRuntimeComponent)
        {
            PlayerOwner->ComboRuntimeComponent->ResetCombo();
        }
    }
}

void UGA_PlayMontage::OnEventReceived(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
	FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	TArray<AActor*> HitActors;
	CollectHitActors(ContainerSpec, HitActors);

	ApplyEffectContainerSpec(ContainerSpec);

	AYogCharacterBase* Owner = ResolveAttackOwner(this, EventData);
	const bool bEnemyHit = HasEnemyHit(HitActors);
	if (bEnemyHit && Owner)
	{
		Owner->bComboHitConnected = true;
		ConsumePendingHitStopOnEnemyHit(Owner, HitActors);
	}

	if (Owner)
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
		Owner->PendingHitStopOverride = AYogCharacterBase::FPendingHitStopOverride();
	}
}

void UGA_PlayMontage::OnComboWindowOpen()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")));
	}
}

void UGA_PlayMontage::OnComboWindowClose()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")), 0);
	}
}

void UGA_PlayMontage::OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 只处理 tag 被加上的时机（NewCount > 0）
	if (NewCount <= 0)
		return;

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Owner)
		return;

	UBufferComponent* Buffer = Owner->GetInputBufferComponent();
	if (!Buffer)
		return;

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));

	// 只接受本次能力激活之后的预输入，避免触发当前能力的那次按键被误判为连击
	if (Buffer->HasBufferedInputSince(EInputCommandType::LightAttack, AbilityActivationTime))
	{
		Buffer->ClearBuffer();
		bool bActivated = false;
		bool bHasComboSource = false;
		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
		{
			bHasComboSource = PlayerOwner->ComboRuntimeComponent && PlayerOwner->ComboRuntimeComponent->HasComboSource();
			bActivated = bHasComboSource
				&& PlayerOwner->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Light, PlayerOwner);
		}
		if (!bActivated && !bHasComboSource)
		{
			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.LightAtk")));
			bActivated = Owner->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
		}
		// 没有下一段连招（已是最后一招），立刻清除 CanCombo
		if (!bActivated && ASC)
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}
		return;
	}

	if (Buffer->HasBufferedInputSince(EInputCommandType::HeavyAttack, AbilityActivationTime))
	{
		Buffer->ClearBuffer();
		bool bActivated = false;
		bool bHasComboSource = false;
		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
		{
			bHasComboSource = PlayerOwner->ComboRuntimeComponent && PlayerOwner->ComboRuntimeComponent->HasComboSource();
			bActivated = bHasComboSource
				&& PlayerOwner->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Heavy, PlayerOwner);
		}
		if (!bActivated && !bHasComboSource)
		{
			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.HeavyAtk")));
			bActivated = Owner->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
		}
		if (!bActivated && ASC)
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}
		return;
	}

	// 没有预输入 → 先确认是否有可激活的下一段连招
	// 若没有（当前已是最后一招），立刻清除 CanCombo，防止前一招 AnimNotifyState 延迟触发导致残留
	{
		TArray<FGameplayAbilitySpec*> NextLightSpecs;
		TArray<FGameplayAbilitySpec*> NextHeavySpecs;
		ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
			FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk"))),
			NextLightSpecs);
		ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
			FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk"))),
			NextHeavySpecs);

		if (NextLightSpecs.IsEmpty() && NextHeavySpecs.IsEmpty())
		{
			// 没有任何可接续的连招 → 清除 CanCombo
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
			return;
		}
	}
	// → 有可接续的连招，CanCombo tag 保持，等待玩家在窗口内手动按键
}


