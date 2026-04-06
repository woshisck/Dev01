#include "AbilitySystem/ExecutionCalculation/DamageExecution.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GameplayEffectAggregator.h"



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
	TargetDmgTaken = FMath::Max(TargetDmgTaken, 1.f); // capture 失败时默认 1，防止全场 0 伤害
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_RateDef,   EvaluationParameters, SourceCritRate);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().Crit_DamageDef, EvaluationParameters, SourceCritDamage);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().HealthDef,      EvaluationParameters, TargetHealth);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxHealthDef,   EvaluationParameters, TargetMaxHealth);

	float FinalDamage = SourceAttackPower * SourceAttack * TargetDmgTaken;

	// ── ASC 引用 ──────────────────────────────────────────────────────
	UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
	UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetSourceAbilitySystemComponent());

	// ── 暴击计算 ──────────────────────────────────────────────────────
	bool bIsCrit = FMath::FRand() < SourceCritRate;
	if (bIsCrit)
	{
		FinalDamage *= (1.f + SourceCritDamage);
		UE_LOG(LogTemp, Log, TEXT("DamageExecution: CRIT! FinalDamage=%f"), FinalDamage);
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
