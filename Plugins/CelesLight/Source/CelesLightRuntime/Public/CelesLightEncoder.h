#pragma once

#include "CelesLightTypes.h"

class CELESLIGHTRUNTIME_API FCelesLightEncoder
{
public:
	static TArray<FLinearColor> EncodeLight(const FCelesLightSourceData& Source);
	static void EncodeLights(TConstArrayView<FCelesLightSourceData> Sources, int32 MaxLightCount, TArray<FLinearColor>& OutPixels);
	static FLinearColor EncodeInvalidLight();
};
