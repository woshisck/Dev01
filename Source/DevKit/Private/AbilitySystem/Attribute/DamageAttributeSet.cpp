// Fill out your copyright notice in the Description page of Project Settings.



#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Component/CombatItemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemBlueprintLibrary.h"

namespace
{
	bool EffectGrantsTag(const FGameplayEffectSpec& Spec, const TCHAR* TagName)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		return Tag.IsValid() && Spec.Def && Spec.Def->GetGrantedTags().HasTagExact(Tag);
	}

	bool ShouldSuppressDamageFeedbackForEffect(const FGameplayEffectSpec& Spec)
	{
		return UCombatItemComponent::IsNoHitReactItemDamage(Spec) ||
			EffectGrantsTag(Spec, TEXT("Buff.Status.Burning")) ||
			EffectGrantsTag(Spec, TEXT("Buff.Status.Poisoned")) ||
			EffectGrantsTag(Spec, TEXT("Buff.Status.Bleeding"));
	}

	bool HasDamageAttributeInvulnerableTag(const UAbilitySystemComponent* ASC)
	{
		static const FGameplayTag InvulnerableTag =
			FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Invulnerable"), false);
		return InvulnerableTag.IsValid() && ASC && ASC->HasMatchingGameplayTag(InvulnerableTag);
	}

	UAbilitySystemComponent* GetTargetASC(const FGameplayEffectModCallbackData& Data)
	{
		return Data.Target.AbilityActorInfo.IsValid()
			? Data.Target.AbilityActorInfo->AbilitySystemComponent.Get()
			: nullptr;
	}

	UBaseAttributeSet* GetValidTargetBaseSet(
		const FGameplayEffectModCallbackData& Data,
		AYogCharacterBase* TargetCharacter,
		const TCHAR* DamagePath)
	{
		if (!TargetCharacter || !IsValid(TargetCharacter->BaseAttributeSet))
		{
			UE_LOG(LogTemp, Warning, TEXT("[DamageAttributeSet] %s skipped: Target=%s has no BaseAttributeSet."),
				DamagePath,
				*GetNameSafe(TargetCharacter));
			return nullptr;
		}

		UAbilitySystemComponent* TargetASC = GetTargetASC(Data);
		if (TargetASC &&
			(!TargetASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute()) ||
			 !TargetASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute())))
		{
			UE_LOG(LogTemp, Warning, TEXT("[DamageAttributeSet] %s skipped: Target=%s ASC is missing Health/MaxHealth attributes."),
				DamagePath,
				*GetNameSafe(TargetCharacter));
			return nullptr;
		}

		return TargetCharacter->BaseAttributeSet;
	}

	struct FDamageAbsorptionResult
	{
		float HealthDamage = 0.0f;
		float ArmorDamage = 0.0f;
		float ShieldDamage = 0.0f;
	};

	void BroadcastDamageValues(AYogCharacterBase* TargetCharacter, const FDamageAbsorptionResult& Result)
	{
		if (!TargetCharacter)
		{
			return;
		}

		if (Result.ArmorDamage > 0.0f)
		{
			TargetCharacter->OnCharacterDamageValue.Broadcast(Result.ArmorDamage, EYogDamageValueType::Armor);
		}

		const float RedDamage = Result.HealthDamage + Result.ShieldDamage;
		if (RedDamage > 0.0f)
		{
			TargetCharacter->OnCharacterDamageValue.Broadcast(RedDamage, EYogDamageValueType::Health);
		}
	}

	void RemoveShieldGameplayEffects(UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return;
		}

		FGameplayTagContainer ShieldStatusTags;
		const FGameplayTag ShieldedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Shielded"), false);
		if (ShieldedTag.IsValid())
		{
			ShieldStatusTags.AddTag(ShieldedTag);
		}

		if (!ShieldStatusTags.IsEmpty())
		{
			ASC->RemoveActiveEffectsWithGrantedTags(ShieldStatusTags);
		}

		FGameplayTagContainer ShieldEffectTags;
		const FGameplayTag ShieldEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Effect.Shield"), false);
		if (ShieldEffectTag.IsValid())
		{
			ShieldEffectTags.AddTag(ShieldEffectTag);
		}

		if (!ShieldEffectTags.IsEmpty())
		{
			ASC->RemoveActiveEffectsWithTags(ShieldEffectTags);
		}
	}

	FDamageAbsorptionResult AbsorbDamageWithShieldAndArmor(
		const FGameplayEffectModCallbackData& Data,
		AYogCharacterBase* TargetCharacter,
		float DamageAmount,
		bool bAllowArmorAbsorption)
	{
		if (DamageAmount <= 0.f || !TargetCharacter || !TargetCharacter->BaseAttributeSet)
		{
			return { FMath::Max(0.0f, DamageAmount), 0.0f, 0.0f };
		}

		UAbilitySystemComponent* TargetASC = GetTargetASC(Data);
		UBaseAttributeSet* BaseSet = TargetCharacter->BaseAttributeSet;

		float RemainingDamage = DamageAmount;
		float ArmorDamage = 0.0f;
		float ShieldDamage = 0.0f;

		const float CurrentShield = FMath::Max(0.f, BaseSet->GetShield());
		if (CurrentShield > 0.f)
		{
			const float ShieldAbsorbed = FMath::Min(RemainingDamage, CurrentShield);
			if (TargetASC)
			{
				// Drain by changing the base value so active Infinite/Additive shield GEs are consumed too.
				TargetASC->ApplyModToAttributeUnsafe(
					UBaseAttributeSet::GetShieldAttribute(),
					EGameplayModOp::Additive,
					-ShieldAbsorbed);
			}
			else
			{
				BaseSet->SetShield(FMath::Max(0.f, CurrentShield - ShieldAbsorbed));
			}

			RemainingDamage -= ShieldAbsorbed;
			ShieldDamage += ShieldAbsorbed;
			const float NewShield = TargetASC
				? FMath::Max(0.f, TargetASC->GetNumericAttribute(UBaseAttributeSet::GetShieldAttribute()))
				: FMath::Max(0.f, BaseSet->GetShield());

			UE_LOG(LogTemp, Warning, TEXT("[Shield] Absorb Target=%s Damage=%.1f Shield %.1f -> %.1f Remaining=%.1f"),
				*GetNameSafe(TargetCharacter),
				DamageAmount,
				CurrentShield,
				NewShield,
				RemainingDamage);

			if (NewShield <= KINDA_SMALL_NUMBER)
			{
				RemoveShieldGameplayEffects(TargetASC);
				if (TargetASC)
				{
					TargetASC->SetNumericAttributeBase(UBaseAttributeSet::GetShieldAttribute(), 0.f);
				}
				else
				{
					BaseSet->SetShield(0.f);
				}
			}

			if (RemainingDamage <= 0.f)
			{
				return { 0.0f, 0.0f, ShieldDamage };
			}
		}

		if (bAllowArmorAbsorption)
		{
			const float CurrentArmor = BaseSet->GetArmorHP();
			if (CurrentArmor > 0.f)
			{
				const float ArmorAbsorbed = FMath::Min(RemainingDamage, CurrentArmor);
				const float NewArmor = FMath::Max(0.f, CurrentArmor - ArmorAbsorbed);
				if (TargetASC)
				{
					TargetASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), NewArmor);
				}
				else
				{
					BaseSet->SetArmorHP(NewArmor);
				}
				RemainingDamage -= ArmorAbsorbed;
				ArmorDamage += ArmorAbsorbed;
				UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][Armor] Absorb Target=%s Damage=%.1f Armor %.1f -> %.1f HealthDamage=%.1f"),
					*GetNameSafe(TargetCharacter),
					DamageAmount,
					CurrentArmor,
					NewArmor,
					RemainingDamage);
			}
		}

		return { FMath::Max(0.f, RemainingDamage), ArmorDamage, ShieldDamage };
	}
}




UDamageAttributeSet::UDamageAttributeSet()
{	
	InitDamagePhysical(0);
	InitDamageMagic(0);
	InitDamagePure(0);
	InitDamageBuff(0);
}

void UDamageAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();

	// Compute the delta between old and new, if it is available
	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AYogCharacterBase* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AYogCharacterBase>(TargetActor);
	}

	if (Data.EvaluatedData.Attribute == GetDamagePureAttribute())
	{
		AActor* SourceActor = nullptr;
		AController* SourceController = nullptr;
		AYogCharacterBase* SourceCharacter = nullptr;

		if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
		{
			SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
			SourceController = Source->AbilityActorInfo->PlayerController.Get();
			if (SourceController == nullptr && SourceActor != nullptr)
			{
				if (APawn* Pawn = Cast<APawn>(SourceActor))
				{
					SourceController = Pawn->GetController();
				}
			}

			// Use the controller to find the source pawn
			if (SourceController)
			{
				SourceCharacter = Cast<AYogCharacterBase>(SourceController->GetPawn());
			}
			else
			{
				SourceCharacter = Cast<AYogCharacterBase>(SourceActor);
			}

			// Set the causer actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Context.GetEffectCauser();
			}
		}

		//Pure Damage deal in health
		const float LocalDamageDone = GetDamagePure();
		SetDamagePure(0.f);

		// 无敌帧：目标冲刺期间持有 Buff.Status.DashInvincible，跳过所有伤害
		{
			static const FGameplayTag TAG_DashInvincible =
				FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.DashInvincible"));
			UAbilitySystemComponent* TargetASC =
				Data.Target.AbilityActorInfo->AbilitySystemComponent.Get();
			if (HasDamageAttributeInvulnerableTag(TargetASC) || (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible)))
			{
				return;
			}
		}

		if (LocalDamageDone > 0)
		{
			UBaseAttributeSet* TargetBaseSet = GetValidTargetBaseSet(Data, TargetCharacter, TEXT("DamagePure"));
			if (!TargetBaseSet)
			{
				return;
			}

			// ── 护甲拦截（护甲先吸收，溢出才扣血）─────────────────────────
			const FDamageAbsorptionResult DamageResult = AbsorbDamageWithShieldAndArmor(Data, TargetCharacter, LocalDamageDone, true);
			const float HealthDamage = DamageResult.HealthDamage;

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			const bool bSuppressDamageFeedback = ShouldSuppressDamageFeedbackForEffect(Data.EffectSpec);
			if (HealthDamage > 0.f && bSuppressDamageFeedback && ASC)
			{
				ASC->SuppressNextDamageFeedback();
			}

			if (HealthDamage > 0.f)
			{
				const float OldHealth = TargetBaseSet->GetHealth();
				TargetBaseSet->SetHealth(FMath::Clamp(OldHealth - HealthDamage, 0.0f, TargetBaseSet->GetMaxHealth()));
			}

			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone, bSuppressDamageFeedback, Context);
				UCombatItemComponent::TryApplyOilFireBonus(SourceYogASC, ASC, Data.EffectSpec);
				const float MaxHealth = TargetBaseSet->GetMaxHealth();
				float percent = MaxHealth > 0.f ? TargetBaseSet->GetHealth() / MaxHealth : 0.f;
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent, LocalDamageDone);
				BroadcastDamageValues(TargetCharacter, DamageResult);

				// 广播 Ability.Event.Damaged 给受击目标（GA_Wound 等监听此事件）
				{
					static const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"), false);
					if (DamagedTag.IsValid() && SourceActor)
					{
						FGameplayEventData DamagedPayload;
						DamagedPayload.Instigator     = SourceActor;
						DamagedPayload.Target         = TargetCharacter;
						DamagedPayload.EventMagnitude = LocalDamageDone;
						DamagedPayload.ContextHandle  = Context;
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, DamagedTag, DamagedPayload);
					}
				}

				// 击杀广播
				if (TargetBaseSet->GetHealth() <= 0.f && SourceYogASC)
				{
					SourceYogASC->OnKilledTarget.Broadcast(TargetCharacter, TargetCharacter->GetActorLocation());

					static const FGameplayTag KillTag  = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Kill"));
					static const FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Death"));
					AActor* KillerActor = SourceYogASC->GetAvatarActor();
					if (KillerActor)
					{
						FGameplayEventData KillPayload;
						KillPayload.Instigator = KillerActor;
						KillPayload.Target     = TargetCharacter;
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(KillerActor, KillTag, KillPayload);
					}
					{
						FGameplayEventData DeathPayload;
						DeathPayload.Instigator = KillerActor;
						DeathPayload.Target     = TargetCharacter;
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, DeathTag, DeathPayload);
					}
				}
			}
		}
	}



	if (Data.EvaluatedData.Attribute == GetDamagePhysicalAttribute())
	{
		AActor* SourceActor = nullptr;
		AController* SourceController = nullptr;
		AYogCharacterBase* SourceCharacter = nullptr;

		if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
		{
			SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
			SourceController = Source->AbilityActorInfo->PlayerController.Get();
			if (SourceController == nullptr && SourceActor != nullptr)
			{
				if (APawn* Pawn = Cast<APawn>(SourceActor))
				{
					SourceController = Pawn->GetController();
				}
			}

			// Use the controller to find the source pawn
			if (SourceController)
			{
				SourceCharacter = Cast<AYogCharacterBase>(SourceController->GetPawn());
			}
			else
			{
				SourceCharacter = Cast<AYogCharacterBase>(SourceActor);
			}

			// Set the causer actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Context.GetEffectCauser();
			}
		}

		//Physical Damage deal in health
		const float LocalDamageDone = GetDamagePhysical();
		SetDamagePhysical(0.f);

		UAbilitySystemComponent* TargetASCForBlock =
			Data.Target.AbilityActorInfo->AbilitySystemComponent.Get();
		static const FGameplayTag TAG_BlockStart =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Start"), false);
		static const FGameplayTag TAG_BlockIdle =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Idle"), false);
		const bool bJustBlocked = TargetASCForBlock && TAG_BlockStart.IsValid() && TargetASCForBlock->HasMatchingGameplayTag(TAG_BlockStart);
		const bool bBlocked = bJustBlocked || (TargetASCForBlock && TAG_BlockIdle.IsValid() && TargetASCForBlock->HasMatchingGameplayTag(TAG_BlockIdle));
		if (LocalDamageDone > 0.f && bBlocked)
		{
			static const FGameplayTag BlockedReactTag =
				FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Blocked"), false);
			static const FGameplayTag ParriedReactTag =
				FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Parried"), false);
			static const FGameplayTag BlockedEventTag =
				FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.Blocked"), false);
			static const FGameplayTag JustBlockedEventTag =
				FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.JustBlocked"), false);

			if (TargetActor && BlockedReactTag.IsValid())
			{
				FGameplayEventData BlockPayload;
				BlockPayload.EventTag = BlockedReactTag;
				BlockPayload.Instigator = SourceActor;
				BlockPayload.Target = TargetCharacter;
				BlockPayload.EventMagnitude = LocalDamageDone;
				BlockPayload.ContextHandle = Context;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, BlockedReactTag, BlockPayload);
			}

			if (TargetActor)
			{
				const FGameplayTag NotifyTag = bJustBlocked ? JustBlockedEventTag : BlockedEventTag;
				if (NotifyTag.IsValid())
				{
					FGameplayEventData NotifyPayload;
					NotifyPayload.EventTag = NotifyTag;
					NotifyPayload.Instigator = SourceActor;
					NotifyPayload.Target = TargetCharacter;
					NotifyPayload.EventMagnitude = LocalDamageDone;
					NotifyPayload.ContextHandle = Context;
					UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, NotifyTag, NotifyPayload);
				}
			}

			if (bJustBlocked && SourceActor && ParriedReactTag.IsValid())
			{
				FGameplayEventData ParryPayload;
				ParryPayload.EventTag = ParriedReactTag;
				ParryPayload.Instigator = TargetActor;
				ParryPayload.Target = SourceActor;
				ParryPayload.EventMagnitude = LocalDamageDone;
				ParryPayload.ContextHandle = Context;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceActor, ParriedReactTag, ParryPayload);
			}

			UE_LOG(LogTemp, Warning, TEXT("[Block] %s Target=%s Source=%s Damage=%.1f"),
				bJustBlocked ? TEXT("JustBlock") : TEXT("Block"),
				*GetNameSafe(TargetActor),
				*GetNameSafe(SourceActor),
				LocalDamageDone);
			return;
		}

		// 无敌帧：目标冲刺期间持有 Buff.Status.DashInvincible，跳过所有伤害
		{
			static const FGameplayTag TAG_DashInvincible =
				FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.DashInvincible"));
			UAbilitySystemComponent* TargetASC =
				Data.Target.AbilityActorInfo->AbilitySystemComponent.Get();
			if (HasDamageAttributeInvulnerableTag(TargetASC) || (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible)))
			{
				return;
			}
		}

		if (LocalDamageDone > 0)
		{
			UBaseAttributeSet* TargetBaseSet = GetValidTargetBaseSet(Data, TargetCharacter, TEXT("DamagePhysical"));
			if (!TargetBaseSet)
			{
				return;
			}

			// ── 护甲拦截 ─────────────────────────────────────────────────────
			const FDamageAbsorptionResult DamageResult = AbsorbDamageWithShieldAndArmor(Data, TargetCharacter, LocalDamageDone, true);
			const float HealthDamage = DamageResult.HealthDamage;

			if (HealthDamage > 0.f)
			{
				const float OldHealth = TargetBaseSet->GetHealth();
				TargetBaseSet->SetHealth(FMath::Clamp(OldHealth - HealthDamage, 0.0f, TargetBaseSet->GetMaxHealth()));
			}

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone, false, Context);
				UCombatItemComponent::TryApplyOilFireBonus(SourceYogASC, ASC, Data.EffectSpec);
				const float MaxHealth = TargetBaseSet->GetMaxHealth();
				float percent = MaxHealth > 0.f ? TargetBaseSet->GetHealth() / MaxHealth : 0.f;
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent, LocalDamageDone);
				BroadcastDamageValues(TargetCharacter, DamageResult);

				// 广播 Ability.Event.Damaged（不在 DamageBuff 路径广播，防止 GA_Wound 递归）
				{
					static const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"), false);
					if (DamagedTag.IsValid() && SourceActor)
					{
						FGameplayEventData DamagedPayload;
						DamagedPayload.Instigator     = SourceActor;
						DamagedPayload.Target         = TargetCharacter;
						DamagedPayload.EventMagnitude = LocalDamageDone;
						DamagedPayload.ContextHandle  = Context;
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, DamagedTag, DamagedPayload);
					}
				}

				// 击杀广播
				if (TargetBaseSet->GetHealth() <= 0.f && SourceYogASC)
				{
					SourceYogASC->OnKilledTarget.Broadcast(TargetCharacter, TargetCharacter->GetActorLocation());

					static const FGameplayTag KillTag  = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Kill"));
					static const FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Death"));
					AActor* KillerActor = SourceYogASC->GetAvatarActor();
					if (KillerActor)
					{
						FGameplayEventData KillPayload;
						KillPayload.Instigator = KillerActor;
						KillPayload.Target     = TargetCharacter;
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(KillerActor, KillTag, KillPayload);
					}
					{
						FGameplayEventData DeathPayload;
						DeathPayload.Instigator = KillerActor;
						DeathPayload.Target     = TargetCharacter;
						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetCharacter, DeathTag, DeathPayload);
					}
				}
			}
		}
	}

	// ── DamageBuff：状态效果伤害，绕过护甲，不能击杀（中毒类），不重播 Damaged 事件 ──
	if (Data.EvaluatedData.Attribute == GetDamageBuffAttribute())
	{
		const float LocalDamageDone = GetDamageBuff();
		SetDamageBuff(0.f);

		static const FGameplayTag TAG_DashInvincible =
			FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.DashInvincible"), false);
		UAbilitySystemComponent* TargetASC =
			Data.Target.AbilityActorInfo->AbilitySystemComponent.Get();
		if (HasDamageAttributeInvulnerableTag(TargetASC) ||
			(TargetASC && TAG_DashInvincible.IsValid() && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible)))
		{
			return;
		}

		if (LocalDamageDone > 0.f)
		{
			UBaseAttributeSet* TargetBaseSet = GetValidTargetBaseSet(Data, TargetCharacter, TEXT("DamageBuff"));
			if (!TargetBaseSet)
			{
				return;
			}

			const FDamageAbsorptionResult DamageResult = AbsorbDamageWithShieldAndArmor(Data, TargetCharacter, LocalDamageDone, false);
			const float HealthDamage = DamageResult.HealthDamage;
			if (HealthDamage <= 0.f)
			{
				return;
			}

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			if (ASC)
			{
				ASC->SuppressNextDamageFeedback();
			}

			const float OldHealth = TargetBaseSet->GetHealth();
			// 不至死：Health 最低保留 1（GEExec_PoisonDamage 在输出前已Clamp，此处双重保险）
			const float NewHealth = FMath::Max(1.f, OldHealth - HealthDamage);
			TargetBaseSet->SetHealth(NewHealth);

			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone, true, Context);
				const float MaxHealth = TargetBaseSet->GetMaxHealth();
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(MaxHealth > 0.f ? NewHealth / MaxHealth : 0.f, LocalDamageDone);
				BroadcastDamageValues(TargetCharacter, DamageResult);
			}
		}
	}
}



