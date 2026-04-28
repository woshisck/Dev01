#include "Projectile/SlashWaveProjectile.h"

#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

ASlashWaveProjectile::ASlashWaveProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->InitBoxExtent(FVector(30.f, 60.f, 35.f));
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

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
		ProjectileMovement->Velocity = GetActorForwardVector() * Speed;
	}
}

void ASlashWaveProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ASlashWaveProjectile::OnOverlapBegin);

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

	UGameplayEffect* DamageGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	DamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UDamageAttributeSet::GetDamagePureAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DamageMagnitude));
	DamageGE->Modifiers.Add(ModInfo);

	FGameplayEffectSpec Spec(DamageGE, CtxHandle, 1.f);
	SourceASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC);

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
