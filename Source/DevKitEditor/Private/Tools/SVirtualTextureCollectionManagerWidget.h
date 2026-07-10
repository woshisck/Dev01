#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UTexture;
class UTextureCollection;
class STextBlock;

struct FTextureCollectionMemberIssue
{
	FString TextureName;
	FText Message;
	bool bBlocker = false;
};

struct FTextureCollectionEntry
{
	FAssetData AssetData;
	TWeakObjectPtr<UTextureCollection> Collection;
	FString AssetName;
	FString PackagePath;
	int32 MemberCount = 0;
	int32 MaxSize = 0;
	int64 TotalVRAMBytes = 0;
	TArray<FTextureCollectionMemberIssue> Issues;
	FString CollectionType;
};

class SVirtualTextureCollectionManagerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVirtualTextureCollectionManagerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static bool IsTextureCompatibleForCollection(UTexture* Texture, FText& OutReason);

private:
	using FCollectionEntryPtr = TSharedPtr<FTextureCollectionEntry>;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildSplitPanels();
	TSharedRef<SWidget> BuildCollectionListPanel();
	TSharedRef<SWidget> BuildDetailPanel();

	FReply RefreshList();
	FReply CreateNewTextureCollection();
	FReply CreateTextureCollectionFromContentBrowserSelection();
	FReply AddContentBrowserSelectionToCurrentTextureCollection();
	FReply RemoveSelectedMembersFromCurrentTextureCollection();
	FReply OpenCurrentTextureCollection();
	FReply ValidateAll();

	void ScanTextureCollections();
	void ValidateEntry(FTextureCollectionEntry& Entry) const;

	TSharedRef<ITableRow> GenerateCollectionRow(FCollectionEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateMemberRow(TSharedPtr<int32> Index, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCollectionSelectionChanged(FCollectionEntryPtr Entry, ESelectInfo::Type SelectInfo);
	void OnMemberSelectionChanged(TSharedPtr<int32> Index, ESelectInfo::Type SelectInfo);

	FText GetSummaryText() const;
	FText GetDetailHeaderText() const;
	FText GetDetailIssuesText() const;
	bool HasCurrentCollection() const;
	bool HasSelectedMembers() const;
	void SetActionStatus(const FText& InText);

	static TArray<UTexture*> CollectTexturesFromContentBrowser();
	static UTextureCollection* CreateTextureCollectionAsset(const FString& PackagePath, const FString& AssetName);
	static void AddTexturesToCollection(UTextureCollection* Collection, const TArray<UTexture*>& Textures);

	TArray<FCollectionEntryPtr> AllEntries;
	TArray<TSharedPtr<int32>> CurrentMemberIndices;
	TArray<TSharedPtr<int32>> SelectedMemberIndices;

	TSharedPtr<SListView<FCollectionEntryPtr>> CollectionListView;
	TSharedPtr<SListView<TSharedPtr<int32>>> MemberListView;
	TSharedPtr<STextBlock> ActionStatusTextBlock;

	FCollectionEntryPtr CurrentEntry;
};
