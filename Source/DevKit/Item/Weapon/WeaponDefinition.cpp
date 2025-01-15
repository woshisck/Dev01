#include "WeaponDefinition.h"
#include "WeaponInstance.h"
UWeaponDefinition::UWeaponDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstanceType = UWeaponInstance::StaticClass();
}
