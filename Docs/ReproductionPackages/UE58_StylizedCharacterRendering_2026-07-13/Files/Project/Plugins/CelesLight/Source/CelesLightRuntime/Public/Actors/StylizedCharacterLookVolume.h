#pragma once

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"
#include "StylizedCharacterLookVolume.generated.h"

/** Spatial character-lighting look override using standard post-process volume blending. */
UCLASS(BlueprintType)
class CELESLIGHTRUNTIME_API AStylizedCharacterLookVolume : public APostProcessVolume
{
	GENERATED_BODY()

public:
	AStylizedCharacterLookVolume(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Character Look")
	bool bEnableCharacterLook = true;

	/** Index into Project Settings > Plugins > Stylized Lighting > Character Lighting Profiles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Character Look", meta = (ClampMin = "0", ClampMax = "7", UIMin = "0", UIMax = "7"))
	int32 LightingProfile = 0;
};
