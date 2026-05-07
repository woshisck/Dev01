#include "Tools/SCharacterBalanceWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/CharacterData.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SCharacterBalanceWidget"

namespace
{
	template <typename T>
	TArray<T*> CollectCharacterBalanceAssetsOfClass()
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

	FYogBaseAttributeData* GetBaseRow(UCharacterData* Character)
	{
		return Character ? Character->YogBaseAttributeDataRow.GetRow<FYogBaseAttributeData>(TEXT("CharacterBalance")) : nullptr;
	}

	FMovementData* GetMovementRow(UCharacterData* Character)
	{
		return Character ? Character->MovementDataRow.GetRow<FMovementData>(TEXT("CharacterBalance")) : nullptr;
	}

	UDataTable* GetBaseTable(UCharacterData* Character)
	{
		return Character ? const_cast<UDataTable*>(Character->YogBaseAttributeDataRow.DataTable.Get()) : nullptr;
	}

	UDataTable* GetMovementTable(UCharacterData* Character)
	{
		return Character ? const_cast<UDataTable*>(Character->MovementDataRow.DataTable.Get()) : nullptr;
	}

	FString RowNameToString(const FDataTableRowHandle& Handle)
	{
		return Handle.RowName.IsNone() ? TEXT("-") : Handle.RowName.ToString();
	}

	float ReadCharacterFloat(UCharacterData* Character, FName ColumnName)
	{
		if (FYogBaseAttributeData* Attr = GetBaseRow(Character))
		{
			if (ColumnName == TEXT("Attack")) return Attr->Attack;
			if (ColumnName == TEXT("AttackPower")) return Attr->AttackPower;
			if (ColumnName == TEXT("MaxHealth")) return Attr->MaxHealth;
			if (ColumnName == TEXT("MaxHeat")) return Attr->MaxHeat;
			if (ColumnName == TEXT("Shield")) return Attr->Shield;
			if (ColumnName == TEXT("AttackSpeed")) return Attr->AttackSpeed;
			if (ColumnName == TEXT("AttackRange")) return Attr->AttackRange;
			if (ColumnName == TEXT("MoveSpeed")) return Attr->MoveSpeed;
			if (ColumnName == TEXT("Dodge")) return Attr->Dodge;
			if (ColumnName == TEXT("Resilience")) return Attr->Resilience;
			if (ColumnName == TEXT("Resist")) return Attr->Resist;
			if (ColumnName == TEXT("DmgTaken")) return Attr->DmgTaken;
			if (ColumnName == TEXT("CritRate")) return Attr->Crit_Rate;
			if (ColumnName == TEXT("CritDamage")) return Attr->Crit_Damage;
			if (ColumnName == TEXT("MaxArmorHP")) return Attr->MaxArmorHP;
			if (ColumnName == TEXT("Sanity")) return Attr->Sanity;
		}

		if (FMovementData* Move = GetMovementRow(Character))
		{
			if (ColumnName == TEXT("MaxWalkSpeed")) return Move->MaxWalkSpeed;
			if (ColumnName == TEXT("GroundFriction")) return Move->GroundFriction;
			if (ColumnName == TEXT("BreakingDeceleration")) return Move->BreakingDeceleration;
			if (ColumnName == TEXT("MaxAcceleration")) return Move->MaxAcceleration;
			if (ColumnName == TEXT("RotationYaw")) return Move->RotationRate.Yaw;
		}

		return 0.f;
	}

	bool IsBaseColumn(FName ColumnName)
	{
		return ColumnName == TEXT("Attack")
			|| ColumnName == TEXT("AttackPower")
			|| ColumnName == TEXT("MaxHealth")
			|| ColumnName == TEXT("MaxHeat")
			|| ColumnName == TEXT("Shield")
			|| ColumnName == TEXT("AttackSpeed")
			|| ColumnName == TEXT("AttackRange")
			|| ColumnName == TEXT("MoveSpeed")
			|| ColumnName == TEXT("Dodge")
			|| ColumnName == TEXT("Resilience")
			|| ColumnName == TEXT("Resist")
			|| ColumnName == TEXT("DmgTaken")
			|| ColumnName == TEXT("CritRate")
			|| ColumnName == TEXT("CritDamage")
			|| ColumnName == TEXT("MaxArmorHP")
			|| ColumnName == TEXT("Sanity");
	}

	bool WriteCharacterFloat(UCharacterData* Character, FName ColumnName, float NewValue)
	{
		if (!Character)
		{
			return false;
		}

		if (IsBaseColumn(ColumnName))
		{
			FYogBaseAttributeData* Attr = GetBaseRow(Character);
			UDataTable* DataTable = GetBaseTable(Character);
			if (!Attr || !DataTable)
			{
				return false;
			}

			if (ColumnName == TEXT("Attack")) { Attr->Attack = NewValue; }
			else if (ColumnName == TEXT("AttackPower")) { Attr->AttackPower = NewValue; }
			else if (ColumnName == TEXT("MaxHealth")) { Attr->MaxHealth = NewValue; }
			else if (ColumnName == TEXT("MaxHeat")) { Attr->MaxHeat = NewValue; }
			else if (ColumnName == TEXT("Shield")) { Attr->Shield = NewValue; }
			else if (ColumnName == TEXT("AttackSpeed")) { Attr->AttackSpeed = NewValue; }
			else if (ColumnName == TEXT("AttackRange")) { Attr->AttackRange = NewValue; }
			else if (ColumnName == TEXT("MoveSpeed")) { Attr->MoveSpeed = NewValue; }
			else if (ColumnName == TEXT("Dodge")) { Attr->Dodge = NewValue; }
			else if (ColumnName == TEXT("Resilience")) { Attr->Resilience = NewValue; }
			else if (ColumnName == TEXT("Resist")) { Attr->Resist = NewValue; }
			else if (ColumnName == TEXT("DmgTaken")) { Attr->DmgTaken = NewValue; }
			else if (ColumnName == TEXT("CritRate")) { Attr->Crit_Rate = NewValue; }
			else if (ColumnName == TEXT("CritDamage")) { Attr->Crit_Damage = NewValue; }
			else if (ColumnName == TEXT("MaxArmorHP")) { Attr->MaxArmorHP = NewValue; }
			else if (ColumnName == TEXT("Sanity")) { Attr->Sanity = NewValue; }
			DataTable->MarkPackageDirty();
			return true;
		}

		if (FMovementData* Move = GetMovementRow(Character))
		{
			UDataTable* DataTable = GetMovementTable(Character);
			if (!DataTable)
			{
				return false;
			}

			if (ColumnName == TEXT("MaxWalkSpeed")) { Move->MaxWalkSpeed = NewValue; }
			else if (ColumnName == TEXT("GroundFriction")) { Move->GroundFriction = NewValue; }
			else if (ColumnName == TEXT("BreakingDeceleration")) { Move->BreakingDeceleration = NewValue; }
			else if (ColumnName == TEXT("MaxAcceleration")) { Move->MaxAcceleration = NewValue; }
			else if (ColumnName == TEXT("RotationYaw")) { Move->RotationRate.Yaw = NewValue; }
			else { return false; }

			DataTable->MarkPackageDirty();
			return true;
		}

		return false;
	}

	TSharedRef<SWidget> MakeCharacterBalanceTextCell(const FString& Text, const FString& ToolTip = FString())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip));
	}
}

class SCharacterBalanceTableRow : public SMultiColumnTableRow<TSharedPtr<FCharacterBalanceRow>>
{
public:
	SLATE_BEGIN_ARGS(SCharacterBalanceTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FCharacterBalanceRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SCharacterBalanceWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FCharacterBalanceRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FCharacterBalanceRow>>::FArguments().Padding(FMargin(3.f, 1.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UCharacterData* Character = Item.IsValid() ? Item->Asset.Get() : nullptr;
		if (!Character)
		{
			return MakeCharacterBalanceTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Asset"))
		{
			return MakeCharacterBalanceTextCell(Character->GetName(), Character->GetPathName());
		}
		if (ColumnName == TEXT("BaseRow"))
		{
			return MakeCharacterBalanceTextCell(RowNameToString(Character->YogBaseAttributeDataRow), GetNameSafe(GetBaseTable(Character)));
		}
		if (ColumnName == TEXT("MoveRow"))
		{
			return MakeCharacterBalanceTextCell(RowNameToString(Character->MovementDataRow), GetNameSafe(GetMovementTable(Character)));
		}
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenCharacter", "Open"))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SCharacterBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->OpenAsset(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenBaseTable", "Base DT"))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SCharacterBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->OpenBaseTable(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenMoveTable", "Move DT"))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SCharacterBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->OpenMovementTable(Row);
						}
						return FReply::Handled();
					})
				];
		}

		const float Value = ReadCharacterFloat(Character, ColumnName);
		return SNew(SNumericEntryBox<float>)
			.MinDesiredValueWidth(58.f)
			.Value_Lambda([Value]() { return TOptional<float>(Value); })
			.OnValueCommitted_Lambda([Owner = OwnerWidget, Row = Item, ColumnName](float NewValue, ETextCommit::Type)
			{
				if (TSharedPtr<SCharacterBalanceWidget> Pinned = Owner.Pin())
				{
					Pinned->CommitFloat(Row, ColumnName, NewValue);
				}
			});
	}

private:
	TSharedPtr<FCharacterBalanceRow> Item;
	TWeakPtr<SCharacterBalanceWidget> OwnerWidget;
};

void SCharacterBalanceWidget::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("CharacterReady", "Ready");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Refresh", "Refresh")).OnClicked(this, &SCharacterBalanceWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Validate", "Validate All")).OnClicked(this, &SCharacterBalanceWidget::OnValidateClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("ExportCsv", "Export CSV")).OnClicked(this, &SCharacterBalanceWidget::OnExportCsvClicked)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 8.f, 6.f)
		[
			SNew(STextBlock).Text(this, &SCharacterBalanceWidget::GetStatsText)
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SBorder).Padding(2.f)
			[
				SAssignNew(ListView, SListView<FRowPtr>)
				.ListItemsSource(&Rows)
				.OnGenerateRow(this, &SCharacterBalanceWidget::GenerateRow)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("Asset")).DefaultLabel(LOCTEXT("Asset", "Asset")).FillWidth(1.1f)
					+ SHeaderRow::Column(TEXT("BaseRow")).DefaultLabel(LOCTEXT("BaseRow", "Base Row")).FixedWidth(95.f)
					+ SHeaderRow::Column(TEXT("MoveRow")).DefaultLabel(LOCTEXT("MoveRow", "Move Row")).FixedWidth(95.f)
					+ SHeaderRow::Column(TEXT("Attack")).DefaultLabel(LOCTEXT("Attack", "Attack")).FixedWidth(76.f)
					+ SHeaderRow::Column(TEXT("AttackPower")).DefaultLabel(LOCTEXT("AttackPower", "AtkPower")).FixedWidth(84.f)
					+ SHeaderRow::Column(TEXT("MaxHealth")).DefaultLabel(LOCTEXT("MaxHealth", "HP")).FixedWidth(72.f)
					+ SHeaderRow::Column(TEXT("MaxHeat")).DefaultLabel(LOCTEXT("MaxHeat", "Heat")).FixedWidth(72.f)
					+ SHeaderRow::Column(TEXT("Shield")).DefaultLabel(LOCTEXT("Shield", "Shield")).FixedWidth(72.f)
					+ SHeaderRow::Column(TEXT("AttackSpeed")).DefaultLabel(LOCTEXT("AttackSpeed", "AtkSpeed")).FixedWidth(84.f)
					+ SHeaderRow::Column(TEXT("AttackRange")).DefaultLabel(LOCTEXT("AttackRange", "AtkRange")).FixedWidth(84.f)
					+ SHeaderRow::Column(TEXT("MoveSpeed")).DefaultLabel(LOCTEXT("MoveSpeed", "MoveSpeed")).FixedWidth(90.f)
					+ SHeaderRow::Column(TEXT("CritRate")).DefaultLabel(LOCTEXT("CritRate", "CritRate")).FixedWidth(84.f)
					+ SHeaderRow::Column(TEXT("CritDamage")).DefaultLabel(LOCTEXT("CritDamage", "CritDmg")).FixedWidth(84.f)
					+ SHeaderRow::Column(TEXT("MaxWalkSpeed")).DefaultLabel(LOCTEXT("MaxWalkSpeed", "Walk")).FixedWidth(76.f)
					+ SHeaderRow::Column(TEXT("GroundFriction")).DefaultLabel(LOCTEXT("GroundFriction", "Friction")).FixedWidth(84.f)
					+ SHeaderRow::Column(TEXT("BreakingDeceleration")).DefaultLabel(LOCTEXT("BreakingDeceleration", "Brake")).FixedWidth(76.f)
					+ SHeaderRow::Column(TEXT("MaxAcceleration")).DefaultLabel(LOCTEXT("MaxAcceleration", "Accel")).FixedWidth(76.f)
					+ SHeaderRow::Column(TEXT("RotationYaw")).DefaultLabel(LOCTEXT("RotationYaw", "RotYaw")).FixedWidth(76.f)
					+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("Actions", "Actions")).FixedWidth(210.f)
				)
			]
		]
	];

	RefreshData(LOCTEXT("InitialStatus", "Character list refreshed."));
}

void SCharacterBalanceWidget::RefreshData(const FText& NewStatus)
{
	TArray<UCharacterData*> Characters = CollectCharacterBalanceAssetsOfClass<UCharacterData>();
	Characters.Sort([](const UCharacterData& A, const UCharacterData& B)
	{
		return A.GetName() < B.GetName();
	});

	Rows.Reset();
	for (UCharacterData* Character : Characters)
	{
		Rows.Add(MakeShared<FCharacterBalanceRow>(Character));
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	if (!NewStatus.IsEmpty())
	{
		StatusText = NewStatus;
	}
}

TSharedRef<ITableRow> SCharacterBalanceWidget::GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SCharacterBalanceTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(SharedThis(this));
}

FText SCharacterBalanceWidget::GetStatsText() const
{
	return FText::Format(LOCTEXT("Stats", "CharacterData: {0} | {1}"), FText::AsNumber(Rows.Num()), StatusText);
}

FReply SCharacterBalanceWidget::OnRefreshClicked()
{
	RefreshData(LOCTEXT("Refreshed", "Character list refreshed."));
	return FReply::Handled();
}

FReply SCharacterBalanceWidget::OnValidateClicked()
{
	int32 Errors = 0;
	int32 Warnings = 0;
	TSet<FString> SeenRows;
	for (const FRowPtr& Row : Rows)
	{
		UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
		if (!Character)
		{
			continue;
		}
		if (!GetBaseRow(Character))
		{
			++Errors;
			UE_LOG(LogTemp, Warning, TEXT("CharacterBalance: missing base row on %s"), *GetNameSafe(Character));
		}
		if (!GetMovementRow(Character))
		{
			++Warnings;
			UE_LOG(LogTemp, Warning, TEXT("CharacterBalance: missing movement row on %s"), *GetNameSafe(Character));
		}

		const FString BaseKey = FString::Printf(TEXT("%s:%s"), *GetNameSafe(GetBaseTable(Character)), *RowNameToString(Character->YogBaseAttributeDataRow));
		if (SeenRows.Contains(BaseKey))
		{
			++Warnings;
			UE_LOG(LogTemp, Warning, TEXT("CharacterBalance: shared base row %s"), *BaseKey);
		}
		SeenRows.Add(BaseKey);
	}

	StatusText = FText::Format(LOCTEXT("ValidateStatus", "Validate All: {0} errors, {1} warnings. See Output Log."), FText::AsNumber(Errors), FText::AsNumber(Warnings));
	return FReply::Handled();
}

FReply SCharacterBalanceWidget::OnExportCsvClicked()
{
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Balance") / FString::Printf(TEXT("CharacterNumbers_%s.csv"), *Stamp);
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FPaths::GetPath(Path));

	FString Csv = TEXT("Asset,BaseRow,MoveRow,Attack,AttackPower,MaxHealth,MaxHeat,Shield,AttackSpeed,AttackRange,MoveSpeed,CritRate,CritDamage,MaxWalkSpeed,GroundFriction,BreakingDeceleration,MaxAcceleration,RotationYaw\n");
	for (const FRowPtr& Row : Rows)
	{
		UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
		if (!Character)
		{
			continue;
		}
		Csv += FString::Printf(TEXT("%s,%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n"),
			*Character->GetName(),
			*RowNameToString(Character->YogBaseAttributeDataRow),
			*RowNameToString(Character->MovementDataRow),
			ReadCharacterFloat(Character, TEXT("Attack")),
			ReadCharacterFloat(Character, TEXT("AttackPower")),
			ReadCharacterFloat(Character, TEXT("MaxHealth")),
			ReadCharacterFloat(Character, TEXT("MaxHeat")),
			ReadCharacterFloat(Character, TEXT("Shield")),
			ReadCharacterFloat(Character, TEXT("AttackSpeed")),
			ReadCharacterFloat(Character, TEXT("AttackRange")),
			ReadCharacterFloat(Character, TEXT("MoveSpeed")),
			ReadCharacterFloat(Character, TEXT("CritRate")),
			ReadCharacterFloat(Character, TEXT("CritDamage")),
			ReadCharacterFloat(Character, TEXT("MaxWalkSpeed")),
			ReadCharacterFloat(Character, TEXT("GroundFriction")),
			ReadCharacterFloat(Character, TEXT("BreakingDeceleration")),
			ReadCharacterFloat(Character, TEXT("MaxAcceleration")),
			ReadCharacterFloat(Character, TEXT("RotationYaw")));
	}

	FFileHelper::SaveStringToFile(Csv, *Path, FFileHelper::EEncodingOptions::ForceUTF8);
	StatusText = FText::Format(LOCTEXT("Exported", "Exported {0}"), FText::FromString(FPaths::GetCleanFilename(Path)));
	return FReply::Handled();
}

void SCharacterBalanceWidget::OpenAsset(TSharedPtr<FCharacterBalanceRow> Row) const
{
	if (GEditor && Row.IsValid())
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(Row->Asset.Get());
		}
	}
}

void SCharacterBalanceWidget::OpenBaseTable(TSharedPtr<FCharacterBalanceRow> Row) const
{
	UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
	if (GEditor && Character)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(GetBaseTable(Character));
		}
	}
}

void SCharacterBalanceWidget::OpenMovementTable(TSharedPtr<FCharacterBalanceRow> Row) const
{
	UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
	if (GEditor && Character)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(GetMovementTable(Character));
		}
	}
}

void SCharacterBalanceWidget::CommitFloat(TSharedPtr<FCharacterBalanceRow> Row, FName ColumnName, float NewValue)
{
	UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
	if (!Character)
	{
		return;
	}

	UDataTable* DataTable = IsBaseColumn(ColumnName) ? GetBaseTable(Character) : GetMovementTable(Character);
	const FScopedTransaction Transaction(LOCTEXT("EditCharacterBalance", "Edit Character Balance Value"));
	if (DataTable)
	{
		DataTable->Modify();
	}
	Character->Modify();

	if (WriteCharacterFloat(Character, ColumnName, NewValue))
	{
		StatusText = FText::Format(LOCTEXT("CommitStatus", "Set {0}.{1} = {2}. DataTable dirty, not auto-saved."),
			FText::FromString(Character->GetName()), FText::FromName(ColumnName), FText::AsNumber(NewValue));
		if (ListView.IsValid())
		{
			ListView->RequestListRefresh();
		}
	}
}

#undef LOCTEXT_NAMESPACE
