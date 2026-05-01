#include "Projectile/SlashWaveProjectile.h"

#include "AbilitySystem/Attribute/DamageAttributeSet.h"
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

void ASlashWaveProjectile::InitProjectileAdvanced(
	ACharacter* InSource,
	float InDamage,
	TSubclassOf<UGameplayEffect> InDamageEffect,
	float InSpeed,
	float InMaxDistance,
	int32 InMaxHitCount,
	FVector InCollisionBoxExtent)
{
	Speed = FMath::Max(1.f, InSpeed);
	MaxDistance = FMath::Max(0.f, InMaxDistance);
	MaxHitCount = InMaxHitCount;
	CollisionBoxExtent = FVector(
		FMath::Max(1.f, InCollisionBoxExtent.X),
		FMath::Max(1.f, InCollisionBoxExtent.Y),
		FMath::Max(1.f, InCollisionBoxExtent.Z));

	if (CollisionBox)
	{
		CollisionBox->SetBoxExtent(CollisionBoxExtent, true);
	}

	InitProjectile(InSource, InDamage, InDamageEffect);
}

void ASlashWaveProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ASlashWaveProjectile::OnOverlapBegin);

	RefreshLifetimeFromDistance();

	GetWorld()->GetTimerManager().SetTimer(
		LifetimeTimerHandle, this, &ASlashWaveProjectile::Expire, Lifetime, false);
}

void ASlashWaveProjectile::OnOverlapBegin(
	UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepHitResult*/)
{
	if (!bProjectileInitialized || !OtherActor || OtherActor == this || OtherActor == SourceCharacter)
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& Weak : HitActors)
	{
		if (Weak.Get() == OtherActor)
		{
			return;
		}
	}

	if (ApplyDamageTo(OtherActor, OtherActor->GetActorLocation()))
	{
		HitActors.Add(OtherActor);
		if (MaxHitCount > 0 && HitActors.Num() >= MaxHitCount)
		{
			Expire();
		}
	}
}

void ASlashWaveProjectile::ApplyImmediateHit(AActor* Target)
{
	if (!bProjectileInitialized || !Target || Target == this || Target == SourceCharacter)
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& Weak : HitActors)
	{
		if (Weak.Get() == Target)
		{
			return;
		}
	}

	if (ApplyDamageTo(Target, Target->GetActorLocation()))
	{
		HitActors.Add(Target);
		if (MaxHitCount > 0 && HitActors.Num() >= MaxHitCount)
		{
			Expire();
		}
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

	if (DamageEffectClass)
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

	UE_LOG(LogTemp, Warning, TEXT("[GA_SlashWaveCounter] Damage Target=%s Amount=%.1f CanKill=1"),
		*GetNameSafe(Target),
		DamageMagnitude);

	BP_OnHitEnemy(Target, HitLocation);
	return true;
}

void ASlashWaveProjectile::Expire()
{
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
