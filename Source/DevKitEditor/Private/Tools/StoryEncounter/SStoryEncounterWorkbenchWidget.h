#pragma once

#include "CoreMinimal.h"
#include "Tools/StoryEncounter/StoryEncounterEditorModel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class SGraphEditor;

class SStoryEncounterWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStoryEncounterWorkbenchWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenAsset(UObject* Asset) const;

private:
	using FBoardItemPtr = TSharedPtr<FStoryEncounterBoardItem>;
	using FProductionItemPtr = TSharedPtr<FStoryEncounterProductionItem>;
	using FMapItemPtr = TSharedPtr<FStoryEncounterMapItem>;
	using FGraphItemPtr = TSharedPtr<FStoryEncounterGraphItem>;
	using FPointItemPtr = TSharedPtr<FStoryEncounterPointItem>;
	using FNodeItemPtr = TSharedPtr<FStoryEncounterNodeItem>;
	using FMessagePtr = TSharedPtr<FStoryEncounterWorkbenchMessage>;

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	void RebuildNodeRows(UStoryEncounterMap* EncounterMap);
	void RebuildGraphEditor();
	void RebuildMessageRows();

	TSharedRef<ITableRow> GenerateBoardRow(FBoardItemPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateProductionRow(FProductionItemPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateMapRow(FMapItemPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateGraphRow(FGraphItemPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GeneratePointRow(FPointItemPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateNodeRow(FNodeItemPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateMessageRow(FMessagePtr Row, const TSharedRef<STableViewBase>& OwnerTable);

	void OnBoardSelectionChanged(FBoardItemPtr Row, ESelectInfo::Type SelectInfo);
	void OnProductionSelectionChanged(FProductionItemPtr Row, ESelectInfo::Type SelectInfo);
	void OnMapSelectionChanged(FMapItemPtr Row, ESelectInfo::Type SelectInfo);
	void OnGraphSelectionChanged(FGraphItemPtr Row, ESelectInfo::Type SelectInfo);
	void OnPointSelectionChanged(FPointItemPtr Row, ESelectInfo::Type SelectInfo);
	void OnGraphCanvasSelectionChanged(const TSet<UObject*>& NewSelection);
	void OnNodeSelectionChanged(FNodeItemPtr Row, ESelectInfo::Type SelectInfo);
	void SelectMapAndNode(FName EncounterId, FName NodeId);

	FReply OnRefreshClicked();
	FReply OnOpenSelectedAssetClicked() const;
	FReply OnCreateProductionBoardClicked();
	FReply OnCreateEncounterMapClicked();
	FReply OnCreateEncounterPointClicked();
	FReply OnOpenSelectedGraphClicked() const;
	FReply OnAddNodeClicked();
	FReply OnDeleteNodeClicked();
	FReply OnAddWeakHintClicked();
	FReply OnAddRecordProgressClicked();

	FText GetStatusText() const;
	FText GetSelectedNodeTitleText() const;
	FText GetSelectedNodeIdText() const;
	FText GetSelectedNodeDisplayNameText() const;
	FText GetSelectedNodeKindText() const;
	FText GetSelectedNodeEventText() const;
	FText GetSelectedNodeFirePolicyText() const;
	FText GetSelectedNodeConditionText() const;
	FText GetSelectedNodeNextNodeText() const;
	FText GetSelectedNodeActionSummaryText() const;
	FText GetSelectedNodeProgressTipText() const;
	bool HasSelectedNode() const;
	bool HasSelectedMap() const;

	void OnSelectedNodeIdCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnSelectedNodeDisplayNameCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnSelectedNodeEventCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnSelectedNodeNextNodeCommitted(const FText& NewText, ETextCommit::Type CommitType);

	UStoryEncounterMap* GetSelectedMap() const;
	UStoryEncounterGraph* GetSelectedGraph() const;
	UStoryEncounterPointDA* GetSelectedPoint() const;
	FStoryEncounterNode* GetMutableSelectedNode() const;
	UObject* CreateDataAsset(UClass* AssetClass, const FString& BasePackagePath) const;
	UObject* CreateGenericGraphAsset(UClass* GraphClass, const FString& BasePackagePath) const;
	void SelectBoard(UStoryProductionBoardDA* Board);
	void SelectMap(UStoryEncounterMap* EncounterMap);
	void SelectGraph(UStoryEncounterGraph* EncounterGraph);
	void SelectPoint(UStoryEncounterPointDA* EncounterPoint);
	void MarkSelectedNodeChanged();

	TArray<UStoryProductionBoardDA*> ProductionBoards;
	TArray<UStoryEncounterMap*> EncounterMaps;
	TArray<UStoryEncounterGraph*> EncounterGraphs;
	TArray<UStoryEncounterPointDA*> EncounterPoints;
	TArray<FBoardItemPtr> BoardRows;
	TArray<FProductionItemPtr> ProductionRows;
	TArray<FMapItemPtr> MapRows;
	TArray<FGraphItemPtr> GraphRows;
	TArray<FPointItemPtr> PointRows;
	TArray<FNodeItemPtr> NodeRows;
	TArray<FMessagePtr> MessageRows;

	TSharedPtr<SListView<FBoardItemPtr>> BoardListView;
	TSharedPtr<SListView<FProductionItemPtr>> ProductionListView;
	TSharedPtr<SListView<FMapItemPtr>> MapListView;
	TSharedPtr<SListView<FGraphItemPtr>> GraphListView;
	TSharedPtr<SListView<FPointItemPtr>> PointListView;
	TSharedPtr<SListView<FNodeItemPtr>> NodeListView;
	TSharedPtr<SListView<FMessagePtr>> MessageListView;
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<SBox> GraphEditorHost;
	TSharedPtr<IDetailsView> DetailsView;

	FBoardItemPtr SelectedBoardRow;
	FProductionItemPtr SelectedProductionRow;
	FMapItemPtr SelectedMapRow;
	FGraphItemPtr SelectedGraphRow;
	FPointItemPtr SelectedPointRow;
	FNodeItemPtr SelectedNodeRow;
	FText StatusText;
};
