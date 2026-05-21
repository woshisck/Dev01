#include "Tools/StoryEncounter/SStoryEncounterWorkbenchWidget.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Factories/DataAssetFactory.h"
#include "Framework/Commands/GenericCommands.h"
#include "GenericGraphAssetEditor/AssetGraphSchema_GenericGraph.h"
#include "GenericGraphAssetEditor/EdGraph_GenericGraph.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphEdge.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"
#include "GenericGraphFactory.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "PropertyEditorModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SStoryEncounterWorkbenchWidget"

namespace
{
template <typename T>
TArray<T*> CollectAssetsOfClass()
{
	TArray<T*> Out;
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.SearchAllAssets(true);
	}

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Asset : Assets)
	{
		if (T* Loaded = Cast<T>(Asset.GetAsset()))
		{
			Out.Add(Loaded);
		}
	}

	Out.Sort([](const T& Left, const T& Right)
	{
		return Left.GetName() < Right.GetName();
	});
	return Out;
}

TSharedRef<SWidget> MakeTextCell(const FString& Text, const FString& ToolTip = FString())
{
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.ToolTipText(FText::FromString(ToolTip));
}

TSharedRef<SWidget> MakeHeaderText(const FText& Text)
{
	return SNew(STextBlock)
		.Text(Text)
		.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")));
}

FName TextToName(const FText& Text)
{
	const FString Value = Text.ToString().TrimStartAndEnd();
	return Value.IsEmpty() ? NAME_None : FName(*Value);
}

TSharedRef<SWidget> MakeDisabledGraphCanvasWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
		.BorderBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f, 1.f))
		.Padding(0.f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 1.f))
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(24.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DisabledGraphCanvasTitle", "未选择流程图"))
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingMedium")))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.42f, 0.42f, 1.f)))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f).HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DisabledGraphCanvasHint", "从左侧选择一张流程图后，这里会切换到可编辑画布。"))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.34f, 0.34f, 0.34f, 1.f)))
					.AutoWrapText(true)
				]
			]
		];
}

void EnsureGraphEditorData(UStoryEncounterGraph* EncounterGraph)
{
	if (!EncounterGraph)
	{
		return;
	}

#if WITH_EDITORONLY_DATA
	if (EncounterGraph->EdGraph)
	{
		return;
	}

	EncounterGraph->Modify();
	EncounterGraph->EdGraph = CastChecked<UEdGraph_GenericGraph>(
		FBlueprintEditorUtils::CreateNewGraph(
			EncounterGraph,
			NAME_None,
			UEdGraph_GenericGraph::StaticClass(),
			UAssetGraphSchema_GenericGraph::StaticClass()));
	EncounterGraph->EdGraph->bAllowDeletion = false;

	const UEdGraphSchema* Schema = EncounterGraph->EdGraph->GetSchema();
	if (Schema)
	{
		Schema->CreateDefaultNodesForGraph(*EncounterGraph->EdGraph);
	}

	EncounterGraph->MarkPackageDirty();
#endif
}

void SyncGraphRuntimeData(UStoryEncounterGraph* EncounterGraph)
{
	if (!EncounterGraph || !EncounterGraph->EdGraph)
	{
		return;
	}

	if (UEdGraph_GenericGraph* EdGraph = Cast<UEdGraph_GenericGraph>(EncounterGraph->EdGraph))
	{
		EdGraph->RebuildGenericGraph();
	}
}
}

class SStoryEncounterBoardTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryEncounterBoardItem>>
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterBoardTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryEncounterBoardItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FStoryEncounterBoardItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryEncounterBoardItem>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UStoryProductionBoardDA* Board = Item.IsValid() ? Item->Board.Get() : nullptr;
		if (!Board)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Board")) return MakeTextCell(Board->GetName(), Board->GetPathName());
		if (ColumnName == TEXT("Rows")) return MakeTextCell(FString::FromInt(Item->RowCount));
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryEncounterBoardItem> Item;
};

class SStoryEncounterProductionTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryEncounterProductionItem>>
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterProductionTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryEncounterProductionItem>, Item)
		SLATE_ARGUMENT(TWeakPtr<SStoryEncounterWorkbenchWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FStoryEncounterProductionItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryEncounterProductionItem>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		const FStoryProductionRow& Row = Item->Row;
		if (ColumnName == TEXT("Requirement")) return MakeTextCell(Row.RequirementId.ToString());
		if (ColumnName == TEXT("Point")) return MakeTextCell(Row.PointName.ToString());
		if (ColumnName == TEXT("Flow")) return MakeTextCell(Row.EncounterId.ToString());
		if (ColumnName == TEXT("Node")) return MakeTextCell(Row.NodeId.ToString());
		if (ColumnName == TEXT("Status")) return MakeTextCell(FStoryEncounterEditorModel::StatusToChinese(Row.Status));
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryEncounterProductionItem> Item;
	TWeakPtr<SStoryEncounterWorkbenchWidget> OwnerWidget;
};

class SStoryEncounterMapTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryEncounterMapItem>>
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterMapTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryEncounterMapItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FStoryEncounterMapItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryEncounterMapItem>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UStoryEncounterMap* EncounterMap = Item.IsValid() ? Item->EncounterMap.Get() : nullptr;
		if (!EncounterMap)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Encounter")) return MakeTextCell(EncounterMap->EncounterId.ToString(), EncounterMap->GetPathName());
		if (ColumnName == TEXT("Display")) return MakeTextCell(EncounterMap->DisplayName.ToString());
		if (ColumnName == TEXT("Nodes")) return MakeTextCell(FString::FromInt(EncounterMap->Nodes.Num()));
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryEncounterMapItem> Item;
};

class SStoryEncounterGraphTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryEncounterGraphItem>>
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterGraphTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryEncounterGraphItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FStoryEncounterGraphItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryEncounterGraphItem>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UStoryEncounterGraph* EncounterGraph = Item.IsValid() ? Item->EncounterGraph.Get() : nullptr;
		if (!EncounterGraph)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Encounter")) return MakeTextCell(EncounterGraph->EncounterId.ToString(), EncounterGraph->GetPathName());
		if (ColumnName == TEXT("Display")) return MakeTextCell(EncounterGraph->DisplayName.ToString());
		if (ColumnName == TEXT("Nodes")) return MakeTextCell(FString::FromInt(Item->NodeCount));
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryEncounterGraphItem> Item;
};

class SStoryEncounterPointTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryEncounterPointItem>>
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterPointTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryEncounterPointItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FStoryEncounterPointItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryEncounterPointItem>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UStoryEncounterPointDA* EncounterPoint = Item.IsValid() ? Item->EncounterPoint.Get() : nullptr;
		if (!EncounterPoint)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Point")) return MakeTextCell(EncounterPoint->DisplayName.ToString(), EncounterPoint->GetPathName());
		if (ColumnName == TEXT("Flow")) return MakeTextCell(EncounterPoint->EncounterId.ToString());
		if (ColumnName == TEXT("Node")) return MakeTextCell(EncounterPoint->GetStableNodeId().ToString());
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryEncounterPointItem> Item;
};

class SStoryEncounterNodeTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryEncounterNodeItem>>
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterNodeTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryEncounterNodeItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FStoryEncounterNodeItem>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryEncounterNodeItem>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		const FStoryEncounterNode& Node = Item->Node;
		if (ColumnName == TEXT("Node")) return MakeTextCell(Node.NodeId.ToString());
		if (ColumnName == TEXT("Display")) return MakeTextCell(Node.DisplayName.ToString());
		if (ColumnName == TEXT("Kind")) return MakeTextCell(FStoryEncounterEditorModel::NodeKindToChinese(Node.Kind));
		if (ColumnName == TEXT("Actions")) return MakeTextCell(FString::FromInt(Node.Actions.Num()));
		if (ColumnName == TEXT("Next")) return MakeTextCell(Node.NextNodeId.ToString());
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryEncounterNodeItem> Item;
};

class SStoryEncounterMessageTableRow : public SMultiColumnTableRow<TSharedPtr<FStoryEncounterWorkbenchMessage>>
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterMessageTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FStoryEncounterWorkbenchMessage>, Item)
		SLATE_ARGUMENT(TWeakPtr<SStoryEncounterWorkbenchWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FStoryEncounterWorkbenchMessage>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FStoryEncounterWorkbenchMessage>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Severity"))
		{
			return MakeTextCell(FStoryEncounterEditorModel::SeverityToChinese(Item->Severity));
		}
		if (ColumnName == TEXT("Message"))
		{
			return MakeTextCell(Item->Message);
		}
		if (ColumnName == TEXT("Jump"))
		{
			return SNew(SButton)
				.Text(LOCTEXT("JumpToMessageAsset", "定位"))
				.OnClicked_Lambda([Owner = OwnerWidget, Message = Item]()
				{
					if (TSharedPtr<SStoryEncounterWorkbenchWidget> Pinned = Owner.Pin())
					{
						Pinned->OpenAsset(Message.IsValid() ? Message->SourceAsset.Get() : nullptr);
					}
					return FReply::Handled();
				});
		}

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FStoryEncounterWorkbenchMessage> Item;
	TWeakPtr<SStoryEncounterWorkbenchWidget> OwnerWidget;
};

void SStoryEncounterWorkbenchWidget::Construct(const FArguments& InArgs)
{
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = PropertyModule.CreateDetailView(DetailsArgs);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateEncounterGraph", "新建流程图"))
				.OnClicked(this, &SStoryEncounterWorkbenchWidget::OnCreateEncounterMapClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateEncounterPoint", "新建剧情点DA"))
				.OnClicked(this, &SStoryEncounterWorkbenchWidget::OnCreateEncounterPointClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "刷新"))
				.OnClicked(this, &SStoryEncounterWorkbenchWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenSelectedAsset", "打开当前资产"))
				.OnClicked(this, &SStoryEncounterWorkbenchWidget::OnOpenSelectedAssetClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenSelectedGraph", "打开流程图编辑器"))
				.OnClicked(this, &SStoryEncounterWorkbenchWidget::OnOpenSelectedGraphClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SStoryEncounterWorkbenchWidget::GetStatusText)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(6.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.28f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
				[
					MakeHeaderText(LOCTEXT("EncounterGraphHeader", "流程图资产"))
				]
				+ SVerticalBox::Slot().FillHeight(0.38f)
				[
					SAssignNew(GraphListView, SListView<FGraphItemPtr>)
					.ListItemsSource(&GraphRows)
					.OnGenerateRow(this, &SStoryEncounterWorkbenchWidget::GenerateGraphRow)
					.OnSelectionChanged(this, &SStoryEncounterWorkbenchWidget::OnGraphSelectionChanged)
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(TEXT("Encounter")).DefaultLabel(LOCTEXT("EncounterColumn", "流程ID"))
						+ SHeaderRow::Column(TEXT("Display")).DefaultLabel(LOCTEXT("DisplayColumn", "名称"))
						+ SHeaderRow::Column(TEXT("Nodes")).DefaultLabel(LOCTEXT("NodesColumn", "节点数")).FixedWidth(56.f)
					)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						MakeHeaderText(LOCTEXT("EncounterPointsHeader", "剧情点DA"))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("CreatePointNearList", "新建剧情点DA"))
						.OnClicked(this, &SStoryEncounterWorkbenchWidget::OnCreateEncounterPointClicked)
					]
				]
				+ SVerticalBox::Slot().FillHeight(0.62f)
				[
					SAssignNew(PointListView, SListView<FPointItemPtr>)
					.ListItemsSource(&PointRows)
					.OnGenerateRow(this, &SStoryEncounterWorkbenchWidget::GeneratePointRow)
					.OnSelectionChanged(this, &SStoryEncounterWorkbenchWidget::OnPointSelectionChanged)
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(TEXT("Point")).DefaultLabel(LOCTEXT("PointAssetColumn", "剧情点"))
						+ SHeaderRow::Column(TEXT("Flow")).DefaultLabel(LOCTEXT("PointFlowColumn", "流程"))
						+ SHeaderRow::Column(TEXT("Node")).DefaultLabel(LOCTEXT("PointNodeColumn", "节点"))
					)
				]
			]
			+ SSplitter::Slot()
			.Value(0.46f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						MakeHeaderText(LOCTEXT("GraphCanvasHeader", "流程画布"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("OpenGraphEditorSmall", "独立打开"))
						.OnClicked(this, &SStoryEncounterWorkbenchWidget::OnOpenSelectedGraphClicked)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("RefreshGraphCanvas", "刷新画布"))
						.OnClicked_Lambda([this]()
						{
							RebuildGraphEditor();
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SAssignNew(GraphEditorHost, SBox)
					[
						MakeDisabledGraphCanvasWidget()
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.26f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
				[
					MakeHeaderText(LOCTEXT("DetailsHeader", "属性"))
				]
				+ SVerticalBox::Slot().FillHeight(0.58f)
				[
					DetailsView.ToSharedRef()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)
				[
					MakeHeaderText(LOCTEXT("ValidationHeader", "校验"))
				]
				+ SVerticalBox::Slot().FillHeight(0.42f)
				[
					SAssignNew(MessageListView, SListView<FMessagePtr>)
					.ListItemsSource(&MessageRows)
					.OnGenerateRow(this, &SStoryEncounterWorkbenchWidget::GenerateMessageRow)
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(TEXT("Severity")).DefaultLabel(LOCTEXT("SeverityColumn", "级别")).FixedWidth(48.f)
						+ SHeaderRow::Column(TEXT("Message")).DefaultLabel(LOCTEXT("MessageColumn", "问题"))
						+ SHeaderRow::Column(TEXT("Jump")).DefaultLabel(LOCTEXT("JumpColumn", "定位")).FixedWidth(54.f)
					)
				]
			]
		]
	];

	RefreshData(LOCTEXT("InitialStatus", "已加载剧情教学工作台。"));
}

void SStoryEncounterWorkbenchWidget::OpenAsset(UObject* Asset) const
{
	if (!Asset || !GEditor)
	{
		return;
	}

	if (UAssetEditorSubsystem* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		AssetEditor->OpenEditorForAsset(Asset);
	}
}

void SStoryEncounterWorkbenchWidget::RefreshData(const FText& NewStatus)
{
	ProductionBoards.Reset();
	EncounterMaps = CollectAssetsOfClass<UStoryEncounterMap>();
	EncounterGraphs = CollectAssetsOfClass<UStoryEncounterGraph>();
	EncounterPoints = CollectAssetsOfClass<UStoryEncounterPointDA>();

	BoardRows.Reset();

	ProductionRows.Reset();

	MapRows.Reset();
	for (const FStoryEncounterMapItem& Item : FStoryEncounterEditorModel::BuildMapItems(EncounterMaps))
	{
		MapRows.Add(MakeShared<FStoryEncounterMapItem>(Item));
	}

	GraphRows.Reset();
	for (const FStoryEncounterGraphItem& Item : FStoryEncounterEditorModel::BuildGraphItems(EncounterGraphs))
	{
		GraphRows.Add(MakeShared<FStoryEncounterGraphItem>(Item));
	}

	PointRows.Reset();
	for (const FStoryEncounterPointItem& Item : FStoryEncounterEditorModel::BuildPointItems(EncounterPoints))
	{
		PointRows.Add(MakeShared<FStoryEncounterPointItem>(Item));
	}

	RebuildNodeRows(GetSelectedMap());
	RebuildGraphEditor();
	RebuildMessageRows();

	if (BoardListView.IsValid()) BoardListView->RequestListRefresh();
	if (ProductionListView.IsValid()) ProductionListView->RequestListRefresh();
	if (MapListView.IsValid()) MapListView->RequestListRefresh();
	if (GraphListView.IsValid()) GraphListView->RequestListRefresh();
	if (PointListView.IsValid()) PointListView->RequestListRefresh();
	if (NodeListView.IsValid()) NodeListView->RequestListRefresh();
	if (MessageListView.IsValid()) MessageListView->RequestListRefresh();

	StatusText = NewStatus.IsEmpty()
		? FText::Format(LOCTEXT("RefreshStatus", "已刷新：{0} 张流程图，{1} 个剧情点DA，{2} 条校验消息。"),
			FText::AsNumber(GraphRows.Num()),
			FText::AsNumber(PointRows.Num()),
			FText::AsNumber(MessageRows.Num()))
		: NewStatus;
}

void SStoryEncounterWorkbenchWidget::RebuildNodeRows(UStoryEncounterMap* EncounterMap)
{
	NodeRows.Reset();
	for (const FStoryEncounterNodeItem& Item : FStoryEncounterEditorModel::BuildNodeItems(EncounterMap))
	{
		NodeRows.Add(MakeShared<FStoryEncounterNodeItem>(Item));
	}
	if (NodeListView.IsValid())
	{
		NodeListView->RequestListRefresh();
	}
}

void SStoryEncounterWorkbenchWidget::RebuildGraphEditor()
{
	if (!GraphEditorHost.IsValid())
	{
		return;
	}

	UStoryEncounterGraph* EncounterGraph = GetSelectedGraph();
	if (!EncounterGraph)
	{
		GraphEditor.Reset();
		GraphEditorHost->SetContent(MakeDisabledGraphCanvasWidget());
		return;
	}

	EnsureGraphEditorData(EncounterGraph);
	SyncGraphRuntimeData(EncounterGraph);
	if (!EncounterGraph->EdGraph)
	{
		GraphEditorHost->SetContent(
			SNew(STextBlock)
			.Text(LOCTEXT("GraphEditorCreateFailed", "流程图画布创建失败。"))
			.AutoWrapText(true));
		return;
	}

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("StoryGraphCornerText", "YOG 剧情流程");

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(
		this, &SStoryEncounterWorkbenchWidget::OnGraphCanvasSelectionChanged);

	SAssignNew(GraphEditor, SGraphEditor)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EncounterGraph->EdGraph)
		.GraphEvents(GraphEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);

	GraphEditorHost->SetContent(GraphEditor.ToSharedRef());
}

void SStoryEncounterWorkbenchWidget::RebuildMessageRows()
{
	MessageRows.Reset();
	for (UStoryEncounterGraph* EncounterGraph : EncounterGraphs)
	{
		SyncGraphRuntimeData(EncounterGraph);
	}
	for (const FStoryEncounterWorkbenchMessage& Message :
		FStoryEncounterEditorModel::Validate(ProductionBoards, EncounterMaps, EncounterGraphs, EncounterPoints))
	{
		MessageRows.Add(MakeShared<FStoryEncounterWorkbenchMessage>(Message));
	}
	if (MessageListView.IsValid())
	{
		MessageListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SStoryEncounterWorkbenchWidget::GenerateBoardRow(
	FBoardItemPtr Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryEncounterBoardTableRow, OwnerTable)
		.Item(Row);
}

TSharedRef<ITableRow> SStoryEncounterWorkbenchWidget::GenerateProductionRow(
	FProductionItemPtr Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryEncounterProductionTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(SharedThis(this));
}

TSharedRef<ITableRow> SStoryEncounterWorkbenchWidget::GenerateMapRow(
	FMapItemPtr Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryEncounterMapTableRow, OwnerTable)
		.Item(Row);
}

TSharedRef<ITableRow> SStoryEncounterWorkbenchWidget::GenerateGraphRow(
	FGraphItemPtr Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryEncounterGraphTableRow, OwnerTable)
		.Item(Row);
}

TSharedRef<ITableRow> SStoryEncounterWorkbenchWidget::GeneratePointRow(
	FPointItemPtr Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryEncounterPointTableRow, OwnerTable)
		.Item(Row);
}

TSharedRef<ITableRow> SStoryEncounterWorkbenchWidget::GenerateNodeRow(
	FNodeItemPtr Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryEncounterNodeTableRow, OwnerTable)
		.Item(Row);
}

TSharedRef<ITableRow> SStoryEncounterWorkbenchWidget::GenerateMessageRow(
	FMessagePtr Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStoryEncounterMessageTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(SharedThis(this));
}

void SStoryEncounterWorkbenchWidget::OnBoardSelectionChanged(FBoardItemPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedBoardRow = Row;
}

void SStoryEncounterWorkbenchWidget::OnProductionSelectionChanged(FProductionItemPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedProductionRow = Row;
	if (Row.IsValid())
	{
		SelectMapAndNode(Row->Row.EncounterId, Row->Row.NodeId);
	}
}

void SStoryEncounterWorkbenchWidget::OnMapSelectionChanged(FMapItemPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedMapRow = Row;
	SelectedNodeRow.Reset();
	RebuildNodeRows(Row.IsValid() ? Row->EncounterMap.Get() : nullptr);
}

void SStoryEncounterWorkbenchWidget::OnGraphSelectionChanged(FGraphItemPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedGraphRow = Row;
	RebuildGraphEditor();
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(Row.IsValid() ? Row->EncounterGraph.Get() : nullptr);
	}
}

void SStoryEncounterWorkbenchWidget::OnPointSelectionChanged(FPointItemPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedPointRow = Row;
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(Row.IsValid() ? Row->EncounterPoint.Get() : nullptr);
	}
}

void SStoryEncounterWorkbenchWidget::OnGraphCanvasSelectionChanged(const TSet<UObject*>& NewSelection)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	UObject* DetailsObject = nullptr;
	for (UObject* Selection : NewSelection)
	{
		if (UEdNode_GenericGraphNode* GraphNode = Cast<UEdNode_GenericGraphNode>(Selection))
		{
			DetailsObject = GraphNode->GenericGraphNode;
			break;
		}
		if (UEdNode_GenericGraphEdge* GraphEdge = Cast<UEdNode_GenericGraphEdge>(Selection))
		{
			DetailsObject = GraphEdge->GenericGraphEdge;
			break;
		}
		DetailsObject = Selection;
		break;
	}

	DetailsView->SetObject(DetailsObject ? DetailsObject : GetSelectedGraph());
}

void SStoryEncounterWorkbenchWidget::OnNodeSelectionChanged(FNodeItemPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedNodeRow = Row;
}

void SStoryEncounterWorkbenchWidget::SelectMapAndNode(FName EncounterId, FName NodeId)
{
	for (const FGraphItemPtr& GraphRow : GraphRows)
	{
		UStoryEncounterGraph* EncounterGraph = GraphRow.IsValid() ? GraphRow->EncounterGraph.Get() : nullptr;
		if (EncounterGraph && EncounterGraph->EncounterId == EncounterId)
		{
			SelectedGraphRow = GraphRow;
			if (GraphListView.IsValid())
			{
				GraphListView->SetSelection(GraphRow);
			}
			RebuildGraphEditor();
			break;
		}
	}

	for (const FPointItemPtr& PointRow : PointRows)
	{
		UStoryEncounterPointDA* EncounterPoint = PointRow.IsValid() ? PointRow->EncounterPoint.Get() : nullptr;
		if (EncounterPoint && EncounterPoint->EncounterId == EncounterId && EncounterPoint->GetStableNodeId() == NodeId)
		{
			SelectedPointRow = PointRow;
			if (PointListView.IsValid())
			{
				PointListView->SetSelection(PointRow);
			}
			if (DetailsView.IsValid())
			{
				DetailsView->SetObject(EncounterPoint);
			}
			break;
		}
	}

	for (const FMapItemPtr& MapRow : MapRows)
	{
		UStoryEncounterMap* EncounterMap = MapRow.IsValid() ? MapRow->EncounterMap.Get() : nullptr;
		if (EncounterMap && EncounterMap->EncounterId == EncounterId)
		{
			SelectedMapRow = MapRow;
			if (MapListView.IsValid())
			{
				MapListView->SetSelection(MapRow);
			}
			RebuildNodeRows(EncounterMap);
			break;
		}
	}

	for (const FNodeItemPtr& NodeRow : NodeRows)
	{
		if (NodeRow.IsValid() && NodeRow->Node.NodeId == NodeId)
		{
			SelectedNodeRow = NodeRow;
			if (NodeListView.IsValid())
			{
				NodeListView->SetSelection(NodeRow);
			}
			break;
		}
	}
}

FReply SStoryEncounterWorkbenchWidget::OnRefreshClicked()
{
	RefreshData();
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnOpenSelectedAssetClicked() const
{
	if (SelectedPointRow.IsValid())
	{
		OpenAsset(SelectedPointRow->EncounterPoint.Get());
	}
	else if (SelectedGraphRow.IsValid())
	{
		OpenAsset(SelectedGraphRow->EncounterGraph.Get());
	}
	else if (SelectedNodeRow.IsValid())
	{
		OpenAsset(SelectedNodeRow->EncounterMap.Get());
	}
	else if (SelectedMapRow.IsValid())
	{
		OpenAsset(SelectedMapRow->EncounterMap.Get());
	}
	else if (SelectedProductionRow.IsValid())
	{
		OpenAsset(SelectedProductionRow->Board.Get());
	}
	else if (SelectedBoardRow.IsValid())
	{
		OpenAsset(SelectedBoardRow->Board.Get());
	}
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnCreateProductionBoardClicked()
{
	UObject* CreatedAsset = CreateDataAsset(
		UStoryProductionBoardDA::StaticClass(),
		TEXT("/Game/Story/Boards/PB_StoryProductionBoard"));
	UStoryProductionBoardDA* Board = Cast<UStoryProductionBoardDA>(CreatedAsset);
	if (!Board)
	{
		StatusText = LOCTEXT("CreateBoardFailed", "创建制作清单失败。");
		return FReply::Handled();
	}

	Board->Modify();
	FStoryEncounterEditorModel::InitializeNewProductionBoard(Board);
	Board->MarkPackageDirty();

	RefreshData(LOCTEXT("CreateBoardSuccess", "已创建制作清单资产。"));
	SelectBoard(Board);
	OpenAsset(Board);
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnCreateEncounterMapClicked()
{
	UObject* CreatedAsset = CreateGenericGraphAsset(
		UStoryEncounterGraph::StaticClass(),
		TEXT("/Game/Story/Encounters/EG_NewEncounter"));
	UStoryEncounterGraph* EncounterGraph = Cast<UStoryEncounterGraph>(CreatedAsset);
	if (!EncounterGraph)
	{
		StatusText = LOCTEXT("CreateEncounterFailed", "创建流程图失败。");
		return FReply::Handled();
	}

	EncounterGraph->Modify();
	FStoryEncounterEditorModel::InitializeNewEncounterGraph(EncounterGraph, FName(*EncounterGraph->GetName()));
	EnsureGraphEditorData(EncounterGraph);
	EncounterGraph->MarkPackageDirty();

	RefreshData(LOCTEXT("CreateEncounterSuccess", "已创建流程图资产。请在画布右键创建节点，并绑定剧情点DA。"));
	SelectGraph(EncounterGraph);
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnCreateEncounterPointClicked()
{
	FName EncounterId = TEXT("EM_NewEncounter");
	if (UStoryEncounterGraph* EncounterGraph = GetSelectedGraph())
	{
		EncounterId = EncounterGraph->EncounterId.IsNone() ? FName(*EncounterGraph->GetName()) : EncounterGraph->EncounterId;
	}

	UObject* CreatedAsset = CreateDataAsset(
		UStoryEncounterPointDA::StaticClass(),
		TEXT("/Game/Story/EncounterPoints/EP_NewPoint"));
	UStoryEncounterPointDA* EncounterPoint = Cast<UStoryEncounterPointDA>(CreatedAsset);
	if (!EncounterPoint)
	{
		StatusText = LOCTEXT("CreatePointFailed", "创建剧情点失败。");
		return FReply::Handled();
	}

	EncounterPoint->Modify();
	FStoryEncounterEditorModel::InitializeNewEncounterPoint(EncounterPoint, EncounterId, TEXT("entry"));
	EncounterPoint->MarkPackageDirty();

	RefreshData(LOCTEXT("CreatePointSuccess", "已创建剧情点DA。把它绑定到流程图节点，或直接放到关卡触发器上。"));
	SelectPoint(EncounterPoint);
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnOpenSelectedGraphClicked() const
{
	OpenAsset(GetSelectedGraph());
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnAddNodeClicked()
{
	UStoryEncounterMap* EncounterMap = GetSelectedMap();
	if (!EncounterMap)
	{
		return FReply::Handled();
	}

	EncounterMap->Modify();

	FStoryEncounterNode NewNode;
	int32 NextIndex = EncounterMap->Nodes.Num() + 1;
	FName CandidateId;
	do
	{
		CandidateId = FName(*FString::Printf(TEXT("Node_%03d"), NextIndex++));
	}
	while (EncounterMap->FindNode(CandidateId));

	NewNode.NodeId = CandidateId;
	NewNode.DisplayName = LOCTEXT("NewNodeDisplayName", "新剧情点");
	NewNode.PlayerFacingEvent = LOCTEXT("NewNodePlayerEvent", "填写玩家在这里看见或做了什么。");
	EncounterMap->Nodes.Add(NewNode);
	EncounterMap->MarkPackageDirty();

	RebuildNodeRows(EncounterMap);
	RebuildMessageRows();
	SelectMapAndNode(EncounterMap->EncounterId, CandidateId);
	StatusText = LOCTEXT("AddNodeStatus", "已新增节点。");
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnDeleteNodeClicked()
{
	UStoryEncounterMap* EncounterMap = GetSelectedMap();
	if (!EncounterMap || !SelectedNodeRow.IsValid() || !EncounterMap->Nodes.IsValidIndex(SelectedNodeRow->NodeIndex))
	{
		return FReply::Handled();
	}

	const FName DeletedNodeId = SelectedNodeRow->Node.NodeId;
	EncounterMap->Modify();
	EncounterMap->Nodes.RemoveAt(SelectedNodeRow->NodeIndex);
	for (FStoryEncounterNode& Node : EncounterMap->Nodes)
	{
		if (Node.NextNodeId == DeletedNodeId)
		{
			Node.NextNodeId = NAME_None;
		}
	}
	EncounterMap->MarkPackageDirty();

	SelectedNodeRow.Reset();
	RebuildNodeRows(EncounterMap);
	RebuildMessageRows();
	StatusText = LOCTEXT("DeleteNodeStatus", "已删除节点，并清理指向它的线性连接。");
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnAddWeakHintClicked()
{
	if (FStoryEncounterNode* Node = GetMutableSelectedNode())
	{
		if (UStoryEncounterMap* EncounterMap = GetSelectedMap())
		{
			EncounterMap->Modify();
			FStoryEncounterAction Action;
			Action.Kind = EStoryEncounterActionKind::WeakHint;
			Action.Title = LOCTEXT("WeakHintDefaultTitle", "细微提示");
			Action.Body = LOCTEXT("WeakHintDefaultBody", "填写玩家需要看见的一句弱提示。");
			Node->Actions.Add(Action);
			EncounterMap->MarkPackageDirty();
			MarkSelectedNodeChanged();
			RebuildMessageRows();
			StatusText = LOCTEXT("AddWeakHintStatus", "已给当前节点添加弱提示。");
		}
	}
	return FReply::Handled();
}

FReply SStoryEncounterWorkbenchWidget::OnAddRecordProgressClicked()
{
	if (FStoryEncounterNode* Node = GetMutableSelectedNode())
	{
		if (UStoryEncounterMap* EncounterMap = GetSelectedMap())
		{
			EncounterMap->Modify();
			FStoryEncounterAction Action;
			Action.Kind = EStoryEncounterActionKind::RecordProgress;
			Action.ProgressKey = FName(*FString::Printf(TEXT("%s_done"), *Node->NodeId.ToString()));
			Action.ProgressLabel = Node->DisplayName.IsEmpty() ? FText::FromName(Node->NodeId) : Node->DisplayName;
			Node->Actions.Add(Action);
			EncounterMap->MarkPackageDirty();
			MarkSelectedNodeChanged();
			RebuildMessageRows();
			StatusText = LOCTEXT("AddRecordProgressStatus", "已给当前节点添加进度记录，隐藏Tag会自动生成。");
		}
	}
	return FReply::Handled();
}

FText SStoryEncounterWorkbenchWidget::GetStatusText() const
{
	return StatusText;
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeTitleText() const
{
	if (!SelectedNodeRow.IsValid())
	{
		return LOCTEXT("NoSelectedNode", "选择一个节点开始配置");
	}

	return FText::Format(LOCTEXT("SelectedNodeTitle", "{0} / {1}"),
		FText::FromName(SelectedNodeRow->Node.NodeId),
		SelectedNodeRow->Node.DisplayName);
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeIdText() const
{
	return SelectedNodeRow.IsValid() ? FText::FromName(SelectedNodeRow->Node.NodeId) : FText::GetEmpty();
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeDisplayNameText() const
{
	return SelectedNodeRow.IsValid() ? SelectedNodeRow->Node.DisplayName : FText::GetEmpty();
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeKindText() const
{
	if (!SelectedNodeRow.IsValid())
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("NodeKindSummary", "类型：{0}"),
		FText::FromString(FStoryEncounterEditorModel::NodeKindToChinese(SelectedNodeRow->Node.Kind)));
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeEventText() const
{
	return SelectedNodeRow.IsValid() ? SelectedNodeRow->Node.PlayerFacingEvent : FText::GetEmpty();
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeFirePolicyText() const
{
	if (!SelectedNodeRow.IsValid())
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("NodeFirePolicySummary", "触发次数：{0}"),
		FText::FromString(FStoryEncounterEditorModel::FirePolicyToChinese(SelectedNodeRow->Node.FirePolicy)));
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeConditionText() const
{
	if (!SelectedNodeRow.IsValid())
	{
		return FText::GetEmpty();
	}

	FName EncounterId = NAME_None;
	if (UStoryEncounterMap* EncounterMap = SelectedNodeRow->EncounterMap.Get())
	{
		EncounterId = EncounterMap->EncounterId;
	}

	return FText::Format(LOCTEXT("NodeConditionSummary", "触发条件：{0}"),
		FText::FromString(FStoryEncounterEditorModel::DescribeCondition(EncounterId, SelectedNodeRow->Node.Condition)));
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeNextNodeText() const
{
	return SelectedNodeRow.IsValid() ? FText::FromName(SelectedNodeRow->Node.NextNodeId) : FText::GetEmpty();
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeActionSummaryText() const
{
	if (!SelectedNodeRow.IsValid())
	{
		return FText::GetEmpty();
	}

	FName EncounterId = NAME_None;
	if (UStoryEncounterMap* EncounterMap = SelectedNodeRow->EncounterMap.Get())
	{
		EncounterId = EncounterMap->EncounterId;
	}

	FString Summary = TEXT("动作：");
	if (SelectedNodeRow->Node.Actions.Num() == 0)
	{
		Summary += TEXT("\n- 暂无动作");
	}
	else
	{
		for (const FStoryEncounterAction& Action : SelectedNodeRow->Node.Actions)
		{
			Summary += FString::Printf(TEXT("\n- %s"),
				*FStoryEncounterEditorModel::DescribeAction(EncounterId, Action));
		}
	}
	return FText::FromString(Summary);
}

FText SStoryEncounterWorkbenchWidget::GetSelectedNodeProgressTipText() const
{
	if (!SelectedNodeRow.IsValid())
	{
		return FText::GetEmpty();
	}

	return LOCTEXT("NodeProgressTip",
		"进度键只写稳定英文，例如 weapon_echo_seen；真正存档Tag会按 Story.Encounter.Progress.<流程ID>.<进度键> 自动生成。");
}

bool SStoryEncounterWorkbenchWidget::HasSelectedNode() const
{
	return SelectedNodeRow.IsValid() && SelectedNodeRow->EncounterMap.IsValid();
}

bool SStoryEncounterWorkbenchWidget::HasSelectedMap() const
{
	return GetSelectedMap() != nullptr;
}

void SStoryEncounterWorkbenchWidget::OnSelectedNodeIdCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (FStoryEncounterNode* Node = GetMutableSelectedNode())
	{
		const FName NewId = TextToName(NewText);
		if (NewId.IsNone() || NewId == Node->NodeId)
		{
			return;
		}

		if (UStoryEncounterMap* EncounterMap = GetSelectedMap())
		{
			if (EncounterMap->FindNode(NewId))
			{
				StatusText = LOCTEXT("DuplicateNodeIdStatus", "节点ID已存在，未修改。");
				return;
			}

			EncounterMap->Modify();
			const FName OldId = Node->NodeId;
			Node->NodeId = NewId;
			for (FStoryEncounterNode& OtherNode : EncounterMap->Nodes)
			{
				if (OtherNode.NextNodeId == OldId)
				{
					OtherNode.NextNodeId = NewId;
				}
			}
			EncounterMap->MarkPackageDirty();
			MarkSelectedNodeChanged();
			RebuildMessageRows();
			StatusText = LOCTEXT("NodeIdChangedStatus", "节点ID已修改。");
		}
	}
}

void SStoryEncounterWorkbenchWidget::OnSelectedNodeDisplayNameCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (FStoryEncounterNode* Node = GetMutableSelectedNode())
	{
		if (UStoryEncounterMap* EncounterMap = GetSelectedMap())
		{
			EncounterMap->Modify();
			Node->DisplayName = NewText;
			EncounterMap->MarkPackageDirty();
			MarkSelectedNodeChanged();
			StatusText = LOCTEXT("NodeDisplayNameChangedStatus", "节点名称已修改。");
		}
	}
}

void SStoryEncounterWorkbenchWidget::OnSelectedNodeEventCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (FStoryEncounterNode* Node = GetMutableSelectedNode())
	{
		if (UStoryEncounterMap* EncounterMap = GetSelectedMap())
		{
			EncounterMap->Modify();
			Node->PlayerFacingEvent = NewText;
			EncounterMap->MarkPackageDirty();
			MarkSelectedNodeChanged();
			StatusText = LOCTEXT("NodeEventChangedStatus", "节点描述已修改。");
		}
	}
}

void SStoryEncounterWorkbenchWidget::OnSelectedNodeNextNodeCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (FStoryEncounterNode* Node = GetMutableSelectedNode())
	{
		if (UStoryEncounterMap* EncounterMap = GetSelectedMap())
		{
			EncounterMap->Modify();
			Node->NextNodeId = TextToName(NewText);
			EncounterMap->MarkPackageDirty();
			MarkSelectedNodeChanged();
			RebuildMessageRows();
			StatusText = LOCTEXT("NodeNextChangedStatus", "下一个节点已修改。");
		}
	}
}

UStoryEncounterMap* SStoryEncounterWorkbenchWidget::GetSelectedMap() const
{
	if (SelectedNodeRow.IsValid() && SelectedNodeRow->EncounterMap.IsValid())
	{
		return SelectedNodeRow->EncounterMap.Get();
	}
	if (SelectedMapRow.IsValid() && SelectedMapRow->EncounterMap.IsValid())
	{
		return SelectedMapRow->EncounterMap.Get();
	}
	return nullptr;
}

UStoryEncounterGraph* SStoryEncounterWorkbenchWidget::GetSelectedGraph() const
{
	return SelectedGraphRow.IsValid() && SelectedGraphRow->EncounterGraph.IsValid()
		? SelectedGraphRow->EncounterGraph.Get()
		: nullptr;
}

UStoryEncounterPointDA* SStoryEncounterWorkbenchWidget::GetSelectedPoint() const
{
	return SelectedPointRow.IsValid() && SelectedPointRow->EncounterPoint.IsValid()
		? SelectedPointRow->EncounterPoint.Get()
		: nullptr;
}

FStoryEncounterNode* SStoryEncounterWorkbenchWidget::GetMutableSelectedNode() const
{
	UStoryEncounterMap* EncounterMap = GetSelectedMap();
	if (!EncounterMap || !SelectedNodeRow.IsValid() || !EncounterMap->Nodes.IsValidIndex(SelectedNodeRow->NodeIndex))
	{
		return nullptr;
	}
	return &EncounterMap->Nodes[SelectedNodeRow->NodeIndex];
}

UObject* SStoryEncounterWorkbenchWidget::CreateDataAsset(UClass* AssetClass, const FString& BasePackagePath) const
{
	if (!AssetClass)
	{
		return nullptr;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(BasePackagePath, TEXT(""), UniquePackageName, UniqueAssetName);

	const FString PackagePath = FPackageName::GetLongPackagePath(UniquePackageName);
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = AssetClass;
	return AssetTools.CreateAsset(UniqueAssetName, PackagePath, AssetClass, Factory);
}

UObject* SStoryEncounterWorkbenchWidget::CreateGenericGraphAsset(UClass* GraphClass, const FString& BasePackagePath) const
{
	if (!GraphClass)
	{
		return nullptr;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(BasePackagePath, TEXT(""), UniquePackageName, UniqueAssetName);

	const FString PackagePath = FPackageName::GetLongPackagePath(UniquePackageName);
	UGenericGraphFactory* Factory = NewObject<UGenericGraphFactory>();
	Factory->GenericGraphClass = GraphClass;
	return AssetTools.CreateAsset(UniqueAssetName, PackagePath, GraphClass, Factory);
}

void SStoryEncounterWorkbenchWidget::SelectBoard(UStoryProductionBoardDA* Board)
{
	if (!Board)
	{
		return;
	}

	for (const FBoardItemPtr& BoardRow : BoardRows)
	{
		if (BoardRow.IsValid() && BoardRow->Board.Get() == Board)
		{
			SelectedBoardRow = BoardRow;
			if (BoardListView.IsValid())
			{
				BoardListView->SetSelection(BoardRow);
			}
			break;
		}
	}
}

void SStoryEncounterWorkbenchWidget::SelectMap(UStoryEncounterMap* EncounterMap)
{
	if (!EncounterMap)
	{
		return;
	}

	for (const FMapItemPtr& MapRow : MapRows)
	{
		if (MapRow.IsValid() && MapRow->EncounterMap.Get() == EncounterMap)
		{
			SelectedMapRow = MapRow;
			if (MapListView.IsValid())
			{
				MapListView->SetSelection(MapRow);
			}
			RebuildNodeRows(EncounterMap);
			break;
		}
	}
}

void SStoryEncounterWorkbenchWidget::SelectGraph(UStoryEncounterGraph* EncounterGraph)
{
	if (!EncounterGraph)
	{
		return;
	}

	for (const FGraphItemPtr& GraphRow : GraphRows)
	{
		if (GraphRow.IsValid() && GraphRow->EncounterGraph.Get() == EncounterGraph)
		{
			SelectedGraphRow = GraphRow;
			if (GraphListView.IsValid())
			{
				GraphListView->SetSelection(GraphRow);
			}
			if (DetailsView.IsValid())
			{
				DetailsView->SetObject(EncounterGraph);
			}
			RebuildGraphEditor();
			break;
		}
	}
}

void SStoryEncounterWorkbenchWidget::SelectPoint(UStoryEncounterPointDA* EncounterPoint)
{
	if (!EncounterPoint)
	{
		return;
	}

	for (const FPointItemPtr& PointRow : PointRows)
	{
		if (PointRow.IsValid() && PointRow->EncounterPoint.Get() == EncounterPoint)
		{
			SelectedPointRow = PointRow;
			if (PointListView.IsValid())
			{
				PointListView->SetSelection(PointRow);
			}
			if (DetailsView.IsValid())
			{
				DetailsView->SetObject(EncounterPoint);
			}
			break;
		}
	}
}

void SStoryEncounterWorkbenchWidget::MarkSelectedNodeChanged()
{
	if (FStoryEncounterNode* Node = GetMutableSelectedNode())
	{
		if (SelectedNodeRow.IsValid())
		{
			SelectedNodeRow->Node = *Node;
		}
		if (NodeListView.IsValid())
		{
			NodeListView->RequestListRefresh();
		}
	}
}

#undef LOCTEXT_NAMESPACE
