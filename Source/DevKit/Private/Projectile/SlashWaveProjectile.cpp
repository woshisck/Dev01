#include "Projectile/SlashWaveProjectile.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

// SetByCaller Tag：与现有伤害管线保持一致（GE_SlashWaveDamage 须配置同一 Tag 的 Modifier）
static const FGameplayTag TAG_SlashActDamage =
	FGameplayTag::RequestGameplayTag(FName("Attribute.ActDamage"));

ASlashWaveProjectile::ASlashWaveProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// ── 碰撞体：扁平水平盒（刀光横截面），只与 Pawn 发生 Overlap ──
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->InitBoxExtent(FVector(30.f, 60.f, 35.f)); // 前后30，左右60，高35
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(CollisionBox);

	// ── 投射物移动：水平匀速，无重力，不弹射 ──────────────────────────────
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed             = 1400.f;
	ProjectileMovement->MaxSpeed                 = 1400.f;
	ProjectileMovement->ProjectileGravityScale   = 0.f;
	ProjectileMovement->bShouldBounce            = false;
	ProjectileMovement->bRotationFollowsVelocity = false;
}

void ASlashWaveProjectile::InitProjectile(ACharacter* InSource, float InDamage,
                                          TSubclassOf<UGameplayEffect> InDamageEffect)
{
	SourceCharacter   = InSource;
	DamageMagnitude   = InDamage;
	DamageEffectClass = InDamageEffect;

	// 覆写默认速度（子类 BP 可在 Class Defaults 中调整 Speed，InitProjectile 后同步给 PM）
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed     = Speed;
		ProjectileMovement->Velocity     = GetActorForwardVector() * Speed;
	}
}

void ASlashWaveProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ASlashWaveProjectile::OnOverlapBegin);

	// 生存计时器：到期后触发 Expire
	GetWorld()->GetTimerManager().SetTimer(
		LifetimeTimerHandle, this, &ASlashWaveProjectile::Expire, Lifetime, false);
}

void ASlashWaveProjectile::OnOverlapBegin(
	UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& SweepHitResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == SourceCharacter)
		return;

	// 穿透检测：已命中过的目标跳过
	for (const TWeakObjectPtr<AActor>& Weak : HitActors)
	{
		if (Weak.Get() == OtherActor) return;
	}
	HitActors.Add(OtherActor);

	const FVector HitLoc = SweepHitResult.ImpactPoint.IsNearlyZero()
		? OtherActor->GetActorLocation()
		: FVector(SweepHitResult.ImpactPoint);
	ApplyDamageTo(OtherActor, HitLoc);
}

void ASlashWaveProjectile::ApplyDamageTo(AActor* Target, const FVector& HitLocation)
{
	if (!Target || !DamageEffectClass || !SourceCharacter)
		return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);

	if (!TargetASC || !SourceASC)
		return;

	FGameplayEffectContextHandle CtxHandle = SourceASC->MakeEffectContext();
	CtxHandle.AddInstigator(SourceCharacter, SourceCharacter);
	CtxHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, CtxHandle);

	if (SpecHandle.IsValid())
	{
		// SetByCaller：GE_SlashWaveDamage 须配置 Attribute.ActDamage Modifier（Type=SetByCaller）
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_SlashActDamage, DamageMagnitude);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}

	BP_OnHitEnemy(Target, HitLocation);
}

void ASlashWaveProjectile::Expire()
{
	BP_OnExpired();
	Destroy();
}