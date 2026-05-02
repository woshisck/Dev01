#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Projectile/SlashWaveProjectile.h"

UBFNode_SpawnSlashWaveProjectile::UBFNode_SpawnSlashWaveProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Projectile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnSlashWaveProjectile::ExecuteInput(const FName& PinName)
{
	if (!ProjectileClass)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* SourceActor = ResolveTarget(SourceSelector);
	ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
	if (!SourceCharacter || !SourceCharacter->GetWorld())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const float ResolvedDamage = ResolveDamage(SourceCharacter);
	ConsumeSourceArmor(SourceCharacter);

	const FVector Forward = SourceCharacter->GetActorForwardVector();
	const FVector Right = SourceCharacter->GetActorRightVector();
	const FVector Up = FVector::UpVector;
	const FVector SpawnLocation = SourceCharacter->GetActorLocation()
		+ Forward * SpawnOffset.X
		+ Right * SpawnOffset.Y
		+ Up * SpawnOffset.Z;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceCharacter;
	SpawnParams.Instigator = SourceCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FSlashWaveProjectileRuntimeConfig Config;
	Config.Damage = ResolvedDamage;
	Config.DamageEffect = DamageEffect;
	Config.Speed = Speed;
	Config.MaxDistance = MaxDistance;
	Config.MaxHitCount = MaxHitCount;
	Config.DamageApplicationsPerTarget = DamageApplicationsPerTarget;
	Config.DamageApplicationInterval = DamageApplicationInterval;
	Config.CollisionBoxExtent = CollisionBoxExtent;
	Config.bScaleVisualWithCollisionExtent = bScaleVisualWithCollisionExtent;
	Config.VisualScaleMultiplier = VisualScaleMultiplier;
	Config.DamageLogType = DamageLogType;
	Config.bDestroyOnWorldStaticHit = bDestroyOnWorldStaticHit;
	Config.bForcePureDamage = bForcePureDamage;
	Config.BonusArmorDamageMultiplier = BonusArmorDamageMultiplier;
	Config.AdditionalHitEffect = AdditionalHitEffect;
	Config.AdditionalHitSetByCallerTag = AdditionalHitSetByCallerTag;
	Config.AdditionalHitSetByCallerValue = AdditionalHitSetByCallerValue;
	Config.bSplitOnFirstHit = bSplitOnFirstHit;
	Config.MaxSplitGenerations = MaxSplitGenerations;
	Config.SplitProjectileCount = SplitProjectileCount;
	Config.SplitConeAngleDegrees = SplitConeAngleDegrees;
	Config.SplitDamageMultiplier = SplitDamageMultiplier;
	Config.SplitSpeedMultiplier = SplitSpeedMultiplier;
	Config.SplitMaxDistanceMultiplier = SplitMaxDistanceMultiplier;
	Config.SplitCollisionBoxExtentMultiplier = SplitCollisionBoxExtentMultiplier;

	const int32 SpawnCount = FMath::Max(1, ProjectileCount);
	const float ClampedCone = FMath::Clamp(ProjectileConeAngleDegrees, 0.f, 180.f);
	const float Step = SpawnCount > 1 ? ClampedCone / static_cast<float>(SpawnCount - 1) : 0.f;
	const float StartYaw = SpawnCount > 1 ? -ClampedCone * 0.5f : 0.f;

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const float YawOffset = StartYaw + Step * Index;
		const FVector Direction = Forward.RotateAngleAxis(YawOffset, FVector::UpVector);
		const FRotator SpawnRotation = Direction.Rotation();

		ASlashWaveProjectile* Projectile = SourceCharacter->GetWorld()->SpawnActor<ASlashWaveProjectile>(
			ProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams);

		if (Projectile)
		{
			Projectile->InitProjectileWithConfig(SourceCharacter, Config);
			++SpawnedCount;
		}
	}

	if (SpawnedCount <= 0)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[SpawnSlashWaveProjectile] Class=%s Count=%d Damage=%.1f Speed=%.1f Distance=%.1f HitCount=%d DamageApps=%d DamageInterval=%.2f CollisionExtent=%s VisualWithCollision=%d VisualMultiplier=%s"),
		*GetNameSafe(ProjectileClass),
		SpawnedCount,
		ResolvedDamage,
		Speed,
		MaxDistance,
		MaxHitCount,
		DamageApplicationsPerTarget,
		DamageApplicationInterval,
		*CollisionBoxExtent.ToString(),
		bScaleVisualWithCollisionExtent ? 1 : 0,
		*VisualScaleMultiplier.ToString());

	TriggerOutput(TEXT("Out"), true);
}

float UBFNode_SpawnSlashWaveProjectile::ResolveDamage(ACharacter* SourceCharacter) const
{
	float ResolvedDamage = Damage;
	if (!bAddSourceArmorToDamage || !SourceCharacter)
	{
		return ResolvedDamage;
	}

	if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter))
	{
		const float SourceArmor = FMath::Max(0.f, SourceASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute()));
		ResolvedDamage += SourceArmor * FMath::Max(0.f, SourceArmorToDamageMultiplier);
	}
	return ResolvedDamage;
}

void UBFNode_SpawnSlashWaveProjectile::ConsumeSourceArmor(ACharacter* SourceCharacter) const
{
	if (!bConsumeSourceArmorOnSpawn || !SourceCharacter)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
	if (!SourceASC)
	{
		return;
	}

	const float SourceArmor = FMath::Max(0.f, SourceASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute()));
	const float ConsumedArmor = FMath::Min(SourceArmor, SourceArmor * FMath::Max(0.f, SourceArmorConsumeMultiplier));
	SourceASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), FMath::Max(0.f, SourceArmor - ConsumedArmor));
}
