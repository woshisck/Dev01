#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Relative, editor-session-only finish adjustments on top of the user's current lighting. */
struct FDevKitCloudLightingFinishParameters
{
    float SceneSpecularScale = .9f;
    float SceneSpecularOffsetDelta = .005f;
    float CharacterSpecularScale = .85f;
    float CharacterReflectionScale = .9f;
    float CharacterOcclusionScale = .85f;
    float CharacterProjectedShadowScale = .9f;
};

/**
 * Reversible shared preview. The first Apply captures only the fields it owns;
 * subsequent Apply calls derive from that snapshot, never from the last trial.
 * No configuration, material, map, transaction or package is written.
 */
class FDevKitCloudLightingFinishTrial
{
public:
    static void Apply(UWorld* World, const FDevKitCloudLightingFinishParameters& Parameters);
    static void Restore();
    static bool IsActive();
    /** Last sanitized, currently applied trial values; default values when inactive. */
    static FDevKitCloudLightingFinishParameters GetParameters();
};
