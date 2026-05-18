#include "Tools/SBuffFlowDebugWidget.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SBuffFlowDebugWidget"

namespace
{
	constexpr int32 MaxTraceRows = 120;

	FString GetNameOrDash(const FName Name)
	{
		return Name.IsNone() ? FString(TEXT("-")) : Name.ToString();
	}

	FString GetStringOrDash(const FString& Value)
	{
		return Value.IsEmpty() ? FString(TEXT("-")) : Value;
	}

	FString GetEnumDisplayName(const UEnum* Enum, const int64 Value)
	{
		return Enum ? Enum->GetDisplayNameTextByValue(Value).ToString() : FString::Printf(TEXT("%lld"), Value);
	}

	FString GetWorldTypeName(const UWorld* World)
	{
		if (!World)
		{
			return TEXT("None");
		}

		switch (World->WorldType)
		{
		case EWorldType::PIE:
			return TEXT("PIE");
		case EWorldType::Game:
			return TEXT("Game");
		default:
			return FString::Printf(TEXT("WorldType %d"), static_cast<int32>(World->WorldType));
		}
	}

	const TCHAR* TraceResultToDebugString(const EBuffFlowTraceResult Result)
	{
		switch (Result)
		{
		case EBuffFlowTraceResult::Success:
			return TEXT("Success");
		case EBuffFlowTraceResult::Failed:
			return TEXT("Failed");
		case EBuffFlowTraceResult::Skipped:
			return TEXT("Skipped");
		default:
			return TEXT("Unknown");
		}
	}

	FString JoinNames(const TArray<FName>& Names, const int32 MaxNames = 6)
	{
		if (Names.IsEmpty())
		{
			return TEXT("-");
		}

		FString Result;
		const int32 Count = FMath::Min(Names.Num(), MaxNames);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Index > 0)
			{
				Result += TEXT(", ");
			}
			Result += GetNameOrDash(Names[Index]);
		}

		if (Names.Num() > MaxNames)
		{
			Result += FString::Printf(TEXT(", +%d more"), Names.Num() - MaxNames);
		}

		return Result;
	}

	FString JoinNodeNames(const TArray<FName>& Names, const TArray<FName>& Classes, const int32 MaxNames = 6)
	{
		if (Names.IsEmpty())
		{
			return TEXT("-");
		}

		FString Result;
		const int32 Count = FMath::Min(Names.Num(), MaxNames);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Index > 0)
			{
				Result += TEXT(", ");
			}

			Result += GetNameOrDash(Names[Index]);
			if (Classes.IsValidIndex(Index) && !Classes[Index].IsNone())
			{
				Result += FString::Printf(TEXT(" (%s)"), *Classes[Index].ToString());
			}
		}

		if (Names.Num() > MaxNames)
		{
			Result += FString::Printf(TEXT(", +%d more"), Names.Num() - MaxNames);
		}

		return Result;
	}

	TSharedRef<SWidget> MakeDebugTextCell(const FString& Text, const FString& ToolTip = FString())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(GetStringOrDash(Text)))
			.ToolTipText(FText::FromString(ToolTip.IsEmpty() ? Text : ToolTip));
	}
}

class SBuffFlowDebugCharacterTableRow : public SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugCharacterRow>>
{
public:
	SLATE_BEGIN_ARGS(SBuffFlowDebugCharacterTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FBuffFlowDebugCharacterRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugCharacterRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugCharacterRow>>::FArguments().Padding(FMargin(3.f, 1.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeDebugTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Character"))
		{
			return MakeDebugTextCell(Item->DisplayName);
		}
		if (ColumnName == TEXT("State"))
		{
			return MakeDebugTextCell(Item->StateName);
		}
		if (ColumnName == TEXT("Weapon"))
		{
			return MakeDebugTextCell(Item->WeaponStateName);
		}
		if (ColumnName == TEXT("Flows"))
		{
			return MakeDebugTextCell(FString::FromInt(Item->ActiveFlowCount));
		}
		if (ColumnName == TEXT("Trace"))
		{
			return MakeDebugTextCell(FString::FromInt(Item->TraceEntryCount));
		}

		return MakeDebugTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FBuffFlowDebugCharacterRow> Item;
};

class SBuffFlowDebugFlowTableRow : public SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugFlowRow>>
{
public:
	SLATE_BEGIN_ARGS(SBuffFlowDebugFlowTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FBuffFlowDebugFlowRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugFlowRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugFlowRow>>::FArguments().Padding(FMargin(3.f, 1.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeDebugTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Rune"))
		{
			return MakeDebugTextCell(Item->RuneGuid);
		}
		if (ColumnName == TEXT("Flow"))
		{
			return MakeDebugTextCell(Item->FlowName);
		}
		if (ColumnName == TEXT("Source"))
		{
			return MakeDebugTextCell(Item->SourceRuneName);
		}
		if (ColumnName == TEXT("Runtime"))
		{
			return MakeDebugTextCell(Item->RuntimeState);
		}
		if (ColumnName == TEXT("Current"))
		{
			return MakeDebugTextCell(Item->ActiveNodes);
		}
		if (ColumnName == TEXT("Recent"))
		{
			return MakeDebugTextCell(Item->RecentNodes);
		}

		return MakeDebugTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FBuffFlowDebugFlowRow> Item;
};

class SBuffFlowDebugTraceTableRow : public SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugTraceRow>>
{
public:
	SLATE_BEGIN_ARGS(SBuffFlowDebugTraceTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FBuffFlowDebugTraceRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugTraceRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FBuffFlowDebugTraceRow>>::FArguments().Padding(FMargin(3.f, 1.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeDebugTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Time"))
		{
			return MakeDebugTextCell(Item->TimeSeconds);
		}
		if (ColumnName == TEXT("Result"))
		{
			return MakeDebugTextCell(Item->Result);
		}
		if (ColumnName == TEXT("Flow"))
		{
			return MakeDebugTextCell(Item->FlowName);
		}
		if (ColumnName == TEXT("Node"))
		{
			return MakeDebugTextCell(Item->NodeName);
		}
		if (ColumnName == TEXT("Profile"))
		{
			return MakeDebugTextCell(Item->ProfileName);
		}
		if (ColumnName == TEXT("Target"))
		{
			return MakeDebugTextCell(Item->TargetName);
		}
		if (ColumnName == TEXT("Card"))
		{
			return MakeDebugTextCell(Item->CardName);
		}
		if (ColumnName == TEXT("Message"))
		{
			return MakeDebugTextCell(Item->Message);
		}
		if (ColumnName == TEXT("Values"))
		{
			return MakeDebugTextCell(Item->Values);
		}

		return MakeDebugTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FBuffFlowDebugTraceRow> Item;
};

void SBuffFlowDebugWidget::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("InitialStatus", "Waiting for PIE.");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.OnClicked(this, &SBuffFlowDebugWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Previous", "Previous"))
				.OnClicked(this, &SBuffFlowDebugWidget::OnPreviousClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Next", "Next"))
				.OnClicked(this, &SBuffFlowDebugWidget::OnNextClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ClearTrace", "Clear Trace"))
				.OnClicked(this, &SBuffFlowDebugWidget::OnClearTraceClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SBuffFlowDebugWidget::GetStatusText)
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.28f)
			[
				SNew(SBorder).Padding(2.f)
				[
					SAssignNew(CharacterListView, SListView<FCharacterRowPtr>)
					.ListItemsSource(&CharacterRows)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SBuffFlowDebugWidget::GenerateCharacterRow)
					.OnSelectionChanged(this, &SBuffFlowDebugWidget::OnCharacterSelectionChanged)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(TEXT("Character")).DefaultLabel(LOCTEXT("CharacterColumn", "Character")).FillWidth(1.3f)
						+ SHeaderRow::Column(TEXT("State")).DefaultLabel(LOCTEXT("StateColumn", "State")).FixedWidth(78.f)
						+ SHeaderRow::Column(TEXT("Weapon")).DefaultLabel(LOCTEXT("WeaponColumn", "Weapon")).FixedWidth(82.f)
						+ SHeaderRow::Column(TEXT("Flows")).DefaultLabel(LOCTEXT("FlowsColumn", "Flows")).FixedWidth(52.f)
						+ SHeaderRow::Column(TEXT("Trace")).DefaultLabel(LOCTEXT("TraceColumn", "Trace")).FixedWidth(52.f)
					)
				]
			]
			+ SSplitter::Slot().Value(0.72f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(this, &SBuffFlowDebugWidget::GetSelectedSummaryText)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock).Text(LOCTEXT("ActiveFlowsLabel", "Active BuffFlows"))
				]
				+ SVerticalBox::Slot().FillHeight(0.42f).Padding(8.f, 0.f, 0.f, 8.f)
				[
					SNew(SBorder).Padding(2.f)
					[
						SAssignNew(FlowListView, SListView<FFlowRowPtr>)
						.ListItemsSource(&FlowRows)
						.SelectionMode(ESelectionMode::None)
						.OnGenerateRow(this, &SBuffFlowDebugWidget::GenerateFlowRow)
						.HeaderRow
						(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("Rune")).DefaultLabel(LOCTEXT("RuneColumn", "Rune")).FixedWidth(72.f)
							+ SHeaderRow::Column(TEXT("Flow")).DefaultLabel(LOCTEXT("FlowColumn", "Flow")).FillWidth(1.1f)
							+ SHeaderRow::Column(TEXT("Source")).DefaultLabel(LOCTEXT("SourceColumn", "Source")).FillWidth(0.8f)
							+ SHeaderRow::Column(TEXT("Runtime")).DefaultLabel(LOCTEXT("RuntimeColumn", "Runtime")).FixedWidth(110.f)
							+ SHeaderRow::Column(TEXT("Current")).DefaultLabel(LOCTEXT("CurrentColumn", "Current Nodes")).FillWidth(1.4f)
							+ SHeaderRow::Column(TEXT("Recent")).DefaultLabel(LOCTEXT("RecentColumn", "Recent Nodes")).FillWidth(1.2f)
						)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock).Text(LOCTEXT("RecentStatementsLabel", "Recent Statements"))
				]
				+ SVerticalBox::Slot().FillHeight(0.58f).Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SBorder).Padding(2.f)
					[
						SAssignNew(TraceListView, SListView<FTraceRowPtr>)
						.ListItemsSource(&TraceRows)
						.SelectionMode(ESelectionMode::None)
						.OnGenerateRow(this, &SBuffFlowDebugWidget::GenerateTraceRow)
						.HeaderRow
						(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(TEXT("Time")).DefaultLabel(LOCTEXT("TimeColumn", "Time")).FixedWidth(62.f)
							+ SHeaderRow::Column(TEXT("Result")).DefaultLabel(LOCTEXT("ResultColumn", "Result")).FixedWidth(70.f)
							+ SHeaderRow::Column(TEXT("Flow")).DefaultLabel(LOCTEXT("TraceFlowColumn", "Flow")).FillWidth(0.8f)
							+ SHeaderRow::Column(TEXT("Node")).DefaultLabel(LOCTEXT("NodeColumn", "Node")).FillWidth(0.9f)
							+ SHeaderRow::Column(TEXT("Profile")).DefaultLabel(LOCTEXT("ProfileColumn", "Profile")).FillWidth(0.8f)
							+ SHeaderRow::Column(TEXT("Target")).DefaultLabel(LOCTEXT("TargetColumn", "Target")).FillWidth(0.8f)
							+ SHeaderRow::Column(TEXT("Card")).DefaultLabel(LOCTEXT("CardColumn", "Card")).FillWidth(0.8f)
							+ SHeaderRow::Column(TEXT("Message")).DefaultLabel(LOCTEXT("MessageColumn", "Message")).FillWidth(1.2f)
							+ SHeaderRow::Column(TEXT("Values")).DefaultLabel(LOCTEXT("ValuesColumn", "Values")).FillWidth(1.2f)
						)
					]
				]
			]
		]
	];

	RefreshData(LOCTEXT("InitialRefresh", "Open PIE to inspect BuffFlow state."));

	FEditorDelegates::BeginPIE.AddSP(this, &SBuffFlowDebugWidget::HandlePIEBegan);
	FEditorDelegates::PrePIEEnded.AddSP(this, &SBuffFlowDebugWidget::HandlePIEEnded);
	FEditorDelegates::EndPIE.AddSP(this, &SBuffFlowDebugWidget::HandlePIEEnded);
	FEditorDelegates::ShutdownPIE.AddSP(this, &SBuffFlowDebugWidget::HandlePIEEnded);
}

SBuffFlowDebugWidget::~SBuffFlowDebugWidget()
{
	FEditorDelegates::PrePIEEnded.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
	FEditorDelegates::ShutdownPIE.RemoveAll(this);
}

FReply SBuffFlowDebugWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::PageUp)
	{
		return SelectCharacterByDelta(-1) ? FReply::Handled() : FReply::Unhandled();
	}

	if (InKeyEvent.GetKey() == EKeys::PageDown)
	{
		return SelectCharacterByDelta(1) ? FReply::Handled() : FReply::Unhandled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SBuffFlowDebugWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bPIEEnding)
	{
		return;
	}

	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= 0.25f)
	{
		RefreshAccumulator = 0.f;
		RefreshData();
	}
}

UWorld* SBuffFlowDebugWidget::FindDebugWorld() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	UWorld* GameWorld = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (!World)
		{
			continue;
		}

		if (World->IsBeingCleanedUp())
		{
			continue;
		}

		if (World->WorldType == EWorldType::PIE)
		{
			return World;
		}

		if (!GameWorld && World->WorldType == EWorldType::Game)
		{
			GameWorld = World;
		}
	}

	return GameWorld;
}

void SBuffFlowDebugWidget::RefreshData(const FText& NewStatus)
{
	UWorld* World = FindDebugWorld();
	if (bPIEEnding || (World && World->IsBeingCleanedUp()))
	{
		ClearDebugData(LOCTEXT("PIEEndingStatus", "PIE is ending. Debug data cleared."));
		return;
	}

	WorldLabel = World
		? FString::Printf(TEXT("%s (%s)"), *World->GetName(), *GetWorldTypeName(World))
		: FString(TEXT("No PIE/Game world"));

	const TWeakObjectPtr<AYogCharacterBase> PreviousSelected = SelectedCharacter;
	const int32 PreviousIndex = SelectedCharacterIndex;

	CharacterRows.Reset();
	if (World)
	{
		TArray<AYogCharacterBase*> Characters;
		for (TActorIterator<AYogCharacterBase> It(World); It; ++It)
		{
			AYogCharacterBase* Character = *It;
			if (IsValid(Character) && !Character->IsActorBeingDestroyed())
			{
				Characters.Add(Character);
			}
		}

		Characters.Sort([](const AYogCharacterBase& Lhs, const AYogCharacterBase& Rhs)
		{
			if (Lhs.IsPlayerControlled() != Rhs.IsPlayerControlled())
			{
				return Lhs.IsPlayerControlled();
			}
			return Lhs.GetName() < Rhs.GetName();
		});

		for (AYogCharacterBase* Character : Characters)
		{
			TSharedRef<FBuffFlowDebugCharacterRow> Row = MakeShared<FBuffFlowDebugCharacterRow>();
			Row->Character = Character;
			Row->bPlayerControlled = Character->IsPlayerControlled();
			Row->DisplayName = FString::Printf(
				TEXT("%s%s"),
				*Character->GetName(),
				Row->bPlayerControlled ? TEXT(" [Player]") : TEXT(""));
			Row->StateName = GetEnumDisplayName(
				StaticEnum<EYogCharacterState>(),
				static_cast<int64>(Character->GetCurrentState()));
			Row->WeaponStateName = GetEnumDisplayName(
				StaticEnum<EWeaponState>(),
				static_cast<int64>(Character->GetWeaponState()));

			if (UBuffFlowComponent* BuffFlowComponent = Character->FindComponentByClass<UBuffFlowComponent>())
			{
				Row->ActiveFlowCount = BuffFlowComponent->GetActiveBuffFlowCount();
				Row->TraceEntryCount = BuffFlowComponent->GetTraceEntries().Num();
			}

			CharacterRows.Add(Row);
		}
	}

	int32 NewSelectedIndex = INDEX_NONE;
	if (PreviousSelected.IsValid())
	{
		for (int32 Index = 0; Index < CharacterRows.Num(); ++Index)
		{
			if (CharacterRows[Index]->Character.Get() == PreviousSelected.Get())
			{
				NewSelectedIndex = Index;
				break;
			}
		}
	}

	if (NewSelectedIndex == INDEX_NONE && !CharacterRows.IsEmpty())
	{
		NewSelectedIndex = FMath::Clamp(PreviousIndex, 0, CharacterRows.Num() - 1);
	}

	if (NewSelectedIndex != INDEX_NONE)
	{
		SelectedCharacterIndex = NewSelectedIndex;
		SelectedCharacter = CharacterRows[SelectedCharacterIndex]->Character;
	}
	else
	{
		SelectedCharacterIndex = 0;
		SelectedCharacter.Reset();
	}

	RebuildSelectedRows();

	bRefreshingSelection = true;
	if (CharacterListView.IsValid())
	{
		CharacterListView->RequestListRefresh();
		CharacterListView->ClearSelection();
		if (CharacterRows.IsValidIndex(SelectedCharacterIndex))
		{
			CharacterListView->SetSelection(CharacterRows[SelectedCharacterIndex], ESelectInfo::Direct);
			CharacterListView->RequestScrollIntoView(CharacterRows[SelectedCharacterIndex]);
		}
	}
	bRefreshingSelection = false;

	if (FlowListView.IsValid())
	{
		FlowListView->RequestListRefresh();
	}
	if (TraceListView.IsValid())
	{
		TraceListView->RequestListRefresh();
	}

	if (!NewStatus.IsEmpty())
	{
		StatusText = NewStatus;
	}
	else if (!World)
	{
		StatusText = LOCTEXT("NoWorldStatus", "No PIE/Game world found.");
	}
}

void SBuffFlowDebugWidget::ClearDebugData(const FText& NewStatus)
{
	bRefreshingSelection = true;
	SelectedCharacter.Reset();
	SelectedCharacterIndex = 0;
	SelectedRunningFlowCount = 0;
	SelectedTrackedFlowCount = 0;
	CharacterRows.Reset();
	FlowRows.Reset();
	TraceRows.Reset();
	WorldLabel = TEXT("No PIE/Game world");
	StatusText = NewStatus;

	if (CharacterListView.IsValid())
	{
		CharacterListView->ClearSelection();
		CharacterListView->RequestListRefresh();
	}
	if (FlowListView.IsValid())
	{
		FlowListView->RequestListRefresh();
	}
	if (TraceListView.IsValid())
	{
		TraceListView->RequestListRefresh();
	}

	bRefreshingSelection = false;
}

void SBuffFlowDebugWidget::HandlePIEEnded(bool bIsSimulating)
{
	bPIEEnding = true;
	ClearDebugData(LOCTEXT("PIEEndedStatus", "PIE ended. Debug data cleared."));
}

void SBuffFlowDebugWidget::HandlePIEBegan(bool bIsSimulating)
{
	bPIEEnding = false;
	RefreshAccumulator = 0.f;
	ClearDebugData(LOCTEXT("PIEBeganStatus", "PIE started. Waiting for debug world."));
}

void SBuffFlowDebugWidget::RebuildSelectedRows()
{
	FlowRows.Reset();
	TraceRows.Reset();
	SelectedRunningFlowCount = 0;
	SelectedTrackedFlowCount = 0;

	AYogCharacterBase* Character = SelectedCharacter.Get();
	if (!Character)
	{
		return;
	}

	UBuffFlowComponent* BuffFlowComponent = Character->FindComponentByClass<UBuffFlowComponent>();
	if (!BuffFlowComponent)
	{
		return;
	}

	const TArray<FBuffFlowActiveFlowDebugEntry> FlowEntries = BuffFlowComponent->GetActiveBuffFlowDebugEntries();
	SelectedTrackedFlowCount = FlowEntries.Num();
	for (const FBuffFlowActiveFlowDebugEntry& Entry : FlowEntries)
	{
		if (Entry.bRuntimeInstanceActive)
		{
			++SelectedRunningFlowCount;
		}

		TSharedRef<FBuffFlowDebugFlowRow> Row = MakeShared<FBuffFlowDebugFlowRow>();
		Row->RuneGuid = Entry.RuneGuid.ToString(EGuidFormats::Digits).Left(8);
		Row->FlowName = GetNameOrDash(Entry.FlowName);
		Row->SourceRuneName = GetNameOrDash(Entry.SourceRuneName);
		Row->RuntimeState = Entry.bFlowAssetValid
			? (Entry.bRuntimeInstanceActive ? TEXT("Running") : TEXT("No active nodes"))
			: TEXT("Missing asset");
		Row->ActiveNodeCount = Entry.ActiveNodeCount;
		Row->RecordedNodeCount = Entry.RecordedNodeCount;
		Row->ActiveNodes = JoinNodeNames(Entry.ActiveNodeNames, Entry.ActiveNodeClasses);
		Row->RecentNodes = JoinNames(Entry.RecordedNodeNames);
		FlowRows.Add(Row);
	}

	const TArray<FBuffFlowTraceEntry> TraceEntries = BuffFlowComponent->GetTraceEntries();
	const int32 FirstIndex = FMath::Max(0, TraceEntries.Num() - MaxTraceRows);
	for (int32 Index = TraceEntries.Num() - 1; Index >= FirstIndex; --Index)
	{
		const FBuffFlowTraceEntry& Entry = TraceEntries[Index];
		TSharedRef<FBuffFlowDebugTraceRow> Row = MakeShared<FBuffFlowDebugTraceRow>();
		Row->TimeSeconds = FString::Printf(TEXT("%.2f"), Entry.TimeSeconds);
		Row->Result = TraceResultToDebugString(Entry.Result);
		Row->FlowName = GetNameOrDash(Entry.FlowName);
		Row->NodeName = GetNameOrDash(Entry.NodeName);
		Row->ProfileName = GetNameOrDash(Entry.ProfileName);
		Row->TargetName = GetNameOrDash(Entry.TargetName);
		Row->CardName = !Entry.CardName.IsNone() ? Entry.CardName.ToString() : Entry.CardIdTag.ToString();
		Row->Message = GetStringOrDash(Entry.Message);
		Row->Values = GetStringOrDash(Entry.Values);
		TraceRows.Add(Row);
	}
}

bool SBuffFlowDebugWidget::SelectCharacterByDelta(const int32 Delta)
{
	if (bPIEEnding)
	{
		return false;
	}

	if (CharacterRows.IsEmpty())
	{
		RefreshData();
	}

	if (CharacterRows.IsEmpty())
	{
		return false;
	}

	SelectedCharacterIndex = (SelectedCharacterIndex + Delta + CharacterRows.Num()) % CharacterRows.Num();
	SelectedCharacter = CharacterRows[SelectedCharacterIndex]->Character;
	RebuildSelectedRows();

	if (CharacterListView.IsValid())
	{
		CharacterListView->SetSelection(CharacterRows[SelectedCharacterIndex], ESelectInfo::OnKeyPress);
		CharacterListView->RequestScrollIntoView(CharacterRows[SelectedCharacterIndex]);
	}
	if (FlowListView.IsValid())
	{
		FlowListView->RequestListRefresh();
	}
	if (TraceListView.IsValid())
	{
		TraceListView->RequestListRefresh();
	}

	StatusText = FText::Format(
		LOCTEXT("SelectedCharacterStatus", "Selected {0} of {1}."),
		FText::AsNumber(SelectedCharacterIndex + 1),
		FText::AsNumber(CharacterRows.Num()));
	return true;
}

TSharedRef<ITableRow> SBuffFlowDebugWidget::GenerateCharacterRow(FCharacterRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SBuffFlowDebugCharacterTableRow, OwnerTable)
		.Item(Row);
}

TSharedRef<ITableRow> SBuffFlowDebugWidget::GenerateFlowRow(FFlowRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SBuffFlowDebugFlowTableRow, OwnerTable)
		.Item(Row);
}

TSharedRef<ITableRow> SBuffFlowDebugWidget::GenerateTraceRow(FTraceRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SBuffFlowDebugTraceTableRow, OwnerTable)
		.Item(Row);
}

void SBuffFlowDebugWidget::OnCharacterSelectionChanged(FCharacterRowPtr Row, ESelectInfo::Type SelectInfo)
{
	if (bRefreshingSelection || !Row.IsValid())
	{
		return;
	}

	const int32 FoundIndex = CharacterRows.IndexOfByKey(Row);
	if (FoundIndex != INDEX_NONE)
	{
		SelectedCharacterIndex = FoundIndex;
	}
	SelectedCharacter = Row->Character;
	RebuildSelectedRows();

	if (FlowListView.IsValid())
	{
		FlowListView->RequestListRefresh();
	}
	if (TraceListView.IsValid())
	{
		TraceListView->RequestListRefresh();
	}
}

FReply SBuffFlowDebugWidget::OnRefreshClicked()
{
	RefreshData(LOCTEXT("ManualRefresh", "Refreshed BuffFlow debug data."));
	return FReply::Handled();
}

FReply SBuffFlowDebugWidget::OnPreviousClicked()
{
	return SelectCharacterByDelta(-1) ? FReply::Handled() : FReply::Unhandled();
}

FReply SBuffFlowDebugWidget::OnNextClicked()
{
	return SelectCharacterByDelta(1) ? FReply::Handled() : FReply::Unhandled();
}

FReply SBuffFlowDebugWidget::OnClearTraceClicked()
{
	AYogCharacterBase* Character = SelectedCharacter.Get();
	UBuffFlowComponent* BuffFlowComponent = Character ? Character->FindComponentByClass<UBuffFlowComponent>() : nullptr;
	if (!BuffFlowComponent)
	{
		StatusText = LOCTEXT("ClearTraceFailed", "Selected character has no BuffFlowComponent.");
		return FReply::Handled();
	}

	BuffFlowComponent->ClearTraceEntries();
	RefreshData(LOCTEXT("TraceCleared", "Cleared trace entries for selected character."));
	return FReply::Handled();
}

FText SBuffFlowDebugWidget::GetStatusText() const
{
	return FText::Format(
		LOCTEXT("StatusText", "World: {0} | Characters: {1} | {2}"),
		FText::FromString(WorldLabel),
		FText::AsNumber(CharacterRows.Num()),
		StatusText);
}

FText SBuffFlowDebugWidget::GetSelectedSummaryText() const
{
	AYogCharacterBase* Character = SelectedCharacter.Get();
	if (!Character)
	{
		return LOCTEXT("NoSelection", "No character selected.");
	}

	const FString CharacterState = GetEnumDisplayName(
		StaticEnum<EYogCharacterState>(),
		static_cast<int64>(Character->GetCurrentState()));
	const FString WeaponState = GetEnumDisplayName(
		StaticEnum<EWeaponState>(),
		static_cast<int64>(Character->GetWeaponState()));

	return FText::Format(
		LOCTEXT("SelectedSummary", "{0} ({1}/{2}) | Character State: {3} | Weapon State: {4} | BuffFlows: {5} tracked, {6} running | Statements: {7}"),
		FText::FromString(Character->GetName()),
		FText::AsNumber(CharacterRows.IsEmpty() ? 0 : SelectedCharacterIndex + 1),
		FText::AsNumber(CharacterRows.Num()),
		FText::FromString(CharacterState),
		FText::FromString(WeaponState),
		FText::AsNumber(SelectedTrackedFlowCount),
		FText::AsNumber(SelectedRunningFlowCount),
		FText::AsNumber(TraceRows.Num()));
}

#undef LOCTEXT_NAMESPACE
