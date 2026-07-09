#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "Component/CombatItemComponent.h"
#include "AbilitySystem/Abilities/GA_Knockback.h"
#include "AbilitySystem/Abilities/YogTargetType.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/GameplayCue/HitCueData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/YogAnimNotifyState_Damage.h"
#include "Animation/AnimInstance.h"
#include "Animation/HitStopManager.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Controller/YogAIController.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Component/MontageVFXBindingComponent.h"
#include "Component/BufferComponent.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/RuneDataAsset.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
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

	struct FBroadAttackComboRuntimeState
	{
		int32 LastResolvedComboSlot = 0;
		bool bPendingContinuation = false;
	};

	TMap<TObjectKey<UAbilitySystemComponent>, FStatBeforeAttackSharedSnapshot> GStatBeforeAttackSnapshots;
	TSet<TObjectKey<UAbilitySystemComponent>> GPendingJustComboSpeedBonus;
	TMap<TObjectKey<UAbilitySystemComponent>, FBroadAttackComboRuntimeState> GBroadAttackComboStates;

	FGameplayTag GetComboWindowTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"), false);
	}

	FGameplayTag GetJustComboWindowTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.JustCombo"), false);
	}

	bool IsAttackComboWindowOpen(UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return false;
		}

		const FGameplayTag ComboWindowTag = GetComboWindowTag();
		const FGameplayTag JustComboWindowTag = GetJustComboWindowTag();
		return (ComboWindowTag.IsValid() && ASC->GetTagCount(ComboWindowTag) > 0)
			|| (JustComboWindowTag.IsValid() && ASC->GetTagCount(JustComboWindowTag) > 0);
	}

	constexpr float AttackSpeedDefaultStat = 100.f;
	constexpr float AttackSpeedMaxStat = 200.f;
	constexpr float AttackSpeedStatToMontageRate = 0.01f;
	constexpr float JustComboNextAttackSpeedMultiplier = 1.2f;

	float ConvertAttackSpeedStatToMontageRate(float AttackSpeedStat)
	{
		if (AttackSpeedStat <= 0.f)
		{
			AttackSpeedStat = AttackSpeedDefaultStat;
		}

		return FMath::Clamp(AttackSpeedStat, 0.f, AttackSpeedMaxStat) * AttackSpeedStatToMontageRate;
	}

	bool TryFacePlayerAttackTowardCursor(APlayerCharacterBase* Player)
	{
		if (!Player)
		{
			return false;
		}

		APlayerController* PC = Cast<APlayerController>(Player->GetController());
		if (!PC)
		{
			return false;
		}

		FHitResult CursorHit;
		const ETraceTypeQuery VisibilityTraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
		if (!PC->GetHitResultUnderCursorByChannel(VisibilityTraceType, false, CursorHit))
		{
			return false;
		}

		FVector AttackDirection = CursorHit.ImpactPoint - Player->GetActorLocation();
		AttackDirection.Z = 0.f;
		if (!AttackDirection.Normalize())
		{
			return false;
		}

		Player->SetActorRotation(FRotator(0.f, AttackDirection.Rotation().Yaw, 0.f));
		return true;
	}

	bool TryActivateAbilityByExactTagName(UAbilitySystemComponent* ASC, const TCHAR* TagName)
	{
		if (!ASC || !TagName)
		{
			return false;
		}

		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		if (!Tag.IsValid())
		{
			return false;
		}

		TArray<FGameplayAbilitySpecHandle> MatchingHandles;
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->AbilityTags.HasTagExact(Tag))
			{
				MatchingHandles.Add(Spec.Handle);
			}
		}

		for (const FGameplayAbilitySpecHandle& MatchingHandle : MatchingHandles)
		{
			if (ASC->TryActivateAbility(MatchingHandle, true))
			{
				return true;
			}
		}

		return false;
	}

	bool TryActivateBroadAttackAbility(UAbilitySystemComponent* ASC)
	{
		return TryActivateAbilityByExactTagName(ASC, TEXT("Character.State.Skill.Attack"))
			|| TryActivateAbilityByExactTagName(ASC, TEXT("PlayerState.AbilityCast.Attack"));
	}

	bool HasComboAbilityTag(const UGameplayAbility* Ability)
	{
		if (!Ability)
		{
			return false;
		}

		for (const FGameplayTag& AbilityTag : Ability->AbilityTags)
		{
			if (AbilityTag.IsValid() && AbilityTag.ToString().Contains(TEXT(".Combo")))
			{
				return true;
			}
		}

		return false;
	}

	UGA_MeleeAttack* FindActiveBroadAttackInstance(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle& OutHandle)
	{
		OutHandle = FGameplayAbilitySpecHandle();
		if (!ASC)
		{
			return nullptr;
		}

		const FGameplayTag CharacterAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"), false);
		const FGameplayTag LegacyAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"), false);

		for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.IsActive() || !Spec.Ability)
			{
				continue;
			}

			const bool bAttackSpec =
				(CharacterAttackTag.IsValid() && Spec.Ability->AbilityTags.HasTagExact(CharacterAttackTag))
				|| (LegacyAttackTag.IsValid() && Spec.Ability->AbilityTags.HasTagExact(LegacyAttackTag));
			if (!bAttackSpec)
			{
				continue;
			}
			if (HasComboAbilityTag(Spec.Ability))
			{
				continue;
			}

			for (UGameplayAbility* AbilityInstance : Spec.GetAbilityInstances())
			{
				if (UGA_MeleeAttack* MeleeAttack = Cast<UGA_MeleeAttack>(AbilityInstance))
				{
					OutHandle = Spec.Handle;
					return MeleeAttack;
				}
			}
		}

		return nullptr;
	}

	FGameplayTag GetComboTagForSlot(const TCHAR* Prefix, int32 ComboSlot)
	{
		if (!Prefix || ComboSlot < 1 || ComboSlot > 4)
		{
			return FGameplayTag();
		}

		return FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("%s.Combo%d"), Prefix, ComboSlot)),
			false);
	}

	bool IsComboTag(const FGameplayTag& Tag)
	{
		return Tag.IsValid() && Tag.ToString().Contains(TEXT(".Combo"));
	}

	FGameplayTag ResolveBroadAttackAbilityDataTag(
		const UAbilityData* AbilityData,
		const FGameplayTagContainer& AbilityTags,
		int32 StartSlot,
		int32& OutComboSlot,
		bool& bOutBroadAttack)
	{
		static const FGameplayTag CharacterAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"), false);
		static const FGameplayTag LegacyAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"), false);

		bOutBroadAttack = false;
		if ((!CharacterAttackTag.IsValid() || !AbilityTags.HasTagExact(CharacterAttackTag))
			&& (!LegacyAttackTag.IsValid() || !AbilityTags.HasTagExact(LegacyAttackTag)))
		{
			return FGameplayTag();
		}

		bOutBroadAttack = true;
		const int32 ClampedStartSlot = FMath::Clamp(StartSlot, 1, 4);
		const bool bPreferCharacterTag = CharacterAttackTag.IsValid() && AbilityTags.HasTagExact(CharacterAttackTag);
		const TCHAR* PrimaryPrefix = bPreferCharacterTag
			? TEXT("Character.State.Skill.Attack")
			: TEXT("PlayerState.AbilityCast.Attack");
		const TCHAR* FallbackPrefix = bPreferCharacterTag
			? TEXT("PlayerState.AbilityCast.Attack")
			: TEXT("Character.State.Skill.Attack");

		for (int32 Slot = ClampedStartSlot; Slot <= 4; ++Slot)
		{
			const FGameplayTag PrimaryComboTag = GetComboTagForSlot(PrimaryPrefix, Slot);
			if (AbilityData && PrimaryComboTag.IsValid() && AbilityData->HasAbility(PrimaryComboTag))
			{
				OutComboSlot = Slot;
				return PrimaryComboTag;
			}

			const FGameplayTag FallbackComboTag = GetComboTagForSlot(FallbackPrefix, Slot);
			if (AbilityData && FallbackComboTag.IsValid() && AbilityData->HasAbility(FallbackComboTag))
			{
				OutComboSlot = Slot;
				return FallbackComboTag;
			}
		}

		if (AbilityData && StartSlot > 1)
		{
			const FGameplayTag PrimaryCombo1Tag = GetComboTagForSlot(PrimaryPrefix, 1);
			const FGameplayTag FallbackCombo1Tag = GetComboTagForSlot(FallbackPrefix, 1);
			bool bOnlyCombo1Configured = false;

			if (PrimaryCombo1Tag.IsValid() && AbilityData->HasAbility(PrimaryCombo1Tag))
			{
				bOnlyCombo1Configured = true;
				for (int32 Slot = 2; Slot <= 4; ++Slot)
				{
					const FGameplayTag PrimaryHigherComboTag = GetComboTagForSlot(PrimaryPrefix, Slot);
					const FGameplayTag FallbackHigherComboTag = GetComboTagForSlot(FallbackPrefix, Slot);
					if ((PrimaryHigherComboTag.IsValid() && AbilityData->HasAbility(PrimaryHigherComboTag))
						|| (FallbackHigherComboTag.IsValid() && AbilityData->HasAbility(FallbackHigherComboTag)))
					{
						bOnlyCombo1Configured = false;
						break;
					}
				}
				if (bOnlyCombo1Configured)
				{
					OutComboSlot = 1;
					return PrimaryCombo1Tag;
				}
			}

			if (FallbackCombo1Tag.IsValid() && AbilityData->HasAbility(FallbackCombo1Tag))
			{
				bOnlyCombo1Configured = true;
				for (int32 Slot = 2; Slot <= 4; ++Slot)
				{
					const FGameplayTag PrimaryHigherComboTag = GetComboTagForSlot(PrimaryPrefix, Slot);
					const FGameplayTag FallbackHigherComboTag = GetComboTagForSlot(FallbackPrefix, Slot);
					if ((PrimaryHigherComboTag.IsValid() && AbilityData->HasAbility(PrimaryHigherComboTag))
						|| (FallbackHigherComboTag.IsValid() && AbilityData->HasAbility(FallbackHigherComboTag)))
					{
						bOnlyCombo1Configured = false;
						break;
					}
				}
				if (bOnlyCombo1Configured)
				{
					OutComboSlot = 1;
					return FallbackCombo1Tag;
				}
			}
		}

		OutComboSlot = 0;
		return CharacterAttackTag.IsValid() && AbilityTags.HasTagExact(CharacterAttackTag)
			? CharacterAttackTag
			: LegacyAttackTag;
	}

}

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = false;

	// Runtime guards that should not depend on Blueprint class defaults.
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	const FGameplayTag CharacterAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"));
	AbilityTags.AddTag(CharacterAttackTag);
	AbilityTags.AddTag(AttackTag);
	ActivationOwnedTags.AddTag(CharacterAttackTag);
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Dead"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.HitReact"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Knockback"));
}

bool UGA_MeleeAttack::TryQueueJustComboSpeedBonus(UAbilitySystemComponent* ASC)
{
	const FGameplayTag JustComboWindowTag = GetJustComboWindowTag();
	if (!ASC || !JustComboWindowTag.IsValid() || ASC->GetTagCount(JustComboWindowTag) <= 0)
	{
		return false;
	}

	GPendingJustComboSpeedBonus.Add(TObjectKey<UAbilitySystemComponent>(ASC));
	UE_LOG(LogTemp, Verbose, TEXT("[JustCombo] Queued next attack speed bonus for ASC=%s"), *GetNameSafe(ASC));
	return true;
}

bool UGA_MeleeAttack::TryConsumeJustComboBonus(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return false;
	}

	const TObjectKey<UAbilitySystemComponent> ASCKey(ASC);
	if (!GPendingJustComboSpeedBonus.Contains(ASCKey))
	{
		return false;
	}

	GPendingJustComboSpeedBonus.Remove(ASCKey);
	UE_LOG(LogTemp, Verbose, TEXT("[JustCombo] Consumed pending JustCombo bonus for ASC=%s"), *GetNameSafe(ASC));
	return true;
}

bool UGA_MeleeAttack::TryActivateBroadAttackComboContinuation(UAbilitySystemComponent* ASC)
{
	if (!ASC || !IsAttackComboWindowOpen(ASC))
	{
		return false;
	}

	const TObjectKey<UAbilitySystemComponent> ASCKey(ASC);
	FBroadAttackComboRuntimeState* State = GBroadAttackComboStates.Find(ASCKey);
	if (!State || State->LastResolvedComboSlot <= 0)
	{
		return false;
	}

	FGameplayAbilitySpecHandle ActiveAttackHandle;
	UGA_MeleeAttack* ActiveAttack = FindActiveBroadAttackInstance(ASC, ActiveAttackHandle);
	if (!ActiveAttack)
	{
		return false;
	}

	AYogCharacterBase* ActiveOwner = Cast<AYogCharacterBase>(ActiveAttack->GetAvatarActorFromActorInfo());
	if (!ActiveOwner)
	{
		ActiveOwner = Cast<AYogCharacterBase>(ActiveAttack->GetOwningActorFromActorInfo());
	}

	UCharacterDataComponent* CharacterDataComponent = ActiveOwner ? ActiveOwner->GetCharacterDataComponent() : nullptr;
	UCharacterData* CharacterData = CharacterDataComponent ? CharacterDataComponent->GetCharacterData() : nullptr;
	const UAbilityData* ActiveAbilityData = CharacterData ? CharacterData->AbilityData.Get() : nullptr;
	if (!ActiveAbilityData)
	{
		return false;
	}

	int32 ResolvedComboSlot = 0;
	bool bBroadAttackAbility = false;
	const FGameplayTag ContinuationTag = ResolveBroadAttackAbilityDataTag(
		ActiveAbilityData,
		ActiveAttack->AbilityTags,
		State->LastResolvedComboSlot + 1,
		ResolvedComboSlot,
		bBroadAttackAbility);
	if (!bBroadAttackAbility || !ContinuationTag.IsValid() || !IsComboTag(ContinuationTag))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[GA_MeleeAttack] Broad Attack combo input handled with no configured continuation after slot %d."),
			State->LastResolvedComboSlot);
		return true;
	}

	State->bPendingContinuation = true;

	ActiveAttack->EndAbility(
		ActiveAttack->GetCurrentAbilitySpecHandle(),
		ActiveAttack->GetCurrentActorInfo(),
		ActiveAttack->GetCurrentActivationInfo(),
		true,
		true);

	const bool bActivated = ActiveAttackHandle.IsValid()
		? ASC->TryActivateAbility(ActiveAttackHandle, true)
		: TryActivateBroadAttackAbility(ASC);
	if (!bActivated)
	{
		if (FBroadAttackComboRuntimeState* CurrentState = GBroadAttackComboStates.Find(ASCKey))
		{
			CurrentState->bPendingContinuation = false;
		}
	}

	return bActivated;
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
	if (bActiveComboAttackConfigValid)
	{
		return ActiveComboAttackConfig.BuildActionData();
	}

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
	return ECardRequiredAction::Any;
}

bool UGA_MeleeAttack::IsCombatDeckComboFinisher() const
{
	return false;
}

bool UGA_MeleeAttack::HasConfiguredAttackData() const
{
	return bActiveComboAttackConfigValid || ActiveComboAttackData != nullptr;
}

FGameplayTag UGA_MeleeAttack::GetConfiguredAttackEventTag(FGameplayTag FallbackTag) const
{
	if (bActiveComboAttackConfigValid && ActiveComboAttackConfig.EventTag.IsValid())
	{
		return ActiveComboAttackConfig.EventTag;
	}
	if (ActiveComboAttackData && ActiveComboAttackData->EventTag.IsValid())
	{
		return ActiveComboAttackData->EventTag;
	}
	return FallbackTag;
}

void UGA_MeleeAttack::SetNextActivationFromDashSave(bool bFromDashSave)
{
	bNextActivationFromDashSave = bFromDashSave;
}

AActor* UGA_MeleeAttack::PreviewFirstCombatDeckHitTarget(const FGameplayEventData& EventData) const
{
	AYogCharacterBase* OwnerCharacter = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	if (!OwnerCharacter || !OwnerCharacter->DefaultMeleeTargetType)
	{
		return nullptr;
	}

	const UYogTargetType* TargetTypeCDO = OwnerCharacter->DefaultMeleeTargetType.GetDefaultObject();
	if (!TargetTypeCDO)
	{
		return nullptr;
	}

	TArray<FHitResult> HitResults;
	TArray<AActor*> TargetActors;
	TargetTypeCDO->GetTargets(OwnerCharacter, GetAvatarActorFromActorInfo(), EventData, HitResults, TargetActors);
	for (AActor* TargetActor : TargetActors)
	{
		if (IsValid(TargetActor))
		{
			return TargetActor;
		}
	}

	for (const FHitResult& HitResult : HitResults)
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			return HitActor;
		}
	}

	return nullptr;
}

void UGA_MeleeAttack::PrimeCombatDeckHitContext(const FGameplayEventData& EventData)
{
	AYogCharacterBase* OwnerCharacter = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	if (!OwnerCharacter)
	{
		return;
	}

	UBuffFlowComponent* BuffFlowComponent = OwnerCharacter->FindComponentByClass<UBuffFlowComponent>();
	if (!BuffFlowComponent)
	{
		return;
	}

	BuffFlowComponent->LastEventContext.DamageCauser = OwnerCharacter;
	BuffFlowComponent->LastEventContext.DamageAmount = 0.f;
	BuffFlowComponent->LastEventContext.AttackDirection =
		UGA_Knockback::ResolveAttackDirectionFromSource(OwnerCharacter);
	BuffFlowComponent->LastEventContext.DamageReceivers.Reset();

	AActor* FirstTarget = nullptr;
	if (OwnerCharacter->DefaultMeleeTargetType)
	{
		const UYogTargetType* TargetTypeCDO = OwnerCharacter->DefaultMeleeTargetType.GetDefaultObject();
		if (TargetTypeCDO)
		{
			TArray<FHitResult> HitResults;
			TArray<AActor*> TargetActors;
			TargetTypeCDO->GetTargets(OwnerCharacter, GetAvatarActorFromActorInfo(), EventData, HitResults, TargetActors);

			for (AActor* Actor : TargetActors)
			{
				if (IsValid(Actor))
				{
					BuffFlowComponent->LastEventContext.DamageReceivers.Add(Actor);
					if (!FirstTarget) FirstTarget = Actor;
				}
			}
			for (const FHitResult& Hit : HitResults)
			{
				AActor* Actor = Hit.GetActor();
				if (IsValid(Actor))
				{
					BuffFlowComponent->LastEventContext.DamageReceivers.AddUnique(Actor);
					if (!FirstTarget) FirstTarget = Actor;
				}
			}
		}
	}

	BuffFlowComponent->LastEventContext.DamageReceiver = FirstTarget;

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatDeckHitContext] Prime Target=%s Owner=%s Event=%s HitCount=%d"),
		*GetNameSafe(FirstTarget),
		*GetNameSafe(OwnerCharacter),
		*EventData.EventTag.ToString(),
		BuffFlowComponent->LastEventContext.DamageReceivers.Num());
}

FCombatCardResolveResult UGA_MeleeAttack::ResolveCombatDeck(ECombatCardTriggerTiming TriggerTiming)
{
	FCombatCardResolveResult EmptyResult;
	const bool bResolveFromHitNotify = TriggerTiming == ECombatCardTriggerTiming::OnHit;
	if (!bResolveFromHitNotify && bCombatDeckCardResolvedThisActivation)
	{
		return EmptyResult;
	}

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
	const FGameplayTag CharacterWeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill"), false);
	const FGameplayTag LegacyWeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill"), false);
	Context.ActionSlot = (CharacterWeaponSkillTag.IsValid() && AbilityTags.HasTag(CharacterWeaponSkillTag))
		|| (LegacyWeaponSkillTag.IsValid() && AbilityTags.HasTag(LegacyWeaponSkillTag))
		? ECombatDeckActionSlot::WeaponSkill
		: ECombatDeckActionSlot::Attack;
	// Legacy Combo1/2/3/4 tags may still select montages, but combat cards no
	// longer use combo count as gameplay context. Card order and FlowRole drive
	// build sequencing now.
	Context.ComboIndex = 0;
	Context.ComboNodeId = NAME_None;
	Context.ComboTags.Reset();
	Context.AbilityTag = FGameplayTag();
	Context.WeaponDef = PlayerOwner->EquippedWeaponDef;
	Context.bIsComboFinisher = false;
	Context.FlowRole = Context.bIsComboFinisher ? ECombatDeckFlowRole::Finisher : CombatDeckFlowRole;
	Context.ReleaseMode = Context.bIsComboFinisher ? ECombatCardReleaseMode::Finisher : ECombatCardReleaseMode::Normal;
	Context.bComboContinued = bComboContinued;
	Context.bExitedComboState = bExitedComboState;
	Context.bFromDashSave = bCombatDeckFromDashSave;
	Context.TriggerTiming = TriggerTiming;
	Context.AttackInstanceGuid = bResolveFromHitNotify
		? FGuid::NewGuid()
		: Context.AttackInstanceGuid.IsValid()
		? Context.AttackInstanceGuid
		: (ActiveAttackGuid.IsValid() ? ActiveAttackGuid : FGuid::NewGuid());
	if (!Context.AbilityTag.IsValid())
	{
		for (const FGameplayTag& Tag : AbilityTags)
		{
			Context.AbilityTag = Tag;
			break;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatDeckContext] Runtime=%d Action=%s Slot=%s Role=%s ComboIndex=%d Node=%s Continued=%d Exited=%d Trigger=%d Ability=%s Guid=%s"),
		0,
		*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(Context.ActionType)),
		*StaticEnum<ECombatDeckActionSlot>()->GetNameStringByValue(static_cast<int64>(Context.ActionSlot)),
		*StaticEnum<ECombatDeckFlowRole>()->GetNameStringByValue(static_cast<int64>(Context.FlowRole)),
		Context.ComboIndex,
		*Context.ComboNodeId.ToString(),
		Context.bComboContinued ? 1 : 0,
		Context.bExitedComboState ? 1 : 0,
		static_cast<int32>(Context.TriggerTiming),
		Context.AbilityTag.IsValid() ? *Context.AbilityTag.ToString() : TEXT("(none)"),
		*Context.AttackInstanceGuid.ToString());

	const FCombatCardResolveResult Result = PlayerOwner->CombatDeckComponent->ResolveAttackCardWithContext(Context);
	if (Result.bHadCard)
	{
		if (!bResolveFromHitNotify)
		{
			bCombatDeckCardResolvedThisActivation = true;
		}
		ActiveCombatCardResult = Result;
	}
	return Result;
}

void UGA_MeleeAttack::TryResolveCombatDeckOnHit()
{
	ResolveCombatDeck(ECombatCardTriggerTiming::OnHit);
}

void UGA_MeleeAttack::OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!PlayerOwner)
	{
		PlayerOwner = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	}
	if (!PlayerOwner)
	{
		return;
	}

	UBufferComponent* Buffer = PlayerOwner->GetInputBufferComponent();
	if (!Buffer)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	EInputCommandType BufferedActionType = EInputCommandType::Attack;
	if (!Buffer->ConsumeLatestActionInputSince(AbilityActivationTime, BufferedActionType))
	{
		// === DIAG: attack-stuck repro (CL564) ===
		// Current attack no longer chains Light/Heavy; the combo window is only
		// retained for cancel/interruption inputs.
		UE_LOG(LogTemp, Warning,
			TEXT("[Melee][DIAG564] OnCanComboTagChanged EARLY-RETURN: no buffered action since AbilityActivationTime=%.3f Tag=%s NewCount=%d"),
			AbilityActivationTime, *Tag.ToString(), NewCount);
		return;
	}

	bool bActivated = false;
	if (UAbilitySystemComponent* PlayerASC = PlayerOwner->GetASC())
	{
		UYogAbilitySystemComponent* PlayerYogASC = Cast<UYogAbilitySystemComponent>(PlayerASC);
		const TCHAR* CharacterTagName = TEXT("Character.State.Skill.Attack");
		const TCHAR* LegacyTagName = TEXT("PlayerState.AbilityCast.Attack");
		bool bUseFallbackTag = true;
		switch (BufferedActionType)
		{
		case EInputCommandType::Attack:
			bUseFallbackTag = false;
			bActivated = TryActivateBroadAttackComboContinuation(PlayerASC);
			if (!bActivated)
			{
				bActivated = PlayerYogASC ? PlayerYogASC->TryActivateNextAttackComboAbility(true, true) : false;
			}
			break;
		case EInputCommandType::WeaponSkill:
			bUseFallbackTag = false;
			bActivated = PlayerYogASC ? PlayerYogASC->TryActivateNextWeaponSkillComboAbility(true, true) : false;
			break;
		case EInputCommandType::Dash:
			CharacterTagName = TEXT("Character.State.Movement.Dash");
			LegacyTagName = TEXT("PlayerState.AbilityCast.Dash");
			break;
		case EInputCommandType::Skill:
			bUseFallbackTag = false;
			bActivated = PlayerOwner->ActiveSkillComponent
				? PlayerOwner->ActiveSkillComponent->UseActiveSkill()
				: false;
			break;
		default:
			break;
		}

		if (bUseFallbackTag)
		{
			bActivated = TryActivateAbilityByExactTagName(PlayerASC, CharacterTagName)
				|| TryActivateAbilityByExactTagName(PlayerASC, LegacyTagName);
		}
	}

	// === DIAG: attack-stuck repro (CL564) ===
	UE_LOG(LogTemp, Verbose,
		TEXT("[Melee][DIAG564] OnCanComboTagChanged buffered=%s HasComboSource=%d Activated=%d Tag=%s"),
		*StaticEnum<EInputCommandType>()->GetNameStringByValue(static_cast<int64>(BufferedActionType)),
		0, bActivated ? 1 : 0, *Tag.ToString());

	if (!bActivated && ASC && Tag.IsValid())
	{
		ASC->SetLooseGameplayTagCount(Tag, 0);
		UE_LOG(LogTemp, Warning,
			TEXT("[Melee][DIAG564] OnCanComboTagChanged ACTIVATION FAILED: cleared %s tag count"),
			*Tag.ToString());
	}
}

void UGA_MeleeAttack::TryStartEnemyRadialLunge()
{
	if (!ActiveEnemyAttackContext.bValid)
	{
		return;
	}

	const FEnemyAIAttackOption& AttackOption = ActiveEnemyAttackContext.AttackOption;
	const bool bReposition = AttackOption.AttackRole == EEnemyAIAttackRole::Reposition;
	// Reposition options drive the dash through their role rather than a movement mode,
	// so they bypass the BT movement-attack cooldown machinery.
	if (!bReposition && AttackOption.AttackMovementMode != EEnemyAIAttackMovementMode::RadialLunge)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AActor* TargetActor = ActiveEnemyAttackContext.TargetActor.Get();
	if (!Character || !TargetActor)
	{
		return;
	}

	const float CurrentDistance = FVector::Dist2D(Character->GetActorLocation(), TargetActor->GetActorLocation());
	// LungeStartRange is a gap-close gate for toward-target lunges; a reposition dash can
	// fire at any range (it usually triggers point-blank after a whiff).
	if ((!bReposition && CurrentDistance < AttackOption.LungeStartRange)
		|| AttackOption.LungeDistance <= 0.0f
		|| AttackOption.LungeDuration <= 0.0f)
	{
		return;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - Character->GetActorLocation();
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}
	ToTarget = ToTarget.GetSafeNormal();

	FVector DashDirection;
	if (bReposition)
	{
		const float Angle = FMath::FRandRange(AttackOption.RepositionAngleMin, AttackOption.RepositionAngleMax);
		const float Sign = FMath::RandBool() ? 1.0f : -1.0f;
		DashDirection = (-ToTarget).RotateAngleAxis(Sign * Angle, FVector::UpVector);
	}
	else
	{
		DashDirection = ToTarget;
	}

	// Always face the target so a follow-up attack is aimed correctly, even when the dash
	// itself travels sideways.
	const FRotator FaceTargetRotation(0.0f, ToTarget.Rotation().Yaw, 0.0f);
	Character->SetActorRotation(FaceTargetRotation);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FaceTargetRotation);
	}

	float MoveDistance;
	if (bReposition)
	{
		MoveDistance = AttackOption.LungeDistance;
	}
	else
	{
		const float MaxUsefulDistance = FMath::Max(CurrentDistance - AttackOption.LungeStopDistance, 0.0f);
		MoveDistance = FMath::Min(AttackOption.LungeDistance, MaxUsefulDistance);
	}
	if (MoveDistance <= 0.0f)
	{
		return;
	}

	const FVector TargetLocation = Character->GetActorLocation() + DashDirection * MoveDistance;
	EnemyLungeTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		FName(TEXT("EnemyRadialLunge")),
		TargetLocation,
		AttackOption.LungeDuration,
		false,
		EMovementMode::MOVE_Walking,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity,
		FVector::ZeroVector,
		0.0f);

	if (EnemyLungeTask)
	{
		EnemyLungeTask->ReadyForActivation();
	}
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
	bComboContinued = false;
	bExitedComboState = false;
	CombatDeckHitResolveCounter = 0;
	bHasStatBeforeAttributeSnapshot = false;
	LocalPreStatBeforeAttack = 0.f;
	LocalPreStatBeforeAttackPower = 0.f;
	LocalStatBeforeAttackDelta = 0.f;
	LocalStatBeforeAttackPowerDelta = 0.f;
	ActiveCombatCardResult = FCombatCardResolveResult();
	ActiveComboAttackData = nullptr;
	ActiveComboAttackConfig = FComboAttackConfig();
	bActiveComboAttackConfigValid = false;
	EnemyLungeTask = nullptr;
	ActiveEnemyAttackContext = FEnemyAIAttackRuntimeContext();
	ActiveAttackGuid.Invalidate();
	ActiveComboIndex = 0;
	ActiveComboTags.Reset();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] ActivateAbility: %s"), *GetName());
	AbilityActivationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		const TObjectKey<UAbilitySystemComponent> ASCKey(ASC);
		FBroadAttackComboRuntimeState& BroadAttackComboState = GBroadAttackComboStates.FindOrAdd(ASCKey);
		if (!BroadAttackComboState.bPendingContinuation)
		{
			BroadAttackComboState.LastResolvedComboSlot = 0;
		}

		const FGameplayTag CharacterCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"));
		const FGameplayTag CharacterJustComboTag = GetJustComboWindowTag();
		if (ASC->GetTagCount(CharacterCanComboTag) > 0)
		{
			ASC->SetLooseGameplayTagCount(CharacterCanComboTag, 0);
		}
		if (CharacterJustComboTag.IsValid() && ASC->GetTagCount(CharacterJustComboTag) > 0)
		{
			ASC->SetLooseGameplayTagCount(CharacterJustComboTag, 0);
		}

		if (CanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CharacterCanComboTag, EGameplayTagEventType::NewOrRemoved);
			CanComboTagHandle.Reset();
		}
		if (JustComboTagHandle.IsValid() && CharacterJustComboTag.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(JustComboTagHandle, CharacterJustComboTag, EGameplayTagEventType::NewOrRemoved);
			JustComboTagHandle.Reset();
		}
		CanComboTagHandle = ASC->RegisterGameplayTagEvent(CharacterCanComboTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UGA_MeleeAttack::OnCanComboTagChanged);
		if (CharacterJustComboTag.IsValid())
		{
			JustComboTagHandle = ASC->RegisterGameplayTagEvent(CharacterJustComboTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UGA_MeleeAttack::OnCanComboTagChanged);
		}
	}

	// 玩家 GA：检查消冷却
	if (bRequireCommit && !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Read montage from AbilityData using this GA's combo tag.
	AYogCharacterBase* ActivateOwner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	if (AEnemyCharacterBase* EnemyOwner = Cast<AEnemyCharacterBase>(ActivateOwner))
	{
		EnemyOwner->ConsumeAIAttackRuntimeContext(ActiveEnemyAttackContext);
	}

	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!PlayerOwner)
	{
		PlayerOwner = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	}
	if (UBufferComponent* Buffer = PlayerOwner ? PlayerOwner->GetInputBufferComponent() : nullptr)
	{
		Buffer->ClearBuffer();
	}
	if (!ActiveAttackGuid.IsValid())
	{
		ActiveAttackGuid = FGuid::NewGuid();
	}

	UCharacterDataComponent* CDC = ActivateOwner ? ActivateOwner->GetCharacterDataComponent() : nullptr;
	UCharacterData* CD = CDC ? CDC->GetCharacterData() : nullptr;
	const UAbilityData* ActiveAbilityData = CD ? CD->AbilityData.Get() : nullptr;

	FGameplayTag FirstTag;
	bool bBroadAttackResolvedFromComboSlot = false;
	bool bBroadAttackAbility = false;
	int32 BroadAttackComboSlot = 0;
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (FBroadAttackComboRuntimeState* BroadAttackComboState = GBroadAttackComboStates.Find(TObjectKey<UAbilitySystemComponent>(ASC)))
		{
			BroadAttackComboSlot = BroadAttackComboState->bPendingContinuation
				? BroadAttackComboState->LastResolvedComboSlot + 1
				: 1;
			BroadAttackComboState->bPendingContinuation = false;
		}
	}

	const FGameplayTag BroadAttackComboTag = ResolveBroadAttackAbilityDataTag(
		ActiveAbilityData,
		AbilityTags,
		BroadAttackComboSlot,
		BroadAttackComboSlot,
		bBroadAttackAbility);
	if (BroadAttackComboTag.IsValid()
		&& (!bBroadAttackAbility || IsComboTag(BroadAttackComboTag)))
	{
		FirstTag = BroadAttackComboTag;
		ActiveComboIndex = BroadAttackComboSlot;
		ActiveComboTags.AddTag(BroadAttackComboTag);
		bBroadAttackResolvedFromComboSlot = bBroadAttackAbility;
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			if (FBroadAttackComboRuntimeState* BroadAttackComboState = GBroadAttackComboStates.Find(TObjectKey<UAbilitySystemComponent>(ASC)))
			{
				BroadAttackComboState->LastResolvedComboSlot = ActiveComboIndex;
			}
		}
	}

	static const FGameplayTag PreferredAbilityTags[] = {
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.LAtk1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.LAtk2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.LAtk3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.LAtk4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.HAtk1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.HAtk2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.HAtk3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Range.HAtk4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Skill.Skill1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Skill.Skill2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Skill.Skill3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Skill.Skill4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash.Combo4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash.Combo4"), false),
	};
	if (!FirstTag.IsValid())
	{
		for (const FGameplayTag& PreferredTag : PreferredAbilityTags)
		{
			if (PreferredTag.IsValid() && AbilityTags.HasTagExact(PreferredTag))
			{
				FirstTag = PreferredTag;
				const FString TagText = PreferredTag.ToString();
				if (TagText.EndsWith(TEXT(".Combo1")))
				{
					ActiveComboIndex = 1;
				}
				else if (TagText.EndsWith(TEXT(".Combo2")))
				{
					ActiveComboIndex = 2;
				}
				else if (TagText.EndsWith(TEXT(".Combo3")))
				{
					ActiveComboIndex = 3;
				}
				else if (TagText.EndsWith(TEXT(".Combo4")))
				{
					ActiveComboIndex = 4;
				}
				ActiveComboTags.AddTag(PreferredTag);
				break;
			}
		}
	}
	if (!FirstTag.IsValid())
	{
		for (const FGameplayTag& Tag : AbilityTags) { FirstTag = Tag; break; }
	}

	UAnimMontage* Montage = (CD && CD->AbilityData && FirstTag.IsValid())
		? CD->AbilityData->GetMontage(FirstTag) : nullptr;

	ActiveMontageConfig = nullptr;
	if (CD && CD->AbilityData && FirstTag.IsValid())
	{
		FGameplayTagContainer ContextTags;
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->GetOwnedGameplayTags(ContextTags);
		}

		ActiveMontageConfig = CD->AbilityData->GetMontageConfig(FirstTag, ContextTags);
		if (ActiveMontageConfig && ActiveMontageConfig->Montage)
		{
			Montage = ActiveMontageConfig->Montage;
		}
	}

	// 缓存第一AN_MeleeDamage，后GetAbilityActionData / StatAfterATK 使用
	CachedDamageNotify = GetFirstDamageNotify(Montage);

	// 施加攻击前摇 GE（玩GA 配置，敌GA 留空跳过	if (StatBeforeATKEffect)
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

	// 重置命中标志（为本次攻击的第一节做准备
	if (ActivateOwner)
	{
		ActivateOwner->bComboHitConnected = false;
	}

	if (!Montage)
	{
		const bool bMontageMapHasKey = ActiveAbilityData && FirstTag.IsValid() && ActiveAbilityData->MontageMap.Contains(FirstTag);
		const bool bHasConfiguredAbility = ActiveAbilityData && FirstTag.IsValid() && ActiveAbilityData->HasAbility(FirstTag);
		UE_LOG(LogTemp, Warning,
			TEXT("[GA_MeleeAttack] No Montage found for %s. Owner=%s LookupTag=%s CharacterData=%s AbilityData=%s MontageMapHasKey=%d HasAbility=%d - ability ended immediately."),
			*GetName(),
			*GetNameSafe(ActivateOwner),
			FirstTag.IsValid() ? *FirstTag.ToString() : TEXT("(none)"),
			*GetNameSafe(CD),
			*GetNameSafe(ActiveAbilityData),
			bMontageMapHasKey ? 1 : 0,
			bHasConfiguredAbility ? 1 : 0);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 监听 AnimNotify 伤害事件：必须明确传Tag，空容器不会注册任何监听
	// Combat cards resolve from AN_MeleeDamage, so effects and UI resolve state
	// happen on the actual attack notify frame instead of montage start.

	FGameplayTagContainer DamageEventTags;
	DamageEventTags.AddTag(FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack")));

	// AttackSpeed is authored as a linear 0-200 stat and converted to montage play rate.
	float AttackSpeedStat = AttackSpeedDefaultStat;
	bool bConsumedJustComboSpeedBonus = false;
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		bool bFound = false;
		const float SpeedValue = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetAttackSpeedAttribute(), bFound);
		if (bFound)
		{
			AttackSpeedStat = SpeedValue;
		}

		if (TryConsumeJustComboBonus(ASC))
		{
			AttackSpeedStat *= JustComboNextAttackSpeedMultiplier;
			bConsumedJustComboSpeedBonus = true;
			ApplyJustComboGE(ActorInfo);
		}
	}

	if (bBroadAttackResolvedFromComboSlot)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[GA_MeleeAttack] Broad Attack resolved AbilityData combo slot %d using %s Montage=%s"),
			ActiveComboIndex,
			*FirstTag.ToString(),
			*GetNameSafe(Montage));
	}
	const float AttackSpeedRate = ConvertAttackSpeedStatToMontageRate(AttackSpeedStat);
	if (bConsumedJustComboSpeedBonus)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[JustCombo] Consumed next attack speed bonus. AttackSpeedStat=%.2f MontagePlayRate=%.2f"), AttackSpeedStat, AttackSpeedRate);
	}

	// 创建复合任务：播放蒙太奇 + 监听 GameplayEvent（AnimNotify 触发伤害事件
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

	TryStartEnemyRadialLunge();

	// Resolve OnCommit cards before the montage starts so pre-montage flows
	// (e.g. weapon trail setup) can execute before the first animation frame.
	// OnHit cards are unaffected: they still fire from OnEventReceived via AN_MeleeDamage.
	ResolveCombatDeck(ECombatCardTriggerTiming::OnCommit);

	TryFacePlayerAttackTowardCursor(PlayerOwner);

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
	if (ASC && CanComboTagHandle.IsValid())
	{
		const FGameplayTag CharacterCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"));
		ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CharacterCanComboTag, EGameplayTagEventType::NewOrRemoved);
		CanComboTagHandle.Reset();
	}
	if (ASC && JustComboTagHandle.IsValid())
	{
		const FGameplayTag CharacterJustComboTag = GetJustComboWindowTag();
		if (CharacterJustComboTag.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(JustComboTagHandle, CharacterJustComboTag, EGameplayTagEventType::NewOrRemoved);
		}
		JustComboTagHandle.Reset();
	}
	if (ASC)
	{
		ASC->SetLooseGameplayTagCount(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo")), 0);
		const FGameplayTag CharacterJustComboTag = GetJustComboWindowTag();
		if (CharacterJustComboTag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(CharacterJustComboTag, 0);
		}
	}

	if (EnemyLungeTask)
	{
		EnemyLungeTask->EndTask();
		EnemyLungeTask = nullptr;
	}

	// 安全清理：技能结束时清空未消费的暂存数据（蒙太奇被打断未触发 OnEventReceived 时保护用
	if (AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo()))
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
		Owner->PendingHitImpactCueTag = FGameplayTag();
		Owner->PendingHitImpactCueData = nullptr;
	}

	// 移除攻击前摇 GE
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

	// 施加攻击后摇 GE（仅正常结束时，Cancel/Interrupt 不触发）
	// 优先用最后命中的 Notify 数据（多段命中代表最后一击），未命中过则 fallback 到第一Notify
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
			const FActionData ActionData = bActiveComboAttackConfigValid
				? ActiveComboAttackConfig.BuildActionData()
				: ActiveComboAttackData
				? ActiveComboAttackData->BuildActionData()
				: LastFiredDamageNotify
				? LastFiredDamageNotify->BuildActionData()
				: LastFiredDamageWindow
				? LastFiredDamageWindow->ResolveActionData(Cast<AYogCharacterBase>(GetOwningActorFromActorInfo()))
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

	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (ActiveCombatCardResult.bHadCard && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.ResolvedCard);
			PlayerOwner->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.LinkedSourceCard);
			PlayerOwner->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.LinkedTargetCard);
		}
	}

	if (const AYogCharacterBase* AvatarCharacter = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (UMontageVFXBindingComponent* VFXBindingComponent =
			AvatarCharacter->FindComponentByClass<UMontageVFXBindingComponent>())
		{
			VFXBindingComponent->ClearAllBindings();
		}
	}

	// Report whiff/connect outcome for enemy AI profile attacks so the BT can react
	// (e.g. reposition on a miss). ActiveEnemyAttackContext is only valid for AI-driven
	// attacks; bComboHitConnected is reset on activation and set true on any landed hit.
	if (!bWasCancelled && ActiveEnemyAttackContext.bValid)
	{
		if (AEnemyCharacterBase* EnemyOwner = Cast<AEnemyCharacterBase>(GetOwningActorFromActorInfo()))
		{
			if (AYogAIController* YogAI = Cast<AYogAIController>(EnemyOwner->GetController()))
			{
				// A reposition move deals no damage by design; treat it as resolved (not a
				// whiff) so it clears the flag instead of re-triggering itself endlessly.
				const bool bRepositionMove =
					ActiveEnemyAttackContext.AttackOption.AttackRole == EEnemyAIAttackRole::Reposition;
				const bool bWhiffed = !bRepositionMove && !EnemyOwner->bComboHitConnected;
				const bool bForceReposition = !bRepositionMove
					&& ActiveEnemyAttackContext.AttackOption.bRequestRepositionOnResolve;
				YogAI->NotifyAttackResolved(bWhiffed, bForceReposition);
			}
		}
	}

	CachedDamageNotify    = nullptr;
	LastFiredDamageNotify = nullptr;
	LastFiredDamageWindow = nullptr;
	ActiveMontageConfig = nullptr;
	ActiveComboAttackData = nullptr;
	ActiveComboAttackConfig = FComboAttackConfig();
	bActiveComboAttackConfigValid = false;
	ActiveEnemyAttackContext = FEnemyAIAttackRuntimeContext();
	bComboContinued = true;
	bExitedComboState = false;
	CombatDeckHitResolveCounter = 0;
	bHasStatBeforeAttributeSnapshot = false;
	LocalPreStatBeforeAttack = 0.f;
	LocalPreStatBeforeAttackPower = 0.f;
	LocalStatBeforeAttackDelta = 0.f;
	LocalStatBeforeAttackPowerDelta = 0.f;
	ActiveCombatCardResult = FCombatCardResolveResult();
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

UGA_MeleeAttack::FMeleeHitAttrSnapshot UGA_MeleeAttack::NormalizeAttrsPreCard(UAbilitySystemComponent* ASC)
{
	FMeleeHitAttrSnapshot Snapshot;
	if (!ASC)
	{
		return Snapshot;
	}

	bool bFoundAttack = false;
	bool bFoundAttackPower = false;
	Snapshot.PreCardAttack      = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetAttackAttribute(), bFoundAttack);
	Snapshot.PreCardAttackPower = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetAttackPowerAttribute(), bFoundAttackPower);
	Snapshot.bValid = bFoundAttack && bFoundAttackPower;

	if (!bHasStatBeforeAttributeSnapshot || !Snapshot.bValid)
	{
		return Snapshot;
	}

	float BaselineAttack      = LocalPreStatBeforeAttack;
	float BaselineAttackPower = LocalPreStatBeforeAttackPower;
	const TObjectKey<UAbilitySystemComponent> ASCKey(ASC);
	if (const FStatBeforeAttackSharedSnapshot* Shared = GStatBeforeAttackSnapshots.Find(ASCKey))
	{
		BaselineAttack      = Shared->Attack;
		BaselineAttackPower = Shared->AttackPower;
	}

	const float IntendedAttack      = BaselineAttack      + LocalStatBeforeAttackDelta;
	const float IntendedAttackPower = BaselineAttackPower + LocalStatBeforeAttackPowerDelta;

	if (!FMath::IsNearlyEqual(Snapshot.PreCardAttack, IntendedAttack, KINDA_SMALL_NUMBER))
	{
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), IntendedAttack);
	}
	if (!FMath::IsNearlyEqual(Snapshot.PreCardAttackPower, IntendedAttackPower, KINDA_SMALL_NUMBER))
	{
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), IntendedAttackPower);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatCardAttrNormalize] PreCard Attack %.2f -> %.2f AttackPower %.2f -> %.2f BaselineAttack=%.2f StatBeforeDelta=%.2f HitIndex=%d"),
		Snapshot.PreCardAttack, IntendedAttack,
		Snapshot.PreCardAttackPower, IntendedAttackPower,
		BaselineAttack, LocalStatBeforeAttackDelta, CombatDeckHitResolveCounter);

	Snapshot.PreCardAttack      = IntendedAttack;
	Snapshot.PreCardAttackPower = IntendedAttackPower;
	return Snapshot;
}

void UGA_MeleeAttack::NormalizeAttrsPostCard(UAbilitySystemComponent* ASC, const FMeleeHitAttrSnapshot& Snapshot)
{
	if (!Snapshot.bValid || !ASC)
	{
		return;
	}

	const float PostCardAttack      = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
	const float PostCardAttackPower = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
	const float CardAttackDelta      = FMath::Max(0.f, PostCardAttack      - Snapshot.PreCardAttack);
	const float CardAttackPowerDelta = FMath::Max(0.f, PostCardAttackPower - Snapshot.PreCardAttackPower);

	const float IntendedAttack      = Snapshot.PreCardAttack      + CardAttackDelta;
	const float IntendedAttackPower = Snapshot.PreCardAttackPower + CardAttackPowerDelta;

	if (!FMath::IsNearlyEqual(PostCardAttack, IntendedAttack, KINDA_SMALL_NUMBER))
	{
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), IntendedAttack);
	}
	if (!FMath::IsNearlyEqual(PostCardAttackPower, IntendedAttackPower, KINDA_SMALL_NUMBER))
	{
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), IntendedAttackPower);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatCardAttrNormalize] Damage Attack %.2f -> %.2f CardDelta=%.2f AttackPower %.2f -> %.2f CardPowerDelta=%.2f"),
		PostCardAttack, IntendedAttack, CardAttackDelta,
		PostCardAttackPower, IntendedAttackPower, CardAttackPowerDelta);
}

void UGA_MeleeAttack::ApplyHitStop(AYogCharacterBase* Owner, const TArray<AActor*>& HitActors)
{
	AYogCharacterBase::FPendingHitStopOverride& Override = Owner->PendingHitStopOverride;
	if (!Override.bActive || Override.Mode == EHitStopMode::None)
	{
		return;
	}

	UAnimInstance* AnimInst = Owner->GetMesh() ? Owner->GetMesh()->GetAnimInstance() : nullptr;
	UHitStopManager* HitStop = Owner->GetWorld() ? Owner->GetWorld()->GetSubsystem<UHitStopManager>() : nullptr;
	Override.bActive = false;

	if (!AnimInst || !HitStop)
	{
		return;
	}

	const float Frozen = (Override.Mode == EHitStopMode::Freeze) ? Override.FrozenDuration : 0.f;
	const float Slow   = (Override.Mode == EHitStopMode::Slow)   ? Override.SlowDuration   : 0.f;
	HitStop->RequestMontageHitStop(AnimInst, Frozen, Slow, Override.SlowRate, Override.CatchUpRate);

	for (AActor* HitActor : HitActors)
	{
		if (AYogCharacterBase* HitChar = Cast<AYogCharacterBase>(HitActor))
		{
			if (UAnimInstance* TargetAnim = HitChar->GetMesh() ? HitChar->GetMesh()->GetAnimInstance() : nullptr)
			{
				HitStop->RequestMontageHitStop(TargetAnim, Frozen, Slow, Override.SlowRate, Override.CatchUpRate);
			}
		}
	}
}

void UGA_MeleeAttack::ApplyHitReactions(AYogCharacterBase* Owner, const FYogGameplayEffectContainerSpec& ContainerSpec)
{
	Owner->bComboHitConnected = true;

	TArray<AActor*> HitActors;
	for (const TSharedPtr<FGameplayAbilityTargetData>& Data : ContainerSpec.TargetData.Data)
	{
		if (Data.IsValid())
		{
			for (TWeakObjectPtr<AActor> WeakActor : Data->GetActors())
			{
				if (AActor* Actor = WeakActor.Get())
				{
					HitActors.AddUnique(Actor);
				}
			}
		}
	}

	if (HitActors.Num() > 0)
	{
		ApplyHitStop(Owner, HitActors);

		APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner);
		if (PlayerOwner)
		{
			for (AActor* HitActor : HitActors)
			{
				if (AEnemyCharacterBase* HitEnemy = Cast<AEnemyCharacterBase>(HitActor))
				{
					HitEnemy->PlayCosmeticHitPushFromLocation(Owner->GetActorLocation());
				}
			}
		}

		static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
		for (AActor* HitActor : HitActors)
		{
			FGameplayEventData Payload;
			Payload.Instigator = Owner;
			Payload.Target     = HitActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitTag, Payload);
		}

		if (PlayerOwner)
		{
			if (PlayerOwner->CombatItemComponent)
			{
				for (AActor* HitActor : HitActors)
				{
					PlayerOwner->CombatItemComponent->ApplyOilBladeHitToTarget(HitActor);
				}
			}
		}

		for (URuneDataAsset* RuneDA : Owner->PendingAdditionalHitRunes)
		{
			if (!RuneDA)
			{
				continue;
			}
			for (AActor* HitActor : HitActors)
			{
				if (AYogCharacterBase* HitChar = Cast<AYogCharacterBase>(HitActor))
				{
					HitChar->ReceiveOnHitRune(RuneDA, Owner);
				}
			}
		}

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

	if (Owner->PendingHitImpactCueTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParams;
			CueParams.Instigator = Owner;
			CueParams.EffectCauser = Owner;
			CueParams.SourceObject = Owner->PendingHitImpactCueData.Get();
			if (!HitActors.IsEmpty() && HitActors[0])
			{
				CueParams.Location = HitActors[0]->GetActorLocation();
				CueParams.Normal = (HitActors[0]->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
			}
			else
			{
				CueParams.Location = Owner->GetActorLocation();
			}
			ASC->ExecuteGameplayCue(Owner->PendingHitImpactCueTag, CueParams);
		}
		Owner->PendingHitImpactCueTag = FGameplayTag();
		Owner->PendingHitImpactCueData = nullptr;
	}

	Owner->PendingAdditionalHitRunes.Empty();
	Owner->PendingOnHitEventTags.Empty();
	Owner->PendingHitStopOverride = AYogCharacterBase::FPendingHitStopOverride();
}

void UGA_MeleeAttack::RestoreAttrsPostCard(
	UAbilitySystemComponent* ASC,
	const FCombatCardResolveResult& CardResult,
	const FMeleeHitAttrSnapshot& Snapshot)
{
	if (!CardResult.bHadCard)
	{
		return;
	}

	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->StopCardFlow(CardResult.ResolvedCard);
			PlayerOwner->CombatDeckComponent->StopCardFlow(CardResult.LinkedSourceCard);
			PlayerOwner->CombatDeckComponent->StopCardFlow(CardResult.LinkedTargetCard);
		}
	}

	if (!Snapshot.bValid || !ASC)
	{
		return;
	}

	TWeakObjectPtr<UAbilitySystemComponent> WeakASC(ASC);
	const float PreAttack      = Snapshot.PreCardAttack;
	const float PreAttackPower = Snapshot.PreCardAttackPower;

	const auto RestoreSnapshot = [WeakASC, PreAttack, PreAttackPower](const TCHAR* Reason)
	{
		UAbilitySystemComponent* SnapshotASC = WeakASC.Get();
		if (!SnapshotASC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CombatCardAttrSnapshot] Restore skipped: ASC invalid Reason=%s"), Reason);
			return;
		}

		const float CurrentAttack      = SnapshotASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
		const float CurrentAttackPower = SnapshotASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
		UE_LOG(LogTemp, Warning,
			TEXT("[CombatCardAttrSnapshot] RestoreCheck Reason=%s CurrentAttack=%.2f PreAttack=%.2f CurrentAttackPower=%.2f PreAttackPower=%.2f"),
			Reason, CurrentAttack, PreAttack, CurrentAttackPower, PreAttackPower);

		if (CurrentAttack > PreAttack + KINDA_SMALL_NUMBER)
		{
			SnapshotASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), PreAttack);
			UE_LOG(LogTemp, Warning, TEXT("[CombatCardAttrSnapshot] Restored Attack %.2f -> %.2f Reason=%s"),
				CurrentAttack, PreAttack, Reason);
		}
		if (CurrentAttackPower > PreAttackPower + KINDA_SMALL_NUMBER)
		{
			SnapshotASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackPowerAttribute(), PreAttackPower);
			UE_LOG(LogTemp, Warning, TEXT("[CombatCardAttrSnapshot] Restored AttackPower %.2f -> %.2f Reason=%s"),
				CurrentAttackPower, PreAttackPower, Reason);
		}
	};

	RestoreSnapshot(TEXT("Immediate"));
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([RestoreSnapshot]()
		{
			RestoreSnapshot(TEXT("NextTick"));
		}));
	}
}

void UGA_MeleeAttack::OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData)
{
	// Re-entry guard: downstream broadcasts (Ability.Event.Attack.Hit / CritHit, DamageExecution events)
	// can be caught by this same task if the BP's EventTags filter is too broad, causing infinite recursion.
	if (bIsHandlingMeleeEvent)
	{
		return;
	}
	TGuardValue<bool> ReentrancyGuard(bIsHandlingMeleeEvent, true);

	UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] OnEventReceived: %s on %s"), *EventTag.ToString(), *GetName());
	++CombatDeckHitResolveCounter;

	if (const UAN_MeleeDamage* FiredNotify = Cast<const UAN_MeleeDamage>(EventData.OptionalObject))
	{
		LastFiredDamageNotify = FiredNotify;
		if (UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
		{
			SourceASC->CurrentActionPoiseBonus = GetAbilityActionData().ActResilience;
		}
	}

	if (const UYogAnimNotifyState_Damage* FiredDamageWindow = Cast<const UYogAnimNotifyState_Damage>(EventData.OptionalObject))
	{
		AYogCharacterBase* OwnerCharacter = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
		const UYogTargetType* TargetTypeCDO = OwnerCharacter && OwnerCharacter->DefaultMeleeTargetType
			? OwnerCharacter->DefaultMeleeTargetType.GetDefaultObject()
			: nullptr;
		TArray<FHitResult> WindowHitResults;
		TArray<AActor*> WindowTargetActors;
		if (TargetTypeCDO)
		{
			TargetTypeCDO->GetTargets(OwnerCharacter, GetAvatarActorFromActorInfo(), EventData, WindowHitResults, WindowTargetActors);
		}
		if (WindowHitResults.IsEmpty() && WindowTargetActors.IsEmpty())
		{
			if (OwnerCharacter)
			{
				OwnerCharacter->PendingAdditionalHitRunes.Empty();
				OwnerCharacter->PendingOnHitEventTags.Empty();
				OwnerCharacter->PendingHitStopOverride = AYogCharacterBase::FPendingHitStopOverride();
				OwnerCharacter->PendingHitImpactCueTag = FGameplayTag();
				OwnerCharacter->PendingHitImpactCueData = nullptr;
			}
			return;
		}

		LastFiredDamageWindow = FiredDamageWindow;
		if (UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
		{
			SourceASC->CurrentActionPoiseBonus = FiredDamageWindow->ResolveActionData(OwnerCharacter).ActResilience;
		}
	}

	UAbilitySystemComponent* CombatCardASC = GetAbilitySystemComponentFromActorInfo();
	const FMeleeHitAttrSnapshot AttrSnapshot = NormalizeAttrsPreCard(CombatCardASC);

	PrimeCombatDeckHitContext(EventData);
	const FCombatCardResolveResult CombatCardResult = ResolveCombatDeck(ECombatCardTriggerTiming::OnHit);

	NormalizeAttrsPostCard(CombatCardASC, AttrSnapshot);

	UE_LOG(LogTemp, Warning,
		TEXT("[CombatCardAttrSnapshot] Resolve Card=%s Had=%d Base=%d Link=%d PreAttack=%.2f PreAttackPower=%.2f ASC=%s"),
		*CombatCardResult.ResolvedCard.Config.DisplayName.ToString(),
		CombatCardResult.bHadCard ? 1 : 0,
		CombatCardResult.bTriggeredBaseFlow ? 1 : 0,
		CombatCardResult.bTriggeredLink ? 1 : 0,
		AttrSnapshot.PreCardAttack,
		AttrSnapshot.PreCardAttackPower,
		*GetNameSafe(CombatCardASC));

	const FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	const TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);
	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());

	if (Handles.Num() > 0 && Owner)
	{
		ApplyHitReactions(Owner, ContainerSpec);
		RestoreAttrsPostCard(CombatCardASC, CombatCardResult, AttrSnapshot);
	}
}
