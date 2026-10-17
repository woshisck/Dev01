#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StylizedLightingSettings.h"
#include "YogShadowTransitionLibrary.generated.h"

/**
 * Retired shadow-tint compatibility API. Kept so existing Blueprint nodes load.
 *
 * Getters return neutral disabled effective parameters. Setters are inert,
 * return false where applicable, and never mutate stored compatibility data,
 * upload renderer profiles, save configuration/assets or change any CVar.
 */
UCLASS()
class CELESLIGHTRUNTIME_API UYogShadowTransitionLibrary : public UBlueprintFunctionLibrary
{

    GENERATED_BODY()

public:
    /** Returns neutral disabled parameters; stored legacy values are not used. */
    UFUNCTION(BlueprintPure, Category = "Rendering|Shadow Transition", meta = (DeprecatedFunction, DeprecationMessage = "Shadow transition tint was retired; this node returns neutral disabled parameters."))
    static FYogShadowTransitionSettings GetGlobalShadowTransition();

    /** Compatibility no-op. Does not mutate or upload the shared settings. */
    UFUNCTION(BlueprintCallable, Category = "Rendering|Shadow Transition", meta = (DeprecatedFunction, DeprecationMessage = "Shadow transition tint was retired; this node has no effect."))
    static void SetGlobalShadowTransition(const FYogShadowTransitionSettings& Settings);

    /**
     * Returns neutral disabled parameters and no override. Returns true only
     * for an existing shared index (0-7); the profile array never expands.
     */
    UFUNCTION(BlueprintPure, Category = "Rendering|Shadow Transition", meta = (DeprecatedFunction, DeprecationMessage = "Shadow transition tint was retired; this node returns neutral disabled parameters."))
    static bool GetCharacterShadowTransition(int32 ProfileIndex,
        FYogShadowTransitionSettings& OutSettings, bool& bOutOverridden);

    /**
     * Compatibility no-op. Returns false because the feature is unavailable.
     */
    UFUNCTION(BlueprintCallable, Category = "Rendering|Shadow Transition", meta = (DeprecatedFunction, DeprecationMessage = "Shadow transition tint was retired; this node has no effect and returns false."))
    static bool SetCharacterShadowTransitionOverride(int32 ProfileIndex,
        const FYogShadowTransitionSettings& Settings);

    /** Compatibility no-op; retains stored legacy values and returns false. */
    UFUNCTION(BlueprintCallable, Category = "Rendering|Shadow Transition", meta = (DeprecatedFunction, DeprecationMessage = "Shadow transition tint was retired; this node has no effect and returns false."))
    static bool ClearCharacterShadowTransitionOverride(int32 ProfileIndex);
};
