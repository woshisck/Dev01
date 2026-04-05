#include "Data/EffectRegistry.h"
#include "Data/RuneDataAsset.h"

URuneDataAsset* UEffectRegistry::FindEffect(FGameplayTag EffectTag) const
{
	if (const TObjectPtr<URuneDataAsset>* Found = EffectMap.Find(EffectTag))
	{
		return Found->Get();
	}
	return nullptr;
}
