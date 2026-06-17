#include "CelesLightEncoder.h"

TArray<FLinearColor> FCelesLightEncoder::EncodeLight(const FCelesLightSourceData& Source)
{
	TArray<FLinearColor> Pixels;
	Pixels.Reserve(CelesLight::LightInfoTextureWidth);

	const bool bValid = Source.Radius > 0.0f && Source.Intensity > 0.0f;
	Pixels.Add(FLinearColor(
		static_cast<float>(Source.WorldPosition.X),
		static_cast<float>(Source.WorldPosition.Y),
		static_cast<float>(Source.WorldPosition.Z),
		bValid ? 1.0f : 0.0f));
	Pixels.Add(FLinearColor(Source.Radius, Source.Intensity, Source.bFillLight ? 1.0f : 0.0f, 0.0f));
	Pixels.Add(Source.Color);
	Pixels.Add(FLinearColor(Source.SmoothStepMin, Source.SmoothStepMax, Source.SpecularOffset, static_cast<float>(Source.EffectType)));

	return Pixels;
}

void FCelesLightEncoder::EncodeLights(TConstArrayView<FCelesLightSourceData> Sources, const int32 MaxLightCount, TArray<FLinearColor>& OutPixels)
{
	const int32 SafeMaxLightCount = FMath::Max(0, MaxLightCount);
	OutPixels.Reset(SafeMaxLightCount * CelesLight::LightInfoTextureWidth);

	for (int32 Index = 0; Index < SafeMaxLightCount; ++Index)
	{
		const TArray<FLinearColor> LightPixels = Sources.IsValidIndex(Index)
			? EncodeLight(Sources[Index])
			: TArray<FLinearColor>{EncodeInvalidLight(), EncodeInvalidLight(), EncodeInvalidLight(), EncodeInvalidLight()};

		OutPixels.Append(LightPixels);
	}
}

FLinearColor FCelesLightEncoder::EncodeInvalidLight()
{
	return FLinearColor::Black;
}
