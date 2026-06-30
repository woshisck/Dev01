#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class FAssetThumbnail;
class FAssetThumbnailPool;
class SModelAssetComplianceViewport;
class SSearchBox;
class UStaticMesh;

enum class EModelAssetComplianceCategory : uint8
{
	Prop,
	Interactive,
	Character,
	Weapon,
	SceneProp,
	SceneBuilding,
	SceneGround,
	SceneOther,
	Decal,
	OtherAsset
};

FORCEINLINE uint32 GetTypeHash(EModelAssetComplianceCategory Category)
{
	return static_cast<uint32>(Category);
}

enum class EModelAssetComplianceStatus : uint8
{
	Pass,
	Warning,
	Blocked,
	Excluded
};

struct FModelAssetComplianceItem
{
	FAssetData AssetData;
	TWeakObjectPtr<UStaticMesh> Mesh;
	FString AssetName;
	FString PackagePath;
	EModelAssetComplianceCategory Category = EModelAssetComplianceCategory::OtherAsset;
	EModelAssetComplianceStatus Status = EModelAssetComplianceStatus::Blocked;
	bool bHasStatusOverride = false;
	EModelAssetComplianceStatus StatusOverride = EModelAssetComplianceStatus::Pass;
	int32 LODCount = 0;
	int32 MaterialSlotCount = 0;
	bool bHasSimpleCollision = false;
	int32 SimpleCollisionShapeCount = 0;
	int32 LOD0Triangles = 0;
	int32 LOD1Triangles = 0;
	TArray<FText> Blockers;
	TArray<FText> Warnings;
	TArray<FText> Infos;
	TSharedPtr<FAssetThumbnail> Thumbnail;
};

struct FModelAssetComplianceTreeNode
{
	FString DisplayName;
	FString FullPath;
	bool bIsFolder = false;
	TSharedPtr<FModelAssetComplianceItem> AssetItem;
	TArray<TSharedPtr<FModelAssetComplianceTreeNode>> Children;
};

class SModelAssetComplianceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SModelAssetComplianceWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FComplianceItemPtr = TSharedPtr<FModelAssetComplianceItem>;
	using FTreeNodePtr = TSharedPtr<FModelAssetComplianceTreeNode>;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildAssetListPanel();
	TSharedRef<SWidget> BuildPreviewPanel();
	TSharedRef<SWidget> BuildSettingsPanel();
	TSharedRef<SWidget> BuildDirectoryCategoryFilterButtons();
	TSharedRef<SWidget> BuildDirectoryCategoryFilterButton(EModelAssetComplianceCategory Category, const FText& Label);
	TSharedRef<SWidget> BuildModelCategoryButtons();
	TSharedRef<SWidget> BuildModelCategoryButton(EModelAssetComplianceCategory Category, const FText& Label);
	TSharedRef<SWidget> BuildStatusSettingButtons();
	TSharedRef<SWidget> BuildAutoStatusButton();
	TSharedRef<SWidget> BuildStatusSettingButton(EModelAssetComplianceStatus Status, const FText& Label);

	FReply RefreshAssets();
	void ScanAssets();
	void ReevaluateAssets();
	void EvaluateAsset(FModelAssetComplianceItem& Item) const;
	void RebuildFilteredItems();
	void RebuildTreeNodes();
	void CaptureExpandedFolderPaths(TSet<FString>& OutExpandedFolderPaths) const;
	void RestoreExpandedFolderPaths(const TSet<FString>& ExpandedFolderPaths, bool bExpandRootFolders);
	void RestoreExpandedFolderPathsRecursive(FTreeNodePtr Node, const TSet<FString>& ExpandedFolderPaths, bool bExpandRootFolders, bool bIsRoot);
	bool SelectTreeNodeForItem(const FComplianceItemPtr& Item);
	bool SelectTreeNodeForItemRecursive(FTreeNodePtr Node, const FComplianceItemPtr& Item);
	bool PassesCurrentFilter(const FModelAssetComplianceItem& Item) const;

	TSharedRef<ITableRow> GenerateTreeRow(FTreeNodePtr Node, const TSharedRef<STableViewBase>& OwnerTable);
	void GetTreeChildren(FTreeNodePtr Node, TArray<FTreeNodePtr>& OutChildren) const;
	TSharedPtr<SWidget> BuildAssetTreeContextMenu();
	void OnTreeSelectionChanged(FTreeNodePtr Node, ESelectInfo::Type SelectInfo);
	void OnSearchTextChanged(const FText& InSearchText);
	void OnDirectoryCategoryFilterChanged(ECheckBoxState NewState, EModelAssetComplianceCategory Category);
	ECheckBoxState IsDirectoryCategoryFilterChecked(EModelAssetComplianceCategory Category) const;
	void OnModelCategoryChanged(ECheckBoxState NewState, EModelAssetComplianceCategory Category);
	ECheckBoxState IsModelCategoryChecked(EModelAssetComplianceCategory Category) const;
	void OnAutoStatusChanged(ECheckBoxState NewState);
	ECheckBoxState IsAutoStatusChecked() const;
	void OnStatusSettingChanged(ECheckBoxState NewState, EModelAssetComplianceStatus Status);
	ECheckBoxState IsStatusSettingChecked(EModelAssetComplianceStatus Status) const;

	void OnIgnoreMaterialSlotChanged(ECheckBoxState NewState);
	ECheckBoxState IsIgnoreMaterialSlotChecked() const;
	void OnIgnoreCollisionChanged(ECheckBoxState NewState);
	ECheckBoxState IsIgnoreCollisionChecked() const;
	bool HasSelectedAssetAction() const;
	void ExecuteOpenSelectedAsset();
	void ExecuteShowSelectedAssetInContentBrowser();
	FReply OpenSelectedModelSettings();

	FText GetSummaryText() const;
	FText GetPreviewLogRichText() const;
	FText GetSelectedAssetTitleText() const;
	FText GetSelectedAssetPathText() const;
	FText GetSelectedCategoryText() const;
	FText GetSelectedStatusText() const;
	FText GetSelectedLODText() const;
	FText GetSelectedMaterialText() const;
	FText GetSelectedCollisionText() const;
	FText GetSelectedTriangleText() const;
	FText GetSelectedMessagesText() const;
	FText GetSelectedDetailsRichText() const;
	FText GetSelectedMessagesRichText() const;
	FSlateColor GetSelectedStatusColor() const;

	static EModelAssetComplianceCategory ClassifyAsset(const FAssetData& AssetData);
	static bool IsWarningStatus(EModelAssetComplianceStatus Status);
	static EModelAssetComplianceStatus GetDisplayStatus(const FModelAssetComplianceItem& Item);
	static FString MakeRelativeFolderPath(const FString& PackagePath);
	static FString CategoryToString(EModelAssetComplianceCategory Category);
	static FString StatusToString(EModelAssetComplianceStatus Status);
	static FSlateColor StatusToColor(EModelAssetComplianceStatus Status);
	static int32 GetTriangleCount(const UStaticMesh* StaticMesh, int32 LODIndex);

	TArray<FComplianceItemPtr> AllItems;
	TArray<FComplianceItemPtr> FilteredItems;
	TArray<FTreeNodePtr> TreeRootNodes;
	TSet<EModelAssetComplianceCategory> EnabledDirectoryCategoryFilters;
	FString SearchText;
	FComplianceItemPtr SelectedItem;

	TSharedPtr<STreeView<FTreeNodePtr>> AssetTreeView;
	TSharedPtr<SModelAssetComplianceViewport> PreviewViewport;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	bool bIgnoreMaterialSlotWarning = false;
	bool bIgnoreCollisionWarning = false;
};
