#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "Component/CombatItemComponent.h"
#include "AbilitySystem/Abilities/GA_Knockback.h"
#include "AbilitySystem/Abilities/YogTargetType.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/YogAnimNotifyState_Damage.h"
#include "Animation/AnimInstance.h"
#include "Animation/HitStopManager.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Component/MontageVFXBindingComponent.h"
#include "Component/BufferComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/RuneDataAsset.h"
#include "GameFramework/Controller.h"
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
	// Required for combo chaining: when ComboRuntime activates the next node
	// (same UGA_MeleeAttack class), the previous instance is still alive
	// (montage in CanCombo window). Without retrigger, TryActivateAbilityByClass
	// returns false and the chain dies on the root. Matches GA_PlayMontage behavior.
	bRetriggerInstancedAbility = true;

	// Runtime guards that should not depend on Blueprint class defaults.
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	AbilityTags.AddTag(AttackTag);
	ActivationOwnedTags.AddTag(AttackTag);
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.Dead"));
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
	const FGameplayTag HeavyAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk"));
	const FGameplayTag LightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk"));
	const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	const FGameplayTag WeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill"));

	if (AbilityTags.HasTag(HeavyAttackTag))
	{
		return ECardRequiredAction::Heavy;
	}

	if (AbilityTags.HasTag(LightAttackTag) || AbilityTags.HasTag(AttackTag))
	{
		return ECardRequiredAction::Light;
	}

	if (AbilityTags.HasTag(WeaponSkillTag))
	{
		return ECardRequiredAction::Heavy;
	}

	return ECardRequiredAction::Any;
}

bool UGA_MeleeAttack::IsCombatDeckComboFinisher() const
{
	const FGameplayTag LightFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo4"));
	const FGameplayTag HeavyFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"));
	const FGameplayTag AttackFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo4"));
	const FGameplayTag WeaponSkillFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo4"));
	const FGameplayTag DashFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash.Combo4"));
	const FGameplayTag SpecialFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo4"));

	return AbilityTags.HasTagExact(LightFinisherTag)
		|| AbilityTags.HasTagExact(HeavyFinisherTag)
		|| AbilityTags.HasTagExact(AttackFinisherTag)
		|| AbilityTags.HasTagExact(WeaponSkillFinisherTag)
		|| AbilityTags.HasTagExact(DashFinisherTag)
		|| AbilityTags.HasTagExact(SpecialFinisherTag);
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
	Context.ActionSlot = AbilityTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill"), false))
		? ECombatDeckActionSlot::WeaponSkill
		: ECombatDeckActionSlot::Attack;
	Context.ComboIndex = ActiveComboIndex;
	Context.ComboNodeId = NAME_None;
	Context.ComboTags = ActiveComboTags;
	Context.AbilityTag = FGameplayTag();
	Context.WeaponDef = PlayerOwner->EquippedWeaponDef;
	Context.bIsComboFinisher = IsCombatDeckComboFinisher();
	Context.FlowRole = Context.bIsComboFinisher ? ECombatDeckFlowRole::Finisher : ECombatDeckFlowRole::Starter;
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
		// 旧版会两次独立尝试 Light/Heavy；新版只看最近一次 action。
		// 若卡攻击是因为此处早退而 CanCombo tag 未清，这条会出现并跟随玩家"卡住"。
		UE_LOG(LogTemp, Warning,
			TEXT("[Melee][DIAG564] OnCanComboTagChanged EARLY-RETURN — no buffered action since AbilityActivationTime=%.3f Tag=%s NewCount=%d"),
			AbilityActivationTime, *Tag.ToString(), NewCount);
		return;
	}

	bool bActivated = false;
	if (BufferedActionType == EInputCommandType::Attack || BufferedActionType == EInputCommandType::WeaponSkill)
	{
		if (UYogAbilitySystemComponent* PlayerASC = Cast<UYogAbilitySystemComponent>(PlayerOwner->GetASC()))
		{
			const bool bHasActiveComboTag = BufferedActionType == EInputCommandType::Attack
				? PlayerASC->HasActiveAttackComboAbilityTag()
				: PlayerASC->HasActiveWeaponSkillComboAbilityTag();
			bActivated = BufferedActionType == EInputCommandType::Attack
				? PlayerASC->TryActivateNextAttackComboAbility(true, true)
				: PlayerASC->TryActivateNextWeaponSkillComboAbility(true, true);
			if (!bActivated && bHasActiveComboTag)
			{
				ASC = PlayerASC;
				if (Tag.IsValid())
				{
					PlayerASC->SetLooseGameplayTagCount(Tag, 0);
				}
				return;
			}
		}
	}
	if (!bActivated)
	{
		if (UAbilitySystemComponent* PlayerASC = PlayerOwner->GetASC())
		{
			const TCHAR* FallbackTagName = TEXT("PlayerState.AbilityCast.Attack");
			switch (BufferedActionType)
			{
			case EInputCommandType::WeaponSkill:
				FallbackTagName = TEXT("PlayerState.AbilityCast.WeaponSkill");
				break;
			case EInputCommandType::Dash:
				FallbackTagName = TEXT("PlayerState.AbilityCast.Dash");
				break;
			case EInputCommandType::Special:
				FallbackTagName = TEXT("PlayerState.AbilityCast.Special");
				break;
			default:
				break;
			}

			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName(FallbackTagName)));
			bActivated = PlayerASC->TryActivateAbilitiesByTag(TagContainer, true);
		}
	}

	// === DIAG: attack-stuck repro (CL564) ===
	UE_LOG(LogTemp, Verbose,
		TEXT("[Melee][DIAG564] OnCanComboTagChanged consumed=%s HasComboSource=%d Activated=%d Tag=%s"),
		*StaticEnum<EInputCommandType>()->GetNameStringByValue(static_cast<int64>(BufferedActionType)),
		0, bActivated ? 1 : 0, *Tag.ToString());

	if (!bActivated && ASC && Tag.IsValid())
	{
		ASC->SetLooseGameplayTagCount(Tag, 0);
		UE_LOG(LogTemp, Warning,
			TEXT("[Melee][DIAG564] OnCanComboTagChanged ACTIVATION FAILED — cleared %s tag count"),
			*Tag.ToString());
	}
}

void UGA_MeleeAttack::TryStartEnemyRadialLunge()
{
	if (!ActiveEnemyAttackContext.bValid
		|| ActiveEnemyAttackContext.AttackOption.AttackMovementMode != EEnemyAIAttackMovementMode::RadialLunge)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AActor* TargetActor = ActiveEnemyAttackContext.TargetActor.Get();
	if (!Character || !TargetActor)
	{
		return;
	}

	const FEnemyAIAttackOption& AttackOption = ActiveEnemyAttackContext.AttackOption;
	const float CurrentDistance = FVector::Dist2D(Character->GetActorLocation(), TargetActor->GetActorLocation());
	if (CurrentDistance < AttackOption.LungeStartRange
		|| AttackOption.LungeDistance <= 0.0f
		|| AttackOption.LungeDuration <= 0.0f)
	{
		return;
	}

	FVector Direction = TargetActor->GetActorLocation() - Character->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	Direction = Direction.GetSafeNormal();
	const FRotator FaceTargetRotation(0.0f, Direction.Rotation().Yaw, 0.0f);
	Character->SetActorRotation(FaceTargetRotation);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FaceTargetRotation);
	}

	const float MaxUsefulDistance = FMath::Max(CurrentDistance - AttackOption.LungeStopDistance, 0.0f);
	const float MoveDistance = FMath::Min(AttackOption.LungeDistance, MaxUsefulDistance);
	if (MoveDistance <= 0.0f)
	{
		return;
	}

	const FVector TargetLocation = Character->GetActorLocation() + Direction * MoveDistance;
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
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (ASC->GetTagCount(CanComboTag) > 0)
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}

		if (CanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CanComboTag, EGameplayTagEventType::NewOrRemoved);
			CanComboTagHandle.Reset();
		}
		CanComboTagHandle = ASC->RegisterGameplayTagEvent(CanComboTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UGA_MeleeAttack::OnCanComboTagChanged);
	}

	// 鐜╁ GA锛氭鏌ユ秷鑰?鍐峰嵈
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
	if (!ActiveAttackGuid.IsValid())
	{
		ActiveAttackGuid = FGuid::NewGuid();
	}

	UCharacterDataComponent* CDC = ActivateOwner ? ActivateOwner->GetCharacterDataComponent() : nullptr;
	UCharacterData* CD = CDC ? CDC->GetCharacterData() : nullptr;

	FGameplayTag FirstTag;
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
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo4"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"), false),
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"), false),
	};
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
		const UAbilityData* ActiveAbilityData = CD ? CD->AbilityData.Get() : nullptr;
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

	// 鐩戝惉 AnimNotify 浼ゅ浜嬩欢锛氬繀椤绘槑纭紶鍏?Tag锛岀┖瀹瑰櫒涓嶄細娉ㄥ唽浠讳綍鐩戝惉
	// Combat cards resolve from AN_MeleeDamage, so effects and UI consumption
	// happen on the actual attack notify frame instead of montage start.

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

	TryStartEnemyRadialLunge();

	// Resolve OnCommit cards before the montage starts so pre-montage flows
	// (e.g. weapon trail setup) can execute before the first animation frame.
	// OnHit cards are unaffected: they still fire from OnEventReceived via AN_MeleeDamage.
	ResolveCombatDeck(ECombatCardTriggerTiming::OnCommit);

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
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CanComboTag, EGameplayTagEventType::NewOrRemoved);
		CanComboTagHandle.Reset();
	}

	if (ASC)
	{
		ASC->SetLooseGameplayTagCount(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")), 0);
	}

	if (EnemyLungeTask)
	{
		EnemyLungeTask->EndTask();
		EnemyLungeTask = nullptr;
	}

	// 瀹夊叏娓呯悊锛氭妧鑳界粨鏉熸椂娓呯┖鏈秷璐圭殑鏆傚瓨鏁版嵁锛堣挋澶琚墦鏂湭瑙﹀彂 OnEventReceived 鏃朵繚鎶ょ敤锛?
	if (AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo()))
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
		Owner->PendingHitImpactCueTag = FGameplayTag();
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
		if (!bComboContinued && !bCombatDeckFromDashSave && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
			UE_LOG(LogTemp, Warning, TEXT("[CombatDeck] Combo ended without continuation; pending link cleared by %s"), *GetName());
		}

		if (ActiveCombatCardResult.bHadCard && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.ConsumedCard);
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

		static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
		for (AActor* HitActor : HitActors)
		{
			FGameplayEventData Payload;
			Payload.Instigator = Owner;
			Payload.Target     = HitActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitTag, Payload);
		}

		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
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
			PlayerOwner->CombatDeckComponent->StopCardFlow(CardResult.ConsumedCard);
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
		*CombatCardResult.ConsumedCard.Config.DisplayName.ToString(),
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
