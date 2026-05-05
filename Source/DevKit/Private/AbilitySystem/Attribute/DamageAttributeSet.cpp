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
			if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible))
			{
				return;
			}
		}

		if (LocalDamageDone > 0)
		{
			// ── 护甲拦截（护甲先吸收，溢出才扣血）─────────────────────────
			float HealthDamage = LocalDamageDone;
			if (TargetCharacter->BaseAttributeSet)
			{
				const float CurrentArmor = TargetCharacter->BaseAttributeSet->GetArmorHP();
				if (CurrentArmor > 0.f)
				{
					const float ArmorAbsorbed = FMath::Min(LocalDamageDone, CurrentArmor);
					const float NewArmor = FMath::Max(0.f, CurrentArmor - ArmorAbsorbed);
					if (UAbilitySystemComponent* ArmorASC = Data.Target.AbilityActorInfo->AbilitySystemComponent.Get())
					{
						ArmorASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), NewArmor);
					}
					else
					{
						TargetCharacter->BaseAttributeSet->SetArmorHP(NewArmor);
					}
					HealthDamage = LocalDamageDone - ArmorAbsorbed;
					UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][Armor] Absorb Target=%s Damage=%.1f Armor %.1f -> %.1f HealthDamage=%.1f"),
						*GetNameSafe(TargetCharacter),
						LocalDamageDone,
						CurrentArmor,
						NewArmor,
						HealthDamage);
				}
			}

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			const bool bSuppressDamageFeedback = ShouldSuppressDamageFeedbackForEffect(Data.EffectSpec);
			if (HealthDamage > 0.f && bSuppressDamageFeedback && ASC)
			{
				ASC->SuppressNextDamageFeedback();
			}

			if (HealthDamage > 0.f)
			{
				const float OldHealth = TargetCharacter->BaseAttributeSet->GetHealth();
				TargetCharacter->BaseAttributeSet->SetHealth(FMath::Clamp(OldHealth - HealthDamage, 0.0f, TargetCharacter->BaseAttributeSet->GetMaxHealth()));
			}

			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone, bSuppressDamageFeedback);
				UCombatItemComponent::TryApplyOilFireBonus(SourceYogASC, ASC, Data.EffectSpec);
				float percent = TargetCharacter->BaseAttributeSet->GetHealth() / TargetCharacter->BaseAttributeSet->GetMaxHealth();
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
				if (TargetCharacter->BaseAttributeSet->GetHealth() <= 0.f && SourceYogASC)
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
			if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible))
			{
				return;
			}
		}

		if (LocalDamageDone > 0)
		{
			// ── 护甲拦截 ─────────────────────────────────────────────────────
			float HealthDamage = LocalDamageDone;
			if (TargetCharacter->BaseAttributeSet)
			{
				const float CurrentArmor = TargetCharacter->BaseAttributeSet->GetArmorHP();
				if (CurrentArmor > 0.f)
				{
					const float ArmorAbsorbed = FMath::Min(LocalDamageDone, CurrentArmor);
					const float NewArmor = FMath::Max(0.f, CurrentArmor - ArmorAbsorbed);
					if (UAbilitySystemComponent* ArmorASC = Data.Target.AbilityActorInfo->AbilitySystemComponent.Get())
					{
						ArmorASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), NewArmor);
					}
					else
					{
						TargetCharacter->BaseAttributeSet->SetArmorHP(NewArmor);
					}
					HealthDamage = LocalDamageDone - ArmorAbsorbed;
					UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][Armor] Absorb Target=%s Damage=%.1f Armor %.1f -> %.1f HealthDamage=%.1f"),
						*GetNameSafe(TargetCharacter),
						LocalDamageDone,
						CurrentArmor,
						NewArmor,
						HealthDamage);
				}
			}

			if (HealthDamage > 0.f)
			{
				const float OldHealth = TargetCharacter->BaseAttributeSet->GetHealth();
				TargetCharacter->BaseAttributeSet->SetHealth(FMath::Clamp(OldHealth - HealthDamage, 0.0f, TargetCharacter->BaseAttributeSet->GetMaxHealth()));
			}

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone);
				UCombatItemComponent::TryApplyOilFireBonus(SourceYogASC, ASC, Data.EffectSpec);
				float percent = TargetCharacter->BaseAttributeSet->GetHealth() / TargetCharacter->BaseAttributeSet->GetMaxHealth();
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
				if (TargetCharacter->BaseAttributeSet->GetHealth() <= 0.f && SourceYogASC)
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
		if (TargetASC && TAG_DashInvincible.IsValid() && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible))
		{
			return;
		}

		if (LocalDamageDone > 0.f && TargetCharacter && TargetCharacter->BaseAttributeSet)
		{
			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			if (ASC)
			{
				ASC->SuppressNextDamageFeedback();
			}

			const float OldHealth = TargetCharacter->BaseAttributeSet->GetHealth();
			// 不至死：Health 最低保留 1（GEExec_PoisonDamage 在输出前已Clamp，此处双重保险）
			const float NewHealth = FMath::Max(1.f, OldHealth - LocalDamageDone);
			TargetCharacter->BaseAttributeSet->SetHealth(NewHealth);

			if (ASC)
			{
				UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
				ASC->ReceiveDamage(SourceYogASC, LocalDamageDone, true);
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(NewHealth / TargetCharacter->BaseAttributeSet->GetMaxHealth(), LocalDamageDone);
			}
		}
	}
}



