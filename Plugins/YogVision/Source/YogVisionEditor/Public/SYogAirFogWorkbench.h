#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "YogAirFogVolume.h"
#include "YogAirFogWorkbenchDraft.h"
#include "YogAirFogEditorService.h"
#include "Widgets/Input/SComboBox.h"

class IDetailsView;

class YOGVISIONEDITOR_API SYogAirFogWorkbench : public SCompoundWidget, public FEditorUndoClient
{
public:
    SLATE_BEGIN_ARGS(SYogAirFogWorkbench) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SYogAirFogWorkbench() override;
    virtual bool MatchesContext(const FTransactionContext& Context, const TArray<TPair<UObject*, FTransactionObjectEvent>>& Objects) const override;
    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

    FReply PreviewAirFog();
    FReply PreviewBaseline();
    FReply PreviewWithFinish();
    FReply SaveDraft();
    FReply SaveDraftAs();
    FReply LoadDraft();
    FReply RefreshDrafts();
    void SetSaveAsName(const FString& Name);
    bool SelectSavedPreset(const FString& Id);
    FString GetCurrentPresetId() const;
    FString GetCurrentPresetFilePath() const;
    bool IsDraftDirty() const;
    FYogAirFogPresetData GetDraftSnapshot() const;
    void ApplyPreset(int32 PresetIndex);
    const FYogAirFogParameters& GetDraftParameters() const;
    /** Compatibility entry point for console callers. Fog now follows the camera. */
    static AActor* ResolveFocusActor();

private:
    FReply BuildAssets();
    FReply OpenPaletteMaterial();
    FReply RevertPreview();
    void OnDraftChanged(const FPropertyChangedEvent& Event);
    void UpdateActivePreview();
    bool PreparePalette();
    void RefreshPresetList(const FString& PreferredId = FString());
    void RememberSavedSnapshot(const FString& Id, const FYogAirFogPresetData& Data);
    FText GetDocumentStatus() const;
    FText GetSelectedPresetLabel() const;
    FText GetDirectionalGuide() const;
    FReply ImportLegacyDraft();
    FText GetCameraLabel() const;
    FText GetRuntimeStatus() const;
    TSharedRef<SWidget> MakeButton(const FText& Label, const FText& Tooltip, FOnClicked Clicked);

    TStrongObjectPtr<UYogAirFogWorkbenchDraft> Draft;
    TSharedPtr<IDetailsView> Details;
    FText Status;
    bool bBaselineMode = false;
    FString SaveAsName = TEXT("油画卷雾_继承参数");
    TArray<TSharedPtr<FYogAirFogNamedPresetInfo>> PresetOptions;
    TSharedPtr<FYogAirFogNamedPresetInfo> SelectedPreset;
    TSharedPtr<SComboBox<TSharedPtr<FYogAirFogNamedPresetInfo>>> PresetCombo;
    TMap<FString, FYogAirFogPresetData> SavedSnapshots;
};
