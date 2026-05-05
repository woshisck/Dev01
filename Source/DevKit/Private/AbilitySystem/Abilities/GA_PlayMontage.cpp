// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/AbilityTask/YogTask_PlayMontageAbility.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/BufferComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Animation/AN_MeleeDamage.h"
#include "Data/MontageConfigDA.h"
#include "Engine/World.h"

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
    FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());

    // Prefer montage set directly on the active combo graph node; fall back to AbilityData lookup.
    UAnimMontage* MontageToPlay = nullptr;
    if (const APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
    {
        if (PlayerOwner->ComboRuntimeComponent)
        {
            if (const FWeaponComboNodeConfig* ActiveComboNode = PlayerOwner->ComboRuntimeComponent->GetActiveNode())
            {
                if (ActiveComboNode->MontageConfig)
                {
                    MontageToPlay = ActiveComboNode->MontageConfig->Montage;
                }
            }
        }
    }
    if (!MontageToPlay && Owner)
    {
        MontageToPlay = Owner->GetCharacterDataComponent()->GetCharacterData()->AbilityData->GetMontage(ability_tag);
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

    if(MontageToPlay)
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
            const FWeaponComboNodeConfig* ActiveComboNode = nullptr;
            if (const APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
            {
                if (PlayerOwner->ComboRuntimeComponent)
                    ActiveComboNode = PlayerOwner->ComboRuntimeComponent->GetActiveNode();
            }
            if (ActiveComboNode && ActiveComboNode->bOverrideComboWindow && ActiveComboNode->MontageConfig)
            {
                const float MontageDuration = MontageToPlay->GetPlayLength();
                const float StartTime = ActiveComboNode->MontageConfig->FrameToNormalized(ActiveComboNode->ComboWindowStartFrame) * MontageDuration;
                const float EndTime   = ActiveComboNode->MontageConfig->FrameToNormalized(ActiveComboNode->ComboWindowEndFrame)   * MontageDuration;

                if (UWorld* World = GetWorld())
                {
                    World->GetTimerManager().SetTimer(ComboWindowOpenHandle,  this, &UGA_PlayMontage::OnComboWindowOpen,  FMath::Max(StartTime, 0.01f), false);
                    World->GetTimerManager().SetTimer(ComboWindowCloseHandle, this, &UGA_PlayMontage::OnComboWindowClose, FMath::Max(EndTime,   0.01f), false);
                }
            }
        }
    }
    else
    {
        //EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
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

	// Reset combo state to root so the next attack starts fresh from the root node.
	// EndAbility fires only when the last montage in a combo chain ends naturally (or the
	// ability is cancelled) — not during retrigger — so this is always the right time to reset.
	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (PlayerOwner->ComboRuntimeComponent)
		{
			PlayerOwner->ComboRuntimeComponent->ResetCombo();
		}
	}

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

void UGA_PlayMontage::OnMontageCompleted()
{

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageBlendOut()
{

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageInterrupted()
{

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageCancelled()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnEventReceived(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
    ApplyEffectContainer(EventTag, EventData, -1);
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


