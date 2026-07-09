#include "STextureVTAuditWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "PixelFormat.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "TextureCompiler.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitTextureVTAudit"

namespace
{
	const FName ColumnStatus(TEXT("Status"));
	const FName ColumnName(TEXT("Name"));
	const FName ColumnSize(TEXT("Size"));
	const FName ColumnFormat(TEXT("Format"));
	const FName ColumnVT(TEXT("VT"));
	const FName ColumnVTC(TEXT("VTC"));
	const FName ColumnVRAM(TEXT("VRAM"));
	const FName ColumnPath(TEXT("Path"));

	bool IsPixelFormatVTCCompatible(EPixelFormat Format)
	{
		switch (Format)
		{
		case PF_DXT1:
		case PF_DXT3:
		case PF_DXT5:
		case PF_BC4:
		case PF_BC5:
		case PF_BC6H:
		case PF_BC7:
		case PF_B8G8R8A8:
		case PF_R8G8B8A8:
		case PF_G8:
		case PF_R8G8:
			return true;
		default:
			return false;
		}
	}

	int64 EstimateVRAMBytes(int32 SizeX, int32 SizeY, EPixelFormat Format)
	{
		const int32 BlockBytes = GPixelFormats[Format].BlockBytes;
		const int32 BlockSizeX = FMath::Max(1, GPixelFormats[Format].BlockSizeX);
		const int32 BlockSizeY = FMath::Max(1, GPixelFormats[Format].BlockSizeY);
		const int32 NumBlocksX = FMath::DivideAndRoundUp(SizeX, BlockSizeX);
		const int32 NumBlocksY = FMath::DivideAndRoundUp(SizeY, BlockSizeY);
		// 含 mip 链约 1.33x
		return static_cast<int64>(NumBlocksX * NumBlocksY * BlockBytes) * 4 / 3;
	}

	bool IsLikelyDedicatedVirtualTexturePath(const FString& PackagePath)
	{
		return PackagePath.Contains(TEXT("/VT"), ESearchCase::IgnoreCase) ||
			PackagePath.Contains(TEXT("/VTC"), ESearchCase::IgnoreCase) ||
			PackagePath.Contains(TEXT("/VirtualTexture"), ESearchCase::IgnoreCase) ||
			PackagePath.Contains(TEXT("/BakeInfo"), ESearchCase::IgnoreCase);
	}

	void RaiseAuditStatus(ETextureVTAuditStatus& Current, ETextureVTAuditStatus Candidate)
	{
		if (Candidate == ETextureVTAuditStatus::Blocked ||
			(Candidate == ETextureVTAuditStatus::Warning && Current == ETextureVTAuditStatus::Pass) ||
			(Candidate == ETextureVTAuditStatus::Pass && Current == ETextureVTAuditStatus::Info))
		{
			Current = Candidate;
		}
	}
}

bool STextureVTAuditWidget::IsPowerOfTwo(int32 V)
{
	return V > 0 && (V & (V - 1)) == 0;
}

void STextureVTAuditWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f, 0.f)
		[
			BuildFilterBar()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(4.f)
		[
			BuildListPanel()
		]
	];

	ScanAssets();
}

TSharedRef<SWidget> STextureVTAuditWidget::BuildToolbar()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Refresh", "重新扫描"))
		.ToolTipText(LOCTEXT("RefreshTip", "扫描 /Game/Art 下所有 Texture2D。"))
		.OnClicked(this, &STextureVTAuditWidget::RefreshAssets)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("EnableVT", "对所选开启 VT"))
		.ToolTipText(LOCTEXT("EnableVTTip", "对当前列表中被选中的贴图批量勾选 Virtual Texture Streaming。"))
		.OnClicked(this, &STextureVTAuditWidget::EnableVTOnSelected)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("DisableVT", "关闭 VT"))
		.ToolTipText(LOCTEXT("DisableVTTip", "关闭所选贴图的 Virtual Texture Streaming。"))
		.OnClicked(this, &STextureVTAuditWidget::DisableVTOnSelected)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Open", "打开资产"))
		.OnClicked(this, &STextureVTAuditWidget::OpenSelectedInEditor)
	]
	+ SHorizontalBox::Slot().AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("ShowInCB", "定位 Content Browser"))
		.OnClicked(this, &STextureVTAuditWidget::ShowSelectedInContentBrowser)
	]
	+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
	[
		SAssignNew(SummaryTextBlock, STextBlock)
		.Text(this, &STextureVTAuditWidget::GetSummaryText)
	];
}

TSharedRef<SWidget> STextureVTAuditWidget::BuildFilterBar()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0).VAlign(VAlign_Center)
	[
		SNew(STextBlock).Text(LOCTEXT("FilterLabel", "筛选:"))
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]() { return bShowPass ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
		.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bShowPass = (S == ECheckBoxState::Checked); RebuildFilteredItems(); })
		.Content()[SNew(STextBlock).Text(LOCTEXT("FilterPass", "Pass")).ColorAndOpacity(FLinearColor(0.4f, 0.9f, 0.4f))]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]() { return bShowWarning ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
		.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bShowWarning = (S == ECheckBoxState::Checked); RebuildFilteredItems(); })
		.Content()[SNew(STextBlock).Text(LOCTEXT("FilterWarning", "Warning")).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.2f))]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]() { return bShowBlocked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
		.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bShowBlocked = (S == ECheckBoxState::Checked); RebuildFilteredItems(); })
		.Content()[SNew(STextBlock).Text(LOCTEXT("FilterBlocked", "Blocked")).ColorAndOpacity(FLinearColor(1.f, 0.3f, 0.3f))]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]() { return bShowInfo ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
		.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bShowInfo = (S == ECheckBoxState::Checked); RebuildFilteredItems(); })
		.Content()[SNew(STextBlock).Text(LOCTEXT("FilterInfo", "Info(小贴图)")).ColorAndOpacity(FLinearColor(0.65f, 0.75f, 0.85f))]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0).VAlign(VAlign_Center)
	[
		SNew(STextBlock).Text(LOCTEXT("MinVTLabel", "VT 建议阈值:"))
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
	[
		SNew(SBox).WidthOverride(80.f)
		[
			SNew(SSpinBox<int32>)
			.MinValue(256).MaxValue(8192).Delta(256)
			.Value_Lambda([this]() { return VTRecommendMinSize; })
			.OnValueChanged_Lambda([this](int32 V) { VTRecommendMinSize = V; ScanAssets(); })
		]
	]
	+ SHorizontalBox::Slot().FillWidth(1.f)
	[
		SNew(SSearchBox)
		.HintText(LOCTEXT("SearchHint", "按名称/路径搜索..."))
		.OnTextChanged_Lambda([this](const FText& T) { SearchText = T.ToString(); RebuildFilteredItems(); })
	];
}

TSharedRef<SWidget> STextureVTAuditWidget::BuildListPanel()
{
	return SNew(SBorder)
	.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
	[
		SAssignNew(ListView, SListView<FAuditItemPtr>)
		.ListItemsSource(&FilteredItems)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &STextureVTAuditWidget::GenerateRow)
		.OnSelectionChanged(this, &STextureVTAuditWidget::OnSelectionChanged)
		.HeaderRow(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(ColumnStatus).DefaultLabel(LOCTEXT("HStatus", "状态")).FixedWidth(70)
			+ SHeaderRow::Column(ColumnName).DefaultLabel(LOCTEXT("HName", "名称")).FillWidth(0.25f)
			+ SHeaderRow::Column(ColumnSize).DefaultLabel(LOCTEXT("HSize", "尺寸")).FixedWidth(90)
			+ SHeaderRow::Column(ColumnFormat).DefaultLabel(LOCTEXT("HFormat", "格式")).FixedWidth(110)
			+ SHeaderRow::Column(ColumnVT).DefaultLabel(LOCTEXT("HVT", "VT")).FixedWidth(40)
			+ SHeaderRow::Column(ColumnVTC).DefaultLabel(LOCTEXT("HVTC", "VTC")).FixedWidth(40)
			+ SHeaderRow::Column(ColumnVRAM).DefaultLabel(LOCTEXT("HVRAM", "VRAM")).FixedWidth(70)
			+ SHeaderRow::Column(ColumnPath).DefaultLabel(LOCTEXT("HPath", "路径")).FillWidth(0.4f)
		)
	];
}

class STextureVTAuditRow : public SMultiColumnTableRow<TSharedPtr<FTextureVTAuditItem>>
{
public:
	SLATE_BEGIN_ARGS(STextureVTAuditRow) {}
		SLATE_ARGUMENT(TSharedPtr<FTextureVTAuditItem>, Item)
		SLATE_ARGUMENT(STextureVTAuditWidget*, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		Item = InArgs._Item;
		Owner = InArgs._Owner;
		SMultiColumnTableRow<TSharedPtr<FTextureVTAuditItem>>::Construct(FSuperRowType::FArguments(), OwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Col) override
	{
		if (Col == ColumnStatus)
		{
			return SNew(SBorder)
				.Padding(2)
				.BorderBackgroundColor(Owner->GetStatusColor(Item->Status))
				[
					SNew(STextBlock).Text(Owner->StatusToText(Item->Status))
				];
		}
		if (Col == ColumnName)     { return SNew(STextBlock).Text(FText::FromString(Item->AssetName)); }
		if (Col == ColumnSize)     { return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%dx%d"), Item->SizeX, Item->SizeY))); }
		if (Col == ColumnFormat)   { return SNew(STextBlock).Text(FText::FromString(Item->PixelFormat)); }
		if (Col == ColumnVT)       { return SNew(STextBlock).Text(FText::FromString(Item->bVirtualTextureStreaming ? TEXT("Yes") : TEXT("--"))).ColorAndOpacity(Item->bVirtualTextureStreaming ? FLinearColor(0.4f, 0.9f, 0.4f) : FLinearColor::White); }
		if (Col == ColumnVTC)      { return SNew(STextBlock).Text(FText::FromString(Item->bVTCCompatible ? TEXT("OK") : TEXT("--"))).ColorAndOpacity(Item->bVTCCompatible ? FLinearColor(0.4f, 0.9f, 0.4f) : FLinearColor(1.f, 0.3f, 0.3f)); }
		if (Col == ColumnVRAM)     { return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%.2f MB"), Item->EstimatedVRAMBytes / 1048576.0f))); }
		if (Col == ColumnPath)     { return SNew(STextBlock).Text(FText::FromString(Item->PackagePath)); }
		return SNullWidget::NullWidget;
	}

	TSharedPtr<FTextureVTAuditItem> Item;
	STextureVTAuditWidget* Owner = nullptr;
};

TSharedRef<ITableRow> STextureVTAuditWidget::GenerateRow(FAuditItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureVTAuditRow, OwnerTable).Item(Item).Owner(this);
}

void STextureVTAuditWidget::OnSelectionChanged(FAuditItemPtr Item, ESelectInfo::Type)
{
	SelectedItems.Reset();
	if (ListView.IsValid())
	{
		SelectedItems = ListView->GetSelectedItems();
	}
}

FReply STextureVTAuditWidget::RefreshAssets()
{
	ScanAssets();
	return FReply::Handled();
}

void STextureVTAuditWidget::ScanAssets()
{
	AllItems.Reset();

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AR = ARM.Get();

	if (AR.IsLoadingAssets())
	{
		AR.SearchAllAssets(true);
	}

	FARFilter Filter;
	Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game/Art"));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetList;
	AR.GetAssets(Filter, AssetList);

	for (const FAssetData& AD : AssetList)
	{
		TSharedPtr<FTextureVTAuditItem> Item = MakeShared<FTextureVTAuditItem>();
		Item->AssetData = AD;
		Item->AssetName = AD.AssetName.ToString();
		Item->PackagePath = AD.PackagePath.ToString();

		if (UTexture2D* Tex = Cast<UTexture2D>(AD.GetAsset()))
		{
			Item->Texture = Tex;
			Item->SizeX = static_cast<int32>(Tex->GetSizeX());
			Item->SizeY = static_cast<int32>(Tex->GetSizeY());
			const EPixelFormat PF = Tex->GetPixelFormat();
			Item->PixelFormat = FString(GPixelFormats[PF].Name);
			Item->CompressionSettings = UEnum::GetValueAsString(Tex->CompressionSettings);
			Item->bVirtualTextureStreaming = Tex->VirtualTextureStreaming;
			Item->bSRGB = Tex->SRGB;
			Item->EstimatedVRAMBytes = EstimateVRAMBytes(Item->SizeX, Item->SizeY, PF);
			Item->bVTCCompatible = IsPixelFormatVTCCompatible(PF)
				&& Item->SizeX <= VTCMaxSize && Item->SizeY <= VTCMaxSize;
		}

		EvaluateItem(*Item);
		AllItems.Add(Item);
	}

	AllItems.Sort([](const FAuditItemPtr& A, const FAuditItemPtr& B)
	{
		if (A->Status != B->Status) return static_cast<uint8>(A->Status) < static_cast<uint8>(B->Status);
		return A->EstimatedVRAMBytes > B->EstimatedVRAMBytes;
	});

	RebuildFilteredItems();
}

void STextureVTAuditWidget::EvaluateItem(FTextureVTAuditItem& Item) const
{
	Item.Recommendations.Reset();

	{
		const int32 AuditMaxSide = FMath::Max(Item.SizeX, Item.SizeY);
		const bool bDedicatedVTPath = IsLikelyDedicatedVirtualTexturePath(Item.PackagePath);
		const bool bTooLargeForVTC = AuditMaxSide > VTCMaxSize;

		if (Item.bVirtualTextureStreaming)
		{
			if (!Item.bVTCCompatible)
			{
				Item.Status = ETextureVTAuditStatus::Blocked;
				Item.Recommendations.Add(FText::Format(
					LOCTEXT("VTBadFormat", "已开启 VT，但格式 {0} 不在当前 VTC/VT 白名单中；请关闭 VT 或调整压缩格式。"),
					FText::FromString(Item.PixelFormat)));
				return;
			}

			Item.Status = bDedicatedVTPath ? ETextureVTAuditStatus::Pass : ETextureVTAuditStatus::Warning;
			Item.Recommendations.Add(bDedicatedVTPath
				? LOCTEXT("DedicatedVTOn", "专用 VT/VTC/BakeInfo 路径贴图已开启 VT，可以继续用于专项流程。")
				: LOCTEXT("RegularVTOn", "普通场景模型贴图默认不建议开启 VT；若不是专用 VTC/VT 资产，请关闭 VT。"));
		}
		else
		{
			Item.Status = ETextureVTAuditStatus::Pass;
			Item.Recommendations.Add(LOCTEXT("RegularNoVTPass", "普通场景模型贴图默认保持 NoVT。只有地面 RVT 输出、专用 VTC/VT 或超大特殊资产再单独审查。"));
		}

		if (bTooLargeForVTC)
		{
			RaiseAuditStatus(Item.Status, ETextureVTAuditStatus::Warning);
			Item.Recommendations.Add(FText::Format(
				LOCTEXT("LargeTextureNoDefaultVT", "尺寸 {0} 超过 VTC 4K 单成员参考上限；普通贴图可保持 NoVT，但需要确认内存和平台 mip 预算。"),
				FText::AsNumber(AuditMaxSide)));
		}

		if (!IsPowerOfTwo(Item.SizeX) || !IsPowerOfTwo(Item.SizeY))
		{
			Item.Recommendations.Add(LOCTEXT("NotPOTDefaultNoVT", "非 2 的幂次尺寸；普通 Texture2D 可以使用，但 mip、打包和专项 VT/VTC 流程仍建议 POT。"));
		}
		return;
	}

}

#if 0

	const int32 MaxSide = FMath::Max(Item.SizeX, Item.SizeY);
	const bool bIsSmall = MaxSide < VTRecommendMinSize;
	const bool bTooLargeForVTC = MaxSide > VTCMaxSize;

	if (bTooLargeForVTC)
	{
		Item.Status = ETextureVTAuditStatus::Blocked;
		Item.Recommendations.Add(FText::Format(
			LOCTEXT("TooLarge", "尺寸 {0} 超过 VTC 4K 上限，需缩小到 ≤4096 才能加入 VTC。"),
			FText::AsNumber(MaxSide)));
		return;
	}

	if (!Item.bVTCCompatible && MaxSide >= VTRecommendMinSize)
	{
		Item.Status = ETextureVTAuditStatus::Blocked;
		Item.Recommendations.Add(FText::Format(
			LOCTEXT("BadFormat", "格式 {0} 不在 VTC 支持的白名单中（推荐 DXT1/5/BC5/BC7）。"),
			FText::FromString(Item.PixelFormat)));
		return;
	}

	if (bIsSmall)
	{
		Item.Status = ETextureVTAuditStatus::Info;
		if (Item.bVirtualTextureStreaming)
		{
			Item.Recommendations.Add(LOCTEXT("SmallVTOn", "尺寸较小 (<1K)，通常不建议开 VT，因为 page 开销不划算。"));
			Item.Status = ETextureVTAuditStatus::Warning;
		}
		return;
	}

	if (Item.bVirtualTextureStreaming)
	{
		Item.Status = ETextureVTAuditStatus::Pass;
		Item.Recommendations.Add(LOCTEXT("VTOn", "VT 已开启。可直接加入 VirtualTextureCollection。"));
	}
	else
	{
		Item.Status = ETextureVTAuditStatus::Warning;
		Item.Recommendations.Add(LOCTEXT("VTOff", "尺寸达到 VT 建议阈值，未开启 VT。选中后点击“对所选开启 VT”。"));
	}

	if (!IsPowerOfTwo(Item.SizeX) || !IsPowerOfTwo(Item.SizeY))
	{
		Item.Recommendations.Add(LOCTEXT("NotPOT", "非 2 的幂次尺寸，VT 效率最佳仍是 POT。"));
	}
}

#endif

void STextureVTAuditWidget::RebuildFilteredItems()
{
	FilteredItems.Reset();
	for (const FAuditItemPtr& It : AllItems)
	{
		const bool bStatusOK =
			(It->Status == ETextureVTAuditStatus::Pass    && bShowPass) ||
			(It->Status == ETextureVTAuditStatus::Warning && bShowWarning) ||
			(It->Status == ETextureVTAuditStatus::Blocked && bShowBlocked) ||
			(It->Status == ETextureVTAuditStatus::Info    && bShowInfo);
		if (!bStatusOK) continue;

		if (!SearchText.IsEmpty())
		{
			const bool bMatch = It->AssetName.Contains(SearchText, ESearchCase::IgnoreCase)
				|| It->PackagePath.Contains(SearchText, ESearchCase::IgnoreCase);
			if (!bMatch) continue;
		}
		FilteredItems.Add(It);
	}
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

FReply STextureVTAuditWidget::EnableVTOnSelected()
{
	int32 Changed = 0;
	for (const FAuditItemPtr& It : SelectedItems)
	{
		if (UTexture2D* Tex = It->Texture.Get())
		{
			if (!Tex->VirtualTextureStreaming)
			{
				Tex->Modify();
				Tex->VirtualTextureStreaming = true;
				Tex->UpdateResource();
				Tex->PostEditChange();
				Tex->MarkPackageDirty();
				++Changed;
			}
		}
	}
	if (Changed > 0)
	{
		FTextureCompilingManager::Get().FinishAllCompilation();
		ScanAssets();
	}
	return FReply::Handled();
}

FReply STextureVTAuditWidget::DisableVTOnSelected()
{
	int32 Changed = 0;
	for (const FAuditItemPtr& It : SelectedItems)
	{
		if (UTexture2D* Tex = It->Texture.Get())
		{
			if (Tex->VirtualTextureStreaming)
			{
				Tex->Modify();
				Tex->VirtualTextureStreaming = false;
				Tex->UpdateResource();
				Tex->PostEditChange();
				Tex->MarkPackageDirty();
				++Changed;
			}
		}
	}
	if (Changed > 0)
	{
		FTextureCompilingManager::Get().FinishAllCompilation();
		ScanAssets();
	}
	return FReply::Handled();
}

FReply STextureVTAuditWidget::OpenSelectedInEditor()
{
	if (GEditor && !SelectedItems.IsEmpty())
	{
		if (UTexture2D* Tex = SelectedItems[0]->Texture.Get())
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Tex);
		}
	}
	return FReply::Handled();
}

FReply STextureVTAuditWidget::ShowSelectedInContentBrowser()
{
	if (SelectedItems.IsEmpty()) return FReply::Handled();

	TArray<FAssetData> Assets;
	for (const FAuditItemPtr& It : SelectedItems)
	{
		Assets.Add(It->AssetData);
	}
	FContentBrowserModule& CBM = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	CBM.Get().SyncBrowserToAssets(Assets);
	return FReply::Handled();
}

FText STextureVTAuditWidget::GetSummaryText() const
{
	int32 Pass = 0, Warn = 0, Block = 0, Info = 0;
	int64 TotalVRAM = 0;
	for (const FAuditItemPtr& It : AllItems)
	{
		switch (It->Status)
		{
		case ETextureVTAuditStatus::Pass:    ++Pass; break;
		case ETextureVTAuditStatus::Warning: ++Warn; break;
		case ETextureVTAuditStatus::Blocked: ++Block; break;
		case ETextureVTAuditStatus::Info:    ++Info; break;
		}
		TotalVRAM += It->EstimatedVRAMBytes;
	}
	return FText::Format(
		LOCTEXT("Summary", "共 {0} | Pass {1} · Warning {2} · Blocked {3} · Info {4} | VRAM 估算 {5} MB | 显示 {6}"),
		FText::AsNumber(AllItems.Num()),
		FText::AsNumber(Pass), FText::AsNumber(Warn), FText::AsNumber(Block), FText::AsNumber(Info),
		FText::AsNumber(TotalVRAM / 1048576),
		FText::AsNumber(FilteredItems.Num()));
}

FSlateColor STextureVTAuditWidget::GetStatusColor(ETextureVTAuditStatus Status) const
{
	switch (Status)
	{
	case ETextureVTAuditStatus::Pass:    return FSlateColor(FLinearColor(0.15f, 0.35f, 0.15f));
	case ETextureVTAuditStatus::Warning: return FSlateColor(FLinearColor(0.5f, 0.4f, 0.1f));
	case ETextureVTAuditStatus::Blocked: return FSlateColor(FLinearColor(0.5f, 0.1f, 0.1f));
	case ETextureVTAuditStatus::Info:    return FSlateColor(FLinearColor(0.2f, 0.25f, 0.35f));
	}
	return FSlateColor(FLinearColor::Gray);
}

FText STextureVTAuditWidget::StatusToText(ETextureVTAuditStatus Status) const
{
	switch (Status)
	{
	case ETextureVTAuditStatus::Pass:    return LOCTEXT("StatusPass", "Pass");
	case ETextureVTAuditStatus::Warning: return LOCTEXT("StatusWarn", "Warning");
	case ETextureVTAuditStatus::Blocked: return LOCTEXT("StatusBlock", "Blocked");
	case ETextureVTAuditStatus::Info:    return LOCTEXT("StatusInfo", "Info");
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
