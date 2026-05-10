#include "Projectile/BuffFlowProjectile.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

namespace
{
	FGameplayTag DefaultActDamageTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Attribute.ActDamage")));
	}

	float ClampPositive(float Value, float Fallback)
	{
		return Value > KINDA_SMALL_NUMBER ? Value : Fallback;
	}
}

ABuffFlowProjectile::ABuffFlowProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(RuntimeConfig.CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	SetRootComponent(CollisionSphere);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = RuntimeConfig.Speed;
	ProjectileMovement->MaxSpeed = RuntimeConfig.Speed;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bRotationFollowsVelocity = true;
}

void ABuffFlowProjectile::SetCreatorForSpawn(AActor* InCreator)
{
	CreatorActor = InCreator;
	if (InCreator)
	{
		if (!GetOwner())
		{
			SetOwner(InCreator);
		}
		if (!GetInstigator())
		{
			SetInstigator(Cast<APawn>(InCreator));
		}
	}
}

void ABuffFlowProjectile::InitBuffFlowProjectile(AActor* InCreator, const FBuffFlowProjectileRuntimeConfig& InConfig)
{
	SetCreatorForSpawn(InCreator);
	RuntimeConfig = InConfig;
	RuntimeConfig.TriggerInterval = FMath::Max(0.01f, RuntimeConfig.TriggerInterval);
	RuntimeConfig.CollisionRadius = FMath::Max(1.f, RuntimeConfig.CollisionRadius);

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(RuntimeConfig.CollisionRadius, true);
		CollisionSphere->SetCollisionResponseToChannel(
			ECC_WorldStatic,
			RuntimeConfig.bDestroyOnWorldStaticHit ? ECR_Overlap : ECR_Ignore);
	}

	SnapshotCreatorAttributes();
	ResolvedLifetime = ResolveLifetime();
	ResolvedBaseSpeed = FMath::Max(0.f, RuntimeConfig.Speed);
	EffectMagnitude = RuntimeConfig.BaseEffectMagnitude
		+ CreatorAttributes.Attack * RuntimeConfig.CreatorAttackMagnitudeScale
		+ CreatorAttributes.AttackPower * RuntimeConfig.CreatorAttackPowerMagnitudeScale;
	bInitialized = CreatorActor.IsValid();

	ApplyResolvedSpeed();
	StartRuntimeTimers();
	ScheduleInitialOverlapCheck();
}

void ABuffFlowProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!CreatorActor.IsValid())
	{
		SetCreatorForSpawn(GetOwner() ? GetOwner() : GetInstigator());
	}

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABuffFlowProjectile::OnOverlapBegin);
		CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ABuffFlowProjectile::OnOverlapEnd);
	}

	if (bInitialized)
	{
		StartRuntimeTimers();
		ScheduleInitialOverlapCheck();
	}
}

void ABuffFlowProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInitialized || !ProjectileMovement)
	{
		return;
	}

	ElapsedSeconds += DeltaSeconds;
	ApplyResolvedSpeed();
}

void ABuffFlowProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(PeriodicTriggerTimerHandle);
	}

	OverlappingTargets.Reset();
	TriggerCountsByActor.Reset();
	Super::EndPlay(EndPlayReason);
}

void ABuffFlowProjectile::OnOverlapBegin(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& SweepHitResult)
{
	if (!bInitialized || !OtherActor || OtherActor == this || OtherActor == CreatorActor.Get())
	{
		return;
	}

	const FVector HitLocation = SweepHitResult.ImpactPoint.IsNearlyZero()
		? OtherActor->GetActorLocation()
		: FVector(SweepHitResult.ImpactPoint);

	if (RuntimeConfig.bDestroyOnWorldStaticHit
		&& OtherComp
		&& OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
	{
		BP_OnWorldHit(HitLocation);
		Expire();
		return;
	}

	if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		return;
	}

	AddOverlappingTarget(OtherActor);

	if (RuntimeConfig.TriggerMode == EBuffFlowProjectileTriggerMode::HitOnce)
	{
		TriggerEffectAt(OtherActor, HitLocation);
		if (RuntimeConfig.bDestroyOnHitTrigger && !IsActorBeingDestroyed())
		{
			Destroy();
		}
	}
}

void ABuffFlowProjectile::OnOverlapEnd(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	RemoveOverlappingTarget(OtherActor);
}

void ABuffFlowProjectile::SnapshotCreatorAttributes()
{
	CreatorAttributes = FBuffFlowProjectileAttributeSnapshot();

	UAbilitySystemComponent* CreatorASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CreatorActor.Get());
	if (!CreatorASC)
	{
		return;
	}

	CreatorAttributes.Attack = CreatorASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
	CreatorAttributes.AttackPower = CreatorASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());
	CreatorAttributes.Health = CreatorASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
	CreatorAttributes.MaxHealth = CreatorASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	CreatorAttributes.Armor = CreatorASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute());
	CreatorAttributes.MaxArmor = CreatorASC->GetNumericAttribute(UBaseAttributeSet::GetMaxArmorHPAttribute());
	CreatorAttributes.MoveSpeed = CreatorASC->GetNumericAttribute(UBaseAttributeSet::GetMoveSpeedAttribute());
}

float ABuffFlowProjectile::GetCurveInputValue(const EBuffFlowProjectileCurveInput Input, const float ConstantInput) const
{
	switch (Input)
	{
	case EBuffFlowProjectileCurveInput::CreatorAttack:
		return CreatorAttributes.Attack;
	case EBuffFlowProjectileCurveInput::CreatorAttackPower:
		return CreatorAttributes.AttackPower;
	case EBuffFlowProjectileCurveInput::CreatorHealth:
		return CreatorAttributes.Health;
	case EBuffFlowProjectileCurveInput::CreatorMaxHealth:
		return CreatorAttributes.MaxHealth;
	case EBuffFlowProjectileCurveInput::CreatorArmor:
		return CreatorAttributes.Armor;
	case EBuffFlowProjectileCurveInput::CreatorMaxArmor:
		return CreatorAttributes.MaxArmor;
	case EBuffFlowProjectileCurveInput::CreatorMoveSpeed:
		return CreatorAttributes.MoveSpeed;
	case EBuffFlowProjectileCurveInput::Constant:
	default:
		return ConstantInput;
	}
}

float ABuffFlowProjectile::ResolveLifetime() const
{
	if (RuntimeConfig.LifetimeCurve)
	{
		const float InputValue = GetCurveInputValue(
			RuntimeConfig.LifetimeCurveInput,
			RuntimeConfig.LifetimeCurveConstantInput);
		return ClampPositive(RuntimeConfig.LifetimeCurve->GetFloatValue(InputValue), RuntimeConfig.Lifetime);
	}

	return ClampPositive(RuntimeConfig.Lifetime, 1.f);
}

float ABuffFlowProjectile::ResolveSpeed() const
{
	if (!RuntimeConfig.SpeedOverLifeCurve || ResolvedLifetime <= KINDA_SMALL_NUMBER)
	{
		return RuntimeConfig.Speed;
	}

	const float NormalizedTime = FMath::Clamp(ElapsedSeconds / ResolvedLifetime, 0.f, 1.f);
	const float CurveValue = RuntimeConfig.SpeedOverLifeCurve->GetFloatValue(NormalizedTime);
	if (RuntimeConfig.SpeedCurveMode == EBuffFlowProjectileSpeedCurveMode::SpeedMultiplier)
	{
		return ResolvedBaseSpeed * CurveValue;
	}

	return CurveValue;
}

void ABuffFlowProjectile::ApplyResolvedSpeed()
{
	if (!ProjectileMovement)
	{
		return;
	}

	const float NewSpeed = FMath::Max(0.f, ResolveSpeed());
	FVector Direction = ProjectileMovement->Velocity.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector().GetSafeNormal();
	}
	if (Direction.IsNearlyZero())
	{
		return;
	}

	ProjectileMovement->InitialSpeed = NewSpeed;
	ProjectileMovement->MaxSpeed = NewSpeed;
	ProjectileMovement->Velocity = Direction * NewSpeed;
	ProjectileMovement->UpdateComponentVelocity();
}

void ABuffFlowProjectile::StartRuntimeTimers()
{
	if (!GetWorld() || IsActorBeingDestroyed())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		LifetimeTimerHandle,
		this,
		&ABuffFlowProjectile::Expire,
		ResolvedLifetime,
		false);

	if (RuntimeConfig.TriggerMode == EBuffFlowProjectileTriggerMode::PeriodicOverlap)
	{
		GetWorld()->GetTimerManager().ClearTimer(PeriodicTriggerTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			PeriodicTriggerTimerHandle,
			this,
			&ABuffFlowProjectile::HandlePeriodicTrigger,
			RuntimeConfig.TriggerInterval,
			true);
	}
}

void ABuffFlowProjectile::ScheduleInitialOverlapCheck()
{
	if (bInitialOverlapCheckScheduled || !bInitialized || !HasActorBegunPlay() || !GetWorld())
	{
		return;
	}

	bInitialOverlapCheckScheduled = true;
	HandleInitialOverlaps();

	if (!IsActorBeingDestroyed() && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ABuffFlowProjectile::HandleInitialOverlaps));
	}
}

void ABuffFlowProjectile::HandleInitialOverlaps()
{
	if (!bInitialized || !CollisionSphere || IsActorBeingDestroyed())
	{
		return;
	}

	CollisionSphere->UpdateOverlaps();

	TArray<AActor*> Actors;
	CollisionSphere->GetOverlappingActors(Actors);
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor == this || Actor == CreatorActor.Get())
		{
			continue;
		}

		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
		{
			continue;
		}

		AddOverlappingTarget(Actor);

		if (RuntimeConfig.TriggerMode == EBuffFlowProjectileTriggerMode::HitOnce)
		{
			TriggerEffectAt(Actor, Actor->GetActorLocation());
			if (RuntimeConfig.bDestroyOnHitTrigger && !IsActorBeingDestroyed())
			{
				Destroy();
			}
			break;
		}
	}
}

void ABuffFlowProjectile::HandlePeriodicTrigger()
{
	if (!bInitialized || IsActorBeingDestroyed())
	{
		return;
	}

	TArray<AActor*> Targets;
	Targets.Reserve(OverlappingTargets.Num());
	for (int32 Index = OverlappingTargets.Num() - 1; Index >= 0; --Index)
	{
		AActor* Target = OverlappingTargets[Index].Get();
		if (!Target || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
		{
			OverlappingTargets.RemoveAtSwap(Index);
			continue;
		}
		Targets.Add(Target);
	}

	TriggerEffectForTargets(Targets, GetActorLocation());
}

void ABuffFlowProjectile::TriggerEffectAt(AActor* DirectTarget, const FVector& TriggerLocation)
{
	TArray<AActor*> Targets;
	if (RuntimeConfig.EffectRadius > KINDA_SMALL_NUMBER && GetWorld())
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BuffFlowProjectileEffectRadius), false, this);
		GetWorld()->OverlapMultiByObjectType(
			Overlaps,
			TriggerLocation,
			FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn),
			FCollisionShape::MakeSphere(RuntimeConfig.EffectRadius),
			QueryParams);

		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Target = Overlap.GetActor();
			if (Target && Target != this && Target != CreatorActor.Get())
			{
				Targets.AddUnique(Target);
			}
		}
	}
	else if (DirectTarget)
	{
		Targets.Add(DirectTarget);
	}

	TriggerEffectForTargets(Targets, TriggerLocation);
}

void ABuffFlowProjectile::TriggerEffectForTargets(const TArray<AActor*>& Targets, const FVector& TriggerLocation)
{
	for (AActor* Target : Targets)
	{
		if (!CanTriggerTarget(Target))
		{
			continue;
		}

		if (ApplyEffectToTarget(Target, TriggerLocation))
		{
			int32& TriggerCount = TriggerCountsByActor.FindOrAdd(TWeakObjectPtr<AActor>(Target));
			++TriggerCount;
			BP_OnEffectTriggered(Target, TriggerLocation);
			SendTriggerGameplayEvent(Target, TriggerLocation);
		}
	}
}

bool ABuffFlowProjectile::ApplyEffectToTarget(AActor* Target, const FVector& TriggerLocation)
{
	if (!Target || Target == CreatorActor.Get())
	{
		return false;
	}

	UAbilitySystemComponent* CreatorASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CreatorActor.Get());
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!CreatorASC || !TargetASC)
	{
		return false;
	}

	FGameplayEffectContextHandle CtxHandle = CreatorASC->MakeEffectContext();
	CtxHandle.AddInstigator(Cast<APawn>(CreatorActor.Get()), CreatorActor.Get());
	CtxHandle.AddSourceObject(this);

	if (RuntimeConfig.EffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = CreatorASC->MakeOutgoingSpec(RuntimeConfig.EffectClass, 1.f, CtxHandle);
		if (!SpecHandle.IsValid())
		{
			return false;
		}

		const FGameplayTag MagnitudeTag = RuntimeConfig.SetByCallerMagnitudeTag.IsValid()
			? RuntimeConfig.SetByCallerMagnitudeTag
			: DefaultActDamageTag();
		if (MagnitudeTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(MagnitudeTag, EffectMagnitude);
		}

		CreatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		return true;
	}

	if (!RuntimeConfig.bApplyPureDamageFallback)
	{
		return RuntimeConfig.TriggerGameplayEventTag.IsValid();
	}

	UGameplayEffect* DamageGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	DamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UDamageAttributeSet::GetDamagePureAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(EffectMagnitude));
	DamageGE->Modifiers.Add(ModInfo);

	FGameplayEffectSpec Spec(DamageGE, CtxHandle, 1.f);
	CreatorASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC);
	return true;
}

void ABuffFlowProjectile::SendTriggerGameplayEvent(AActor* Target, const FVector& TriggerLocation) const
{
	if (!RuntimeConfig.TriggerGameplayEventTag.IsValid() || !CreatorActor.IsValid() || !Target)
	{
		return;
	}

	UAbilitySystemComponent* CreatorASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CreatorActor.Get());
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* EventASC = RuntimeConfig.bSendTriggerEventToCreator ? CreatorASC : TargetASC;
	if (!CreatorASC || !EventASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = RuntimeConfig.TriggerGameplayEventTag;
	Payload.Instigator = CreatorActor.Get();
	Payload.Target = Target;
	Payload.OptionalObject = const_cast<ABuffFlowProjectile*>(this);
	Payload.EventMagnitude = EffectMagnitude;
	Payload.ContextHandle = CreatorASC->MakeEffectContext();
	Payload.ContextHandle.AddInstigator(Cast<APawn>(CreatorActor.Get()), CreatorActor.Get());
	Payload.ContextHandle.AddSourceObject(const_cast<ABuffFlowProjectile*>(this));
	EventASC->HandleGameplayEvent(RuntimeConfig.TriggerGameplayEventTag, &Payload);
}

void ABuffFlowProjectile::SendExpireGameplayEvent() const
{
	if (!RuntimeConfig.ExpireGameplayEventTag.IsValid() || !CreatorActor.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* CreatorASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CreatorActor.Get());
	if (!CreatorASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = RuntimeConfig.ExpireGameplayEventTag;
	Payload.Instigator = CreatorActor.Get();
	Payload.Target = CreatorActor.Get();
	Payload.OptionalObject = const_cast<ABuffFlowProjectile*>(this);
	Payload.EventMagnitude = EffectMagnitude;
	Payload.ContextHandle = CreatorASC->MakeEffectContext();
	Payload.ContextHandle.AddInstigator(Cast<APawn>(CreatorActor.Get()), CreatorActor.Get());
	Payload.ContextHandle.AddSourceObject(const_cast<ABuffFlowProjectile*>(this));
	CreatorASC->HandleGameplayEvent(RuntimeConfig.ExpireGameplayEventTag, &Payload);
}

bool ABuffFlowProjectile::CanTriggerTarget(AActor* Target) const
{
	if (!Target || Target == this || Target == CreatorActor.Get())
	{
		return false;
	}

	if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		return false;
	}

	if (RuntimeConfig.MaxTriggersPerTarget <= 0)
	{
		return true;
	}

	const int32* TriggerCount = TriggerCountsByActor.Find(TWeakObjectPtr<AActor>(Target));
	return !TriggerCount || *TriggerCount < RuntimeConfig.MaxTriggersPerTarget;
}

void ABuffFlowProjectile::AddOverlappingTarget(AActor* Target)
{
	if (CanTriggerTarget(Target))
	{
		OverlappingTargets.AddUnique(TWeakObjectPtr<AActor>(Target));
	}
}

void ABuffFlowProjectile::RemoveOverlappingTarget(AActor* Target)
{
	OverlappingTargets.Remove(TWeakObjectPtr<AActor>(Target));
}

void ABuffFlowProjectile::Expire()
{
	if (IsActorBeingDestroyed())
	{
		return;
	}

	SendExpireGameplayEvent();
	BP_OnExpired();
	Destroy();
}
