#include "STextureVTAuditWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "IContentBrowserSingleton.h"
#include "PixelFormat.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "TextureCompiler.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "Tools/DevKitArtToolUI.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "DevKitTextureNoVTAudit"

namespace
{
	const FName ColumnStatus(TEXT("Status"));
	const FName ColumnName(TEXT("Name"));
	const FName ColumnSize(TEXT("Size"));
	const FName ColumnFormat(TEXT("Format"));
	const FName ColumnVT(TEXT("VTStreaming"));
	const FName ColumnTC(TEXT("TextureCollection"));
	const FName ColumnVRAM(TEXT("VRAM"));
	const FName ColumnPath(TEXT("Path"));

	bool IsPixelFormatTextureCollectionCompatible(EPixelFormat Format)
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
		return static_cast<int64>(NumBlocksX * NumBlocksY * BlockBytes) * 4 / 3;
	}

	void RaiseAuditStatus(ETextureNoVTAuditStatus& Current, ETextureNoVTAuditStatus Candidate)
	{
		if (Candidate == ETextureNoVTAuditStatus::Blocked ||
			(Candidate == ETextureNoVTAuditStatus::Warning && Current != ETextureNoVTAuditStatus::Blocked) ||
			(Candidate == ETextureNoVTAuditStatus::Pass && Current == ETextureNoVTAuditStatus::Info))
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
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 12.f, 12.f, 8.f)
		[
			DevKitArtToolUI::MakeHeader(
				LOCTEXT("Title", "贴图 NoVT 审计"),
				LOCTEXT("Description", "扫描 /Game/Art 的 Texture2D，定位误开启 Virtual Texture Streaming、尺寸和格式风险。批量关闭 VT 会修改资产并支持撤销。"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f, 12.f, 6.f)
		[
			DevKitArtToolUI::MakeSectionHeader(1, LOCTEXT("AuditSection", "扫描与筛选"), LOCTEXT("AuditSectionDesc", "先刷新资产，再按状态、尺寸或名称缩小结果范围。"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(12.f, 0.f, 12.f, 6.f)
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(12.f, 0.f, 12.f, 8.f)
		[
			BuildFilterBar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f, 12.f, 6.f)
		[
			DevKitArtToolUI::MakeSectionHeader(2, LOCTEXT("ResultSection", "审计结果"), LOCTEXT("ResultSectionDesc", "可多选结果并批量关闭源贴图 VT；修改后资产会标记为待保存。"))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(12.f, 0.f, 12.f, 6.f)
		[
			BuildListPanel()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f, 12.f, 12.f)
		[
			SNew(SBorder)
			.Padding(8.f)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			[
				SAssignNew(ActionStatusTextBlock, STextBlock)
				.Text(LOCTEXT("InitialActionStatus", "就绪。选择表格中的贴图后可打开、定位或批量关闭 VT。"))
				.AutoWrapText(true)
			]
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
		.ToolTipText(LOCTEXT("RefreshTip", "扫描 /Game/Art 下所有 Texture2D。普通场景贴图默认必须保持 NoVT。"))
		.OnClicked(this, &STextureVTAuditWidget::RefreshAssets)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("DisableVT", "关闭所选 VT"))
		.ToolTipText(LOCTEXT("DisableVTTip", "关闭所选贴图的 Virtual Texture Streaming。地面 RVT 使用 RuntimeVirtualTexture 资产，源贴图仍应保持 NoVT。"))
		.IsEnabled(this, &STextureVTAuditWidget::HasSelectedVirtualTexture)
		.OnClicked(this, &STextureVTAuditWidget::DisableVTOnSelected)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Open", "打开资产"))
		.IsEnabled(this, &STextureVTAuditWidget::HasSelection)
		.OnClicked(this, &STextureVTAuditWidget::OpenSelectedInEditor)
	]
	+ SHorizontalBox::Slot().AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("ShowInCB", "定位 Content Browser"))
		.IsEnabled(this, &STextureVTAuditWidget::HasSelection)
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
		SNew(STextBlock).Text(LOCTEXT("FilterLabel", "筛选"))
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
		.Content()[SNew(STextBlock).Text(LOCTEXT("FilterInfo", "Info")).ColorAndOpacity(FLinearColor(0.65f, 0.75f, 0.85f))]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0).VAlign(VAlign_Center)
	[
		SNew(STextBlock).Text(LOCTEXT("LargeTextureLabel", "大图提醒阈值"))
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
	[
		SNew(SBox).WidthOverride(80.f)
		[
			SNew(SSpinBox<int32>)
			.MinValue(512).MaxValue(16384).Delta(512)
			.Value_Lambda([this]() { return LargeTextureWarningSize; })
			.OnValueChanged_Lambda([this](int32 V) { LargeTextureWarningSize = V; ScanAssets(); })
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
			+ SHeaderRow::Column(ColumnStatus).DefaultLabel(LOCTEXT("HStatus", "状态")).FixedWidth(76)
			+ SHeaderRow::Column(ColumnName).DefaultLabel(LOCTEXT("HName", "名称")).FillWidth(0.25f)
			+ SHeaderRow::Column(ColumnSize).DefaultLabel(LOCTEXT("HSize", "尺寸")).FixedWidth(90)
			+ SHeaderRow::Column(ColumnFormat).DefaultLabel(LOCTEXT("HFormat", "格式")).FixedWidth(110)
			+ SHeaderRow::Column(ColumnVT).DefaultLabel(LOCTEXT("HVT", "源VT")).FixedWidth(56)
			+ SHeaderRow::Column(ColumnTC).DefaultLabel(LOCTEXT("HTC", "TC")).FixedWidth(48)
			+ SHeaderRow::Column(ColumnVRAM).DefaultLabel(LOCTEXT("HVRAM", "VRAM")).FixedWidth(78)
			+ SHeaderRow::Column(ColumnPath).DefaultLabel(LOCTEXT("HPath", "路径")).FillWidth(0.4f)
		)
	];
}

class STextureNoVTAuditRow : public SMultiColumnTableRow<TSharedPtr<FTextureNoVTAuditItem>>
{
public:
	SLATE_BEGIN_ARGS(STextureNoVTAuditRow) {}
		SLATE_ARGUMENT(TSharedPtr<FTextureNoVTAuditItem>, Item)
		SLATE_ARGUMENT(STextureVTAuditWidget*, Owner)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		Item = InArgs._Item;
		Owner = InArgs._Owner;
		SMultiColumnTableRow<TSharedPtr<FTextureNoVTAuditItem>>::Construct(FSuperRowType::FArguments(), OwnerTable);
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
		if (Col == ColumnName)   { return SNew(STextBlock).Text(FText::FromString(Item->AssetName)); }
		if (Col == ColumnSize)   { return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%dx%d"), Item->SizeX, Item->SizeY))); }
		if (Col == ColumnFormat) { return SNew(STextBlock).Text(FText::FromString(Item->PixelFormat)); }
		if (Col == ColumnVT)
		{
			return SNew(STextBlock)
				.Text(Item->bVirtualTextureStreaming ? LOCTEXT("VTYes", "Yes") : LOCTEXT("VTNo", "No"))
				.ColorAndOpacity(Item->bVirtualTextureStreaming ? FLinearColor(1.f, 0.25f, 0.25f) : FLinearColor(0.4f, 0.9f, 0.4f));
		}
		if (Col == ColumnTC)
		{
			return SNew(STextBlock)
				.Text(Item->bTextureCollectionCompatible ? LOCTEXT("TCOK", "OK") : LOCTEXT("TCNo", "--"))
				.ColorAndOpacity(Item->bTextureCollectionCompatible ? FLinearColor(0.4f, 0.9f, 0.4f) : FLinearColor(1.f, 0.45f, 0.35f));
		}
		if (Col == ColumnVRAM) { return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%.2f MB"), Item->EstimatedVRAMBytes / 1048576.0f))); }
		if (Col == ColumnPath) { return SNew(STextBlock).Text(FText::FromString(Item->PackagePath)); }
		return SNullWidget::NullWidget;
	}

	TSharedPtr<FTextureNoVTAuditItem> Item;
	STextureVTAuditWidget* Owner = nullptr;
};

TSharedRef<ITableRow> STextureVTAuditWidget::GenerateRow(FAuditItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureNoVTAuditRow, OwnerTable).Item(Item).Owner(this);
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
		TSharedPtr<FTextureNoVTAuditItem> Item = MakeShared<FTextureNoVTAuditItem>();
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
			Item->bTextureCollectionCompatible = IsPixelFormatTextureCollectionCompatible(PF)
				&& !Item->bVirtualTextureStreaming;
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

void STextureVTAuditWidget::EvaluateItem(FTextureNoVTAuditItem& Item) const
{
	Item.Recommendations.Reset();
	Item.Status = ETextureNoVTAuditStatus::Pass;

	const int32 MaxSide = FMath::Max(Item.SizeX, Item.SizeY);
	if (Item.bVirtualTextureStreaming)
	{
		Item.Status = ETextureNoVTAuditStatus::Blocked;
		Item.Recommendations.Add(LOCTEXT("VTOnBlocked",
			"普通场景模型贴图不再允许开启 VirtualTextureStreaming。只有地面 RuntimeVirtualTexture 资产走 RVT，源 Texture2D 也应保持 NoVT。"));
	}
	else
	{
		Item.Recommendations.Add(LOCTEXT("NoVTPass",
			"NoVT OK。该贴图可以按普通 Texture2D 使用，也可以加入普通 Texture Collection。"));
	}

	if (!Item.bTextureCollectionCompatible)
	{
		RaiseAuditStatus(Item.Status, ETextureNoVTAuditStatus::Warning);
		Item.Recommendations.Add(FText::Format(
			LOCTEXT("TCFormatWarning", "当前格式 {0} 不在项目 Texture Collection 推荐白名单中；如需加入集合，请确认平台压缩格式。"),
			FText::FromString(Item.PixelFormat)));
	}

	if (MaxSide > LargeTextureWarningSize)
	{
		RaiseAuditStatus(Item.Status, ETextureNoVTAuditStatus::Warning);
		Item.Recommendations.Add(FText::Format(
			LOCTEXT("LargeTextureWarning", "尺寸 {0} 超过当前大图提醒阈值；保持 NoVT，但需要确认平台显存和 mip 预算。"),
			FText::AsNumber(MaxSide)));
	}

	if (!IsPowerOfTwo(Item.SizeX) || !IsPowerOfTwo(Item.SizeY))
	{
		RaiseAuditStatus(Item.Status, ETextureNoVTAuditStatus::Info);
		Item.Recommendations.Add(LOCTEXT("NotPOTInfo", "非 2 的幂尺寸。普通 Texture2D 可以使用，但建议确认 mip、压缩和打包预算。"));
	}
}

void STextureVTAuditWidget::RebuildFilteredItems()
{
	FilteredItems.Reset();
	for (const FAuditItemPtr& It : AllItems)
	{
		const bool bStatusOK =
			(It->Status == ETextureNoVTAuditStatus::Pass    && bShowPass) ||
			(It->Status == ETextureNoVTAuditStatus::Warning && bShowWarning) ||
			(It->Status == ETextureNoVTAuditStatus::Blocked && bShowBlocked) ||
			(It->Status == ETextureNoVTAuditStatus::Info    && bShowInfo);
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

FReply STextureVTAuditWidget::DisableVTOnSelected()
{
	if (SelectedItems.IsEmpty())
	{
		return FReply::Handled();
	}

	const EAppReturnType::Type Confirmation = FMessageDialog::Open(
		EAppMsgType::YesNo,
		FText::Format(LOCTEXT("ConfirmDisableVT", "确定关闭所选 {0} 张贴图的 Virtual Texture Streaming 吗？\n\n操作支持撤销，修改后的资产仍需保存。"), FText::AsNumber(SelectedItems.Num())));
	if (Confirmation != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("DisableVTTransaction", "批量关闭贴图 Virtual Texture Streaming"));
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
	if (ActionStatusTextBlock.IsValid())
	{
		ActionStatusTextBlock->SetText(FText::Format(
			LOCTEXT("DisableVTResult", "已关闭 {0} 张贴图的 VT。资产已标记为待保存，可使用 Ctrl+Z 撤销。"),
			FText::AsNumber(Changed)));
	}
	return FReply::Handled();
}

bool STextureVTAuditWidget::HasSelection() const
{
	return !SelectedItems.IsEmpty();
}

bool STextureVTAuditWidget::HasSelectedVirtualTexture() const
{
	for (const FAuditItemPtr& Item : SelectedItems)
	{
		if (Item.IsValid() && Item->bVirtualTextureStreaming)
		{
			return true;
		}
	}
	return false;
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
		case ETextureNoVTAuditStatus::Pass:    ++Pass; break;
		case ETextureNoVTAuditStatus::Warning: ++Warn; break;
		case ETextureNoVTAuditStatus::Blocked: ++Block; break;
		case ETextureNoVTAuditStatus::Info:    ++Info; break;
		}
		TotalVRAM += It->EstimatedVRAMBytes;
	}
	return FText::Format(
		LOCTEXT("Summary", "共 {0} | Pass {1} · Warning {2} · Blocked {3} · Info {4} | 估算 VRAM {5} MB | 显示 {6} | 规则：普通贴图 NoVT，只有地面 RVT 使用虚拟纹理"),
		FText::AsNumber(AllItems.Num()),
		FText::AsNumber(Pass), FText::AsNumber(Warn), FText::AsNumber(Block), FText::AsNumber(Info),
		FText::AsNumber(TotalVRAM / 1048576),
		FText::AsNumber(FilteredItems.Num()));
}

FSlateColor STextureVTAuditWidget::GetStatusColor(ETextureNoVTAuditStatus Status) const
{
	switch (Status)
	{
	case ETextureNoVTAuditStatus::Pass:    return FSlateColor(FLinearColor(0.15f, 0.35f, 0.15f));
	case ETextureNoVTAuditStatus::Warning: return FSlateColor(FLinearColor(0.5f, 0.4f, 0.1f));
	case ETextureNoVTAuditStatus::Blocked: return FSlateColor(FLinearColor(0.5f, 0.1f, 0.1f));
	case ETextureNoVTAuditStatus::Info:    return FSlateColor(FLinearColor(0.2f, 0.25f, 0.35f));
	}
	return FSlateColor(FLinearColor::Gray);
}

FText STextureVTAuditWidget::StatusToText(ETextureNoVTAuditStatus Status) const
{
	switch (Status)
	{
	case ETextureNoVTAuditStatus::Pass:    return LOCTEXT("StatusPass", "Pass");
	case ETextureNoVTAuditStatus::Warning: return LOCTEXT("StatusWarn", "Warning");
	case ETextureNoVTAuditStatus::Blocked: return LOCTEXT("StatusBlock", "Blocked");
	case ETextureNoVTAuditStatus::Info:    return LOCTEXT("StatusInfo", "Info");
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
