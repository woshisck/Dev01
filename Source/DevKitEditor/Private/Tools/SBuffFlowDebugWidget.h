#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class AYogCharacterBase;
class UWorld;

struct FBuffFlowDebugCharacterRow
{
	TWeakObjectPtr<AYogCharacterBase> Character;
	FString DisplayName;
	FString StateName;
	FString WeaponStateName;
	int32 ActiveFlowCount = 0;
	int32 TraceEntryCount = 0;
	bool bPlayerControlled = false;
};

struct FBuffFlowDebugFlowRow
{
	FString RuneGuid;
	FString FlowName;
	FString SourceRuneName;
	FString RuntimeState;
	FString ActiveNodes;
	FString RecentNodes;
	int32 ActiveNodeCount = 0;
	int32 RecordedNodeCount = 0;
};

struct FBuffFlowDebugTraceRow
{
	FString TimeSeconds;
	FString Result;
	FString FlowName;
	FString NodeName;
	FString ProfileName;
	FString TargetName;
	FString CardName;
	FString Message;
	FString Values;
};

class SBuffFlowDebugWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBuffFlowDebugWidget) {}
	SLATE_END_ARGS()

	virtual ~SBuffFlowDebugWidget() override;

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	using FCharacterRowPtr = TSharedPtr<FBuffFlowDebugCharacterRow>;
	using FFlowRowPtr = TSharedPtr<FBuffFlowDebugFlowRow>;
	using FTraceRowPtr = TSharedPtr<FBuffFlowDebugTraceRow>;

	UWorld* FindDebugWorld() const;
	void RefreshData(const FText& NewStatus = FText::GetEmpty());
	void RebuildSelectedRows();
	void ClearDebugData(const FText& NewStatus);
	void HandlePIEBegan(bool bIsSimulating);
	void HandlePIEEnded(bool bIsSimulating);
	bool SelectCharacterByDelta(int32 Delta);

	TSharedRef<ITableRow> GenerateCharacterRow(FCharacterRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateFlowRow(FFlowRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateTraceRow(FTraceRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);
	void OnCharacterSelectionChanged(FCharacterRowPtr Row, ESelectInfo::Type SelectInfo);

	FReply OnRefreshClicked();
	FReply OnPreviousClicked();
	FReply OnNextClicked();
	FReply OnClearTraceClicked();

	FText GetStatusText() const;
	FText GetSelectedSummaryText() const;

	TArray<FCharacterRowPtr> CharacterRows;
	TArray<FFlowRowPtr> FlowRows;
	TArray<FTraceRowPtr> TraceRows;

	TSharedPtr<SListView<FCharacterRowPtr>> CharacterListView;
	TSharedPtr<SListView<FFlowRowPtr>> FlowListView;
	TSharedPtr<SListView<FTraceRowPtr>> TraceListView;

	TWeakObjectPtr<AYogCharacterBase> SelectedCharacter;
	int32 SelectedCharacterIndex = 0;
	int32 SelectedRunningFlowCount = 0;
	int32 SelectedTrackedFlowCount = 0;
	FString WorldLabel;
	FText StatusText;
	float RefreshAccumulator = 0.f;
	bool bRefreshingSelection = false;
	bool bPIEEnding = false;
};
