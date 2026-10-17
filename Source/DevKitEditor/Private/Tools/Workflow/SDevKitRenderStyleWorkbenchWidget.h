#pragma once

#include "CoreMinimal.h"
#include "Tools/Workflow/DevKitCloudLightingFinishTrial.h"
#include "RenderStyle/YogRenderStyleSettings.h"
#include "StylizedLightingSettings.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class FConfigCacheIni;

/** Cloud lighting controls and reversible finishing preview. Never implicitly saves assets. */
class SDevKitRenderStyleWorkbenchWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDevKitRenderStyleWorkbenchWidget) {} SLATE_END_ARGS()
    void Construct(const FArguments& InArgs);
    virtual ~SDevKitRenderStyleWorkbenchWidget() override;
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
    static bool IsCharacterPropertyVisible(FName PropertyName) { return false; }

private:
    friend class FDevKitCloudFinishWidgetTest;
    friend class FDevKitCharacterShadowWorkbenchTest;
    friend class FDevKitRenderStyleDraftTest;
    friend class FDevKitRenderStylePresetSaveTest;
    static bool SavePresetTableToConfig(UYogRenderStyleSettings* Settings, const FString& ConfigPath, FConfigCacheIni* ConfigSystem = nullptr);
    static bool SaveCloudLightingDefaultsToConfig(UStylizedLightingSettings* Settings, const FString& ConfigPath, FConfigCacheIni* ConfigSystem = nullptr);
    TSharedRef<SWidget> MakeLightingControls();
    TSharedRef<SWidget> MakeCharacterShadowControls();
    TSharedRef<SWidget> MakeMetalArtControls();
    TSharedRef<SWidget> MakeReflectionAlphaControls();
    FStylizedMetalArtSettings& EditMetalArt() const;
    FReply SaveMetalArtDefaults();
    TSharedPtr<class SDevKitWorkbenchColorPicker> MetalColorPicker;
    int32 MetalProfileIndex = -1;
    TArray<TSharedPtr<FString>> MetalProfileOptions;
    void ApplyCharacterShadowChanges();
    FReply SaveCharacterShadowDefaults();
    static bool SetSavedDefaultNPRMetal(TArray<FString>& Profiles, bool bEnabled);
    TSharedRef<SWidget> MakeFinishControls();
    TSharedRef<SWidget> MakeFinishParameterRow(const TCHAR* Label, const TCHAR* Help, float* Value, float Min, float Max, float Delta);
    TSharedRef<SWidget> MakeLayerParameterRow(const TCHAR* Label, const TCHAR* Help, float* Value, float Min, float Max, float Delta);
    TSharedRef<SWidget> MakePreviewLinks();
    void RefreshPresets();
    void LoadBuiltInPreset(bool bDisableScene);
    void OnPresetSelected(TSharedPtr<FString> Name, ESelectInfo::Type SelectInfo);
    FReply ResetDraft();
    FReply ApplyDraft();
    FReply ApplyFinishTrial();
    FReply RestoreFinishTrial();
    void ReadSceneDraft();
    FText GetFinishStatus() const;
    FReply SaveProjectDefaults();
    FReply SavePreset();
    FText GetSourceText() const;
    FText GetAppliedText() const;
    void MarkDraftChanged(bool bKeepPresetSelection = false);

    bool bDraftEnabled = true;
    float DraftBands = 2.f;
    float DraftSoftness = .18f;
    float DraftGlossInfluence = .65f;
    float DraftSpecularIntensity = 1.f;
    float DraftSpecularOffset = 0.f;
    float DraftIndirectBlend = 1.f;
    float DraftDirectBlend = 1.f;
    FDevKitCloudLightingFinishParameters FinishParameters;
    bool bLastFinishActive = false;
    TSharedPtr<SEditableTextBox> PresetName;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> PresetCombo;
    TArray<TSharedPtr<FString>> PresetNames;
    TSharedPtr<FString> SelectedPreset;
    bool bHasPendingChanges = false;
    FString Status;
};
