#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YogPlanarFogEditorLibrary.generated.h"

/** Editor-only generation of the independent planar-field native fog material. */
UCLASS()
class YOGVISIONEDITOR_API UYogPlanarFogEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Rebuild and save /YogVision/Planar/M_PlanarVolumeFog; never saves its input textures. */
    UFUNCTION(BlueprintCallable, Category="YogVision|Planar Fog")
    static bool BuildPlanarFogMaterial();
    UFUNCTION(BlueprintCallable, Category="YogVision|Planar Fog")
    static bool BuildPlanarFogFinish();
};
