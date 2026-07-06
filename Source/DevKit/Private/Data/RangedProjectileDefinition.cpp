#include "Data/RangedProjectileDefinition.h"

URangedProjectileDefinition::URangedProjectileDefinition()
{
	HitEventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.RangeAttack.Hit"), false);
}
