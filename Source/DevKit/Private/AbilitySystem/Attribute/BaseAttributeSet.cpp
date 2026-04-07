// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/BackpackGridComponent.h"
#include "GameFramework/CharacterMovementComponent.h"



UBaseAttributeSet::UBaseAttributeSet()
{	

}


void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Attack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Heat, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHeat, COND_None, REPNOTIFY_Always);


	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Sanity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Dodge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Resilience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Resist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Crit_Rate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Crit_Damage, COND_None, REPNOTIFY_Always);

}

UYogAbilitySystemComponent* UBaseAttributeSet::GetASC() const
{
	return Cast<UYogAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}

void UBaseAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const
{
	Super::PreAttributeBaseChange(attribute, newValue);

}

void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}

	if (Attribute == GetHeatAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHeat());
	}

}

bool UBaseAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
		return false;

	if (Data.EvaluatedData.Attribute == GetHeatAttribute())
	{
		CachedPreEffectHeat = GetHeat();

		// 热度减少时，若持有 Buff.Status.Heat.Active 则阻断（战斗状态保热）
		// GE 侧的 Ongoing Tag Requirements 在 UE5.4 周期性 GE 上不可靠，改用 C++ 保证
		if (Data.EvaluatedData.Magnitude < 0.f)
		{
			static const FGameplayTag HeatActiveTag =
				FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Active"), false);
			if (HeatActiveTag.IsValid() &&
				GetOwningAbilitySystemComponent()->HasMatchingGameplayTag(HeatActiveTag))
			{
				return false;
			}
		}
	}
	return true;
}

void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();


	//// Compute the delta between old and new, if it is available
	// DeltaValue = 0;
	//if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	//{
	//	// If this was additive, store the raw delta value to be passed along later
	//	DeltaValue = Data.EvaluatedData.Magnitude;
	//}


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

	if (Data.EvaluatedData.Attribute == GetHeatAttribute())
	{
		float NewHeat = GetHeat();

		// 先判断升阶条件：热度在本次 GE 施加【之前】就已满，且本次是 LastHit
		// 若热度不足就先自然堆叠到上限，等下一次 LastHit 再升阶
		const bool bWasAlreadyFull = (CachedPreEffectHeat >= GetMaxHeat());

		if (NewHeat >= GetMaxHeat())
		{
			static const FGameplayTag CanPhaseUpTag =
				FGameplayTag::RequestGameplayTag(TEXT("Action.Heat.CanPhaseUp"), false);

			const bool bHasTagInAsset   = Data.EffectSpec.Def && Data.EffectSpec.Def->GetAssetTags().HasTag(CanPhaseUpTag);
			const bool bHasTagInDynamic = Data.EffectSpec.GetDynamicAssetTags().HasTag(CanPhaseUpTag);
			const bool bCanPhaseUp      = CanPhaseUpTag.IsValid() && (bHasTagInAsset || bHasTagInDynamic);

			if (bCanPhaseUp && bWasAlreadyFull)
			{
				// 热度已满 + LastHit → 升阶
				APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningActor());
				if (Player)
				{
					UBackpackGridComponent* BGC = Player->GetBackpackGridComponent();
					if (BGC)
					{
						BGC->OnPhaseUpReady.Broadcast();
					}
				}
				SetHeat(0.f);
			}
			else
			{
				// 热度不足（LastHit 打过来但还没满）或普通攻击：卡在上限
				SetHeat(GetMaxHeat());
			}
		}
		else
		{
			SetHeat(FMath::Clamp(NewHeat, 0.f, GetMaxHeat()));
		}
	}

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		float NewHealth = FMath::Clamp(GetHealth(), 0.f, GetMaxHealth());
		SetHealth(NewHealth);

		// ── 击杀广播 ──────────────────────────────────────────────
		if (NewHealth <= 0.f && TargetActor)
		{
			UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(Source);
			if (SourceYogASC)
			{
				SourceYogASC->OnKilledTarget.Broadcast(TargetActor, TargetActor->GetActorLocation());
			}
		}

	}

	//if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	//{
	//	AActor* SourceActor = nullptr;
	//	AController* SourceController = nullptr;
	//	AYogCharacterBase* SourceCharacter = nullptr;

	//	if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
	//	{
	//		SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
	//		SourceController = Source->AbilityActorInfo->PlayerController.Get();
	//		if (SourceController == nullptr && SourceActor != nullptr)
	//		{
	//			if (APawn* Pawn = Cast<APawn>(SourceActor))
	//			{
	//				SourceController = Pawn->GetController();
	//			}
	//		}

	//		// Use the controller to find the source pawn
	//		if (SourceController)
	//		{
	//			SourceCharacter = Cast<AYogCharacterBase>(SourceController->GetPawn());
	//		}
	//		else
	//		{
	//			SourceCharacter = Cast<AYogCharacterBase>(SourceActor);
	//		}
	//		// Set the causer actor based on context if it's set
	//		if (Context.GetEffectCauser())
	//		{
	//			SourceActor = Context.GetEffectCauser();
	//		}
	//	}


	//	// IF HOLY DMAGE \
	//	const  LocalDamageDone = GetDamage(); + HOLY DAMAGE;
	//	const  LocalDamageDone = GetDamage();
	//	SetDamage(0.f);

	//	if (LocalDamageDone > 0)
	//	{
	//		// Apply the health change and then clamp it
	//		const  OldHealth = GetHealth();
	//		SetHealth(FMath::Clamp(OldHealth - LocalDamageDone, 0.0f, GetMaxHealth()));
	//		UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
	//		if (ASC)
	//		{
	//			//UYogAbilitySystemComponent* SourceASC,  Damage
	//			ASC->ReceiveDamage(ASC, GetDamage());
	//			 percent = GetHealth() / GetMaxHealth();
	//			TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent);
	//			// This is proper damage
	//		}
	//	}
	//}

}

void UBaseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetHeatAttribute())
	{
		// 通知背包组件更新热度等级（传绝对值）
		if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningActor()))
		{
			if (UBackpackGridComponent* BGC = Player->GetBackpackGridComponent())
			{
				BGC->OnHeatValueChanged(NewValue);
			}
		}
	}

	// MoveSpeed 属性变化时同步到角色移动组件
	if (Attribute == GetMoveSpeedAttribute())
	{
		if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwningActor()))
		{
			if (UCharacterMovementComponent* MoveComp = OwnerChar->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = NewValue;
			}
		}
	}

}

void UBaseAttributeSet::Init(UCharacterData* data)
{
	const FYogBaseAttributeData* YogBaseData = data->GetBaseAttributeData();

	SetAttack(YogBaseData->Attack);
	SetAttackPower(YogBaseData->AttackPower);
	SetMaxHealth(YogBaseData->MaxHealth);
	SetMaxHeat(YogBaseData->MaxHeat);
	SetShield(YogBaseData->Shield);
	SetAttackSpeed(YogBaseData->AttackSpeed);
	SetAttackRange(YogBaseData->AttackRange);
	SetSanity(YogBaseData->Sanity);
	SetMoveSpeed(YogBaseData->MoveSpeed);
	SetDodge(YogBaseData->Dodge);
	SetResilience(YogBaseData->Resilience);
	SetResist(YogBaseData->Resist);
	SetDmgTaken(YogBaseData->DmgTaken);
	SetCrit_Rate(YogBaseData->Crit_Rate);
	SetCrit_Damage(YogBaseData->Crit_Damage);

}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Health, OldValue);
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHealth, OldValue);
}

void UBaseAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Attack, OldValue);
}

void UBaseAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackPower, OldValue);
}

void UBaseAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Shield, OldValue);
}

void UBaseAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackSpeed, OldValue);
}

void UBaseAttributeSet::OnRep_Sanity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Sanity, OldValue);
}

void UBaseAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MoveSpeed, OldValue);
}

void UBaseAttributeSet::OnRep_Dodge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Dodge, OldValue);
}

void UBaseAttributeSet::OnRep_Resilience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Resilience, OldValue);
}

void UBaseAttributeSet::OnRep_Resist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Resist, OldValue);
}

void UBaseAttributeSet::OnRep_DmgTaken(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DmgTaken, OldValue);
}

void UBaseAttributeSet::OnRep_Crit_Rate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Crit_Rate, OldValue);
}

void UBaseAttributeSet::OnRep_Crit_Damage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Crit_Damage, OldValue);
}

void UBaseAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackRange, OldValue);
}

void UBaseAttributeSet::OnRep_Heat(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Heat, OldValue);
}

void UBaseAttributeSet::OnRep_MaxHeat(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHeat, OldValue);
}

void UBaseAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		// Change current value to maintain the current Val / Max percent
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		 float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}

FGameplayCueParameters UBaseAttributeSet::MakeCueParams(const FGameplayEffectCustomExecutionParameters& ExecutionParams, float DamageAmount, bool bWasCritical) const
{
	return FGameplayCueParameters();
}
