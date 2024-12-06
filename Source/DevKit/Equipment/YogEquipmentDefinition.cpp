#include <DevKit/Equipment/YogEquipmentDefinition.h>

#include "YogEquipmentInstance.h"

UYogEquipmentDefinition::UYogEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstanceType = UYogEquipmentInstance::StaticClass();
}