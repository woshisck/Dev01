#pragma once

#include "CoreMinimal.h"
#include "Data/RuneDataAsset.h"
#include "GameplayTagContainer.h"
#include "RuneEditor/RuneEditorFlowAuthoring.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class URuneDataAsset;
class UFlowAsset;
class UFlowNode;
class FUICommandList;
class IDetailsView;
class SBox;
class SGraphEditor;
class SWidgetSwitcher;
class SWidget;
class SEditableTextBox;
class SMultiLineEditableTextBox;

struct FRuneEditorRuneRow
{
	explicit FRuneEditorRuneRow(URuneDataAsset* InAsset)
		: Asset(InAsset)
	{
	}

	TWeakObjectPtr<URuneDataAsset> Asset;
};

struct FRuneEditorFlowNodeRow
{
	explicit FRuneEditorFlowNodeRow(const FRuneEditorFlowNodeSummary& InSummary)
		: Summary(InSummary)
	{
	}

	FRuneEditorFlowNodeSummary Summary;
};

struct FRuneEditorTuningRow
{
	explicit FRuneEditorTuningRow(int32 InIndex)
		: Index(InIndex)
	{
	}

	int32 Index = INDEX_NONE;
};

class SRuneEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRuneEditorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FRuneRowPtr = TSharedPtr<FRuneEditorRuneRow>;
	using FFlowNodeRowPtr = TSharedPtr<FRuneEditorFlowNodeRow>;
	using FTuningRowPtr = TSharedPtr<FRuneEditorTuningRow>;

	enum class ECenterPanelTab : uint8
	{
		ValueTable,
		FlowGraph
	};

	enum class EBottomPanelTab : uint8
	{
		NodeLibrary,
		Validation,
		RunLog,
		SelectedNode
	};

	enum class EDetailsPanelTab : uint8
	{
		BasicInfo,
		CombatCard
	};

	enum class ENodeLibraryFilter : uint8
	{
		All,
		Skill,
		Effect,
		Task,
		Spawn,
		Condition,
		Presentation,
		Lifecycle
	};

	enum class EResourceFilter : uint8
	{
		All,
		Base,
		Enemy,
		Level,
		Finisher,
		ComboCard
	};

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildResourceManagerPanel();
	TSharedRef<SWidget> BuildRuneResourceListPanel();
	TSharedRef<SWidget> BuildCenterPanel();
	TSharedRef<SWidget> BuildDetailsPanel();
	TSharedRef<SWidget> BuildGraphEditorPanel();
	TSharedRef<SWidget> BuildBottomDiagnosticsPanel();
	TSharedRef<SWidget> BuildCenterTabButton(const FText& Label, ECenterPanelTab Tab);
	TSharedRef<SWidget> BuildValueTablePanel();
	TSharedRef<SWidget> BuildBottomTabButton(const FText& Label, EBottomPanelTab Tab);
	TSharedRef<SWidget> BuildResourceFilterButton(const FText& Label, EResourceFilter Filter);
	TSharedRef<SWidget> BuildLibraryCategoryToggle(const FText& Label, const FGameplayTag& CategoryTag);
	TSharedRef<SWidget> BuildNodeLibraryFilterButton(const FText& Label, ENodeLibraryFilter Filter);
	TSharedRef<SWidget> BuildNodeLibraryPanel();
	TSharedRef<SWidget> BuildValidationPanel();
	TSharedRef<SWidget> BuildRunLogPanel();
	TSharedRef<SWidget> BuildSelectedNodePanel();
	TSharedRef<SWidget> BuildCombatCardPanel();
	TSharedRef<SWidget> BuildDetailsPanelTabButton(const FText& Label, EDetailsPanelTab Tab);
	TSharedRef<SWidget> BuildNodeLibraryItem(ENodeLibraryFilter Filter, UClass* NodeClass, const FText& DisplayName, const FText& Description);
	TSharedRef<ITableRow> GenerateRuneRow(FRuneRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> BuildTuningRow(FTuningRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);

	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	void RefreshFlowNodes();
	void RefreshTuningRows();
	void BindGraphEditorCommands();
	void RebuildGraphEditor();
	void OnSearchTextChanged(const FText& NewText);
	void OnRuneSelectionChanged(FRuneRowPtr Row, ESelectInfo::Type SelectInfo);
	void OnGraphSelectionChanged(const TSet<UObject*>& Nodes);
	FReply OnCenterTabSelected(ECenterPanelTab Tab);
	FReply OnBottomTabSelected(EBottomPanelTab Tab);
	FReply OnResourceFilterSelected(EResourceFilter Filter);
	FReply OnLibraryCategoryToggled(FGameplayTag CategoryTag);
	FReply OnNodeLibraryFilterSelected(ENodeLibraryFilter Filter);
	FReply OnAddNodeFromLibrary(UClass* NodeClass);
	FReply OnRefreshClicked();
	FReply OnCreateRuneClicked();
	FReply OnCopyAssetClicked();
	FReply OnPasteAssetClicked();
	FReply OnRenameAssetClicked();
	FReply OnDeleteAssetClicked();
	FReply OnLocateAssetClicked();
	FReply OnSaveBasicInfoClicked();
	FReply OnExportTuningClicked();
	FReply OnImportTuningClicked();
	FReply OnAddTuningRowClicked();
	FReply OnDeleteTuningRowClicked(int32 RowIndex);
	FReply OnTuningSourceClicked(int32 RowIndex);
	FReply OnDetailsPanelTabSelected(EDetailsPanelTab Tab);
	FReply OnSaveCardInfoClicked();
	FReply OnToggleIsCombatCardClicked();
	FReply OnToggleComboScalingClicked();
	FReply OnRunRuneClicked();
	FReply OnOpenRuneClicked() const;
	FReply OnOpenFlowClicked() const;
	FReply SelectFlowNode(UFlowNode* FlowNode);
	void DeleteSelectedGraphNodes();
	bool CanDeleteSelectedGraphNodes() const;

	FText GetStatusText() const;
	FText GetSelectedRuneNameText() const;
	FText GetSelectedRuneTagText() const;
	FText GetRuneLibraryCategoryText() const;
	FText GetSelectedDurationText() const;
	FText GetSelectedFlowText() const;
	FText GetSelectedFlowNodeText() const;
	FText GetSelectedSummaryText() const;
	FText GetValidationSummaryText() const;
	FText GetValidationDetailsText() const;
	FText GetRunFeedbackText() const;
	FText GetSelectedFlowNodeDescriptionText() const;
	FText GetSelectedFlowNodeOutgoingText() const;
	FText GetSelectedCardIsCombatCardText() const;
	FText GetSelectedCardComboScalingText() const;
	EVisibility GetHasSelectedRuneVisibility() const;

	void OpenAsset(UObject* Asset) const;
	UObject* GetSelectedResource() const;
	URuneDataAsset* GetSelectedRune() const;
	UFlowAsset* GetSelectedFlowAsset() const;
	UFlowNode* GetSelectedFlowNode() const;
	URuneDataAsset* FindRuneForFlow(UFlowAsset* FlowAsset) const;
	FGameplayTag GetResourceFilterTag(EResourceFilter Filter) const;
	bool DoesRuneMatchResourceFilter(const URuneDataAsset* Rune) const;
	bool IsRuneLibraryCategoryActive(FGameplayTag CategoryTag) const;
	void SelectResourceAsset(UObject* Asset);
	void SyncSelectedRuneEditorFields();
	void SyncNodeInspector();
	FString GetNewRuneNameText() const;
	FString GetNewRuneTagText() const;
	FString GetNewRuneFolderText() const;

	TArray<FRuneRowPtr> RuneRows;
	TArray<FFlowNodeRowPtr> FlowNodeRows;
	TArray<FTuningRowPtr> TuningRows;
	TSharedPtr<SListView<FRuneRowPtr>> RuneListView;
	TSharedPtr<SListView<FTuningRowPtr>> TuningListView;
	TSharedPtr<SBox> GraphEditorContainer;
	TSharedPtr<SWidgetSwitcher> CenterPanelSwitcher;
	TSharedPtr<SWidgetSwitcher> BottomPanelSwitcher;
	TSharedPtr<SWidgetSwitcher> DetailsPanelSwitcher;
	TSharedPtr<SGraphEditor> RuneGraphEditor;
	TSharedPtr<FUICommandList> GraphEditorCommands;
	TSharedPtr<SEditableTextBox> NewRuneNameTextBox;
	TSharedPtr<SEditableTextBox> NewRuneTagTextBox;
	TSharedPtr<SEditableTextBox> NewRuneFolderTextBox;
	TSharedPtr<SEditableTextBox> SearchTextBox;
	TSharedPtr<SEditableTextBox> ResourceRenameTextBox;
	TSharedPtr<SEditableTextBox> SelectedRuneNameTextBox;
	TSharedPtr<SEditableTextBox> SelectedRuneTagTextBox;
	TSharedPtr<SMultiLineEditableTextBox> SelectedSummaryTextBox;
	TSharedPtr<SEditableTextBox> CardIdTagTextBox;
	TSharedPtr<SEditableTextBox> CardDisplayNameTextBox;
	TSharedPtr<SEditableTextBox> CardHUDReasonTextBox;
	TSharedPtr<SMultiLineEditableTextBox> CardHUDSummaryTextBox;
	TSharedPtr<IDetailsView> NodeDetailsView;
	TWeakObjectPtr<UObject> SelectedResource;
	TWeakObjectPtr<UObject> CopiedResource;
	TWeakObjectPtr<URuneDataAsset> SelectedRune;
	TWeakObjectPtr<UFlowNode> SelectedFlowNode;
	FString SearchText;
	FText StatusText;
	FText RunFeedbackText;
	ECenterPanelTab ActiveCenterTab = ECenterPanelTab::FlowGraph;
	EBottomPanelTab ActiveBottomTab = EBottomPanelTab::NodeLibrary;
	EDetailsPanelTab ActiveDetailsTab = EDetailsPanelTab::BasicInfo;
	ENodeLibraryFilter ActiveNodeLibraryFilter = ENodeLibraryFilter::All;
	EResourceFilter ActiveResourceFilter = EResourceFilter::All;
	ERuneType CreateRuneType = ERuneType::Buff;
	ERuneRarity CreateRarity = ERuneRarity::Common;
	ERuneTriggerType CreateTriggerType = ERuneTriggerType::Passive;

	using FStringCombo = SComboBox<TSharedPtr<FString>>;
	TArray<TSharedPtr<FString>> RuneTypeOptions;
	TArray<TSharedPtr<FString>> RarityOptions;
	TArray<TSharedPtr<FString>> TriggerTypeOptions;
	TArray<TSharedPtr<FString>> CardTypeOptions;
	TArray<TSharedPtr<FString>> RequiredActionOptions;
	TArray<TSharedPtr<FString>> TriggerTimingOptions;
	TSharedPtr<FStringCombo> CreateRuneTypeCombo;
	TSharedPtr<FStringCombo> CreateRarityCombo;
	TSharedPtr<FStringCombo> CreateTriggerTypeCombo;
	TSharedPtr<FStringCombo> RuneTypeCombo;
	TSharedPtr<FStringCombo> RarityCombo;
	TSharedPtr<FStringCombo> TriggerTypeCombo;
	TSharedPtr<FStringCombo> CardTypeCombo;
	TSharedPtr<FStringCombo> RequiredActionCombo;
	TSharedPtr<FStringCombo> TriggerTimingCombo;
};
