#include "Data/EffectRegistry.h"
#include "Data/YogBuffDefinition.h"

UYogBuffDefinition* UEffectRegistry::FindEffect(FGameplayTag EffectTag) const
{
	if (const TObjectPtr<UYogBuffDefinition>* Found = EffectMap.Find(EffectTag))
	{
		return Found->Get();
	}
	return nullptr;
}
