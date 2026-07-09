#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class STextBlock;
class UTexture2D;

enum class ETextureVTAuditStatus : uint8
{
	Pass,       // VT 已开启且尺寸合适
	Warning,    // 尺寸/格式建议开 VT
	Blocked,    // 超出 VTC 4K 上限或格式不兼容
	Info        // 小贴图，不建议 VT
};

struct FTextureVTAuditItem
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

	ETextureVTAuditStatus Status = ETextureVTAuditStatus::Info;
	TArray<FText> Recommendations;
	bool bVTCCompatible = false;
};

class STextureVTAuditWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STextureVTAuditWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Row 类需要访问格式化辅助函数
	FSlateColor GetStatusColor(ETextureVTAuditStatus Status) const;
	FText StatusToText(ETextureVTAuditStatus Status) const;

private:
	using FAuditItemPtr = TSharedPtr<FTextureVTAuditItem>;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildListPanel();
	TSharedRef<SWidget> BuildFilterBar();

	FReply RefreshAssets();
	FReply EnableVTOnSelected();
	FReply DisableVTOnSelected();
	FReply OpenSelectedInEditor();
	FReply ShowSelectedInContentBrowser();

	void ScanAssets();
	void EvaluateItem(FTextureVTAuditItem& Item) const;
	void RebuildFilteredItems();

	TSharedRef<ITableRow> GenerateRow(FAuditItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSelectionChanged(FAuditItemPtr Item, ESelectInfo::Type SelectInfo);

	FText GetSummaryText() const;

	static bool IsPowerOfTwo(int32 V);

	TArray<FAuditItemPtr> AllItems;
	TArray<FAuditItemPtr> FilteredItems;
	TArray<FAuditItemPtr> SelectedItems;

	TSharedPtr<SListView<FAuditItemPtr>> ListView;
	TSharedPtr<STextBlock> SummaryTextBlock;

	FString SearchText;

	bool bShowPass = true;
	bool bShowWarning = true;
	bool bShowBlocked = true;
	bool bShowInfo = false;

	int32 VTRecommendMinSize = 8192;
	int32 VTCMaxSize = 4096;
};
