#include "AbilitySystem/ExecutionCalculation/DamageExecution.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Animation/HitStopManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameplayEffectAggregator.h"

namespace
{
	float NormalizeCritRate(float RawRate)
	{
		// Designers may enter either 0.2 or 20 for 20%.
		const float Normalized = RawRate > 1.f ? RawRate / 100.f : RawRate;
		return FMath::Clamp(Normalized, 0.f, 1.f);
	}

	float NormalizeCritMultiplier(float RawMultiplier)
	{
		// Crit_Damage is a final multiplier: 1 = no bonus, 1.5 = 150%, 2 = double.
		return RawMultiplier > 0.f ? FMath::Max(RawMultiplier, 1.f) : 1.f;
	}

	void RequestPlayerCritFreeze(UYogAbilitySystemComponent* SourceASC)
	{
		if (!SourceASC) return;

		AActor* SourceActor = SourceASC->GetAvatarActor();
		APawn* SourcePawn = Cast<APawn>(SourceActor);
		if (!SourcePawn || !SourcePawn->IsPlayerControlled()) return;

		ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
		UAnimInstance* AnimInst = SourceCharacter && SourceCharacter->GetMesh()
			? SourceCharacter->GetMesh()->GetAnimInstance()
			: nullptr;
		if (!AnimInst) return;

		if (UWorld* World = SourceActor->GetWorld())
		{
			if (UHitStopManager* HitStop = World->GetSubsystem<UHitStopManager>())
			{
				HitStop->RequestMontageHitStop(AnimInst, 0.06f);
			}
		}
	}
}



static const FYogDamageStatics& DamageStatics()
{
	static FYogDamageStatics DmgStatics;
	return DmgStatics;
}

UDamageExecution::UDamageExecution()
{

	


	//// Capture the source's AttackPower attribute
	//RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(
	//	UBaseAttributeSet::GetAttackPowerAttribute(),
	//	EGameplayEffectAttributeCaptureSource::Source,
	//	false /* bSnapshot */
	//));

	//// Capture the source's Attack attribute
	//RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(
	//	UBaseAttributeSet::GetAttackAttribute(),
	//	EGameplayEffectAttributeCaptureSource::Source,
	//	false
	//));

	//// Capture the target's DmgTaken attribute
	//RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(
	//	UBaseAttributeSet::GetDmgTakenAttribute(),
	//	EGameplayEffectAttributeCaptureSource::Target,
	//	false
	//));


	RelevantAttributesToCapture.Add(DamageStatics().AttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().SanityDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResilienceDef);
	RelevantAttributesToCapture.Add(DamageStatics().ResistDef);
	RelevantAttributesToCapture.Add(DamageStatics().ShieldDef);
	RelevantAttributesToCapture.Add(DamageStatics().DodgeDef);
	RelevantAttributesToCapture.Add(DamageStatics().DmgTakenDef);
	RelevantAttributesToCapture.Add(DamageStatics().Crit_RateDef);
	RelevantAttributesToCapture.Add(DamageStatics().Crit_DamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
	RelevantAttributesToCapture.Add(DamageStatics().MaxHealthDef);

	RelevantAttributesToCapture.Add(DamageStatics().DamagePhysicalDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageMagicDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamagePureDef);


}


void UDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	//---------------------------------------------------------
	//	DamageDone = AttackPower * Attack * DmgTaken(target)
	//---------------------------------------------------------

	FAggregatorEvaluateParameters EvaluationParameters;
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();
	EvaluationParameters.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	const FGameplayTagContainer* SourceTags = EvaluationParameters.SourceTags;

	// ── 基础属性捕获 ────────────────────────────────────────────────
	float SourceAttackPower = 0.f, SourceAttack = 0.f, TargetDmgTaken = 0.f;
	float SourceCritRate = 0.f, SourceCritDamage = 0.f;
	float TargetHealth = 0.f, TargetMaxHealth = 0.f;

	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, SourceAttackPower);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDef,      EvaluationParameters, SourceAttack);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DmgTakenDef,    EvaluationParameters, TargetDmgTaken);
	// capture 失败（返回 0）时用 1.0 兜底；成功时允许 < 1.0（减伤符文/无畏符文）但不低于 0.01
	TargetDmgTaken = (TargetDmgTaken <= 0.f) ? 1.f : FMath::Max(TargetDmgTaken, 0.01f);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_RateDef,   EvaluationParameters, SourceCritRate);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_DamageDef, EvaluationParameters, SourceCritDamage);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().HealthDef,      EvaluationParameters, TargetHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxHealthDef,   EvaluationParameters, TargetMaxHealth);

	float FinalDamage = SourceAttackPower * SourceAttack * TargetDmgTaken;
	const float CritRate = NormalizeCritRate(SourceCritRate);
	const float CritMultiplier = NormalizeCritMultiplier(SourceCritDamage);

	// ── ASC 引用 ──────────────────────────────────────────────────────
	UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	// ── 暴击计算 ──────────────────────────────────────────────────────
	bool bForceCrit = false;
	if (SourceASC)
	{
		static const FGameplayTag NextHitCritTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.NextHitCrit"), false);
		if (NextHitCritTag.IsValid() && SourceASC->HasMatchingGameplayTag(NextHitCritTag))
		{
			bForceCrit = true;
			SourceASC->RemoveLooseGameplayTag(NextHitCritTag);
		}
	}

	bool bIsCrit = bForceCrit || FMath::FRand() < CritRate;
	if (bIsCrit)
	{
		FinalDamage *= CritMultiplier;
		UE_LOG(LogTemp, Log, TEXT("DamageExecution: CRIT! FinalDamage=%f Rate=%.3f Multiplier=%.2f Forced=%d"),
			FinalDamage, CritRate, CritMultiplier, (int32)bForceCrit);
		RequestPlayerCritFreeze(SourceASC);
	}

	// 诊断：对比聚合器捕获值 vs ASC 直接读值
	if (SourceASC)
	{
		float DirectAttack = SourceASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
		UE_LOG(LogTemp, Warning, TEXT("[DMG] Captured Attack=%f | Direct GetNumericAttribute Attack=%f | SourceASC=%p"),
			SourceAttack, DirectAttack, (void*)SourceASC);
	}
	UE_LOG(LogTemp, Warning, TEXT("AttackPower=%f Attack=%f DmgTaken=%f FinalDamage=%f IsCrit=%d"),
		SourceAttackPower, SourceAttack, TargetDmgTaken, FinalDamage, (int)bIsCrit);

	// ── 应用伤害 ──────────────────────────────────────────────────────
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamagePhysicalProperty, EGameplayModOp::Override, FinalDamage));

	// ReceiveDamage 由 DamageAttributeSet.PostGameplayEffectExecute 在 DamagePhysical 落地时调用，此处不重复广播
	if (bIsCrit && SourceASC)
	{
		SourceASC->OnCritHit.Broadcast(TargetASC, FinalDamage);

		AActor* CritSourceActor = SourceASC->GetAvatarActor();
		if (CritSourceActor)
		{
			static const FGameplayTag CritHitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.CritHit"), false);
			if (CritHitTag.IsValid())
			{
				FGameplayEventData CritPayload;
				CritPayload.EventTag = CritHitTag;
				CritPayload.Instigator = CritSourceActor;
				CritPayload.Target = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
				CritPayload.EventMagnitude = FinalDamage;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(CritSourceActor, CritHitTag, CritPayload);
			}
		}
	}

	// ── 伤害日志（仅玩家来源）────────────────────────────────────────────
	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	APawn* SourcePawn = Cast<APawn>(SourceActor);
	if (SourceASC && SourcePawn && SourcePawn->IsPlayerControlled())
	{
		// 从 SourceTags 识别当前动作名称（源自 GA 的 AbilityTags 在 GE Spec 快照中）
		static const struct { const TCHAR* Tag; const TCHAR* Name; } ActionMap[] =
		{
			{ TEXT("PlayerState.AbilityCast.LightAtk.Combo1"), TEXT("轻击1") },
			{ TEXT("PlayerState.AbilityCast.LightAtk.Combo2"), TEXT("轻击2") },
			{ TEXT("PlayerState.AbilityCast.LightAtk.Combo3"), TEXT("轻击3") },
			{ TEXT("PlayerState.AbilityCast.LightAtk.Combo4"), TEXT("轻击4") },
			{ TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1"), TEXT("重击1") },
			{ TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"), TEXT("重击2") },
			{ TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"), TEXT("重击3") },
			{ TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"), TEXT("重击4") },
		};

		FName ActionName = FName("Attack");
		if (SourceTags)
		{
			for (const auto& M : ActionMap)
			{
				if (SourceTags->HasTag(FGameplayTag::RequestGameplayTag(FName(M.Tag), false)))
				{
					ActionName = FName(M.Name);
					break;
				}
			}
		}

		FDamageBreakdown Breakdown;
		Breakdown.BaseAttack      = SourceAttack;
		Breakdown.ActionMultiplier = SourceAttackPower;
		Breakdown.DmgTakenMult    = TargetDmgTaken;
		Breakdown.FinalDamage     = FinalDamage;
		Breakdown.bIsCrit         = bIsCrit;
		Breakdown.ActionName      = ActionName;
		Breakdown.DamageType      = bIsCrit ? FName("Attack_Crit") : FName("Attack");
		Breakdown.TargetName      = GetNameSafe(TargetASC ? TargetASC->GetAvatarActor() : nullptr);
		Breakdown.SourceName      = GetNameSafe(SourceActor);

		SourceASC->LogDamageDealtDetailed(TargetASC ? TargetASC->GetAvatarActor() : nullptr, Breakdown);
	}


	//const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	//const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();



	//FGameplayEffectSpec* spec = ExecutionParams.GetOwningSpecForPreExecuteMod();


	//UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	//UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	//if (!SourceASC || !TargetASC)
	//{
	//	return;
	//}

	////Get Avatar Actor
	//AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor_Direct() : nullptr;
	//AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor_Direct() : nullptr;

	////Get Current GameplayAbility
	////UYogGameplayAbility* target_CurrentAbilityClass = TargetASC->GetCurrentAbilityClass();
	////UYogGameplayAbility* source_CurrentAbilityClass = SourceASC->GetCurrentAbilityClass();


	////Get GameplayEffect Instance
	//const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();


	////get both tags container
	//FAggregatorEvaluateParameters EvalParams;

	//const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	//const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	//EvalParams.SourceTags = SourceTags;
	//EvalParams.TargetTags = TargetTags;
	

	//Crit_Rate +  = Final_Crit_rate
	//Crit_Damage +  = Crit_Damage
	//(Attack + ) * (AttackPower + ) * (DmgTaken + Add_DmgTaken)

	////////////////////////////////////////////////// Source //////////////////////////////////////////////////


	//float SourceAttackPower = 0.0f;
	//float SourceAttack = 0.0f;
	//float TargetDmgTaken = 0.0f;


	//// Attempt to capture the attributes
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
	//	FGameplayEffectAttributeCaptureDefinition(UBaseAttributeSet::GetAttackPowerAttribute(), 
	//	EGameplayEffectAttributeCaptureSource::Source, false),
	//	EvalParams,
	//	SourceAttackPower);

	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
	//	FGameplayEffectAttributeCaptureDefinition(UBaseAttributeSet::GetAttackAttribute(), 
	//	EGameplayEffectAttributeCaptureSource::Source, false),
	//	EvalParams,
	//	SourceAttack);

	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
	//	FGameplayEffectAttributeCaptureDefinition(UBaseAttributeSet::GetDmgTakenAttribute(), 
	//	EGameplayEffectAttributeCaptureSource::Target, false),
	//	EvalParams,
	//	TargetDmgTaken);


	//// Optional: clamp values to avoid negative damage
	//SourceAttackPower = FMath::Max(0.0f, SourceAttackPower);
	//SourceAttack = FMath::Max(0.0f, SourceAttack);
	//TargetDmgTaken = FMath::Max(0.0f, TargetDmgTaken);

	//float FinalDamage = SourceAttackPower * SourceAttack * TargetDmgTaken;


	//float Attack = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDef, EvaluationParameters, Attack);

	//float AttackPower = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, AttackPower);

	//float Crit_Rate = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_RateDef, EvaluationParameters, Crit_Rate);

	//float Crit_Damage = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_DamageDef, EvaluationParameters, Crit_Damage);


	////////////////////////////////////////////////// Target //////////////////////////////////////////////////
	//float DmgTaken = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DmgTakenDef, EvaluationParameters, DmgTaken);
	//DmgTaken = FMath::Max(DmgTaken, 0);

	//float Dodge = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DodgeDef, EvaluationParameters, Dodge);
	//Dodge = FMath::Max(Dodge, 0);

	//float Shield = 0.f;
	//ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ShieldDef, EvaluationParameters, Shield);
	//Shield = FMath::Max(Shield, 0);


	//float DamageDone = 0.f;
	//float RandomFloatValue = FMath::FRand();

	//if (Shield < 0.f)
	//{	//Crit_Value <= Crit_Rate IS Critical Hit
	//	if (RandomFloatValue <= Crit_Rate)
	//	{
	//		DamageDone = (AttackPower ) * (Attack ) * (Crit_Damage ) * (DmgTaken );
	//		
	//	}
	//	else
	//	{
	//		DamageDone = (AttackPower ) * (Attack ) * (DmgTaken );
	//		
	//	}
	//}
	//else
	//{
	//	if (RandomFloatValue <= Crit_Rate)
	//	{
	//		DamageDone = (AttackPower ) * (Attack ) * (Crit_Damage ) * (DmgTaken );
	//		
	//	}
	//	else
	//	{
	//		DamageDone = (AttackPower ) * (Attack ) * (DmgTaken );
	//		
	//	}
	//}
	

	//OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamagePhysicalProperty, EGameplayModOp::Additive, FinalDamage));
	////Broadcast damages to Target ASC
	//if (TargetASC)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Damage deal total: %f"), FinalDamage);
	//	TargetASC->ReceiveDamage(SourceASC, FinalDamage);
	//}
}
