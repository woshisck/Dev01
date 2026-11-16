#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YogFogComputeCaptureLibrary.generated.h"

class UTextureRenderTargetVolume;

/** Explicit acceptance/debug captures only. Never used by the simulation's per-frame path. */
UCLASS()
class YOGVISIONEDITOR_API UYogFogComputeCaptureLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Synchronously reads an RGBA16F GPU volume and writes Field.rgba16f.raw, Metadata.json,
     * and middle XY/XZ/YZ PNGs (R plus four-channel panels).
     * The absolute directory must be a fresh child of ProjectSavedDir/Stage12/Compute.
     * This deliberately stalls rendering/GPU; do not call during performance measurements.
     */
    UFUNCTION(BlueprintCallable, Category="YOG|Fog Compute|Debug",
        meta=(DevelopmentOnly, DisplayName="捕获三维雾场（同步GPU读回）"))
    static bool CaptureVolume(UTextureRenderTargetVolume* Field, const FString& AbsoluteOutputDirectory);
};
