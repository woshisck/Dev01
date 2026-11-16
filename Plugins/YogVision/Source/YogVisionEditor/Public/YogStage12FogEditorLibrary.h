#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YogStage12FogEditorLibrary.generated.h"
class UMaterialInterface;

UCLASS()
class YOGVISIONEDITOR_API UYogStage12FogEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Stage12 Fog")
    static bool BuildStage12Material();
    UFUNCTION(BlueprintCallable, Category="Stage12 Fog")
    static bool BuildNativeAtlasFogMaterial();
    UFUNCTION(BlueprintCallable, Category="Stage12 Fog")
    static bool BuildComputeFogMaterial();
    /** Local research reference only; registers a staged read-only FluidNinja package root. */
    UFUNCTION(BlueprintCallable, Category="Stage12 Fog")
    static UMaterialInterface* LoadNativeFogReference(const FString& StagedFluidNinjaRoot);
};
