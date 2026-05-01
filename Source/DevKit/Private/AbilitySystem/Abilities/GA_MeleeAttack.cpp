#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/RuneDataAsset.h"
#include "TimerManager.h"
#include "UObject/ObjectKey.h"

namespace
{
	struct FStatBeforeAttackSharedSnapshot
	{
		float Attack = 0.f;
		float AttackPower = 1.f;
		int32 ActiveCount = 0;
	};

	TMap<TObjectKey<UAbilitySystemComponent>, FStatBeforeAttackSharedSnapshot> GStatBeforeAttackSnapshots;
}

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 鍙楀嚮纭洿 / 鍑婚€€鏈熼棿涓嶅厑璁稿彂鍔ㄦ敾鍑?
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.HitReact"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.Knockback"));
}

UAN_MeleeDamage* UGA_MeleeAttack::GetFirstDamageNotify(UAnimMontage* Montage)
{
	if (!Montage) return nullptr;
	for (FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (UAN_MeleeDamage* DmgNotify = Cast<UAN_MeleeDamage>(Event.Notify))
			return DmgNotify;
	}
	return nullptr;
}

FActionData UGA_MeleeAttack::GetAbilityActionData_Implementation() const
{
	if (ActiveComboAttackData)
	{
		return ActiveComboAttackData->BuildActionData();
	}

	if (CachedDamageNotify)
	{
		return CachedDamageNotify->BuildActionData();
	}

	if (ActiveMontageConfig)
	{
		FGameplayTagContainer ContextTags;
		if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->GetOwnedGameplayTags(ContextTags);
		}

		if (const UMontageAttackDataAsset* AttackData = ActiveMontageConfig->ResolveAttackData(ContextTags))
		{
			return AttackData->BuildActionData();
		}
	}

	return FActionData();
}

ECardRequiredAction UGA_MeleeAttack::GetCombatDeckActionType() const
{
	if (bActiveComboNodeValid)
	{
		return ActiveComboNode.InputAction;
	}

	const FGameplayTag HeavyAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk"));
	const FGameplayTag LightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk"));

	if (AbilityTags.HasTag(HeavyAttackTag))
	{
		return ECardRequiredAction::Heavy;
	}

	if (AbilityTags.HasTag(LightAttackTag))
	{
		return ECardRequiredAction::Light;
	}

	return ECardRequiredAction::Any;
}

bool UGA_MeleeAttack::IsCombatDeckComboFinisher() const
{
	if (bActiveComboNodeValid)
	{
		return ActiveComboNode.bIsComboFinisher;
	}

	const FGameplayTag LightFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo4"));
	const FGameplayTag HeavyFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"));

	return AbilityTags.HasTagExact(LightFinisherTag) || AbilityTags.HasTagExact(HeavyFinisherTag);
}

bool UGA_MeleeAttack::HasConfiguredAttackData() const
{
	return ActiveComboAttackData != nullptr;
}

void UGA_MeleeAttack::SetNextActivationFromDashSave(bool bFromDashSave)
{
	bNextActivationFromDashSave = bFromDashSave;
}

FCombatCardResolveResult UGA_MeleeAttack::ResolveCombatDeck(ECombatCardTriggerTiming TriggerTiming)
{
	FCombatCardResolveResult EmptyResult;

	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!PlayerOwner)
	{
		PlayerOwner = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	}

	if (!PlayerOwner || !PlayerOwner->CombatDeckComponent)
	{
		return EmptyResult;
	}

	FCombatDeckActionContext Context;
	Context.ActionType = GetCombatDeckActionType();
	Context.ComboIndex = ActiveComboIndex;
	Context.ComboNodeId = bActiveComboNodeValid ? ActiveComboNode.NodeId : NAME_None;
	Context.ComboTags = ActiveComboTags;
	Context.AbilityTag = bActiveComboNodeValid ? ActiveComboNode.AbilityTag : FGameplayTag();
	Context.WeaponDef = PlayerOwner->EquippedWeaponDef;
	Context.bIsComboFinisher = IsCombatDeckComboFinisher();
	Context.bComboContinued = bComboContinued;
	Context.bExitedComboState = bExitedComboState;
	Context.bFromDashSave = bCombatDeckFromDashSave;
	Context.TriggerTiming = TriggerTiming;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	if (!Context.AbilityTag.IsValid())
	{
		for (const FGameplayTag& Tag : AbilityTags)
		{
			Context.AbilityTag = Tag;
			break;
		}
	}

	const FCombatCardResolveResult Result = PlayerOwner->CombatDeckComponent->ResolveAttackCardWithContext(Context);
	if (Result.bHadCard)
	{
		bCombatDeckCardResolvedThisActivation = true;
	}
	return Result;
}

void UGA_MeleeAttack::TryResolveCombatDeckOnHit()
{
	ResolveCombatDeck(ECombatCardTriggerTiming::OnHit);
}

void UGA_MeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bCombatDeckCardResolvedThisActivation = false;
	bCombatDeckFromDashSave = bNextActivationFromDashSave;
	bNextActivationFromDashSave = false;
	bActiveComboNodeValid = false;
	bComboContinued = true;
	bExitedComboState = false;
	CombatDeckHitResolveCounter = 0;
	bHasStatBeforeAttributeSnapshot = false;
	LocalPreStatBeforeAttack = 0.f;
	LocalPreStatBeforeAttackPower = 0.f;
	LocalStatBeforeAttackDelta = 0.f;
	LocalStatBeforeAttackPowerDelta = 0.f;
	ActiveComboNode = FWeaponComboNodeConfig();
	ActiveComboAttackData = nullptr;
	ActiveAttackGuid.Invalidate();
	ActiveComboIndex = 0;
	ActiveComboTags.Reset();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] ActivateAbility: %s"), *GetName());

	// 鐜╁ GA锛氭鏌ユ秷鑰?鍐峰嵈
	if (bRequireCommit && !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Read montage from combo config first, then fall back to the legacy MontageMap.
	AYogCharacterBase* ActivateOwner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!PlayerOwner)
	{
		PlayerOwner = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	}
	if (PlayerOwner && PlayerOwner->ComboRuntimeComponent)
	{
		if (const FWeaponComboNodeConfig* RuntimeNode = PlayerOwner->ComboRuntimeComponent->GetActiveNode())
		{
			ActiveComboNode = *RuntimeNode;
			ActiveComboAttackData = RuntimeNode->AttackDataOverride;
			ActiveAttackGuid = PlayerOwner->ComboRuntimeComponent->GetActiveAttackGuid();
			ActiveComboIndex = PlayerOwner->ComboRuntimeComponent->GetComboIndex();
			ActiveComboTags = PlayerOwner->ComboRuntimeComponent->GetComboTags();
			bComboContinued = PlayerOwner->ComboRuntimeComponent->DidComboContinue();
			bExitedComboState = PlayerOwner->ComboRuntimeComponent->DidExitComboState();
			bCombatDeckFromDashSave = bCombatDeckFromDashSave || PlayerOwner->ComboRuntimeComponent->ConsumeActivationFromDashSave();
			bActiveComboNodeValid = true;
		}
	}
	if (!ActiveAttackGuid.IsValid())
	{
		ActiveAttackGuid = FGuid::NewGuid();
	}

	UCharacterDataComponent* CDC = ActivateOwner ? ActivateOwner->GetCharacterDataComponent() : nullptr;
	UCharacterData* CD = CDC ? CDC->GetCharacterData() : nullptr;

	FGameplayTag FirstTag;
	for (const FGameplayTag& Tag : AbilityTags) { FirstTag = Tag; break; }

	UAnimMontage* Montage = (CD && CD->AbilityData && FirstTag.IsValid())
		? CD->AbilityData->GetMontage(FirstTag) : nullptr;

	ActiveMontageConfig = nullptr;
	if (bActiveComboNodeValid && ActiveComboNode.MontageConfig)
	{
		ActiveMontageConfig = ActiveComboNode.MontageConfig;
		if (ActiveMontageConfig->Montage)
		{
			Montage = ActiveMontageConfig->Montage;
		}
	}
	else if (CD && CD->AbilityData && FirstTag.IsValid())
	{
		FGameplayTagContainer ContextTags;
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->GetOwnedGameplayTags(ContextTags);
		}
		if (bActiveComboNodeValid && ActiveComboNode.AbilityTag.IsValid())
		{
			ContextTags.AddTag(ActiveComboNode.AbilityTag);
		}

		ActiveMontageConfig = CD->AbilityData->GetMontageConfig(FirstTag, ContextTags);
		if (ActiveMontageConfig && ActiveMontageConfig->Montage)
		{
			Montage = ActiveMontageConfig->Montage;
		}
	}

	// 缂撳瓨绗竴涓?AN_MeleeDamage锛屽悗缁?GetAbilityActionData / StatAfterATK 浣跨敤
	CachedDamageNotify = GetFirstDamageNotify(Montage);

	// 鏂藉姞鏀诲嚮鍓嶆憞 GE锛堢帺瀹?GA 閰嶇疆锛屾晫浜?GA 鐣欑┖璺宠繃锛?	if (StatBeforeATKEffect)
	if (StatBeforeATKEffect)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
			static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
			static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
			static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddSourceObject(this);
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StatBeforeATKEffect, GetAbilityLevel(), ContextHandle);
			if (SpecHandle.IsValid())
			{
				LocalPreStatBeforeAttack = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
				LocalPreStatBeforeAttackPower = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
				bHasStatBeforeAttributeSnapshot = true;

				const TObjectKey<UAbilitySystemComponent> ASCKey(ASC);
				FStatBeforeAttackSharedSnapshot& SharedSnapshot = GStatBeforeAttackSnapshots.FindOrAdd(ASCKey);
				if (SharedSnapshot.ActiveCount <= 0)
				{
					SharedSnapshot.Attack = LocalPreStatBeforeAttack;
					SharedSnapshot.AttackPower = LocalPreStatBeforeAttackPower;
					SharedSnapshot.ActiveCount = 0;
				}
				++SharedSnapshot.ActiveCount;

				const FActionData ActionData = GetAbilityActionData();
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDamage,    ActionData.ActDamage);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRange,     ActionData.ActRange);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRes,       ActionData.ActResilience);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, ActionData.ActDmgReduce);
				StatBeforeATKHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				const float PostStatBeforeAttack = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
				const float PostStatBeforeAttackPower = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
				LocalStatBeforeAttackDelta = FMath::Max(0.f, PostStatBeforeAttack - LocalPreStatBeforeAttack);
				LocalStatBeforeAttackPowerDelta = FMath::Max(0.f, PostStatBeforeAttackPower - LocalPreStatBeforeAttackPower);
				UE_LOG(LogTemp, Warning,
					TEXT("[StatBeforeATKAttrSnapshot] Effect=%s Attack %.2f -> %.2f Delta=%.2f AttackPower %.2f -> %.2f Delta=%.2f Handle=%d"),
					*GetNameSafe(StatBeforeATKEffect),
					LocalPreStatBeforeAttack,
					PostStatBeforeAttack,
					LocalStatBeforeAttackDelta,
					LocalPreStatBeforeAttackPower,
					PostStatBeforeAttackPower,
					LocalStatBeforeAttackPowerDelta,
					StatBeforeATKHandle.IsValid() ? 1 : 0);
				UE_LOG(LogTemp, Warning,
					TEXT("[StatBeforeATKAttrSnapshot] SharedBaseline Attack=%.2f AttackPower=%.2f ActiveCount=%d"),
					SharedSnapshot.Attack,
					SharedSnapshot.AttackPower,
					SharedSnapshot.ActiveCount);
			}
		}
	}

	// 閲嶇疆鍛戒腑鏍囧織锛堜负鏈鏀诲嚮鐨勭涓€鑺傚仛鍑嗗锛?
	if (ActivateOwner)
	{
		ActivateOwner->bComboHitConnected = false;
	}

	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] No Montage found for %s 鈥?ability ended immediately."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 鐩戝惉 AnimNotify 浼ゅ浜嬩欢锛氬繀椤绘槑纭紶鍏?Tag锛岀┖瀹瑰櫒涓嶄細娉ㄥ唽浠讳綍鐩戝惉
	FGameplayTagContainer DamageEventTags;
	DamageEventTags.AddTag(FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack")));

	// 璇诲彇 AttackSpeed 灞炴€т綔涓鸿挋澶鎾斁閫熺巼
	float AttackSpeedRate = 1.0f;
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		bool bFound = false;
		const float SpeedValue = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetAttackSpeedAttribute(), bFound);
		if (bFound && SpeedValue > 0.f)
		{
			AttackSpeedRate = SpeedValue;
		}
	}

	// 鍒涘缓澶嶅悎浠诲姟锛氭挱鏀捐挋澶 + 鐩戝惉 GameplayEvent锛圓nimNotify 瑙﹀彂浼ゅ浜嬩欢锛?
	UYogAbilityTask_PlayMontageAndWaitForEvent* Task =
		UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
			this,
			NAME_None,
			Montage,
			DamageEventTags,
			AttackSpeedRate,
			NAME_None,
			true,   // bStopWhenAbilityEnds
			1.0f);

	Task->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCompleted);
	Task->OnBlendOut.AddDynamic(this, &UGA_MeleeAttack::OnMontageBlendOut);
	Task->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	Task->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnEventReceived);

	Task->ReadyForActivation();
}

void UGA_MeleeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	// 瀹夊叏娓呯悊锛氭妧鑳界粨鏉熸椂娓呯┖鏈秷璐圭殑鏆傚瓨鏁版嵁锛堣挋澶琚墦鏂湭瑙﹀彂 OnEventReceived 鏃朵繚鎶ょ敤锛?
	if (AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo()))
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
	}

	// 绉婚櫎鏀诲嚮鍓嶆憞 GE
	if (StatBeforeATKHandle.IsValid())
	{
		if (ASC) ASC->RemoveActiveGameplayEffect(StatBeforeATKHandle);
		StatBeforeATKHandle = FActiveGameplayEffectHandle();
	}

	if (bHasStatBeforeAttributeSnapshot && ASC)
	{
		const TObjectKey<UAbilitySystemComponent> ASCKey(ASC);
		FStatBeforeAttackSharedSnapshot* SharedSnapshot = GStatBeforeAttackSnapshots.Find(ASCKey);
		if (SharedSnapshot)
		{
			SharedSnapshot->ActiveCount = FMath::Max(0, SharedSnapshot->ActiveCount - 1);
		}

		const float CurrentAttack = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
		const float CurrentAttackPower = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
		UE_LOG(LogTemp, Warning,
			TEXT("[StatBeforeATKAttrSnapshot] EndRestoreCheck CurrentAttack=%.2f LocalPreAttack=%.2f CurrentAttackPower=%.2f LocalPreAttackPower=%.2f SharedAttack=%.2f SharedAttackPower=%.2f ActiveCount=%d"),
			CurrentAttack,
			LocalPreStatBeforeAttack,
			CurrentAttackPower,
			LocalPreStatBeforeAttackPower,
			SharedSnapshot ? SharedSnapshot->Attack : 0.f,
			SharedSnapshot ? SharedSnapshot->AttackPower : 0.f,
			SharedSnapshot ? SharedSnapshot->ActiveCount : -1);

		if (SharedSnapshot && SharedSnapshot->ActiveCount <= 0)
		{
			if (!FMath::IsNearlyEqual(CurrentAttack, SharedSnapshot->Attack, KINDA_SMALL_NUMBER))
			{
				ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), SharedSnapshot->Attack);
				UE_LOG(LogTemp, Warning, TEXT("[StatBeforeATKAttrSnapshot] Restored Shared Attack %.2f -> %.2f"),
					CurrentAttack, SharedSnapshot->Attack);
			}
			if (!FMath::IsNearlyEqual(CurrentAttackPower, SharedSnapshot->AttackPower, KINDA_SMALL_NUMBER))
			{
				ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), SharedSnapshot->AttackPower);
				UE_LOG(LogTemp, Warning, TEXT("[StatBeforeATKAttrSnapshot] Restored Shared AttackPower %.2f -> %.2f"),
					CurrentAttackPower, SharedSnapshot->AttackPower);
			}
			GStatBeforeAttackSnapshots.Remove(ASCKey);
		}
	}

	// 鏂藉姞鏀诲嚮鍚庢憞 GE锛堜粎姝ｅ父缁撴潫鏃讹紝Cancel/Interrupt 涓嶈Е鍙戯級
	// 浼樺厛鐢ㄦ渶鍚庡懡涓殑 Notify 鏁版嵁锛堝娈靛懡涓唬琛ㄦ渶鍚庝竴鍑伙級锛屾湭鍛戒腑杩囧垯 fallback 鍒扮涓€涓?Notify銆?
	if (!bWasCancelled && StatAfterATKEffect && ASC)
	{
		static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
		static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
		static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
		static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

		const float PreStatAfterAttack = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
		const float PreStatAfterAttackPower = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());

		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StatAfterATKEffect, GetAbilityLevel(), Ctx);
		if (Spec.IsValid())
		{
			const FActionData ActionData = ActiveComboAttackData
				? ActiveComboAttackData->BuildActionData()
				: LastFiredDamageNotify
				? LastFiredDamageNotify->BuildActionData()
				: GetAbilityActionData();
			Spec.Data->SetSetByCallerMagnitude(TAG_ActDamage,    ActionData.ActDamage);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActRange,     ActionData.ActRange);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActRes,       ActionData.ActResilience);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, ActionData.ActDmgReduce);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

			const float PostStatAfterAttack = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
			const float PostStatAfterAttackPower = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
			UE_LOG(LogTemp, Warning,
				TEXT("[StatAfterATKAttrSnapshot] Effect=%s Attack %.2f -> %.2f AttackPower %.2f -> %.2f"),
				*GetNameSafe(StatAfterATKEffect),
				PreStatAfterAttack,
				PostStatAfterAttack,
				PreStatAfterAttackPower,
				PostStatAfterAttackPower);

			if (PostStatAfterAttack > PreStatAfterAttack + KINDA_SMALL_NUMBER)
			{
				ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), PreStatAfterAttack);
				UE_LOG(LogTemp, Warning, TEXT("[StatAfterATKAttrSnapshot] Restored Attack %.2f -> %.2f"),
					PostStatAfterAttack, PreStatAfterAttack);
			}
			if (PostStatAfterAttackPower > PreStatAfterAttackPower + KINDA_SMALL_NUMBER)
			{
				ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), PreStatAfterAttackPower);
				UE_LOG(LogTemp, Warning, TEXT("[StatAfterATKAttrSnapshot] Restored AttackPower %.2f -> %.2f"),
					PostStatAfterAttackPower, PreStatAfterAttackPower);
			}
		}
	}
	CachedDamageNotify    = nullptr;
	LastFiredDamageNotify = nullptr;
	ActiveMontageConfig = nullptr;
	ActiveComboNode = FWeaponComboNodeConfig();
	ActiveComboAttackData = nullptr;
	bActiveComboNodeValid = false;
	bComboContinued = true;
	bExitedComboState = false;
	CombatDeckHitResolveCounter = 0;
	bHasStatBeforeAttributeSnapshot = false;
	LocalPreStatBeforeAttack = 0.f;
	LocalPreStatBeforeAttackPower = 0.f;
	LocalStatBeforeAttackDelta = 0.f;
	LocalStatBeforeAttackPowerDelta = 0.f;
	ActiveAttackGuid.Invalidate();
	ActiveComboIndex = 0;
	ActiveComboTags.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MeleeAttack::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_MeleeAttack::OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_MeleeAttack::OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_MeleeAttack::OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_MeleeAttack::OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData)
{
	UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] OnEventReceived: %s on %s"), *EventTag.ToString(), *GetName());
	++CombatDeckHitResolveCounter;

	// OptionalObject 宸茬敱 AN_MeleeDamage::Notify 璁剧疆涓哄彂灏勮浜嬩欢鐨?Notify 鏈韩锛?
	// YogTargetType_Melee::GetActionData 浠庝腑璇诲彇 HitboxTypes / ActRange 绛夊弬鏁般€?
	// 鍚屾椂鏇存柊 LastFiredDamageNotify锛屼緵 StatAfterATK 浣跨敤锛堝娈靛懡涓椂浠ｈ〃鏈€鍚庝竴鍑伙級銆?
	if (const UAN_MeleeDamage* FiredNotify = Cast<const UAN_MeleeDamage>(EventData.OptionalObject))
	{
		LastFiredDamageNotify = FiredNotify;

		// 鍔ㄤ綔闊ф€э細鍛戒腑绐楀彛鏈熼棿鏀诲嚮鏂归煣鎬т复鏃舵彁鍗囷紝ReceiveDamage 璇诲彇鍚庤嚜鍔ㄦ竻闆?
		if (UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
			SourceASC->CurrentActionPoiseBonus = FiredNotify->ActResilience;
	}

	UAbilitySystemComponent* CombatCardASC = GetAbilitySystemComponentFromActorInfo();
	bool bHasCombatCardAttributeSnapshot = false;
	float PreCardAttack = 0.f;
	float PreCardAttackPower = 0.f;
	float IntendedPreCardAttack = 0.f;
	float IntendedPreCardAttackPower = 0.f;
	if (CombatCardASC)
	{
		bool bFoundAttack = false;
		bool bFoundAttackPower = false;
		PreCardAttack = CombatCardASC->GetGameplayAttributeValue(UBaseAttributeSet::GetAttackAttribute(), bFoundAttack);
		PreCardAttackPower = CombatCardASC->GetGameplayAttributeValue(UBaseAttributeSet::GetAttackPowerAttribute(), bFoundAttackPower);
		bHasCombatCardAttributeSnapshot = bFoundAttack && bFoundAttackPower;

		IntendedPreCardAttack = PreCardAttack;
		IntendedPreCardAttackPower = PreCardAttackPower;

		if (bHasStatBeforeAttributeSnapshot)
		{
			float BaselineAttack = LocalPreStatBeforeAttack;
			float BaselineAttackPower = LocalPreStatBeforeAttackPower;
			const TObjectKey<UAbilitySystemComponent> ASCKey(CombatCardASC);
			if (const FStatBeforeAttackSharedSnapshot* SharedSnapshot = GStatBeforeAttackSnapshots.Find(ASCKey))
			{
				BaselineAttack = SharedSnapshot->Attack;
				BaselineAttackPower = SharedSnapshot->AttackPower;
			}

			IntendedPreCardAttack = BaselineAttack + LocalStatBeforeAttackDelta;
			IntendedPreCardAttackPower = BaselineAttackPower + LocalStatBeforeAttackPowerDelta;

			if (!FMath::IsNearlyEqual(PreCardAttack, IntendedPreCardAttack, KINDA_SMALL_NUMBER))
			{
				CombatCardASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), IntendedPreCardAttack);
			}
			if (!FMath::IsNearlyEqual(PreCardAttackPower, IntendedPreCardAttackPower, KINDA_SMALL_NUMBER))
			{
				CombatCardASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), IntendedPreCardAttackPower);
			}

			UE_LOG(LogTemp, Warning,
				TEXT("[CombatCardAttrNormalize] PreCard Attack %.2f -> %.2f AttackPower %.2f -> %.2f BaselineAttack=%.2f StatBeforeDelta=%.2f HitIndex=%d"),
				PreCardAttack,
				IntendedPreCardAttack,
				PreCardAttackPower,
				IntendedPreCardAttackPower,
				BaselineAttack,
				LocalStatBeforeAttackDelta,
				CombatDeckHitResolveCounter);

			PreCardAttack = IntendedPreCardAttack;
			PreCardAttackPower = IntendedPreCardAttackPower;
		}
	}

	const FCombatCardResolveResult CombatCardResult = ResolveCombatDeck(ECombatCardTriggerTiming::OnHit);
	float CardAttackDelta = 0.f;
	float CardAttackPowerDelta = 0.f;
	if (bHasCombatCardAttributeSnapshot && CombatCardASC)
	{
		const float PostCardAttack = CombatCardASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
		const float PostCardAttackPower = CombatCardASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
		CardAttackDelta = FMath::Max(0.f, PostCardAttack - PreCardAttack);
		CardAttackPowerDelta = FMath::Max(0.f, PostCardAttackPower - PreCardAttackPower);

		const float IntendedDamageAttack = IntendedPreCardAttack + CardAttackDelta;
		const float IntendedDamageAttackPower = IntendedPreCardAttackPower + CardAttackPowerDelta;
		if (!FMath::IsNearlyEqual(PostCardAttack, IntendedDamageAttack, KINDA_SMALL_NUMBER))
		{
			CombatCardASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), IntendedDamageAttack);
		}
		if (!FMath::IsNearlyEqual(PostCardAttackPower, IntendedDamageAttackPower, KINDA_SMALL_NUMBER))
		{
			CombatCardASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), IntendedDamageAttackPower);
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[CombatCardAttrNormalize] Damage Attack %.2f -> %.2f CardDelta=%.2f AttackPower %.2f -> %.2f CardPowerDelta=%.2f"),
			PostCardAttack,
			IntendedDamageAttack,
			CardAttackDelta,
			PostCardAttackPower,
			IntendedDamageAttackPower,
			CardAttackPowerDelta);
	}
	UE_LOG(LogTemp, Warning,
		TEXT("[CombatCardAttrSnapshot] Resolve Card=%s Had=%d Base=%d Link=%d PreAttack=%.2f PreAttackPower=%.2f ASC=%s"),
		*CombatCardResult.ConsumedCard.Config.DisplayName.ToString(),
		CombatCardResult.bHadCard ? 1 : 0,
		CombatCardResult.bTriggeredBaseFlow ? 1 : 0,
		CombatCardResult.bTriggeredLink ? 1 : 0,
		PreCardAttack,
		PreCardAttackPower,
		*GetNameSafe(CombatCardASC));

	// 鎷嗗垎涓轰袱姝ヤ互澶嶇敤 ContainerSpec锛堢洰鏍囨暟鎹?+ 闄勫姞 Effect 闇€瑕佸悓涓€鎵圭洰鏍囷級
	FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());

	// 鑻ユ湁 GE 鍛戒腑鐩爣锛屾爣璁版湰鑺傚凡鍛戒腑锛堜緵 AN_EnemyComboSection::bRequireHit 浣跨敤锛?
	if (Handles.Num() > 0 && Owner)
	{
		Owner->bComboHitConnected = true;
		// 鏀堕泦鏈鍛戒腑鐨勬墍鏈夌洰鏍囷紙渚?Ability.Event 骞挎挱鍜?AdditionalRuneEffects 鍏辩敤锛?
		TArray<AActor*> HitActors;
		for (const TSharedPtr<FGameplayAbilityTargetData>& Data : ContainerSpec.TargetData.Data)
		{
			if (Data.IsValid())
			{
				for (TWeakObjectPtr<AActor> WeakActor : Data->GetActors())
				{
					if (AActor* Actor = WeakActor.Get())
						HitActors.AddUnique(Actor);
				}
			}
		}

		// 骞挎挱 Ability.Event.Attack.Hit 缁欐敾鍑昏€咃紙BGC 浜嬩欢椹卞姩鍨嬬鏂囩洃鍚浜嬩欢锛?
		static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
		for (AActor* HitActor : HitActors)
		{
			FGameplayEventData Payload;
			Payload.Instigator = Owner;
			Payload.Target     = HitActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitTag, Payload);
		}

		// 瑙﹀彂鏉ヨ嚜 AN_MeleeDamage::AdditionalRuneEffects 鐨勯檮鍔犵鏂囷紙澶嶇敤鍚屾壒鐩爣锛?
		for (URuneDataAsset* RuneDA : Owner->PendingAdditionalHitRunes)
		{
			if (!RuneDA) continue;
			for (AActor* HitActor : HitActors)
			{
				if (AYogCharacterBase* HitChar = Cast<AYogCharacterBase>(HitActor))
				{
					HitChar->ReceiveOnHitRune(RuneDA, Owner); // Owner = 鏀诲嚮鍙戣捣鑰?
				}
			}
		}

		// 骞挎挱鏉ヨ嚜 AN_MeleeDamage::OnHitEventTags 鐨勫懡涓簨浠讹紙渚涢暅澶存姈鍔ㄣ€侀煶鏁堢瓑绯荤粺鐩戝惉锛?
		for (const FGameplayTag& EvtTag : Owner->PendingOnHitEventTags)
		{
			for (AActor* HitActor : HitActors)
			{
				FGameplayEventData EvtPayload;
				EvtPayload.Instigator = Owner;
				EvtPayload.Target     = HitActor;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EvtTag, EvtPayload);
			}
		}
	}

	// 娑堣垂鏆傚瓨鏁版嵁锛堟棤璁烘槸鍚﹀懡涓兘娓呯┖锛岄槻姝㈡畫鐣欏埌涓嬩竴娆?Notify锛?
	if (Owner)
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
	}

	if (CombatCardResult.bHadCard)
	{
		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
		{
			if (PlayerOwner->CombatDeckComponent)
			{
				PlayerOwner->CombatDeckComponent->StopCardFlow(CombatCardResult.ConsumedCard);
				PlayerOwner->CombatDeckComponent->StopCardFlow(CombatCardResult.LinkedSourceCard);
				PlayerOwner->CombatDeckComponent->StopCardFlow(CombatCardResult.LinkedTargetCard);
			}
		}

		if (bHasCombatCardAttributeSnapshot && CombatCardASC)
		{
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC(CombatCardASC);
			const auto RestoreAttackSnapshot = [WeakASC, PreCardAttack, PreCardAttackPower](const TCHAR* Reason)
			{
				UAbilitySystemComponent* SnapshotASC = WeakASC.Get();
				if (!SnapshotASC)
				{
					UE_LOG(LogTemp, Warning, TEXT("[CombatCardAttrSnapshot] Restore skipped ASC invalid Reason=%s"), Reason);
					return;
				}

				const float CurrentAttack = SnapshotASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
				const float CurrentAttackPower = SnapshotASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
				UE_LOG(LogTemp, Warning,
					TEXT("[CombatCardAttrSnapshot] RestoreCheck Reason=%s CurrentAttack=%.2f PreAttack=%.2f CurrentAttackPower=%.2f PreAttackPower=%.2f"),
					Reason,
					CurrentAttack,
					PreCardAttack,
					CurrentAttackPower,
					PreCardAttackPower);

				if (CurrentAttack > PreCardAttack + KINDA_SMALL_NUMBER)
				{
					SnapshotASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), PreCardAttack);
					UE_LOG(LogTemp, Warning, TEXT("[CombatCardAttrSnapshot] Restored Attack %.2f -> %.2f Reason=%s"),
						CurrentAttack, PreCardAttack, Reason);
				}
				if (CurrentAttackPower > PreCardAttackPower + KINDA_SMALL_NUMBER)
				{
					SnapshotASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), PreCardAttackPower);
					UE_LOG(LogTemp, Warning, TEXT("[CombatCardAttrSnapshot] Restored AttackPower %.2f -> %.2f Reason=%s"),
						CurrentAttackPower, PreCardAttackPower, Reason);
				}
			};

			RestoreAttackSnapshot(TEXT("Immediate"));
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([RestoreAttackSnapshot]()
				{
					RestoreAttackSnapshot(TEXT("NextTick"));
				}));
			}
		}
	}
}
