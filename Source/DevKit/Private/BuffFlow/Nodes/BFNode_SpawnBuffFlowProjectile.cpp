#include "BuffFlow/Nodes/BFNode_SpawnBuffFlowProjectile.h"

#include "BuffFlow/BuffFlowComponent.h"

UBFNode_SpawnBuffFlowProjectile::UBFNode_SpawnBuffFlowProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Projectile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnBuffFlowProjectile::ExecuteInput(const FName& PinName)
{
	AActor* SourceActor = ResolveTarget(SourceSelector);
	if (!SourceActor || !SourceActor->GetWorld())
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			BFC->RecordTrace(this, nullptr, nullptr, EBuffFlowTraceResult::Failed, TEXT("Source actor is null"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	TSubclassOf<ABuffFlowProjectile> ResolvedProjectileClass = ProjectileClass;
	if (!ResolvedProjectileClass)
	{
		ResolvedProjectileClass = ABuffFlowProjectile::StaticClass();
	}
	const FTransform SpawnTransform = ResolveSpawnTransform(SourceActor);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor;
	SpawnParams.Instigator = Cast<APawn>(SourceActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABuffFlowProjectile* Projectile = SourceActor->GetWorld()->SpawnActorDeferred<ABuffFlowProjectile>(
		ResolvedProjectileClass,
		SpawnTransform,
		SpawnParams.Owner,
		SpawnParams.Instigator,
		SpawnParams.SpawnCollisionHandlingOverride);

	if (!Projectile)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			BFC->RecordTrace(this, nullptr, SourceActor, EBuffFlowTraceResult::Failed, TEXT("SpawnActor failed"), *GetNameSafe(ResolvedProjectileClass.Get()));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	Projectile->SetCreatorForSpawn(SourceActor);
	Projectile->FinishSpawning(SpawnTransform);
	Projectile->InitBuffFlowProjectile(SourceActor, BuildRuntimeConfig());

	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		BFC->RecordTrace(
			this,
			nullptr,
			SourceActor,
			EBuffFlowTraceResult::Success,
			TEXT("Spawned BuffFlow projectile"),
			FString::Printf(
				TEXT("Class=%s TriggerMode=%d Interval=%.2f Lifetime=%.2f Speed=%.1f Effect=%s"),
				*GetNameSafe(ResolvedProjectileClass.Get()),
				static_cast<int32>(TriggerMode),
				TriggerInterval,
				Lifetime,
				Speed,
				*GetNameSafe(EffectClass.Get())));
	}

	TriggerOutput(TEXT("Out"), true);
}

FTransform UBFNode_SpawnBuffFlowProjectile::ResolveSpawnTransform(AActor* SourceActor) const
{
	FTransform SourceTransform;
	const bool bUseSourceTransformOverride = GetBuffFlowComponent()
		&& GetBuffFlowComponent()->GetActiveSourceTransformOverride(SourceTransform);
	if (!bUseSourceTransformOverride)
	{
		SourceTransform = SourceActor ? SourceActor->GetActorTransform() : FTransform::Identity;
	}

	FVector Forward = SourceTransform.GetRotation().GetForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();
	if (Forward.IsNearlyZero() && SourceActor)
	{
		Forward = SourceActor->GetActorForwardVector().GetSafeNormal();
	}
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}

	FVector Right = SourceTransform.GetRotation().GetRightVector();
	Right.Z = 0.f;
	Right = Right.GetSafeNormal();
	if (Right.IsNearlyZero() && SourceActor)
	{
		Right = SourceActor->GetActorRightVector().GetSafeNormal();
	}
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}

	const FVector Location = SourceTransform.GetLocation()
		+ Forward * SpawnOffset.X
		+ Right * SpawnOffset.Y
		+ FVector::UpVector * SpawnOffset.Z;

	return FTransform(Forward.Rotation(), Location);
}

FBuffFlowProjectileRuntimeConfig UBFNode_SpawnBuffFlowProjectile::BuildRuntimeConfig() const
{
	FBuffFlowProjectileRuntimeConfig Config;
	Config.TriggerMode = TriggerMode;
	Config.TriggerInterval = TriggerInterval;
	Config.Lifetime = Lifetime;
	Config.LifetimeCurve = LifetimeCurve;
	Config.LifetimeCurveInput = LifetimeCurveInput;
	Config.LifetimeCurveConstantInput = LifetimeCurveConstantInput;
	Config.Speed = Speed;
	Config.SpeedOverLifeCurve = SpeedOverLifeCurve;
	Config.SpeedCurveMode = SpeedCurveMode;
	Config.CollisionRadius = CollisionRadius;
	Config.bDestroyOnHitTrigger = bDestroyOnHitTrigger;
	Config.bDestroyOnWorldStaticHit = bDestroyOnWorldStaticHit;
	Config.EffectClass = EffectClass;
	Config.bApplyPureDamageFallback = bApplyPureDamageFallback;
	Config.SetByCallerMagnitudeTag = SetByCallerMagnitudeTag;
	Config.BaseEffectMagnitude = BaseEffectMagnitude;
	Config.CreatorAttackMagnitudeScale = CreatorAttackMagnitudeScale;
	Config.CreatorAttackPowerMagnitudeScale = CreatorAttackPowerMagnitudeScale;
	Config.EffectRadius = EffectRadius;
	Config.MaxTriggersPerTarget = MaxTriggersPerTarget;
	Config.TriggerGameplayEventTag = TriggerGameplayEventTag;
	Config.bSendTriggerEventToCreator = bSendTriggerEventToCreator;
	Config.ExpireGameplayEventTag = ExpireGameplayEventTag;
	return Config;
}
