#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class STextBlock;
class UTexture;
class UTexture2D;
class UTextureCollection;
class UVirtualTextureCollection;

struct FVTCMemberIssue
{
	FString TextureName;
	FText Message;
	bool bBlocker = false;
};

struct FVTCEntry
{
	FAssetData AssetData;
	TWeakObjectPtr<UVirtualTextureCollection> VTC;
	FString AssetName;
	FString PackagePath;
	int32 MemberCount = 0;
	int32 MaxSize = 0;
	int64 TotalVRAMBytes = 0;
	TArray<FVTCMemberIssue> Issues;
	FString RuntimeFormat;
	bool bSRGB = true;
};

class SVirtualTextureCollectionManagerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVirtualTextureCollectionManagerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Row 类需要调用的检查函数
	static bool IsTextureCompatibleForVTC(UTexture* Texture, FText& OutReason);

private:
	using FVTCEntryPtr = TSharedPtr<FVTCEntry>;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildSplitPanels();
	TSharedRef<SWidget> BuildVTCListPanel();
	TSharedRef<SWidget> BuildDetailPanel();

	FReply RefreshList();
	FReply CreateNewVTC();
	FReply CreateVTCFromContentBrowserSelection();
	FReply AddContentBrowserSelectionToCurrentVTC();
	FReply RemoveSelectedMembersFromCurrentVTC();
	FReply OpenCurrentVTC();
	FReply ValidateAll();

	void ScanVTCs();
	void ValidateEntry(FVTCEntry& Entry) const;

	TSharedRef<ITableRow> GenerateVTCRow(FVTCEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateMemberRow(TSharedPtr<int32> Index, const TSharedRef<STableViewBase>& OwnerTable);
	void OnVTCSelectionChanged(FVTCEntryPtr Entry, ESelectInfo::Type SelectInfo);
	void OnMemberSelectionChanged(TSharedPtr<int32> Index, ESelectInfo::Type SelectInfo);

	FText GetSummaryText() const;
	FText GetDetailHeaderText() const;
	FText GetDetailIssuesText() const;

	static TArray<UTexture*> CollectTexturesFromContentBrowser();
	static UVirtualTextureCollection* CreateVTCAsset(const FString& PackagePath, const FString& AssetName);
	static void AddTexturesToVTC(UVirtualTextureCollection* VTC, const TArray<UTexture*>& Textures);

	TArray<FVTCEntryPtr> AllEntries;
	TArray<TSharedPtr<int32>> CurrentMemberIndices;
	TArray<TSharedPtr<int32>> SelectedMemberIndices;

	TSharedPtr<SListView<FVTCEntryPtr>> VTCListView;
	TSharedPtr<SListView<TSharedPtr<int32>>> MemberListView;

	FVTCEntryPtr CurrentEntry;
};
