#include "Tools/SMetaProgressionWorkbenchWidget.h"

#include "Tools/UMetaNodeEditProxy.h"
#include "MetaProgression/MetaProgressionSettings.h"
#include "DesktopPlatformModule.h"
#include "Engine/DataTable.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformFileManager.h"
#include "IDesktopPlatform.h"
#include "IDetailsView.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SMetaProgressionWorkbenchWidget"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
	TSharedRef<SWidget> MakeTextCell(const FString& Text, const FString& ToolTip = {})
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip));
	}

	FString EffectTypeLabel(EMetaUpgradeEffectType Type)
	{
		switch (Type)
		{
		case EMetaUpgradeEffectType::StatBoost:    return TEXT("数值");
		case EMetaUpgradeEffectType::FeatureUnlock: return TEXT("解锁");
		default: return TEXT("?");
		}
	}

	FString CostSummary(const TArray<FMetaCurrencyCost>& Costs)
	{
		if (Costs.IsEmpty()) return TEXT("-");
		FString Out;
		for (const FMetaCurrencyCost& C : Costs)
		{
			if (!Out.IsEmpty()) Out += TEXT(", ");
			Out += FString::Printf(TEXT("%s×%d"), *C.CurrencyTag.ToString(), C.Amount);
		}
		return Out;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Table row widget
// ─────────────────────────────────────────────────────────────────────────────

class SMetaNodeTableRow : public SMultiColumnTableRow<TSharedPtr<FMetaNodeListRow>>
{
public:
	SLATE_BEGIN_ARGS(SMetaNodeTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FMetaNodeListRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FMetaNodeListRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FMetaNodeListRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid()) return MakeTextCell(TEXT("-"));
		const FMetaUpgradeNodeRow& D = Item->Data;

		if (ColumnName == TEXT("RowName"))  return MakeTextCell(Item->RowName.ToString());
		if (ColumnName == TEXT("Display"))  return MakeTextCell(D.DisplayName.ToString());
		if (ColumnName == TEXT("Side"))     return MakeTextCell(D.Side == EMetaSide::Mystic ? TEXT("神秘") : TEXT("血肉"));
		if (ColumnName == TEXT("MaxLevel")) return MakeTextCell(FString::FromInt(D.MaxLevel));
		if (ColumnName == TEXT("Effect"))   return MakeTextCell(EffectTypeLabel(D.EffectType));
		if (ColumnName == TEXT("Cost"))     return MakeTextCell(CostSummary(D.CostsPerLevel));
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FMetaNodeListRow> Item;
};

// ─────────────────────────────────────────────────────────────────────────────
// Widget
// ─────────────────────────────────────────────────────────────────────────────

void SMetaProgressionWorkbenchWidget::Construct(const FArguments& InArgs)
{
	EditProxy = TStrongObjectPtr<UMetaNodeEditProxy>(
		NewObject<UMetaNodeEditProxy>(GetTransientPackage(), NAME_None, RF_Transient));

	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bSearchInitialKeyFocus = false;
	NodeDetailsView = PropModule.CreateDetailView(DetailsArgs);
	NodeDetailsView->SetObject(nullptr);

	ChildSlot
	[
		SNew(SVerticalBox)
		// Toolbar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "刷新"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ExportCSV", "导出 CSV"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnExportCsvClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ImportCSV", "导入 CSV"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnImportCsvClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Validate", "验证"))
				.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnValidateClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock).Text_Lambda([this]() { return StatusText; })
			]
		]
		// Main splitter
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.38f)
			[
				SNew(SBorder).Padding(4.f)
				[
					SNew(SVerticalBox)
					// Side filter buttons
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("AllSide", "全部"))
							.ButtonColorAndOpacity(this, &SMetaProgressionWorkbenchWidget::GetSideFilterColor, -1)
							.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSideFilterClicked, -1)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("FleshSide", "血肉侧"))
							.ButtonColorAndOpacity(this, &SMetaProgressionWorkbenchWidget::GetSideFilterColor, 0)
							.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSideFilterClicked, 0)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("MysticSide", "神秘侧"))
							.ButtonColorAndOpacity(this, &SMetaProgressionWorkbenchWidget::GetSideFilterColor, 1)
							.OnClicked(this, &SMetaProgressionWorkbenchWidget::OnSideFilterClicked, 1)
						]
					]
					// Node list
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(NodeListView, SListView<FNodeRowPtr>)
						.ListItemsSource(&FilteredRows)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SMetaProgressionWorkbenchWidget::GenerateNodeRow)
						.OnSelectionChanged(this, &SMetaProgressionWorkbenchWidget::OnNodeSelectionChanged)
						.HeaderRow(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("RowName")).DefaultLabel(LOCTEXT("RowNameCol", "行名")).FillWidth(0.22f)
							+ SHeaderRow::Column(TEXT("Display")).DefaultLabel(LOCTEXT("DisplayCol", "显示名")).FillWidth(0.24f)
							+ SHeaderRow::Column(TEXT("Side")).DefaultLabel(LOCTEXT("SideCol", "侧")).FixedWidth(46.f)
							+ SHeaderRow::Column(TEXT("MaxLevel")).DefaultLabel(LOCTEXT("MaxLvCol", "最高级")).FixedWidth(52.f)
							+ SHeaderRow::Column(TEXT("Effect")).DefaultLabel(LOCTEXT("EffectCol", "效果")).FixedWidth(44.f)
							+ SHeaderRow::Column(TEXT("Cost")).DefaultLabel(LOCTEXT("CostCol", "花费")).FillWidth(0.30f)
						)
					]
				]
			]
			+ SSplitter::Slot().Value(0.62f)
			[
				SNew(SBorder).Padding(4.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DetailsTitle", "节点详情（编辑后即时写入 DataTable）"))
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						NodeDetailsView.ToSharedRef()
					]
				]
			]
		]
		// Status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f, 2.f)
		[
			SNew(STextBlock)
			.Text(this, &SMetaProgressionWorkbenchWidget::GetStatsSummaryText)
		]
	];

	RefreshData();
}

SMetaProgressionWorkbenchWidget::~SMetaProgressionWorkbenchWidget()
{
	EditProxy.Reset();
}

UDataTable* SMetaProgressionWorkbenchWidget::GetUpgradeTable() const
{
	const UMetaProgressionSettings* Settings = GetDefault<UMetaProgressionSettings>();
	if (!Settings) return nullptr;
	return Settings->MetaUpgradeNodeTable.LoadSynchronous();
}

void SMetaProgressionWorkbenchWidget::RefreshData(const FText& NewStatus)
{
	AllRows.Reset();
	UDataTable* Table = GetUpgradeTable();
	if (Table)
	{
		for (const auto& Pair : Table->GetRowMap())
		{
			const FMetaUpgradeNodeRow* Row = reinterpret_cast<const FMetaUpgradeNodeRow*>(Pair.Value);
			if (Row)
			{
				AllRows.Add(MakeShared<FMetaNodeListRow>(Pair.Key, *Row));
			}
		}
		AllRows.Sort([](const FNodeRowPtr& A, const FNodeRowPtr& B)
		{
			return A->RowName.LexicalLess(B->RowName);
		});
	}

	RebuildFilteredRows();

	StatusText = !NewStatus.IsEmpty()
		? NewStatus
		: FText::Format(LOCTEXT("LoadedStatus", "已加载 {0} 个节点"), FText::AsNumber(AllRows.Num()));
}

void SMetaProgressionWorkbenchWidget::RebuildFilteredRows()
{
	FilteredRows.Reset();
	for (const FNodeRowPtr& Row : AllRows)
	{
		const bool bPass =
			SideFilter == -1 ||
			(SideFilter == 0 && Row->Data.Side == EMetaSide::Flesh) ||
			(SideFilter == 1 && Row->Data.Side == EMetaSide::Mystic);
		if (bPass) FilteredRows.Add(Row);
	}
	if (NodeListView) NodeListView->RequestListRefresh();
}

TSharedRef<ITableRow> SMetaProgressionWorkbenchWidget::GenerateNodeRow(FNodeRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SMetaNodeTableRow, OwnerTable).Item(Row);
}

void SMetaProgressionWorkbenchWidget::OnNodeSelectionChanged(FNodeRowPtr Row, ESelectInfo::Type /*SelectInfo*/)
{
	SelectedRow = Row;
	if (Row.IsValid() && EditProxy)
	{
		EditProxy->LoadFromRow(GetUpgradeTable(), Row->RowName);
		NodeDetailsView->SetObject(EditProxy.Get());
	}
	else
	{
		NodeDetailsView->SetObject(nullptr);
	}
}

FReply SMetaProgressionWorkbenchWidget::OnRefreshClicked()
{
	RefreshData();
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnExportCsvClicked()
{
	UDataTable* Table = GetUpgradeTable();
	if (!Table)
	{
		StatusText = LOCTEXT("NoTableExport", "未配置 MetaUpgradeNodeTable，无法导出");
		return FReply::Handled();
	}

	const FString CSV = Table->GetTableAsCSV();
	const FString TimeStamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString OutPath = FPaths::ProjectSavedDir() / TEXT("Balance") / FString::Printf(TEXT("MetaNodes_%s.csv"), *TimeStamp);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutPath), true);

	if (FFileHelper::SaveStringToFile(CSV, *OutPath))
	{
		StatusText = FText::Format(LOCTEXT("ExportOK", "导出成功：{0}"), FText::FromString(OutPath));
	}
	else
	{
		StatusText = LOCTEXT("ExportFail", "导出失败，检查路径权限");
	}
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnImportCsvClicked()
{
	UDataTable* Table = GetUpgradeTable();
	if (!Table)
	{
		StatusText = LOCTEXT("NoTableImport", "未配置 MetaUpgradeNodeTable，无法导入");
		return FReply::Handled();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();

	TArray<FString> OutFiles;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("选择 CSV 文件"),
		FPaths::ProjectSavedDir() / TEXT("Balance"),
		TEXT(""),
		TEXT("CSV 文件 (*.csv)|*.csv"),
		EFileDialogFlags::None,
		OutFiles);

	if (!bOpened || OutFiles.IsEmpty()) return FReply::Handled();

	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *OutFiles[0]))
	{
		StatusText = LOCTEXT("ImportReadFail", "读取文件失败");
		return FReply::Handled();
	}

	// Validate via temporary table
	UDataTable* TempTable = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
	TempTable->RowStruct = Table->RowStruct;
	const TArray<FString> Problems = TempTable->CreateTableFromCSVString(CSVContent);
	if (!Problems.IsEmpty())
	{
		FString ProblemStr;
		for (const FString& P : Problems) { ProblemStr += TEXT("• ") + P + TEXT("\n"); }
		StatusText = FText::Format(LOCTEXT("ImportValidateFail", "预检失败：\n{0}"), FText::FromString(ProblemStr));
		return FReply::Handled();
	}

	// Apply to the real table
	{
		FScopedTransaction Transaction(LOCTEXT("ImportCSVTx", "导入 MetaNode CSV"));
		Table->Modify();
		Table->CreateTableFromCSVString(CSVContent);
		Table->MarkPackageDirty();
	}

	RefreshData(LOCTEXT("ImportOK", "CSV 导入成功"));
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnValidateClicked()
{
	UDataTable* Table = GetUpgradeTable();
	if (!Table)
	{
		StatusText = LOCTEXT("NoTableValidate", "未配置 MetaUpgradeNodeTable");
		return FReply::Handled();
	}

	TArray<FString> Errors;
	const TMap<FName, uint8*>& RowMap = Table->GetRowMap();

	for (const auto& Pair : RowMap)
	{
		const FName RowName = Pair.Key;
		const FMetaUpgradeNodeRow* Row = reinterpret_cast<const FMetaUpgradeNodeRow*>(Pair.Value);
		if (!Row) continue;

		if (Row->MaxLevel <= 0)
		{
			Errors.Add(FString::Printf(TEXT("[%s] MaxLevel 必须 > 0"), *RowName.ToString()));
		}
		for (const FMetaCurrencyCost& Cost : Row->CostsPerLevel)
		{
			if (!Cost.CurrencyTag.IsValid())
			{
				Errors.Add(FString::Printf(TEXT("[%s] 货币 Tag 无效"), *RowName.ToString()));
			}
			if (Cost.Amount < 0)
			{
				Errors.Add(FString::Printf(TEXT("[%s] 货币数量不能为负"), *RowName.ToString()));
			}
		}
		for (const FName& Prereq : Row->Prerequisites)
		{
			if (!RowMap.Contains(Prereq))
			{
				Errors.Add(FString::Printf(TEXT("[%s] 前置节点 '%s' 不存在"), *RowName.ToString(), *Prereq.ToString()));
			}
		}
	}

	if (Errors.IsEmpty())
	{
		StatusText = FText::Format(LOCTEXT("ValidateOK", "验证通过：{0} 个节点，无问题"), FText::AsNumber(RowMap.Num()));
	}
	else
	{
		FString ErrStr;
		for (const FString& E : Errors) { ErrStr += TEXT("• ") + E + TEXT("\n"); }
		StatusText = FText::Format(LOCTEXT("ValidateFail", "验证失败 {0} 项：\n{1}"), FText::AsNumber(Errors.Num()), FText::FromString(ErrStr));
	}
	return FReply::Handled();
}

FReply SMetaProgressionWorkbenchWidget::OnSideFilterClicked(int32 NewFilter)
{
	SideFilter = NewFilter;
	RebuildFilteredRows();
	return FReply::Handled();
}

FText SMetaProgressionWorkbenchWidget::GetStatsSummaryText() const
{
	int32 Mystic = 0, Flesh = 0;
	for (const FNodeRowPtr& R : AllRows)
	{
		if (R->Data.Side == EMetaSide::Mystic) ++Mystic; else ++Flesh;
	}
	return FText::Format(
		LOCTEXT("StatsSummary", "共 {0} 个节点  |  血肉侧: {1}  |  神秘侧: {2}"),
		FText::AsNumber(AllRows.Num()),
		FText::AsNumber(Flesh),
		FText::AsNumber(Mystic));
}

FSlateColor SMetaProgressionWorkbenchWidget::GetSideFilterColor(int32 Filter) const
{
	return SideFilter == Filter
		? FSlateColor(FLinearColor(0.2f, 0.5f, 0.9f))
		: FSlateColor(FLinearColor(0.28f, 0.28f, 0.28f));
}

#undef LOCTEXT_NAMESPACE
