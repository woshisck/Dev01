#pragma once

#include "CoreMinimal.h"

enum class EDevKitLookDevKeyType : uint8 { Directional, Point, Rect };

/** Candidate light rigs, not approved production lighting or a second material model. */
struct FDevKitCharacterLookDevPreset
{
	FString Id;
	FString Label;
	FString Layer;
	FString Intent;
	FString Basis; // Unit/geometry rationale; authored values, not measured reference-image data.
	EDevKitLookDevKeyType KeyType = EDevKitLookDevKeyType::Directional;
	float KeyIntensity = 1000.f; // Directional: lux. Point/Rect: lumens.
	FLinearColor KeyColor = FLinearColor::White;
	FVector KeyLocation = FVector(-250, -180, 320);
	FRotator KeyRotation = FRotator(-40, 35, 0);
	float FillLumens = 600.f; // Native Rect, broad low-frequency fill.
	FLinearColor FillColor = FLinearColor::White;
	FVector FillLocation = FVector(-200, 200, 240);
	FRotator FillRotation = (FVector(0, 0, 100) - FVector(-200, 200, 240)).Rotation();
	FVector2D KeySourceSizeCm = FVector2D(160, 240); // Rect width/height, centimeters.
	FVector2D FillSourceSizeCm = FVector2D(240, 240);
	float AttenuationRadiusCm = 2000.f; // Culling extent; inverse-square falloff remains active.
	float SourceRadiusCm = 8.f; // Point emitter radius, centimeters.
	float SourceAngleDegrees = 3.5f; // Directional source angular diameter.
	float SkyIntensity = 0.f; // Legacy temporary-stage field. Uncalibrated grey cubemap is disabled.
	float EV100 = 5.f;
	float FogDensity = 0.f;
	FLinearColor FogColor = FLinearColor(0.2f, 0.2f, 0.2f);
};

/** One layer/template mapping shared by the existing Level generator and LookDev. */
class YOGLOOKDEVEDITOR_API FDevKitCharacterLookDevPresets
{
public:
	static const TArray<FDevKitCharacterLookDevPreset>& GetPresets();
	static const FDevKitCharacterLookDevPreset* Find(const FString& Id);
	static FString GetLightTemplate(const FString& Layer);
	static constexpr int32 Revision = 2;
};
