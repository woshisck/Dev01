#include "Data/RangedProjectileDefinition.h"

URangedProjectileDefinition::URangedProjectileDefinition()
{
	ProjectileClass = ABuffFlowProjectile::StaticClass();
	HitEventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.RangeAttack.Hit"), false);

	ProjectileConfig.Speed = 1600.f;
	ProjectileConfig.Lifetime = 1.25f;
	ProjectileConfig.CollisionBoxExtent = FVector(18.f, 18.f, 18.f);
	ProjectileConfig.bDestroyOnHitTrigger = true;
	ProjectileConfig.bDestroyOnWorldStaticHit = true;
	ProjectileConfig.BaseEffectMagnitude = 0.f;
	ProjectileConfig.CreatorAttackMagnitudeScale = 1.f;
	ProjectileConfig.CreatorAttackPowerMagnitudeScale = 0.f;
	ProjectileConfig.TriggerGameplayEventTag = HitEventTag;
	ProjectileConfig.bSendTriggerEventToCreator = true;
}
