#include "Projectile/SlashWaveProjectile.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

namespace
{
	FGameplayTag SlashWaveActDamageTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Attribute.ActDamage")));
	}

	float SafePositiveScale(float Value)
	{
		return FMath::Max(0.01f, Value);
	}

	FVector SafePositiveScale(const FVector& Value)
	{
		return FVector(
			SafePositiveScale(Value.X),
			SafePositiveScale(Value.Y),
			SafePositiveScale(Value.Z));
	}

	float SafeExtentRatio(float NewExtent, float DefaultExtent)
	{
		return DefaultExtent > KINDA_SMALL_NUMBER ? NewExtent / DefaultExtent : 1.f;
	}
}

ASlashWaveProjectile::ASlashWaveProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->InitBoxExtent(CollisionBoxExtent);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(CollisionBox);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 1400.f;
	ProjectileMovement->MaxSpeed = 1400.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bRotationFollowsVelocity = false;
}

void ASlashWaveProjectile::InitProjectile(ACharacter* InSource, float InDamage,
                                          TSubclassOf<UGameplayEffect> InDamageEffect)
{
	SourceCharacter = InSource;
	DamageMagnitude = InDamage;
	DamageEffectClass = InDamageEffect;
	bProjectileInitialized = SourceCharacter != nullptr;

	RefreshLifetimeFromDistance();

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
	}

	if (HasActorBegunPlay() && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			LifetimeTimerHandle, this, &ASlashWaveProjectile::Expire, Lifetime, false);
	}
}

void ASlashWaveProjectile::InitProjectileWithConfig(ACharacter* InSource, const FSlashWaveProjectileRuntimeConfig& InConfig)
{
	const FVector DefaultCollisionBoxExtent = FVector(
		FMath::Max(1.f, CollisionBoxExtent.X),
		FMath::Max(1.f, CollisionBoxExtent.Y),
		FMath::Max(1.f, CollisionBoxExtent.Z));

	Speed = FMath::Max(1.f, InConfig.Speed);
	MaxDistance = FMath::Max(0.f, InConfig.MaxDistance);
	MaxHitCount = InConfig.MaxHitCount;
	DamageApplicationsPerTarget = FMath::Clamp(InConfig.DamageApplicationsPerTarget, 1, 20);
	DamageApplicationInterval = FMath::Max(0.f, InConfig.DamageApplicationInterval);
	DamageLogType = InConfig.DamageLogType.IsNone() ? TEXT("Rune_SlashWave") : InConfig.DamageLogType;
	CollisionBoxExtent = FVector(
		FMath::Max(1.f, InConfig.CollisionBoxExtent.X),
		FMath::Max(1.f, InConfig.CollisionBoxExtent.Y),
		FMath::Max(1.f, InConfig.CollisionBoxExtent.Z));
	VisualScaleMultiplier = SafePositiveScale(InConfig.VisualScaleMultiplier);
	bDestroyOnWorldStaticHit = InConfig.bDestroyOnWorldStaticHit;
	bForcePureDamage = InConfig.bForcePureDamage;
	BonusArmorDamageMultiplier = FMath::Max(0.f, InConfig.BonusArmorDamageMultiplier);
	AdditionalHitEffectClass = InConfig.AdditionalHitEffect;
	AdditionalHitSetByCallerTag = InConfig.AdditionalHitSetByCallerTag;
	AdditionalHitSetByCallerValue = InConfig.AdditionalHitSetByCallerValue;
	bSplitOnFirstHit = InConfig.bSplitOnFirstHit;
	ProjectileGeneration = FMath::Max(0, InConfig.ProjectileGeneration);
	MaxSplitGenerations = FMath::Max(0, InConfig.MaxSplitGenerations);
	SplitProjectileCount = FMath::Max(1, InConfig.SplitProjectileCount);
	SplitConeAngleDegrees = FMath::Clamp(InConfig.SplitConeAngleDegrees, 0.f, 180.f);
	SplitDamageMultiplier = FMath::Max(0.f, InConfig.SplitDamageMultiplier);
	SplitSpeedMultiplier = FMath::Max(0.01f, InConfig.SplitSpeedMultiplier);
	SplitMaxDistanceMultiplier = FMath::Max(0.f, InConfig.SplitMaxDistanceMultiplier);
	SplitCollisionBoxExtentMultiplier = SafePositiveScale(InConfig.SplitCollisionBoxExtentMultiplier);

	if (CollisionBox)
	{
		CollisionBox->SetCollisionResponseToChannel(
			ECC_WorldStatic,
			bDestroyOnWorldStaticHit ? ECR_Overlap : ECR_Ignore);
	}

	FVector FinalVisualScale = VisualScaleMultiplier;
	if (InConfig.bScaleVisualWithCollisionExtent)
	{
		const FVector CollisionScale = FVector(
			SafeExtentRatio(CollisionBoxExtent.X, DefaultCollisionBoxExtent.X),
			SafeExtentRatio(CollisionBoxExtent.Y, DefaultCollisionBoxExtent.Y),
			SafeExtentRatio(CollisionBoxExtent.Z, DefaultCollisionBoxExtent.Z));
		FinalVisualScale *= SafePositiveScale(CollisionScale);
	}

	FinalVisualScale = SafePositiveScale(FinalVisualScale);
	SetActorScale3D(FinalVisualScale);

	if (CollisionBox)
	{
		const FVector LocalBoxExtent = FVector(
			FMath::Max(1.f, CollisionBoxExtent.X / FinalVisualScale.X),
			FMath::Max(1.f, CollisionBoxExtent.Y / FinalVisualScale.Y),
			FMath::Max(1.f, CollisionBoxExtent.Z / FinalVisualScale.Z));
		CollisionBox->SetBoxExtent(LocalBoxExtent, true);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[SlashWaveProjectile] InitConfig Damage=%.1f Speed=%.1f Distance=%.1f HitCount=%d DamageApps=%d DamageInterval=%.2f Split=%d Generation=%d CollisionExtent=%s ActorScale=%s"),
		InConfig.Damage,
		Speed,
		MaxDistance,
		MaxHitCount,
		DamageApplicationsPerTarget,
		DamageApplicationInterval,
		bSplitOnFirstHit ? 1 : 0,
		ProjectileGeneration,
		*CollisionBoxExtent.ToString(),
		*FinalVisualScale.ToString());

	InitProjectile(InSource, InConfig.Damage, InConfig.DamageEffect);
}

void ASlashWaveProjectile::InitProjectileAdvanced(
	ACharacter* InSource,
	float InDamage,
	TSubclassOf<UGameplayEffect> InDamageEffect,
	float InSpeed,
	float InMaxDistance,
	int32 InMaxHitCount,
	int32 InDamageApplicationsPerTarget,
	float InDamageApplicationInterval,
	FVector InCollisionBoxExtent,
	FName InDamageLogType,
	bool bInScaleVisualWithCollisionExtent,
	FVector InVisualScaleMultiplier)
{
	FSlashWaveProjectileRuntimeConfig Config;
	Config.Damage = InDamage;
	Config.DamageEffect = InDamageEffect;
	Config.Speed = InSpeed;
	Config.MaxDistance = InMaxDistance;
	Config.MaxHitCount = InMaxHitCount;
	Config.DamageApplicationsPerTarget = InDamageApplicationsPerTarget;
	Config.DamageApplicationInterval = InDamageApplicationInterval;
	Config.CollisionBoxExtent = InCollisionBoxExtent;
	Config.DamageLogType = InDamageLogType;
	Config.bScaleVisualWithCollisionExtent = bInScaleVisualWithCollisionExtent;
	Config.VisualScaleMultiplier = InVisualScaleMultiplier;
	InitProjectileWithConfig(InSource, Config);
}

void ASlashWaveProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ASlashWaveProjectile::OnOverlapBegin);

	RefreshLifetimeFromDistance();

	GetWorld()->GetTimerManager().SetTimer(
		LifetimeTimerHandle, this, &ASlashWaveProjectile::Expire, Lifetime, false);
}

void ASlashWaveProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRepeatTimers();
	Super::EndPlay(EndPlayReason);
}

void ASlashWaveProjectile::OnOverlapBegin(
	UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepHitResult*/)
{
	if (!bProjectileInitialized || !OtherActor || OtherActor == this || OtherActor == SourceCharacter)
	{
		return;
	}

	if (bDestroyOnWorldStaticHit
		&& OtherComp
		&& OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
	{
		Expire();
		return;
	}

	TryStartDamageSequence(OtherActor);
}

void ASlashWaveProjectile::ApplyImmediateHit(AActor* Target)
{
	if (!bProjectileInitialized || !Target || Target == this || Target == SourceCharacter)
	{
		return;
	}

	TryStartDamageSequence(Target);
}

bool ASlashWaveProjectile::TryStartDamageSequence(AActor* Target)
{
	if (!bProjectileInitialized || !Target || Target == this || Target == SourceCharacter)
	{
		return false;
	}

	if (FindHitRecordIndex(Target) != INDEX_NONE)
	{
		return false;
	}

	if (MaxHitCount > 0 && HitRecords.Num() >= MaxHitCount)
	{
		return false;
	}

	FSlashWaveHitRecord& Record = HitRecords.AddDefaulted_GetRef();
	Record.Actor = Target;

	const int32 RecordIndex = HitRecords.Num() - 1;
	ApplyDamageTickForRecord(RecordIndex);
	if (Record.AppliedCount > 0 && HitRecords.Num() == 1)
	{
		TrySplitFromFirstHit(Target);
	}
	return true;
}

void ASlashWaveProjectile::ApplyDamageTickForRecord(int32 RecordIndex)
{
	if (!HitRecords.IsValidIndex(RecordIndex))
	{
		return;
	}

	FSlashWaveHitRecord& Record = HitRecords[RecordIndex];
	AActor* Target = Record.Actor.Get();
	if (!Target || Record.AppliedCount >= DamageApplicationsPerTarget)
	{
		return;
	}

	if (!ApplyDamageTo(Target, Target->GetActorLocation()))
	{
		return;
	}

	++Record.AppliedCount;
	UE_LOG(LogTemp, Warning, TEXT("[SlashWaveProjectile] DamageTick Target=%s Count=%d/%d"),
		*GetNameSafe(Target),
		Record.AppliedCount,
		DamageApplicationsPerTarget);

	if (Record.AppliedCount < DamageApplicationsPerTarget)
	{
		if (DamageApplicationInterval > KINDA_SMALL_NUMBER && GetWorld())
		{
			FTimerDelegate RepeatDelegate;
			RepeatDelegate.BindUObject(this, &ASlashWaveProjectile::ApplyDamageTickForRecord, RecordIndex);
			GetWorld()->GetTimerManager().SetTimer(
				Record.RepeatTimerHandle,
				RepeatDelegate,
				DamageApplicationInterval,
				false);
		}
		else
		{
			ApplyDamageTickForRecord(RecordIndex);
		}
		return;
	}

	if (MaxHitCount > 0 && HitRecords.Num() >= MaxHitCount)
	{
		Expire();
	}
}

int32 ASlashWaveProjectile::FindHitRecordIndex(AActor* Target) const
{
	if (!Target)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < HitRecords.Num(); ++Index)
	{
		if (HitRecords[Index].Actor.Get() == Target)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void ASlashWaveProjectile::ClearRepeatTimers()
{
	if (!GetWorld())
	{
		return;
	}

	for (FSlashWaveHitRecord& Record : HitRecords)
	{
		GetWorld()->GetTimerManager().ClearTimer(Record.RepeatTimerHandle);
	}
}

bool ASlashWaveProjectile::ApplyDamageTo(AActor* Target, const FVector& HitLocation)
{
	if (!Target || !SourceCharacter)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);

	if (!TargetASC || !SourceASC)
	{
		return false;
	}

	FGameplayEffectContextHandle CtxHandle = SourceASC->MakeEffectContext();
	CtxHandle.AddInstigator(SourceCharacter, SourceCharacter);
	CtxHandle.AddSourceObject(this);

	if (DamageEffectClass && !bForcePureDamage)
	{
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, CtxHandle);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(SlashWaveActDamageTag(), DamageMagnitude);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
	else
	{
		UGameplayEffect* DamageGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
		DamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		FGameplayModifierInfo ModInfo;
		ModInfo.Attribute = UDamageAttributeSet::GetDamagePureAttribute();
		ModInfo.ModifierOp = EGameplayModOp::Additive;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DamageMagnitude));
		DamageGE->Modifiers.Add(ModInfo);

		FGameplayEffectSpec Spec(DamageGE, CtxHandle, 1.f);
		SourceASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC);
	}

	ApplyBonusArmorDamageTo(Target, TargetASC);
	ApplyAdditionalHitEffectTo(Target, SourceASC, TargetASC);

	UE_LOG(LogTemp, Warning, TEXT("[GA_SlashWaveCounter] Damage Target=%s Amount=%.1f CanKill=1"),
		*GetNameSafe(Target),
		DamageMagnitude);

	if (UYogAbilitySystemComponent* SourceYogASC = Cast<UYogAbilitySystemComponent>(SourceASC))
	{
		SourceYogASC->LogDamageDealt(Target, DamageMagnitude, DamageLogType);
	}

	BP_OnHitEnemy(Target, HitLocation);
	return true;
}

void ASlashWaveProjectile::ApplyBonusArmorDamageTo(AActor* Target, UAbilitySystemComponent* TargetASC) const
{
	if (!Target || !TargetASC || BonusArmorDamageMultiplier <= 0.f)
	{
		return;
	}

	const float CurrentArmor = TargetASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute());
	if (CurrentArmor <= 0.f)
	{
		return;
	}

	const float ArmorDamage = FMath::Min(CurrentArmor, DamageMagnitude * BonusArmorDamageMultiplier);
	const float NewArmor = FMath::Max(0.f, CurrentArmor - ArmorDamage);
	TargetASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), NewArmor);

	UE_LOG(LogTemp, Warning,
		TEXT("[SlashWaveProjectile] BonusArmorDamage Target=%s Armor %.1f -> %.1f Multiplier=%.2f"),
		*GetNameSafe(Target),
		CurrentArmor,
		NewArmor,
		BonusArmorDamageMultiplier);
}

void ASlashWaveProjectile::ApplyAdditionalHitEffectTo(AActor* Target, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC)
{
	if (!Target || !SourceASC || !TargetASC || !AdditionalHitEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle CtxHandle = SourceASC->MakeEffectContext();
	CtxHandle.AddInstigator(SourceCharacter, SourceCharacter);
	CtxHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(AdditionalHitEffectClass, 1.f, CtxHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	if (AdditionalHitSetByCallerTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(AdditionalHitSetByCallerTag, AdditionalHitSetByCallerValue);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void ASlashWaveProjectile::TrySplitFromFirstHit(AActor* FirstHitTarget)
{
	if (!bSplitOnFirstHit
		|| bHasSplit
		|| ProjectileGeneration >= MaxSplitGenerations
		|| SplitProjectileCount <= 0
		|| !FirstHitTarget
		|| !SourceCharacter
		|| !GetWorld()
		|| !GetClass())
	{
		return;
	}

	bHasSplit = true;

	const FVector Forward = GetActorForwardVector();
	const FVector SpawnOrigin = FirstHitTarget->GetActorLocation() + Forward * 35.f + FVector(0.f, 0.f, 35.f);
	const float Step = SplitProjectileCount > 1 ? SplitConeAngleDegrees / static_cast<float>(SplitProjectileCount - 1) : 0.f;
	const float StartYaw = SplitProjectileCount > 1 ? -SplitConeAngleDegrees * 0.5f : 0.f;

	FSlashWaveProjectileRuntimeConfig ChildConfig;
	ChildConfig.Damage = DamageMagnitude * SplitDamageMultiplier;
	ChildConfig.DamageEffect = DamageEffectClass;
	ChildConfig.Speed = Speed * SplitSpeedMultiplier;
	ChildConfig.MaxDistance = MaxDistance * SplitMaxDistanceMultiplier;
	ChildConfig.MaxHitCount = MaxHitCount;
	ChildConfig.DamageApplicationsPerTarget = 1;
	ChildConfig.DamageApplicationInterval = DamageApplicationInterval;
	ChildConfig.CollisionBoxExtent = FVector(
		FMath::Max(1.f, CollisionBoxExtent.X * SplitCollisionBoxExtentMultiplier.X),
		FMath::Max(1.f, CollisionBoxExtent.Y * SplitCollisionBoxExtentMultiplier.Y),
		FMath::Max(1.f, CollisionBoxExtent.Z * SplitCollisionBoxExtentMultiplier.Z));
	ChildConfig.bScaleVisualWithCollisionExtent = true;
	ChildConfig.VisualScaleMultiplier = VisualScaleMultiplier;
	ChildConfig.DamageLogType = TEXT("Rune_SlashWave_Split");
	ChildConfig.bDestroyOnWorldStaticHit = bDestroyOnWorldStaticHit;
	ChildConfig.bForcePureDamage = bForcePureDamage;
	ChildConfig.BonusArmorDamageMultiplier = BonusArmorDamageMultiplier;
	ChildConfig.AdditionalHitEffect = AdditionalHitEffectClass;
	ChildConfig.AdditionalHitSetByCallerTag = AdditionalHitSetByCallerTag;
	ChildConfig.AdditionalHitSetByCallerValue = AdditionalHitSetByCallerValue;
	ChildConfig.bSplitOnFirstHit = bSplitOnFirstHit;
	ChildConfig.ProjectileGeneration = ProjectileGeneration + 1;
	ChildConfig.MaxSplitGenerations = MaxSplitGenerations;
	ChildConfig.SplitProjectileCount = SplitProjectileCount;
	ChildConfig.SplitConeAngleDegrees = SplitConeAngleDegrees;
	ChildConfig.SplitDamageMultiplier = SplitDamageMultiplier;
	ChildConfig.SplitSpeedMultiplier = SplitSpeedMultiplier;
	ChildConfig.SplitMaxDistanceMultiplier = SplitMaxDistanceMultiplier;
	ChildConfig.SplitCollisionBoxExtentMultiplier = SplitCollisionBoxExtentMultiplier;

	for (int32 Index = 0; Index < SplitProjectileCount; ++Index)
	{
		const float YawOffset = StartYaw + Step * Index;
		const FVector Direction = Forward.RotateAngleAxis(YawOffset, FVector::UpVector);
		const FRotator SpawnRotation = Direction.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = SourceCharacter;
		SpawnParams.Instigator = SourceCharacter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ASlashWaveProjectile* Child = GetWorld()->SpawnActor<ASlashWaveProjectile>(
			GetClass(),
			SpawnOrigin,
			SpawnRotation,
			SpawnParams);
		if (Child)
		{
			Child->InitProjectileWithConfig(SourceCharacter, ChildConfig);
		}
	}
}

void ASlashWaveProjectile::Expire()
{
	ClearRepeatTimers();
	BP_OnExpired();
	Destroy();
}

void ASlashWaveProjectile::RefreshLifetimeFromDistance()
{
	if (MaxDistance > KINDA_SMALL_NUMBER && Speed > KINDA_SMALL_NUMBER)
	{
		Lifetime = MaxDistance / Speed;
	}
}
