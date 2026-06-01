#include "Data/MontageVFXBindingDataAsset.h"

const FMontageVFXBindingConfig* UMontageVFXBindingDataAsset::ResolveBinding(FName SlotName) const
{
	if (!SlotName.IsNone())
	{
		for (const FMontageVFXBindingAssetEntry& Entry : Bindings)
		{
			if (Entry.SlotName == SlotName)
			{
				return &Entry.Config;
			}
		}
	}

	return bUseFallbackConfig ? &FallbackConfig : nullptr;
}
