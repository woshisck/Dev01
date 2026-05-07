// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Character/PlayerCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr bool bDisableLegacyHeatRuntimeForCardTest = true;
}

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
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, ArmorHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxArmorHP, COND_None, REPNOTIFY_Always);

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
		// 诅咒机制：MaxHP 下降时按比例缩放 HP；上升时（自然恢复）不缩放
		// 净化时由净化 GE 额外补血
		if (NewValue < MaxHealth.GetCurrentValue())
		{
			AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
		}
	}

	if (Attribute == GetArmorHPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxArmorHP());
	}

	if (Attribute == GetMaxArmorHPAttribute())
	{
		AdjustAttributeForMaxChange(ArmorHP, MaxArmorHP, NewValue, GetArmorHPAttribute());
	}

	if (Attribute == GetHeatAttribute())
	{
		if (bDisableLegacyHeatRuntimeForCardTest)
		{
			NewValue = 0.f;
			return;
		}

		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHeat());
	}

}

bool UBaseAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
		return false;

	if (Data.EvaluatedData.Attribute == GetHeatAttribute())
	{
		if (bDisableLegacyHeatRuntimeForCardTest)
		{
			return false;
		}

		CachedPreEffectHeat = GetHeat();

		// 热度减少时，以下任一 Tag 存在则阻断（return false = GE 不执行）：
		//   Buff.Status.Heat.Active       — 战斗状态保热（攻击/受伤后短暂持有）
		//   Buff.Status.HeatDecayBlocked  — 符文主动阻断（如 Rune 1002 热度提升，激活期间禁止衰减）
		if (Data.EvaluatedData.Magnitude < 0.f)
		{
			static const FGameplayTag HeatActiveTag =
				FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Active"), false);
			static const FGameplayTag HeatDecayBlockedTag =
				FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HeatDecayBlocked"), false);

			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if ((HeatActiveTag.IsValid()        && ASC->HasMatchingGameplayTag(HeatActiveTag))       ||
			    (HeatDecayBlockedTag.IsValid()   && ASC->HasMatchingGameplayTag(HeatDecayBlockedTag)))
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

	// 追踪 DmgTaken 的 GE 修改源（值/Op/GE 名/SourceTags）
	if (Data.EvaluatedData.Attribute == GetDmgTakenAttribute())
	{
		const UGameplayEffect* GEDef = Data.EffectSpec.Def;
		UE_LOG(LogTemp, Verbose,
			TEXT("[DmgTakenTrace] GE_EXEC on %s | GE=%s | Op=%d | Magnitude=%.4f | SourceTags=%s"),
			*GetNameSafe(Data.Target.AbilityActorInfo->AvatarActor.Get()),
			*GetNameSafe(GEDef),
			(int32)Data.EvaluatedData.ModifierOp,
			Data.EvaluatedData.Magnitude,
			*SourceTags.ToString());
	}


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
		if (bDisableLegacyHeatRuntimeForCardTest)
		{
			SetHeat(0.f);
			return;
		}

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

			if (!bCanPhaseUp || !bWasAlreadyFull)
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

	if (Attribute == GetDmgTakenAttribute())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[DmgTakenTrace] CHANGE %s : %.2f -> %.2f"),
			*GetNameSafe(GetOwningActor()), OldValue, NewValue);
	}

	// ArmorHP 变化时同步 Buff.Status.Armored Tag
	if (Attribute == GetArmorHPAttribute())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][Armor] ArmorHP %s %.1f -> %.1f / Max %.1f"),
			*GetNameSafe(GetOwningActor()),
			OldValue,
			NewValue,
			GetMaxArmorHP());

		static const FGameplayTag ArmoredTag =
			FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Armored"), false);
		if (ArmoredTag.IsValid())
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ASC)
			{
				if (NewValue > 0.f && !ASC->HasMatchingGameplayTag(ArmoredTag))
				{
					ASC->AddLooseGameplayTag(ArmoredTag);
				}
				else if (NewValue <= 0.f && ASC->HasMatchingGameplayTag(ArmoredTag))
				{
					ASC->RemoveLooseGameplayTag(ArmoredTag);
				}
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
	UE_LOG(LogTemp, Warning, TEXT("[DmgTakenTrace] INIT %s -> %.2f (from CharacterData %s)"),
		*GetNameSafe(GetOwningActor()), YogBaseData->DmgTaken, *GetNameSafe(data));
	SetCrit_Rate(YogBaseData->Crit_Rate);
	SetCrit_Damage(YogBaseData->Crit_Damage);
	SetMaxArmorHP(YogBaseData->MaxArmorHP);
	SetArmorHP(YogBaseData->MaxArmorHP);

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

void UBaseAttributeSet::OnRep_ArmorHP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, ArmorHP, OldValue);
}

void UBaseAttributeSet::OnRep_MaxArmorHP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxArmorHP, OldValue);
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
