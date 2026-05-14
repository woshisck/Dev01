#include "BuffFlow/Nodes/BFNode_SpawnBuffFlowProjectile.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

UBFNode_SpawnBuffFlowProjectile::UBFNode_SpawnBuffFlowProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Projectile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnBuffFlowProjectile::ExecuteBuffFlowInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	AActor* SourceActor = ResolveTarget(SourceSelector);
	if (!SourceActor || !SourceActor->GetWorld())
	{
		if (BFC)
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
	int32 ComboBonusProjectiles = 0;
	int32 ComboBonusStacks = 0;
	const int32 SpawnCount = ResolveSpawnCount(BFC, ComboBonusProjectiles, ComboBonusStacks);
	const FBuffFlowProjectileRuntimeConfig RuntimeProjectileConfig = BuildRuntimeConfig();
	const float ResolvedSpawnInterval = FMath::Max(0.f, SpawnInterval);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor;
	SpawnParams.Instigator = Cast<APawn>(SourceActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TWeakObjectPtr<AActor> WeakSourceActor(SourceActor);
	const TSubclassOf<ABuffFlowProjectile> SpawnProjectileClass = ResolvedProjectileClass;
	const FActorSpawnParameters BaseSpawnParams = SpawnParams;
	auto SpawnProjectile = [WeakSourceActor, SpawnProjectileClass, SpawnTransform, BaseSpawnParams, RuntimeProjectileConfig]() -> bool
	{
		AActor* Source = WeakSourceActor.Get();
		if (!Source || !Source->GetWorld() || !SpawnProjectileClass)
		{
			return false;
		}

		ABuffFlowProjectile* Projectile = Source->GetWorld()->SpawnActorDeferred<ABuffFlowProjectile>(
			SpawnProjectileClass,
			SpawnTransform,
			BaseSpawnParams.Owner,
			BaseSpawnParams.Instigator,
			BaseSpawnParams.SpawnCollisionHandlingOverride);

		if (!Projectile)
		{
			return false;
		}

		Projectile->SetRuntimeConfigForSpawn(Source, RuntimeProjectileConfig);
		Projectile->FinishSpawning(SpawnTransform);
		Projectile->InitBuffFlowProjectile(Source, RuntimeProjectileConfig);
		return true;
	};

	int32 SpawnedOrScheduledCount = 0;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		if (Index > 0 && ResolvedSpawnInterval > KINDA_SMALL_NUMBER)
		{
			FTimerHandle TimerHandle;
			SourceActor->GetWorld()->GetTimerManager().SetTimer(
				TimerHandle,
				FTimerDelegate::CreateLambda([SpawnProjectile]()
				{
					SpawnProjectile();
				}),
				ResolvedSpawnInterval * static_cast<float>(Index),
				false);
			++SpawnedOrScheduledCount;
			continue;
		}

		if (SpawnProjectile())
		{
			++SpawnedOrScheduledCount;
		}
	}

	if (SpawnedOrScheduledCount <= 0)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, nullptr, SourceActor, EBuffFlowTraceResult::Failed, TEXT("SpawnActor failed"), *GetNameSafe(ResolvedProjectileClass.Get()));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	if (BFC)
	{
		BFC->RecordTrace(
			this,
			nullptr,
			SourceActor,
			EBuffFlowTraceResult::Success,
			TEXT("Spawned BuffFlow projectile"),
			FString::Printf(
				TEXT("Class=%s Count=%d BaseCount=%d SpawnInterval=%.2f AddCombo=%d HasCardContext=%d ComboStacks=%d PerStack=%d MaxBonus=%d ComboBonus=%d TriggerMode=%d TriggerInterval=%.2f Lifetime=%.2f Speed=%.1f Capsule=(R=%.1f HH=%.1f) Effect=%s"),
				*GetNameSafe(ResolvedProjectileClass.Get()),
				SpawnedOrScheduledCount,
				FMath::Max(1, ProjectileCount),
				ResolvedSpawnInterval,
				bAddComboStacksToProjectileCount ? 1 : 0,
				BFC && BFC->HasCombatCardEffectContext() ? 1 : 0,
				ComboBonusStacks,
				FMath::Max(0, ProjectilesPerComboStack),
				FMath::Max(0, MaxBonusProjectiles),
				ComboBonusProjectiles,
				static_cast<int32>(TriggerMode),
				TriggerInterval,
				Lifetime,
				Speed,
				CollisionCapsuleRadius,
				CollisionCapsuleHalfHeight,
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

int32 UBFNode_SpawnBuffFlowProjectile::ResolveSpawnCount(
	const UBuffFlowComponent* BuffFlowComponent,
	int32& OutComboBonusProjectiles,
	int32& OutComboBonusStacks) const
{
	OutComboBonusProjectiles = 0;
	OutComboBonusStacks = 0;
	if (bAddComboStacksToProjectileCount && BuffFlowComponent && BuffFlowComponent->HasCombatCardEffectContext())
	{
		const FCombatCardEffectContext& CombatCardContext = BuffFlowComponent->GetLastCombatCardEffectContext();
		OutComboBonusStacks = FMath::Max(0, CombatCardContext.ComboBonusStacks);
		const int32 UncappedBonusProjectiles = OutComboBonusStacks * FMath::Max(0, ProjectilesPerComboStack);
		OutComboBonusProjectiles = MaxBonusProjectiles > 0
			? FMath::Min(FMath::Max(0, MaxBonusProjectiles), UncappedBonusProjectiles)
			: UncappedBonusProjectiles;
	}

	return FMath::Max(1, FMath::Max(1, ProjectileCount) + OutComboBonusProjectiles);
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
	Config.VisualCoefficient = VisualCoefficient;
	Config.ProjectileVisualColor = ProjectileVisualColor;
	Config.ProjectileVisualNiagaraSystem = ProjectileVisualNiagaraSystem;
	Config.ProjectileVisualNiagaraScale = ProjectileVisualNiagaraScale;
	Config.CollisionCapsuleRadius = CollisionCapsuleRadius;
	Config.CollisionCapsuleHalfHeight = CollisionCapsuleHalfHeight;
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
