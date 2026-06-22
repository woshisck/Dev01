#include "Tools/SComboGraphManagerWidget.h"

#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimMontage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SComboGraphManagerWidget"

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
		return Out;
	}

	FString ObjectName(const UObject* Object)
	{
		return Object ? Object->GetName() : TEXT("-");
	}

	FString ObjectPath(const UObject* Object)
	{
		return Object ? Object->GetPathName() : TEXT("-");
	}

	UAN_MeleeDamage* FindFirstDamageNotify(UAnimMontage* Montage)
	{
		if (!Montage)
		{
			return nullptr;
		}
		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			if (UAN_MeleeDamage* DamageNotify = Cast<UAN_MeleeDamage>(NotifyEvent.Notify))
			{
				return DamageNotify;
			}
		}
		return nullptr;
	}

	FString ComboNodeStatus(const UGameplayAbilityComboGraphNode* Node, const UAN_MeleeDamage* Notify)
	{
		TArray<FString> Issues;
		if (!Node)
		{
			return TEXT("Invalid");
		}
		if (!Node->Montage)
		{
			Issues.Add(TEXT("Missing Montage"));
		}
		if (Node->bUseNodeComboWindow && Node->ComboWindowEndFrame < Node->ComboWindowStartFrame)
		{
			Issues.Add(TEXT("Bad window"));
		}
		if (!Notify)
		{
			Issues.Add(TEXT("No montage damage notify"));
		}
		return Issues.IsEmpty() ? TEXT("OK") : FString::Join(Issues, TEXT(", "));
	}

	float ReadFloat(const FComboGraphManagerRow& Row, FName ColumnName)
	{
		return 0.f;
	}

	TSharedRef<SWidget> MakeTextCell(const FString& Text, const FString& ToolTip = FString())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip));
	}
}

class SComboGraphManagerTableRow : public SMultiColumnTableRow<TSharedPtr<FComboGraphManagerRow>>
{
public:
	SLATE_BEGIN_ARGS(SComboGraphManagerTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FComboGraphManagerRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SComboGraphManagerWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FComboGraphManagerRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FComboGraphManagerRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		const UGameplayAbilityComboGraphNode* Node = Item.IsValid() ? Item->Node.Get() : nullptr;
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}
		if (ColumnName == TEXT("OwnerType")) return MakeTextCell(Item->OwnerType);
		if (ColumnName == TEXT("Owner")) return MakeTextCell(Item->OwnerName);
		if (ColumnName == TEXT("Graph")) return MakeTextCell(ObjectName(Item->Graph.Get()), ObjectPath(Item->Graph.Get()));
		if (ColumnName == TEXT("Node")) return MakeTextCell(Node ? Node->NodeId.ToString() : TEXT("-"));
		if (ColumnName == TEXT("Input"))
		{
			return MakeTextCell(Node && StaticEnum<EYogComboGraphInputAction>()
				? StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(Node->RootInputAction))
				: TEXT("-"));
		}
		if (ColumnName == TEXT("Ability"))
		{
			return MakeTextCell(TEXT("GA_PlayMontage"));
		}
		if (ColumnName == TEXT("Montage")) return MakeTextCell(ObjectName(Item->Montage.Get()), ObjectPath(Item->Montage.Get()));
		if (ColumnName == TEXT("Attack"))
		{
			return MakeTextCell(Item->DamageNotify.IsValid() ? TEXT("Montage Notify") : TEXT("-"));
		}
		if (ColumnName == TEXT("Status")) return MakeTextCell(Item->Status);
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenAsset", "Open"))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SComboGraphManagerWidget> Pinned = Owner.Pin())
						{
							Pinned->OpenPrimaryAsset(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
				SNew(SButton)
					.Text(LOCTEXT("MigrateNotify", "Migrate Notify"))
					.IsEnabled_Lambda([Row = Item]()
					{
						return false;
					})
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SComboGraphManagerWidget> Pinned = Owner.Pin())
						{
							Pinned->MigrateNodeFromNotify(Row);
						}
						return FReply::Handled();
					})
				];
		}
		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FComboGraphManagerRow> Item;
	TWeakPtr<SComboGraphManagerWidget> OwnerWidget;
};

void SComboGraphManagerWidget::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("Ready", "Ready");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "Combo Manager"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Refresh", "Refresh")).OnClicked(this, &SComboGraphManagerWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Text(this, &SComboGraphManagerWidget::GetStatsText).AutoWrapText(true)
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.64f)
			[
				SAssignNew(ListView, SListView<FRowPtr>)
				.ListItemsSource(&Rows)
				.OnGenerateRow(this, &SComboGraphManagerWidget::GenerateRow)
				.OnSelectionChanged(this, &SComboGraphManagerWidget::OnSelectionChanged)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("OwnerType")).DefaultLabel(LOCTEXT("OwnerTypeColumn", "Type")).FixedWidth(120.f)
					+ SHeaderRow::Column(TEXT("Owner")).DefaultLabel(LOCTEXT("OwnerColumn", "Owner")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("Graph")).DefaultLabel(LOCTEXT("GraphColumn", "Graph")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("Node")).DefaultLabel(LOCTEXT("NodeColumn", "Node")).FixedWidth(110.f)
					+ SHeaderRow::Column(TEXT("Input")).DefaultLabel(LOCTEXT("InputColumn", "Input")).FixedWidth(70.f)
					+ SHeaderRow::Column(TEXT("Ability")).DefaultLabel(LOCTEXT("AbilityColumn", "Ability")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("Montage")).DefaultLabel(LOCTEXT("MontageColumn", "Montage")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("Attack")).DefaultLabel(LOCTEXT("AttackColumn", "Damage")).FixedWidth(150.f)
					+ SHeaderRow::Column(TEXT("Status")).DefaultLabel(LOCTEXT("StatusColumn", "Status")).FixedWidth(150.f)
					+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("ActionsColumn", "Actions")).FixedWidth(210.f)
				)
			]
			+ SSplitter::Slot().Value(0.36f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(this, &SComboGraphManagerWidget::GetSelectionText).AutoWrapText(true)
				]
				+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
				[
					BuildNumberRow(TEXT("ActDamage"), TEXT("ActDamage"), TEXT("Damage authored directly on the combo graph node."))
				]
				+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
				[
					BuildNumberRow(TEXT("ActRange"), TEXT("ActRange"), TEXT("Attack query range authored directly on the combo graph node."))
				]
				+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
				[
					BuildNumberRow(TEXT("ActResilience"), TEXT("ActResilience"), TEXT("Poise/resilience bonus for this hit."))
				]
				+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
				[
					BuildNumberRow(TEXT("ActDmgReduce"), TEXT("ActDmgReduce"), TEXT("Damage reduction authored directly on the node."))
				]
				+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SBox).MinDesiredHeight(460.f)
					[
						DetailsView.ToSharedRef()
					]
				]
			]
		]
	];

	RefreshData(LOCTEXT("InitialRefresh", "Combo Manager refreshed."));
}

void SComboGraphManagerWidget::RefreshData(const FText& NewStatus)
{
	Rows.Reset();
	auto AddGraphRows = [this](UGameplayAbilityComboGraph* Graph, const FString& OwnerType, const FString& OwnerName)
	{
		if (!Graph)
		{
			return;
		}
		for (UGenericGraphNode* GenericNode : Graph->AllNodes)
		{
			UGameplayAbilityComboGraphNode* Node = Cast<UGameplayAbilityComboGraphNode>(GenericNode);
			if (!Node)
			{
				continue;
			}
			UAnimMontage* Montage = Node->Montage.Get();
			UAN_MeleeDamage* DamageNotify = FindFirstDamageNotify(Montage);

			TSharedRef<FComboGraphManagerRow> Row = MakeShared<FComboGraphManagerRow>();
			Row->Graph = Graph;
			Row->Node = Node;
			Row->Montage = Montage;
			Row->DamageNotify = DamageNotify;
			Row->OwnerType = OwnerType;
			Row->OwnerName = OwnerName;
			Row->Status = ComboNodeStatus(Node, DamageNotify);
			Rows.Add(Row);
		}
		if (Graph->AllNodes.IsEmpty())
		{
			TSharedRef<FComboGraphManagerRow> Row = MakeShared<FComboGraphManagerRow>();
			Row->Graph = Graph;
			Row->OwnerType = OwnerType;
			Row->OwnerName = OwnerName;
			Row->Status = TEXT("Graph has no nodes");
			Rows.Add(Row);
		}
	};

	TArray<UGameplayAbilityComboGraph*> Graphs = CollectAssetsOfClass<UGameplayAbilityComboGraph>();
	Graphs.Sort([](const UGameplayAbilityComboGraph& A, const UGameplayAbilityComboGraph& B) { return A.GetName() < B.GetName(); });
	for (UGameplayAbilityComboGraph* Graph : Graphs)
	{
		if (Graph)
		{
			AddGraphRows(Graph, TEXT("ComboGraph Asset"), Graph->GetName());
		}
	}

	Rows.Sort([](const FRowPtr& A, const FRowPtr& B)
	{
		const FString AKey = A.IsValid() ? FString::Printf(TEXT("%s|%s|%s"), *A->OwnerType, *A->OwnerName, A->Node.IsValid() ? *A->Node->NodeId.ToString() : TEXT("")) : FString();
		const FString BKey = B.IsValid() ? FString::Printf(TEXT("%s|%s|%s"), *B->OwnerType, *B->OwnerName, B->Node.IsValid() ? *B->Node->NodeId.ToString() : TEXT("")) : FString();
		return AKey < BKey;
	});

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
		if (Rows.Num() > 0)
		{
			ListView->SetSelection(Rows[0]);
		}
	}
	OnSelectionChanged(Rows.Num() > 0 ? Rows[0] : nullptr, ESelectInfo::Direct);
	if (!NewStatus.IsEmpty())
	{
		StatusText = NewStatus;
	}
}

TSharedRef<ITableRow> SComboGraphManagerWidget::GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SComboGraphManagerTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(SharedThis(this));
}

void SComboGraphManagerWidget::OnSelectionChanged(FRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedRow = Row;
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(Row.IsValid() ? GetDetailsObject(*Row) : nullptr);
	}
}

TSharedRef<SWidget> SComboGraphManagerWidget::BuildNumberRow(FName ColumnName, const FString& Label, const FString& Description)
{
	return SNew(SBorder).Padding(4.f)
		.Visibility_Lambda([this, ColumnName]()
		{
			return SelectedRow.IsValid() && (CanEditFloat(*SelectedRow, ColumnName) || ReadFloat(*SelectedRow, ColumnName) != 0.f)
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock).Text(FText::FromString(Label)).ToolTipText(FText::FromString(Description))
			]
			+ SHorizontalBox::Slot().FillWidth(0.55f)
			[
				SNew(SNumericEntryBox<float>)
				.MinDesiredValueWidth(120.f)
				.IsEnabled_Lambda([this, ColumnName]() { return SelectedRow.IsValid() && CanEditFloat(*SelectedRow, ColumnName); })
				.Value_Lambda([this, ColumnName]()
				{
					return SelectedRow.IsValid() ? TOptional<float>(ReadFloat(*SelectedRow, ColumnName)) : TOptional<float>();
				})
				.OnValueCommitted_Lambda([this, ColumnName](float NewValue, ETextCommit::Type)
				{
					CommitFloat(SelectedRow, ColumnName, NewValue);
				})
			]
		];
}

FText SComboGraphManagerWidget::GetStatsText() const
{
	return FText::Format(LOCTEXT("Stats", "Rows: {0} | {1}"), FText::AsNumber(Rows.Num()), StatusText);
}

FText SComboGraphManagerWidget::GetSelectionText() const
{
	if (!SelectedRow.IsValid())
	{
		return LOCTEXT("NoSelection", "Select a combo node.");
	}
	const UGameplayAbilityComboGraphNode* Node = SelectedRow->Node.Get();
	return FText::FromString(FString::Printf(
		TEXT("Type: %s\nOwner: %s\nGraph: %s\nNode: %s\nInput: %s\nMontage: %s\nMontage Damage Notify: %s\nStatus: %s"),
		*SelectedRow->OwnerType,
		*SelectedRow->OwnerName,
		*ObjectPath(SelectedRow->Graph.Get()),
		Node ? *Node->NodeId.ToString() : TEXT("-"),
		Node && StaticEnum<EYogComboGraphInputAction>()
			? *StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(Node->RootInputAction))
			: TEXT("-"),
		*ObjectPath(SelectedRow->Montage.Get()),
		SelectedRow->DamageNotify.IsValid() ? *SelectedRow->DamageNotify->GetName() : TEXT("-"),
		*SelectedRow->Status));
}

FReply SComboGraphManagerWidget::OnRefreshClicked()
{
	RefreshData(LOCTEXT("Refreshed", "Combo Manager refreshed."));
	return FReply::Handled();
}

void SComboGraphManagerWidget::OpenPrimaryAsset(TSharedPtr<FComboGraphManagerRow> Row) const
{
	if (!Row.IsValid())
	{
		return;
	}
	UObject* Object = Row->Graph.Get();
	if (GEditor && Object)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(Object);
		}
	}
}

void SComboGraphManagerWidget::CommitFloat(TSharedPtr<FComboGraphManagerRow> Row, FName ColumnName, float NewValue)
{
	if (!Row.IsValid() || !CanEditFloat(*Row, ColumnName))
	{
		return;
	}
	StatusText = LOCTEXT("EditUnsupported", "Node attack editing moved out of the standalone combo graph plugin.");
}

void SComboGraphManagerWidget::MigrateNodeFromNotify(TSharedPtr<FComboGraphManagerRow> Row)
{
	if (!Row.IsValid() || !Row->Node.IsValid() || !Row->DamageNotify.IsValid())
	{
		return;
	}

	Row->Status = ComboNodeStatus(Row->Node.Get(), Row->DamageNotify.Get());
	StatusText = FText::Format(LOCTEXT("MigrateUnsupported", "Node {0} now stores combo data only; keep damage, hit reaction, and FX on the montage."),
		FText::FromString(Row->Node->NodeId.ToString()));
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(Row->Node.Get());
	}
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

UObject* SComboGraphManagerWidget::GetDetailsObject(const FComboGraphManagerRow& Row) const
{
	if (Row.Node.IsValid()) return Row.Node.Get();
	if (Row.Graph.IsValid()) return Row.Graph.Get();
	return nullptr;
}

bool SComboGraphManagerWidget::CanEditFloat(const FComboGraphManagerRow& Row, FName ColumnName) const
{
	return Row.Node.IsValid()
		&& false
		&& (ColumnName == TEXT("ActDamage")
			|| ColumnName == TEXT("ActRange")
			|| ColumnName == TEXT("ActResilience")
			|| ColumnName == TEXT("ActDmgReduce"));
}

#undef LOCTEXT_NAMESPACE
