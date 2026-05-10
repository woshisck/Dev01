#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeDilationVisualSubsystem.generated.h"

class APostProcessVolume;

/**
 * Shared screen desaturation for gameplay time dilation.
 * Call BeginTimeDilationVisual when slow motion starts and EndTimeDilationVisual when it ends.
 */
UCLASS()
class DEVKIT_API UTimeDilationVisualSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static void BeginTimeDilationVisual(const UObject* WorldContextObject);
	static void EndTimeDilationVisual(const UObject* WorldContextObject);
	static void ForceClearTimeDilationVisual(const UObject* WorldContextObject);

	void BeginVisual();
	void EndVisual();
	void ForceClearVisual();

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TObjectPtr<APostProcessVolume> DesaturationVolume;

	int32 ActiveRequestCount = 0;
	float VisualAlpha = 0.f;

	float TargetSaturation = 0.03f;
	float FadeInDuration = 0.06f;
	float FadeOutDuration = 0.12f;
	float MinimumStartAlpha = 0.35f;

	void EnsureVolume();
	void ApplyVisualState();
};
