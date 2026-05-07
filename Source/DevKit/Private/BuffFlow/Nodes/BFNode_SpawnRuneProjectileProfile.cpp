#include "BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RuneCardEffectProfileDA.h"
#include "GameFramework/Character.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Projectile/SlashWaveProjectile.h"
#include "TimerManager.h"

UBFNode_SpawnRuneProjectileProfile::UBFNode_SpawnRuneProjectileProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Profile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnRuneProjectileProfile::ExecuteInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!Profile || !Profile->Projectile.ProjectileClass)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, nullptr, EBuffFlowTraceResult::Failed, TEXT("Profile or projectile class is null"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const FRuneCardProfileProjectileConfig& Projectile = Profile->Projectile;
	AActor* SourceActor = ResolveTarget(Projectile.SourceSelector);
	ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
	if (!SourceCharacter || !SourceCharacter->GetWorld())
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, SourceActor, EBuffFlowTraceResult::Failed, TEXT("Source character is invalid"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const float ResolvedDamage = ResolveDamage(SourceCharacter);
	ConsumeSourceArmor(SourceCharacter);

	FTransform SourceTransformOverride;
	const bool bUseSourceTransformOverride = BFC && BFC->GetActiveSourceTransformOverride(SourceTransformOverride);
	FVector Forward = bUseSourceTransformOverride ? SourceTransformOverride.GetRotation().GetForwardVector() : SourceCharacter->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = SourceCharacter->GetActorForwardVector();
	}
	FVector Right = bUseSourceTransformOverride ? SourceTransformOverride.GetRotation().GetRightVector() : SourceCharacter->GetActorRightVector();
	Right.Z = 0.0f;
	Right = Right.GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = SourceCharacter->GetActorRightVector();
	}

	const FVector SourceLocation = bUseSourceTransformOverride ? SourceTransformOverride.GetLocation() : SourceCharacter->GetActorLocation();
	const FVector SpawnLocation = SourceLocation
		+ Forward * Projectile.SpawnOffset.X
		+ Right * Projectile.SpawnOffset.Y
		+ FVector::UpVector * Projectile.SpawnOffset.Z;
	const FRotator BaseSpawnRotation = Forward.Rotation();

	SpawnLaunchNiagara(SourceCharacter, SpawnLocation, BaseSpawnRotation);

	FSlashWaveProjectileRuntimeConfig Config;
	Config.Damage = ResolvedDamage;
	Config.DamageEffect = Projectile.DamageEffect;
	Config.Speed = Projectile.Speed;
	Config.MaxDistance = Projectile.MaxDistance;
	Config.MaxHitCount = Projectile.MaxHitCount;
	Config.DamageApplicationsPerTarget = Projectile.DamageApplicationsPerTarget;
	Config.DamageApplicationInterval = Projectile.DamageApplicationInterval;
	Config.CollisionBoxExtent = Projectile.CollisionBoxExtent;
	Config.bScaleVisualWithCollisionExtent = Projectile.bScaleVisualWithCollisionExtent;
	Config.VisualScaleMultiplier = Projectile.VisualScaleMultiplier;
	Config.ProjectileVisualNiagaraSystem = Projectile.ProjectileVisualNiagaraSystem;
	Config.ProjectileVisualNiagaraScale = Projectile.ProjectileVisualNiagaraScale;
	Config.bHideDefaultProjectileVisuals = Projectile.bHideDefaultProjectileVisuals;
	Config.HitNiagaraSystem = Projectile.HitNiagaraSystem;
	Config.HitNiagaraScale = Projectile.HitNiagaraScale;
	Config.ExpireNiagaraSystem = Projectile.ExpireNiagaraSystem;
	Config.ExpireNiagaraScale = Projectile.ExpireNiagaraScale;
	Config.HitGameplayEventTag = Projectile.HitGameplayEventTag;
	Config.ExpireGameplayEventTag = Projectile.ExpireGameplayEventTag;
	Config.DamageLogType = Profile->DamageLogType;
	Config.bDestroyOnWorldStaticHit = Projectile.bDestroyOnWorldStaticHit;
	Config.bForcePureDamage = Projectile.bForcePureDamage;
	Config.BonusArmorDamageMultiplier = Projectile.BonusArmorDamageMultiplier;
	Config.AdditionalHitEffect = Projectile.AdditionalHitEffect;
	Config.AdditionalHitSetByCallerTag = Projectile.AdditionalHitSetByCallerTag;
	Config.AdditionalHitSetByCallerValue = Projectile.AdditionalHitSetByCallerValue;
	Config.bSplitOnFirstHit = Projectile.bSplitOnFirstHit;
	Config.MaxSplitGenerations = Projectile.MaxSplitGenerations;
	Config.SplitProjectileCount = Projectile.SplitProjectileCount;
	Config.SplitConeAngleDegrees = Projectile.SplitConeAngleDegrees;
	Config.bRandomizeSplitDirections = Projectile.bRandomizeSplitDirections;
	Config.SplitRandomYawJitterDegrees = Projectile.SplitRandomYawJitterDegrees;
	Config.SplitRandomPitchDegrees = Projectile.SplitRandomPitchDegrees;
	Config.SplitDamageMultiplier = Projectile.SplitDamageMultiplier;
	Config.SplitSpeedMultiplier = Projectile.SplitSpeedMultiplier;
	Config.SplitMaxDistanceMultiplier = Projectile.SplitMaxDistanceMultiplier;
	Config.SplitCollisionBoxExtentMultiplier = Projectile.SplitCollisionBoxExtentMultiplier;
	Config.bBounceOnEnemyHit = Projectile.bBounceSplitChildrenOnEnemyHit;
	Config.MaxEnemyBounces = Projectile.SplitChildMaxEnemyBounces;

	int32 ComboBonusProjectiles = 0;
	if (Projectile.bAddComboStacksToProjectileCount && BFC && BFC->HasCombatCardEffectContext())
	{
		const FCombatCardEffectContext& CombatCardContext = BFC->GetLastCombatCardEffectContext();
		ComboBonusProjectiles = CombatCardContext.ComboBonusStacks * FMath::Max(0, Projectile.ProjectilesPerComboStack);
		ComboBonusProjectiles = FMath::Min(FMath::Max(0, Projectile.MaxBonusProjectiles), ComboBonusProjectiles);
	}

	const int32 SpawnCount = FMath::Max(1, Projectile.ProjectileCount + ComboBonusProjectiles);
	const bool bUseSequentialSpawn = Projectile.bSpawnProjectilesSequentially && SpawnCount > 1;
	const float ClampedCone = bUseSequentialSpawn ? 0.f : FMath::Clamp(Projectile.ProjectileConeAngleDegrees, 0.f, 180.f);
	const float Step = SpawnCount > 1 ? ClampedCone / static_cast<float>(SpawnCount - 1) : 0.f;
	const float StartYaw = SpawnCount > 1 ? -ClampedCone * 0.5f : 0.f;
	const float SequentialInterval = FMath::Max(0.f, Projectile.SequentialProjectileSpawnInterval);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceCharacter;
	SpawnParams.Instigator = SourceCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TWeakObjectPtr<ACharacter> WeakSourceCharacter(SourceCharacter);
	const TSubclassOf<ASlashWaveProjectile> SpawnProjectileClass = Projectile.ProjectileClass;
	const FSlashWaveProjectileRuntimeConfig SpawnConfig = Config;
	const FActorSpawnParameters BaseSpawnParams = SpawnParams;
	auto SpawnProjectileAtRotation = [WeakSourceCharacter, SpawnProjectileClass, SpawnConfig, BaseSpawnParams, SpawnLocation](const FRotator& SpawnRotation) -> bool
	{
		ACharacter* Source = WeakSourceCharacter.Get();
		if (!Source || !Source->GetWorld() || !SpawnProjectileClass)
		{
			return false;
		}

		ASlashWaveProjectile* SpawnedProjectile = Source->GetWorld()->SpawnActor<ASlashWaveProjectile>(
			SpawnProjectileClass,
			SpawnLocation,
			SpawnRotation,
			BaseSpawnParams);
		if (SpawnedProjectile)
		{
			SpawnedProjectile->InitProjectileWithConfig(Source, SpawnConfig);
			return true;
		}
		return false;
	};

	int32 SpawnedOrScheduledCount = 0;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const float YawOffset = StartYaw + Step * Index;
		const FVector Direction = Forward.RotateAngleAxis(YawOffset, FVector::UpVector);
		const FRotator SpawnRotation = bUseSequentialSpawn ? BaseSpawnRotation : Direction.Rotation();
		if (bUseSequentialSpawn && Index > 0 && SequentialInterval > KINDA_SMALL_NUMBER)
		{
			FTimerHandle TimerHandle;
			SourceCharacter->GetWorld()->GetTimerManager().SetTimer(
				TimerHandle,
				FTimerDelegate::CreateLambda([SpawnProjectileAtRotation, SpawnRotation]()
				{
					SpawnProjectileAtRotation(SpawnRotation);
				}),
				SequentialInterval * static_cast<float>(Index),
				false);
			++SpawnedOrScheduledCount;
			continue;
		}

		if (SpawnProjectileAtRotation(SpawnRotation))
		{
			++SpawnedOrScheduledCount;
		}
	}

	if (SpawnedOrScheduledCount <= 0)
	{
		if (BFC)
		{
			BFC->RecordTrace(this, Profile, SourceCharacter, EBuffFlowTraceResult::Failed, TEXT("No projectile spawned"), TEXT(""));
		}
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	if (BFC)
	{
		BFC->RecordTrace(
			this,
			Profile,
			SourceCharacter,
			EBuffFlowTraceResult::Success,
			TEXT("Spawned projectile profile"),
			FString::Printf(
				TEXT("Class=%s Count=%d ComboBonus=%d Damage=%.2f Speed=%.1f Distance=%.1f Sequential=%d Cone=%.1f"),
				*GetNameSafe(Projectile.ProjectileClass.Get()),
				SpawnedOrScheduledCount,
				ComboBonusProjectiles,
				ResolvedDamage,
				Projectile.Speed,
				Projectile.MaxDistance,
				bUseSequentialSpawn ? 1 : 0,
				ClampedCone));
	}

	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_SpawnRuneProjectileProfile::Cleanup()
{
	if (Profile && Profile->Projectile.bDestroyLaunchNiagaraWithFlow && Profile->Projectile.LaunchNiagaraEffectName != NAME_None)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			if (TObjectPtr<UNiagaraComponent>* Found = BFC->ActiveNiagaraEffects.Find(Profile->Projectile.LaunchNiagaraEffectName))
			{
				if (*Found)
				{
					(*Found)->DeactivateImmediate();
				}
				BFC->ActiveNiagaraEffects.Remove(Profile->Projectile.LaunchNiagaraEffectName);
			}
		}
	}
	Super::Cleanup();
}

float UBFNode_SpawnRuneProjectileProfile::ResolveDamage(ACharacter* SourceCharacter) const
{
	if (!Profile)
	{
		return 0.f;
	}

	float ResolvedDamage = Profile->DamageValue;
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		if (Profile->DamageTuningKey != NAME_None)
		{
			ResolvedDamage = BFC->GetRuneTuningValueForFlow(GetFlowAsset(), Profile->DamageTuningKey, ResolvedDamage);
		}
		if (Profile->DamageMode == ERuneCardProfileDamageMode::LastDamage)
		{
			ResolvedDamage = BFC->LastEventContext.DamageAmount;
		}
		else if (Profile->DamageMode == ERuneCardProfileDamageMode::CombatCardAttackDamage && BFC->HasCombatCardEffectContext())
		{
			ResolvedDamage = BFC->GetLastCombatCardEffectContext().ActionContext.AttackDamage;
		}
	}

	const FRuneCardProfileProjectileConfig& Projectile = Profile->Projectile;
	if (Projectile.bAddSourceArmorToDamage && SourceCharacter)
	{
		if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter))
		{
			const float SourceArmor = FMath::Max(0.f, SourceASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute()));
			ResolvedDamage += SourceArmor * FMath::Max(0.f, Projectile.SourceArmorToDamageMultiplier);
		}
	}
	return ResolvedDamage;
}

void UBFNode_SpawnRuneProjectileProfile::ConsumeSourceArmor(ACharacter* SourceCharacter) const
{
	if (!Profile || !Profile->Projectile.bConsumeSourceArmorOnSpawn || !SourceCharacter)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
	if (!SourceASC)
	{
		return;
	}

	const float SourceArmor = FMath::Max(0.f, SourceASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute()));
	const float ConsumedArmor = FMath::Min(SourceArmor, SourceArmor * FMath::Max(0.f, Profile->Projectile.SourceArmorConsumeMultiplier));
	SourceASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), FMath::Max(0.f, SourceArmor - ConsumedArmor));
}

void UBFNode_SpawnRuneProjectileProfile::SpawnLaunchNiagara(
	ACharacter* SourceCharacter,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation) const
{
	if (!Profile || !Profile->Projectile.LaunchNiagaraSystem || !SourceCharacter)
	{
		return;
	}

	const FRuneCardProfileProjectileConfig& Projectile = Profile->Projectile;
	UNiagaraComponent* NiagaraComp = nullptr;
	if (Projectile.bAttachLaunchNiagaraToSource)
	{
		USceneComponent* AttachComp = SourceCharacter->GetRootComponent();
		if (USkeletalMeshComponent* SkelMesh = SourceCharacter->FindComponentByClass<USkeletalMeshComponent>())
		{
			AttachComp = SkelMesh;
		}
		if (!AttachComp)
		{
			return;
		}
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Projectile.LaunchNiagaraSystem,
			AttachComp,
			NAME_None,
			Projectile.SpawnOffset + Projectile.LaunchNiagaraOffset,
			Projectile.LaunchNiagaraRotationOffset,
			EAttachLocation::KeepRelativeOffset,
			true);
		if (NiagaraComp)
		{
			NiagaraComp->SetRelativeScale3D(Projectile.LaunchNiagaraScale);
		}
	}
	else if (UWorld* World = SourceCharacter->GetWorld())
	{
		const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			Projectile.LaunchNiagaraSystem,
			SpawnTransform.TransformPosition(Projectile.LaunchNiagaraOffset),
			(SpawnRotation + Projectile.LaunchNiagaraRotationOffset).GetNormalized(),
			Projectile.LaunchNiagaraScale,
			true);
	}

	if (NiagaraComp && Projectile.LaunchNiagaraEffectName != NAME_None)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			BFC->ActiveNiagaraEffects.Add(Projectile.LaunchNiagaraEffectName, NiagaraComp);
		}
	}
}
