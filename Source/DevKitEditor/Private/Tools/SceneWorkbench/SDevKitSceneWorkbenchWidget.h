#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Data/RoomDataAsset.h"
#include "LevelEnvironment/YogLevelEnvironment.h"
#include "RenderStyle/YogRenderStyleSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IPropertyRowGenerator;
class SDevKitSceneRoomConfigWidget;
class UStylizedLightingSettings;
class SBox;
struct FPropertyChangedEvent;

struct FDevKitSceneAssetRow
{
    FAssetData Map;
    TArray<TWeakObjectPtr<URoomDataAsset>> Rooms;
    FString Name;
    FString Path;
};

/** One asset explorer for map packages, room variants and level environments. */
class SDevKitSceneWorkbenchWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDevKitSceneWorkbenchWidget) {}
    SLATE_END_ARGS()
    void Construct(const FArguments& InArgs);
    virtual ~SDevKitSceneWorkbenchWidget() override;
    static bool DoesRoomMatchMap(const URoomDataAsset* Room, const FAssetData& Map, bool bUniqueLegacyName);
    static const FString& GetMapDataRoot();
    /** Pure path filter shared by the resource scan and map-package creation. */
    static bool IsPrimaryMapPackage(const FString& PackageName);

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FSceneWorkbenchNativeTest;
    friend class FSceneEnvironmentInheritanceTest;
#endif
    using FRow = TSharedPtr<FDevKitSceneAssetRow>;
    void RefreshAssets();
    void FilterAssets();
    void SelectAsset(FRow Row, ESelectInfo::Type);
    void SelectRoom(TSharedPtr<TWeakObjectPtr<URoomDataAsset>> Room, ESelectInfo::Type);
    TSharedRef<ITableRow> MakeAssetRow(FRow Row, const TSharedRef<STableViewBase>& Owner);
    void RebuildRightPanel();
    void LoadEnvironmentDraft();
    void SetEnvironmentOverride(bool bOverride);
    void RestoreGlobalEnvironment();
    void SetNativeEnvironment();
    bool CanEditEnvironmentValues() const;
    FYogLevelEnvironmentSettings GetEnvironmentDraftSettings() const;
    FText GetEnvironmentSourceText() const;
    void OnGlobalEnvironmentChanged(UObject* Object, FPropertyChangedEvent& Event);
    bool ConfirmDiscardEnvironmentDraft();
    void QueueEnvironmentRefresh();
    TSharedRef<SWidget> MakeNewMapPanel();
    TSharedRef<SWidget> MakeSublevelsPanel();
    TSharedRef<SWidget> MakeEnvironmentPanel();
    FReply OpenSelectedMap();
    FReply SaveSelected();
    FReply CreateRoom();
    FReply AssociateRoom();
    FReply ApplyEnvironment();
    FReply SaveEnvironmentPreset();
    FReply ApplyNativeStyle();
    TSharedRef<SWidget> MakeNativeStylePanel();
    UWorld* GetSelectedEditorWorld() const;
    FString GetSelectedPackage() const;

    TArray<FRow> AllRows, VisibleRows;
    TArray<TStrongObjectPtr<URoomDataAsset>> LoadedRooms;
    TArray<TSharedPtr<TWeakObjectPtr<URoomDataAsset>>> RoomVariants;
    TSharedPtr<SListView<FRow>> AssetList;
    TSharedPtr<SBox> RightPanel;
    TSharedPtr<SDevKitSceneRoomConfigWidget> RoomForm;
    TStrongObjectPtr<UYogLevelEnvironmentComponent> EnvironmentDraft;
    // The read-only effective display must not overwrite dormant level values.
    TStrongObjectPtr<UYogLevelEnvironmentComponent> EnvironmentValues;
    TSharedPtr<IPropertyRowGenerator> EnvironmentGenerator;
    TSharedPtr<IPropertyRowGenerator> EnvironmentValuesGenerator;
    TWeakObjectPtr<AYogLevelEnvironment> EnvironmentEditedActor;
    TWeakObjectPtr<UStylizedLightingSettings> GlobalSettingsObject;
    FDelegateHandle GlobalEnvironmentChangedHandle;
    FRow Selected;
    TWeakObjectPtr<URoomDataAsset> SelectedRoom;
    FString Search, LayerFilter;
    FString CreateRoot = TEXT("/Game/Art/Map/Map_Data");
    FString NewSuffix;
    FString NewLayer = TEXT("L1");
    FString NewType = TEXT("CommonLevel");
    FString SublevelFolder;
    FString ExistingSublevel;
    int32 ActivePage = 0;
    bool bCreatingMap = false;
    bool bEnvironmentDraftDirty = false;
    FYogNativeRenderStyleSettings NativeStyleDraft;
    TWeakObjectPtr<AYogRenderStyleActor> NativeStyleActor;
    bool bOverrideNativeStyleDraft = false;
    bool bNativeStyleDraftDirty = false;
    int32 NativeStylePriorityDraft = 0;
    bool bEnvironmentRefreshQueued = false;
    FText Status;
};
