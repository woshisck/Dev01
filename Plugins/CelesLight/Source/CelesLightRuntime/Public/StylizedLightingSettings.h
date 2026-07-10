#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StylizedLightingSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Stylized Lighting"))
class CELESLIGHTRUNTIME_API UStylizedLightingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting")
	bool bEnableStylizedLumenLighting = true;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "1.0", ClampMax = "8.0", UIMin = "1.0", UIMax = "8.0"))
	float BandCount = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BandSoftness = 0.18f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlossInfluence = 0.65f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DirectBlend = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndirectBlend = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float SpecularIntensity = 1.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (UIMin = "-1.0", UIMax = "1.0"))
	float SpecularOffset = 0.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Scene Lighting")
	void ApplyToConsoleVariables() const;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("Stylized Lighting"); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
