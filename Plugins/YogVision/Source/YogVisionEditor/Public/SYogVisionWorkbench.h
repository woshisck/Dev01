#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "UObject/StrongObjectPtr.h"
#include "EditorUndoClient.h"
#include "YogVisionSettings.h"
#include "YogVisionWorkbenchDraft.h"

class UYogVisionWorkbenchDraft;
class UYogVisionSubsystem;
class IDetailsView;
class SVerticalBox;
class UActorComponent;
class UWorld;

class YOGVISIONEDITOR_API SYogVisionWorkbench : public SCompoundWidget, public FEditorUndoClient
{
public:
    SLATE_BEGIN_ARGS(SYogVisionWorkbench) {}
        SLATE_EVENT(FSimpleDelegate, OnBuildAssets)
        SLATE_EVENT(FSimpleDelegate, OnCreateStage)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SYogVisionWorkbench() override;
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

    const FYogVisionParameters& GetDraftParameters() const;
    void ApplyPreset(int32 PresetIndex);

private:
    UWorld* GetEditorWorld() const;
    AActor* GetSelectedActor() const;
    AActor* ResolvePreviewObserver(UWorld* World);
    FReply PreviewCurrentLevel();
    FReply ApplyToPIE();
    FReply RevertPreview();
    FReply SaveProjectDefaults();
    FReply ReloadProjectDefaults();
    FReply BuildAssets();
    FReply CreateStage();
    FReply AddSelectedComponent(UClass* ComponentClass);
    FReply RefreshReadback();
    void OnDraftChanged(const FPropertyChangedEvent& Event);
    void UpdateActivePreviews();
    void CleanupPreviews();
    void UpdateSelectionDetails();
    FText GetRuntimeStatus() const;
    FText GetSceneReadback() const;
    FText GetSelectionLabel() const;
    TSharedRef<SWidget> MakeButton(const FText& Label, const FText& Tooltip, FOnClicked Clicked);

    TStrongObjectPtr<UYogVisionWorkbenchDraft> Draft;
    TSharedPtr<IDetailsView> DraftDetails;
    TSharedPtr<IDetailsView> SelectionDetails;
    TWeakObjectPtr<AActor> InspectedActor;
    TArray<TWeakObjectPtr<UActorComponent>> InspectedComponents;
    TWeakObjectPtr<AActor> TransientObserver;
    TArray<TWeakObjectPtr<UYogVisionSubsystem>> PreviewSubsystems;
    FSimpleDelegate BuildAssetsDelegate;
    FSimpleDelegate CreateStageDelegate;
    FText Status;
    FText SceneReadback;
    float ReadbackAccumulator = 0.f;
};
