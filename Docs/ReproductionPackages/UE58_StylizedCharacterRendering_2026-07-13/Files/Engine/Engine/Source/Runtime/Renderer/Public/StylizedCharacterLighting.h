// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RHIResources.h"

namespace StylizedCharacterLighting
{
	inline constexpr uint32 MaxProfiles = 8;
	inline constexpr uint32 Float4sPerProfile = 11;
	inline constexpr uint32 TotalFloat4s = MaxProfiles * Float4sPerProfile;
}

/**
 * Updates the compact character-lighting profile table consumed by deferred
 * character shading. Each profile occupies eleven FVector4f values.
 */
RENDERER_API void SetStylizedCharacterLightingProfiles(TConstArrayView<FVector4f> ProfileData, uint32 ProfileCount);

/** Copies the current profile table for deferred-light and indirect-light shader parameters. */
RENDERER_API void GetStylizedCharacterLightingProfiles(TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s>& OutProfileData, uint32& OutProfileCount);

/** Overrides material-selected profiles from a camera look volume. A zero blend disables the override. */
RENDERER_API void SetStylizedCharacterLightingProfileOverride(int32 ProfileIndex, float BlendWeight);

/** Returns the current camera-volume profile override. */
RENDERER_API void GetStylizedCharacterLightingProfileOverride(int32& OutProfileIndex, float& OutBlendWeight);

/** Sets the Curve Linear Color Atlas sampled by stylized character direct lighting. */
RENDERER_API void SetStylizedCharacterRampAtlas(FTextureRHIRef RampAtlas);

/** Returns a strong reference to the current character ramp atlas. */
RENDERER_API FTextureRHIRef GetStylizedCharacterRampAtlas();
