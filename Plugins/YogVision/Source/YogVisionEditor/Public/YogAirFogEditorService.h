#pragma once
#include "CoreMinimal.h"
#include "YogAirFogVolume.h"
#include "UObject/SoftObjectPath.h"

struct YOGVISIONEDITOR_API FYogAirFogPresetData
{
    FYogAirFogParameters Parameters;
    bool bFinishEnabled = true;
    FSoftObjectPath PaletteMaterial;
};

struct YOGVISIONEDITOR_API FYogAirFogNamedPresetInfo
{
    FString Id;
    FString DisplayName;
    FString FilePath;
    FDateTime ModifiedTime;
    bool bLegacy = false;
};

class YOGVISIONEDITOR_API FYogAirFogEditorService
{
public:
    static bool BuildAssets(FText& Message);
    static bool Preview(const FYogAirFogParameters& Parameters, AActor* Focus, bool bFinish, FText& Message);
    static void UpdatePreview(const FYogAirFogParameters& Parameters, bool bFinish);
    static void RevertPreview();
    static bool IsPreviewActive();
    static FString GetPreviewStatus();
    static UMaterialInterface* GetPaletteMaterial();
    /** Session-only identity selected by the workbench; never writes an asset. */
    static void SetPreviewPaletteMaterial(UMaterialInterface* Material);
    static void BeginSave(UWorld* World);
    static void EndSave(UWorld* World);
    static void RecoverInterruptedSaves();
    static bool Capture(const FString& Name);
    static bool SavePreset(const FYogAirFogParameters& Parameters, FText& Message);
    static bool LoadPreset(FYogAirFogParameters& Parameters, FText& Message);
    static FString GetPresetDirectory();
    static FString GetLegacyPresetId();
    static bool ValidatePresetName(const FString& Name, FText& Message);
    static bool ListNamedPresets(TArray<FYogAirFogNamedPresetInfo>& Out, FText& Message);
    static bool SaveNamedPreset(const FString& Name, const FYogAirFogPresetData& Data, bool bAllowOverwrite, FText& Message);
    static bool LoadNamedPreset(const FString& Id, FYogAirFogPresetData& Out, FText& Message);
    /** Explicit migration into a new unique name; retains ArtDraft.json. */
    static bool ImportLegacyPreset(FString& OutName, FText& Message);
};
