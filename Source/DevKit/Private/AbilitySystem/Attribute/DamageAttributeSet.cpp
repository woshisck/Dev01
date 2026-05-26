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

	void LogTargetHealthTrace(
		const FGameplayEffectModCallbackData& Data,
		AYogCharacterBase* TargetCharacter,
		const TCHAR* DamagePath,
		float LocalDamageDone,
		float HealthDamage,
		const UBaseAttributeSet* TargetBaseSet,
		float OldHealth,
		float OldMaxHealth,
		float NewHealth)
	{
		UAbilitySystemComponent* TargetASC = GetTargetASC(Data);
		const UBaseAttributeSet* RegisteredBaseSet = TargetASC
			? Cast<const UBaseAttributeSet>(TargetASC->GetAttributeSet(UBaseAttributeSet::StaticClass()))
			: nullptr;
		const bool bHasHealthAttribute = TargetASC && TargetASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute());
		const bool bHasMaxHealthAttribute = TargetASC && TargetASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		const float ASCHealth = bHasHealthAttribute
			? TargetASC->GetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute())
			: -1.0f;
		const float ASCMaxHealth = bHasMaxHealthAttribute
			? TargetASC->GetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute())
			: -1.0f;

		UE_LOG(LogTemp, Warning,
			TEXT("[DamageHealthTrace][%s] Target=%s Damage=%.2f HealthDamage=%.2f CharBase=%s(%p) ASCBase=%s(%p) SameBase=%d Old=%.2f Max=%.2f New=%.2f ASCHealth=%.2f ASCMax=%.2f Dead=%d"),
			DamagePath,
			*GetNameSafe(TargetCharacter),
			LocalDamageDone,
			HealthDamage,
			*GetNameSafe(TargetBaseSet),
			static_cast<const void*>(TargetBaseSet),
			*GetNameSafe(RegisteredBaseSet),
			static_cast<const void*>(RegisteredBaseSet),
			TargetBaseSet == RegisteredBaseSet ? 1 : 0,
			OldHealth,
			OldMaxHealth,
			NewHealth,
			ASCHealth,
			ASCMaxHealth,
			TargetCharacter && TargetCharacter->bIsDead ? 1 : 0);
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

	float AbsorbDamageWithShieldAndArmor(
		const FGameplayEffectModCallbackData& Data,
		AYogCharacterBase* TargetCharacter,
		float DamageAmount,
		bool bAllowArmorAbsorption)
	{
		if (DamageAmount <= 0.f || !TargetCharacter || !TargetCharacter->BaseAttributeSet)
		{
			return DamageAmount;
		}

		UAbilitySystemComponent* TargetASC = GetTargetASC(Data);
		UBaseAttributeSet* BaseSet = TargetCharacter->BaseAttributeSet;

		float RemainingDamage = DamageAmount;

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
				return 0.f;
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
				UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][Armor] Absorb Target=%s Damage=%.1f Armor %.1f -> %.1f HealthDamage=%.1f"),
					*GetNameSafe(TargetCharacter),
					DamageAmount,
					CurrentArmor,
					NewArmor,
					RemainingDamage);
			}
		}

		return FMath::Max(0.f, RemainingDamage);
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
			const float HealthDamage = AbsorbDamageWithShieldAndArmor(Data, TargetCharacter, LocalDamageDone, true);

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			const bool bSuppressDamageFeedback = ShouldSuppressDamageFeedbackForEffect(Data.EffectSpec);
			if (HealthDamage > 0.f && bSuppressDamageFeedback && ASC)
			{
				ASC->SuppressNextDamageFeedback();
			}

			if (HealthDamage > 0.f)
			{
				const float OldHealth = TargetBaseSet->GetHealth();
				const float OldMaxHealth = TargetBaseSet->GetMaxHealth();
				const float NewHealth = FMath::Clamp(OldHealth - HealthDamage, 0.0f, OldMaxHealth);
				LogTargetHealthTrace(Data, TargetCharacter, TEXT("DamagePure"), LocalDamageDone, HealthDamage, TargetBaseSet, OldHealth, OldMaxHealth, NewHealth);
				TargetBaseSet->SetHealth(NewHealth);
			}

			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone, bSuppressDamageFeedback);
				UCombatItemComponent::TryApplyOilFireBonus(SourceYogASC, ASC, Data.EffectSpec);
				const float MaxHealth = TargetBaseSet->GetMaxHealth();
				float percent = MaxHealth > 0.f ? TargetBaseSet->GetHealth() / MaxHealth : 0.f;
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent, LocalDamageDone);

				// 广播 Ability.Event.Damaged 给受击目标（GA_Wound 等监听此事件）
				{
					static const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"), false);
					if (DamagedTag.IsValid() && SourceActor)
					{
						FGameplayEventData DamagedPayload;
						DamagedPayload.Instigator     = SourceActor;
						DamagedPayload.Target         = TargetCharacter;
						DamagedPayload.EventMagnitude = LocalDamageDone;
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
			const float HealthDamage = AbsorbDamageWithShieldAndArmor(Data, TargetCharacter, LocalDamageDone, true);

			if (HealthDamage > 0.f)
			{
				const float OldHealth = TargetBaseSet->GetHealth();
				const float OldMaxHealth = TargetBaseSet->GetMaxHealth();
				const float NewHealth = FMath::Clamp(OldHealth - HealthDamage, 0.0f, OldMaxHealth);
				LogTargetHealthTrace(Data, TargetCharacter, TEXT("DamagePhysical"), LocalDamageDone, HealthDamage, TargetBaseSet, OldHealth, OldMaxHealth, NewHealth);
				TargetBaseSet->SetHealth(NewHealth);
			}

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone);
				UCombatItemComponent::TryApplyOilFireBonus(SourceYogASC, ASC, Data.EffectSpec);
				const float MaxHealth = TargetBaseSet->GetMaxHealth();
				float percent = MaxHealth > 0.f ? TargetBaseSet->GetHealth() / MaxHealth : 0.f;
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent, LocalDamageDone);

				// 广播 Ability.Event.Damaged（不在 DamageBuff 路径广播，防止 GA_Wound 递归）
				{
					static const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"), false);
					if (DamagedTag.IsValid() && SourceActor)
					{
						FGameplayEventData DamagedPayload;
						DamagedPayload.Instigator     = SourceActor;
						DamagedPayload.Target         = TargetCharacter;
						DamagedPayload.EventMagnitude = LocalDamageDone;
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

			const float HealthDamage = AbsorbDamageWithShieldAndArmor(Data, TargetCharacter, LocalDamageDone, false);
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
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone, true);
				const float MaxHealth = TargetBaseSet->GetMaxHealth();
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(MaxHealth > 0.f ? NewHealth / MaxHealth : 0.f, LocalDamageDone);
			}
		}
	}
}



