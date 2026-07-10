#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class STextBlock;
class UTexture2D;

enum class ETextureNoVTAuditStatus : uint8
{
	Pass,
	Warning,
	Blocked,
	Info
};

struct FTextureNoVTAuditItem
{
	FAssetData AssetData;
	TWeakObjectPtr<UTexture2D> Texture;
	FString AssetName;
	FString PackagePath;
	int32 SizeX = 0;
	int32 SizeY = 0;
	FString PixelFormat;
	FString CompressionSettings;
	bool bVirtualTextureStreaming = false;
	bool bSRGB = false;
	int64 EstimatedVRAMBytes = 0;

	ETextureNoVTAuditStatus Status = ETextureNoVTAuditStatus::Info;
	TArray<FText> Recommendations;
	bool bTextureCollectionCompatible = false;
};

class STextureVTAuditWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STextureVTAuditWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FSlateColor GetStatusColor(ETextureNoVTAuditStatus Status) const;
	FText StatusToText(ETextureNoVTAuditStatus Status) const;

private:
	using FAuditItemPtr = TSharedPtr<FTextureNoVTAuditItem>;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildListPanel();
	TSharedRef<SWidget> BuildFilterBar();

	FReply RefreshAssets();
	FReply DisableVTOnSelected();
	FReply OpenSelectedInEditor();
	FReply ShowSelectedInContentBrowser();

	void ScanAssets();
	void EvaluateItem(FTextureNoVTAuditItem& Item) const;
	void RebuildFilteredItems();

	TSharedRef<ITableRow> GenerateRow(FAuditItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSelectionChanged(FAuditItemPtr Item, ESelectInfo::Type SelectInfo);

	FText GetSummaryText() const;
	bool HasSelection() const;
	bool HasSelectedVirtualTexture() const;

	static bool IsPowerOfTwo(int32 V);

	TArray<FAuditItemPtr> AllItems;
	TArray<FAuditItemPtr> FilteredItems;
	TArray<FAuditItemPtr> SelectedItems;

	TSharedPtr<SListView<FAuditItemPtr>> ListView;
	TSharedPtr<STextBlock> SummaryTextBlock;
	TSharedPtr<STextBlock> ActionStatusTextBlock;

	FString SearchText;

	bool bShowPass = true;
	bool bShowWarning = true;
	bool bShowBlocked = true;
	bool bShowInfo = false;

	int32 LargeTextureWarningSize = 4096;
};
