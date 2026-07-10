#include "StylizedLightingSettings.h"

#include "HAL/IConsoleManager.h"

namespace
{
	void SetConsoleVariable(const TCHAR* Name, const int32 Value)
	{
		if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			ConsoleVariable->Set(Value, ECVF_SetByProjectSetting);
		}
	}

	void SetConsoleVariable(const TCHAR* Name, const float Value)
	{
		if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			ConsoleVariable->Set(Value, ECVF_SetByProjectSetting);
		}
	}
}

void UStylizedLightingSettings::ApplyToConsoleVariables() const
{
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.Enable"), bEnableStylizedLumenLighting ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandCount"), BandCount);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandSoftness"), BandSoftness);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.GlossInfluence"), GlossInfluence);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.DirectBlend"), DirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.IndirectBlend"), IndirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularIntensity"), SpecularIntensity);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularOffset"), SpecularOffset);
}

#if WITH_EDITOR
void UStylizedLightingSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyToConsoleVariables();
}
#endif
