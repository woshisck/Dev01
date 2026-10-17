#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Shared physical light builder plus the retained temporary-stage compatibility API. */
class FDevKitCharacterLookDevService
{
public:
	/** Adds a serializable rig to a new /Game/Art/Map/LookDev/LightDev world; does not open or save it. */
	static bool BuildLightingLevel(UWorld* World, const FString& PresetId, FText& OutMessage);
	static bool CreateStage(const FString& PresetId, bool bUseExistingTemplate, FText& OutMessage);
	static bool ApplyPreset(const FString& PresetId, FText& OutMessage);
	static bool SetSubject(UObject* MeshAsset, FText& OutMessage);
	static bool RotateSubject(float DeltaDegrees, FText& OutMessage);
	static bool SetCamera(bool bGameplay, FText& OutMessage);
	static bool ExportReport(FText& OutMessage);
	static bool Capture(FText& OutMessage);
	static bool IsStageWorld();
	static FText GetStageDescription();
};
