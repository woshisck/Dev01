#include "Tools/SActionBalanceWidget.h"

#include "AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h"
#include "AbilitySystem/Abilities/Musket/GA_Musket_LightAttack.h"
#include "AbilitySystem/Abilities/Musket/GA_Musket_SprintAttack.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/MontageNotifyEntry.h"
#include "Data/MusketActionTuningDataAsset.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformFileManager.h"
#include "IAssetTools.h"
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

#define LOCTEXT_NAMESPACE "SActionBalanceWidget"

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

	FString RowTypeToString(EActionBalanceRowType Type)
	{
		switch (Type)
		{
		case EActionBalanceRowType::MeleeAttackData: return TEXT("Melee AttackData");
		case EActionBalanceRowType::MeleeHitWindow:  return TEXT("Melee HitWindow");
		case EActionBalanceRowType::MeleeComboWindow:return TEXT("Melee Combo");
		case EActionBalanceRowType::MeleeEarlyExit:  return TEXT("Melee EarlyExit");
		case EActionBalanceRowType::MusketLight:     return TEXT("Musket Light");
		case EActionBalanceRowType::MusketHeavy:     return TEXT("Musket Heavy");
		case EActionBalanceRowType::MusketSprint:    return TEXT("Musket Sprint");
		}
		return TEXT("Unknown");
	}

	FString PrimaryName(const FActionBalanceRow& Row)
	{
		if (UObject* Object = Row.PrimaryObject.Get())
		{
			return Object->GetName();
		}
		if (UBlueprint* Blueprint = Row.BlueprintAsset.Get())
		{
			return Blueprint->GetName();
		}
		return TEXT("-");
	}

	UMusketActionTuningDataAsset* GetTuning(const FActionBalanceRow& Row)
	{
		return Row.MusketCDO.IsValid() ? Row.MusketCDO->TuningData.Get() : nullptr;
	}

	UObject* GetEditObject(const FActionBalanceRow& Row)
	{
		if (UMusketActionTuningDataAsset* Tuning = GetTuning(Row))
		{
			return Tuning;
		}
		if (Row.MusketCDO.IsValid())
		{
			return Row.MusketCDO.Get();
		}
		if (Row.MontageEntry.IsValid())
		{
			return Row.MontageEntry.Get();
		}
		return Row.PrimaryObject.Get();
	}

	float ReadValue(const FActionBalanceRow& Row, FName ColumnName)
	{
		if (UMontageAttackDataAsset* AttackData = Cast<UMontageAttackDataAsset>(Row.PrimaryObject.Get()))
		{
			if (ColumnName == TEXT("A")) return AttackData->ActDamage;
			if (ColumnName == TEXT("B")) return AttackData->ActRange;
			if (ColumnName == TEXT("C")) return AttackData->ActResilience;
			if (ColumnName == TEXT("D")) return AttackData->ActDmgReduce;
		}
		if (UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("A")) return HitWindow->StartFrame;
			if (ColumnName == TEXT("B")) return HitWindow->EndFrame;
			if (ColumnName == TEXT("C")) return HitWindow->AttackDataCandidates.Num();
		}
		if (UMNE_ComboWindow* ComboWindow = Cast<UMNE_ComboWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("A")) return ComboWindow->StartFrame;
			if (ColumnName == TEXT("B")) return ComboWindow->EndFrame;
		}
		if (UMNE_EarlyExit* EarlyExit = Cast<UMNE_EarlyExit>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("A")) return EarlyExit->Frame;
			if (ColumnName == TEXT("B")) return EarlyExit->BlendTime;
		}

		if (const UMusketActionTuningDataAsset* Tuning = GetTuning(Row))
		{
			if (Row.Type == EActionBalanceRowType::MusketLight)
			{
				if (ColumnName == TEXT("A")) return Tuning->LightDamageMultiplier;
				if (ColumnName == TEXT("B")) return Tuning->LightHalfAngleDeg;
			}
			if (Row.Type == EActionBalanceRowType::MusketHeavy)
			{
				if (ColumnName == TEXT("A")) return Tuning->HeavyChargeTime;
				if (ColumnName == TEXT("B")) return Tuning->HeavyBaseDamageMultiplier;
				if (ColumnName == TEXT("C")) return Tuning->HeavyFullChargeMultiplier;
				if (ColumnName == TEXT("D")) return Tuning->HeavyEndRadius;
			}
			if (Row.Type == EActionBalanceRowType::MusketSprint)
			{
				if (ColumnName == TEXT("A")) return Tuning->SprintDamageMultiplier;
				if (ColumnName == TEXT("B")) return Tuning->SprintHalfFanAngle;
			}
		}

		if (UGA_Musket_LightAttack* Light = Cast<UGA_Musket_LightAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("A")) return Light->DamageMultiplier;
			if (ColumnName == TEXT("B")) return Light->HalfAngleDeg;
		}
		if (UGA_Musket_HeavyAttack* Heavy = Cast<UGA_Musket_HeavyAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("A")) return Heavy->ChargeTime;
			if (ColumnName == TEXT("B")) return Heavy->BaseDamageMultiplier;
			if (ColumnName == TEXT("C")) return Heavy->FullChargeMultiplier;
			if (ColumnName == TEXT("D")) return Heavy->EndRadius;
		}
		if (UGA_Musket_SprintAttack* Sprint = Cast<UGA_Musket_SprintAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("A")) return Sprint->DamageMultiplier;
			if (ColumnName == TEXT("B")) return Sprint->HalfFanAngle;
		}

		return 0.f;
	}

	bool WriteValue(const FActionBalanceRow& Row, FName ColumnName, float NewValue)
	{
		if (UMontageAttackDataAsset* AttackData = Cast<UMontageAttackDataAsset>(Row.PrimaryObject.Get()))
		{
			if (ColumnName == TEXT("A")) { AttackData->ActDamage = NewValue; }
			else if (ColumnName == TEXT("B")) { AttackData->ActRange = NewValue; }
			else if (ColumnName == TEXT("C")) { AttackData->ActResilience = NewValue; }
			else if (ColumnName == TEXT("D")) { AttackData->ActDmgReduce = NewValue; }
			else { return false; }
			AttackData->MarkPackageDirty();
			return true;
		}
		if (UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("A")) { HitWindow->StartFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else if (ColumnName == TEXT("B")) { HitWindow->EndFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else { return false; }
			if (Row.MontageConfig.IsValid()) Row.MontageConfig->MarkPackageDirty();
			return true;
		}
		if (UMNE_ComboWindow* ComboWindow = Cast<UMNE_ComboWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("A")) { ComboWindow->StartFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else if (ColumnName == TEXT("B")) { ComboWindow->EndFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else { return false; }
			if (Row.MontageConfig.IsValid()) Row.MontageConfig->MarkPackageDirty();
			return true;
		}
		if (UMNE_EarlyExit* EarlyExit = Cast<UMNE_EarlyExit>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("A")) { EarlyExit->Frame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else if (ColumnName == TEXT("B")) { EarlyExit->BlendTime = FMath::Max(0.f, NewValue); }
			else { return false; }
			if (Row.MontageConfig.IsValid()) Row.MontageConfig->MarkPackageDirty();
			return true;
		}

		if (UMusketActionTuningDataAsset* Tuning = GetTuning(Row))
		{
			if (Row.Type == EActionBalanceRowType::MusketLight)
			{
				if (ColumnName == TEXT("A")) { Tuning->LightDamageMultiplier = NewValue; }
				else if (ColumnName == TEXT("B")) { Tuning->LightHalfAngleDeg = NewValue; }
				else { return false; }
			}
			else if (Row.Type == EActionBalanceRowType::MusketHeavy)
			{
				if (ColumnName == TEXT("A")) { Tuning->HeavyChargeTime = NewValue; }
				else if (ColumnName == TEXT("B")) { Tuning->HeavyBaseDamageMultiplier = NewValue; }
				else if (ColumnName == TEXT("C")) { Tuning->HeavyFullChargeMultiplier = NewValue; }
				else if (ColumnName == TEXT("D")) { Tuning->HeavyEndRadius = NewValue; }
				else { return false; }
			}
			else if (Row.Type == EActionBalanceRowType::MusketSprint)
			{
				if (ColumnName == TEXT("A")) { Tuning->SprintDamageMultiplier = NewValue; }
				else if (ColumnName == TEXT("B")) { Tuning->SprintHalfFanAngle = NewValue; }
				else { return false; }
			}
			Tuning->MarkPackageDirty();
			return true;
		}

		if (UGA_Musket_LightAttack* Light = Cast<UGA_Musket_LightAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("A")) { Light->DamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("B")) { Light->HalfAngleDeg = NewValue; }
			else { return false; }
		}
		else if (UGA_Musket_HeavyAttack* Heavy = Cast<UGA_Musket_HeavyAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("A")) { Heavy->ChargeTime = NewValue; }
			else if (ColumnName == TEXT("B")) { Heavy->BaseDamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("C")) { Heavy->FullChargeMultiplier = NewValue; }
			else if (ColumnName == TEXT("D")) { Heavy->EndRadius = NewValue; }
			else { return false; }
		}
		else if (UGA_Musket_SprintAttack* Sprint = Cast<UGA_Musket_SprintAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("A")) { Sprint->DamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("B")) { Sprint->HalfFanAngle = NewValue; }
			else { return false; }
		}
		else
		{
			return false;
		}

		if (Row.BlueprintAsset.IsValid())
		{
			Row.BlueprintAsset->MarkPackageDirty();
		}
		return true;
	}

	TSharedRef<SWidget> MakeTextCell(const FString& Text, const FString& ToolTip = FString())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip));
	}
}

class SActionBalanceTableRow : public SMultiColumnTableRow<TSharedPtr<FActionBalanceRow>>
{
public:
	SLATE_BEGIN_ARGS(SActionBalanceTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FActionBalanceRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SActionBalanceWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FActionBalanceRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FActionBalanceRow>>::FArguments().Padding(FMargin(3.f, 1.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Source"))
		{
			return MakeTextCell(PrimaryName(*Item), Item->PrimaryObject.IsValid() ? Item->PrimaryObject->GetPathName() : FString());
		}
		if (ColumnName == TEXT("Type"))
		{
			return MakeTextCell(RowTypeToString(Item->Type));
		}
		if (ColumnName == TEXT("Tuning"))
		{
			if (UMusketActionTuningDataAsset* Tuning = GetTuning(*Item))
			{
				return MakeTextCell(Tuning->GetName(), Tuning->GetPathName());
			}
			if (Item->MusketCDO.IsValid())
			{
				return MakeTextCell(TEXT("Class Defaults"));
			}
			if (UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Item->MontageEntry.Get()))
			{
				return MakeTextCell(FString::Printf(TEXT("%d AttackData"), HitWindow->AttackDataCandidates.Num()));
			}
			return MakeTextCell(TEXT("-"));
		}
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Open", "Open"))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SActionBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->OpenPrimaryAsset(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CreateAttackData", "Create AttackData"))
					.IsEnabled_Lambda([Row = Item]() { return Row.IsValid() && Row->Type == EActionBalanceRowType::MeleeHitWindow; })
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SActionBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->CreateAttackDataForHitWindow(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("CreateTuning", "Create Tuning"))
					.IsEnabled_Lambda([Row = Item]() { return Row.IsValid() && Row->MusketCDO.IsValid() && !GetTuning(*Row); })
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SActionBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->CreateMusketTuningData(Row);
						}
						return FReply::Handled();
					})
				];
		}

		const float Value = ReadValue(*Item, ColumnName);
		return SNew(SNumericEntryBox<float>)
			.MinDesiredValueWidth(64.f)
			.Value_Lambda([Value]() { return TOptional<float>(Value); })
			.IsEnabled_Lambda([Row = Item, ColumnName]()
			{
				return Row.IsValid() && !(Row->Type == EActionBalanceRowType::MeleeHitWindow && (ColumnName == TEXT("C") || ColumnName == TEXT("D")));
			})
			.OnValueCommitted_Lambda([Owner = OwnerWidget, Row = Item, ColumnName](float NewValue, ETextCommit::Type)
			{
				if (TSharedPtr<SActionBalanceWidget> Pinned = Owner.Pin())
				{
					Pinned->CommitValue(Row, ColumnName, NewValue);
				}
			});
	}

private:
	TSharedPtr<FActionBalanceRow> Item;
	TWeakPtr<SActionBalanceWidget> OwnerWidget;
};

void SActionBalanceWidget::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("ActionReady", "Ready");
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Refresh", "Refresh")).OnClicked(this, &SActionBalanceWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Validate", "Validate All")).OnClicked(this, &SActionBalanceWidget::OnValidateClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("ExportCsv", "Export CSV")).OnClicked(this, &SActionBalanceWidget::OnExportCsvClicked)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 8.f, 6.f)
		[
			SNew(STextBlock).Text(this, &SActionBalanceWidget::GetStatsText)
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SBorder).Padding(2.f)
			[
				SAssignNew(ListView, SListView<FRowPtr>)
				.ListItemsSource(&Rows)
				.OnGenerateRow(this, &SActionBalanceWidget::GenerateRow)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("Source")).DefaultLabel(LOCTEXT("Source", "Source")).FillWidth(1.4f)
					+ SHeaderRow::Column(TEXT("Type")).DefaultLabel(LOCTEXT("Type", "Type")).FixedWidth(130.f)
					+ SHeaderRow::Column(TEXT("A")).DefaultLabel(LOCTEXT("A", "A: Damage/Start/Charge")).FixedWidth(120.f)
					+ SHeaderRow::Column(TEXT("B")).DefaultLabel(LOCTEXT("B", "B: Range/End/Base")).FixedWidth(120.f)
					+ SHeaderRow::Column(TEXT("C")).DefaultLabel(LOCTEXT("C", "C: Res/Full")).FixedWidth(100.f)
					+ SHeaderRow::Column(TEXT("D")).DefaultLabel(LOCTEXT("D", "D: Reduce/Radius")).FixedWidth(110.f)
					+ SHeaderRow::Column(TEXT("Tuning")).DefaultLabel(LOCTEXT("Tuning", "Tuning Source")).FixedWidth(150.f)
					+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("Actions", "Actions")).FixedWidth(300.f)
				)
			]
		]
	];

	RefreshData(LOCTEXT("InitialStatus", "Action list refreshed."));
}

void SActionBalanceWidget::RefreshData(const FText& NewStatus)
{
	Rows.Reset();
	MusketBlueprintCount = 0;

	TArray<UMontageAttackDataAsset*> AttackDataAssets = CollectAssetsOfClass<UMontageAttackDataAsset>();
	AttackDataCount = AttackDataAssets.Num();
	for (UMontageAttackDataAsset* AttackData : AttackDataAssets)
	{
		TSharedRef<FActionBalanceRow> Row = MakeShared<FActionBalanceRow>();
		Row->Type = EActionBalanceRowType::MeleeAttackData;
		Row->PrimaryObject = AttackData;
		Rows.Add(Row);
	}

	TArray<UMontageConfigDA*> MontageConfigs = CollectAssetsOfClass<UMontageConfigDA>();
	MontageConfigCount = MontageConfigs.Num();
	for (UMontageConfigDA* Config : MontageConfigs)
	{
		for (UMontageNotifyEntry* Entry : Config->Entries)
		{
			EActionBalanceRowType Type;
			if (Cast<UMNE_HitWindow>(Entry)) Type = EActionBalanceRowType::MeleeHitWindow;
			else if (Cast<UMNE_ComboWindow>(Entry)) Type = EActionBalanceRowType::MeleeComboWindow;
			else if (Cast<UMNE_EarlyExit>(Entry)) Type = EActionBalanceRowType::MeleeEarlyExit;
			else continue;

			TSharedRef<FActionBalanceRow> Row = MakeShared<FActionBalanceRow>();
			Row->Type = Type;
			Row->PrimaryObject = Config;
			Row->MontageConfig = Config;
			Row->MontageEntry = Entry;
			Rows.Add(Row);
		}
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> BlueprintAssets;
	FARFilter BlueprintFilter;
	BlueprintFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	BlueprintFilter.PackagePaths.Add(FName(TEXT("/Game/Code/GAS/Abilities")));
	BlueprintFilter.bRecursiveClasses = true;
	BlueprintFilter.bRecursivePaths = true;
	AssetRegistry.GetAssets(BlueprintFilter, BlueprintAssets);
	for (const FAssetData& Asset : BlueprintAssets)
	{
		const FString PackageName = Asset.PackageName.ToString();
		const FString AssetName = Asset.AssetName.ToString();
		const bool bLooksLikeMusketAbility =
			AssetName.Contains(TEXT("Musket"), ESearchCase::IgnoreCase) ||
			PackageName.Contains(TEXT("/Musket/"), ESearchCase::IgnoreCase);
		if (!bLooksLikeMusketAbility)
		{
			continue;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			continue;
		}

		EActionBalanceRowType Type;
		if (Blueprint->GeneratedClass->IsChildOf(UGA_Musket_LightAttack::StaticClass())) Type = EActionBalanceRowType::MusketLight;
		else if (Blueprint->GeneratedClass->IsChildOf(UGA_Musket_HeavyAttack::StaticClass())) Type = EActionBalanceRowType::MusketHeavy;
		else if (Blueprint->GeneratedClass->IsChildOf(UGA_Musket_SprintAttack::StaticClass())) Type = EActionBalanceRowType::MusketSprint;
		else continue;

		TSharedRef<FActionBalanceRow> Row = MakeShared<FActionBalanceRow>();
		Row->Type = Type;
		Row->PrimaryObject = Blueprint;
		Row->BlueprintAsset = Blueprint;
		Row->MusketCDO = Cast<UGA_MusketBase>(Blueprint->GeneratedClass->GetDefaultObject());
		Rows.Add(Row);
		++MusketBlueprintCount;
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

TSharedRef<ITableRow> SActionBalanceWidget::GenerateRow(FRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SActionBalanceTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(SharedThis(this));
}

FText SActionBalanceWidget::GetStatsText() const
{
	return FText::Format(
		LOCTEXT("Stats", "Rows: {0} | MontageConfigDA: {1} | MontageAttackData: {2} | Musket BP: {3} | {4}"),
		FText::AsNumber(Rows.Num()),
		FText::AsNumber(MontageConfigCount),
		FText::AsNumber(AttackDataCount),
		FText::AsNumber(MusketBlueprintCount),
		StatusText);
}

FReply SActionBalanceWidget::OnRefreshClicked()
{
	RefreshData(LOCTEXT("Refreshed", "Action list refreshed."));
	return FReply::Handled();
}

FReply SActionBalanceWidget::OnValidateClicked()
{
	int32 Errors = 0;
	int32 Warnings = 0;
	for (const FRowPtr& Row : Rows)
	{
		if (!Row.IsValid())
		{
			continue;
		}
		if (Row->Type == EActionBalanceRowType::MeleeHitWindow)
		{
			UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Row->MontageEntry.Get());
			if (HitWindow && HitWindow->StartFrame > HitWindow->EndFrame)
			{
				++Errors;
				UE_LOG(LogTemp, Warning, TEXT("ActionBalance: invalid HitWindow on %s"), *PrimaryName(*Row));
			}
			if (HitWindow && HitWindow->AttackDataCandidates.IsEmpty())
			{
				++Warnings;
			}
		}
		if (Row->MusketCDO.IsValid() && !GetTuning(*Row))
		{
			++Warnings;
		}
	}

	StatusText = FText::Format(LOCTEXT("ValidateStatus", "Validate All: {0} errors, {1} warnings. Missing Musket TuningData is warning only."), FText::AsNumber(Errors), FText::AsNumber(Warnings));
	return FReply::Handled();
}

FReply SActionBalanceWidget::OnExportCsvClicked()
{
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Balance") / FString::Printf(TEXT("ActionNumbers_%s.csv"), *Stamp);
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FPaths::GetPath(Path));

	FString Csv = TEXT("Source,Type,A,B,C,D,Tuning\n");
	for (const FRowPtr& Row : Rows)
	{
		if (!Row.IsValid())
		{
			continue;
		}
		Csv += FString::Printf(TEXT("%s,%s,%.3f,%.3f,%.3f,%.3f,%s\n"),
			*PrimaryName(*Row),
			*RowTypeToString(Row->Type),
			ReadValue(*Row, TEXT("A")),
			ReadValue(*Row, TEXT("B")),
			ReadValue(*Row, TEXT("C")),
			ReadValue(*Row, TEXT("D")),
			*GetNameSafe(GetTuning(*Row)));
	}
	FFileHelper::SaveStringToFile(Csv, *Path, FFileHelper::EEncodingOptions::ForceUTF8);
	StatusText = FText::Format(LOCTEXT("Exported", "Exported {0}"), FText::FromString(FPaths::GetCleanFilename(Path)));
	return FReply::Handled();
}

void SActionBalanceWidget::OpenPrimaryAsset(TSharedPtr<FActionBalanceRow> Row) const
{
	if (!GEditor || !Row.IsValid())
	{
		return;
	}

	UObject* Object = Row->PrimaryObject.Get();
	if (UMusketActionTuningDataAsset* Tuning = GetTuning(*Row))
	{
		Object = Tuning;
	}

	if (Object)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(Object);
		}
	}
}

void SActionBalanceWidget::CommitValue(TSharedPtr<FActionBalanceRow> Row, FName ColumnName, float NewValue)
{
	if (!Row.IsValid())
	{
		return;
	}

	UObject* EditObject = GetEditObject(*Row);
	const FScopedTransaction Transaction(LOCTEXT("EditActionBalance", "Edit Action Balance Value"));
	if (EditObject)
	{
		EditObject->Modify();
	}
	if (Row->MontageConfig.IsValid())
	{
		Row->MontageConfig->Modify();
	}
	if (Row->BlueprintAsset.IsValid())
	{
		Row->BlueprintAsset->Modify();
	}

	if (WriteValue(*Row, ColumnName, NewValue))
	{
		StatusText = FText::Format(LOCTEXT("CommitStatus", "Set {0}.{1} = {2}. Dirty, not auto-saved."),
			FText::FromString(PrimaryName(*Row)), FText::FromName(ColumnName), FText::AsNumber(NewValue));
		if (ListView.IsValid())
		{
			ListView->RequestListRefresh();
		}
	}
}

void SActionBalanceWidget::CreateAttackDataForHitWindow(TSharedPtr<FActionBalanceRow> Row)
{
	if (!Row.IsValid())
	{
		return;
	}

	UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Row->MontageEntry.Get());
	UMontageConfigDA* Config = Row->MontageConfig.Get();
	if (!HitWindow || !Config)
	{
		return;
	}

	FString PackageName;
	FString AssetName;
	const FString BasePackageName = FString::Printf(TEXT("/Game/Docs/Data/Melee/AttackData/DA_%s_AttackData"), *Config->GetName());
	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateUniqueAssetName(BasePackageName, TEXT(""), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	UMontageAttackDataAsset* NewAsset = NewObject<UMontageAttackDataAsset>(Package, UMontageAttackDataAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	FAssetRegistryModule::AssetCreated(NewAsset);

	const FScopedTransaction Transaction(LOCTEXT("CreateAttackDataTx", "Create Montage Attack Data"));
	Config->Modify();
	HitWindow->Modify();
	FTaggedMontageAttackData Candidate;
	Candidate.AttackData = NewAsset;
	HitWindow->AttackDataCandidates.Add(Candidate);
	Config->MarkPackageDirty();
	NewAsset->MarkPackageDirty();

	StatusText = FText::Format(LOCTEXT("CreatedAttackData", "Created {0} and assigned it to {1}. Save assets when ready."),
		FText::FromString(AssetName), FText::FromString(Config->GetName()));
	RefreshData(StatusText);
}

void SActionBalanceWidget::CreateMusketTuningData(TSharedPtr<FActionBalanceRow> Row)
{
	if (!Row.IsValid() || !Row->MusketCDO.IsValid())
	{
		return;
	}

	FString SourceName = PrimaryName(*Row);
	FString PackageName;
	FString AssetName;
	const FString BasePackageName = FString::Printf(TEXT("/Game/Docs/Data/Musket/Tuning/DA_%s_Tuning"), *SourceName);
	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateUniqueAssetName(BasePackageName, TEXT(""), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	UMusketActionTuningDataAsset* NewAsset = NewObject<UMusketActionTuningDataAsset>(Package, UMusketActionTuningDataAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);

	if (UGA_Musket_LightAttack* Light = Cast<UGA_Musket_LightAttack>(Row->MusketCDO.Get()))
	{
		NewAsset->LightDamageMultiplier = Light->DamageMultiplier;
		NewAsset->LightHalfAngleDeg = Light->HalfAngleDeg;
	}
	if (UGA_Musket_HeavyAttack* Heavy = Cast<UGA_Musket_HeavyAttack>(Row->MusketCDO.Get()))
	{
		NewAsset->HeavyChargeTime = Heavy->ChargeTime;
		NewAsset->HeavyStartHalfAngle = Heavy->StartHalfAngle;
		NewAsset->HeavyEndHalfAngle = Heavy->EndHalfAngle;
		NewAsset->HeavyStartRadius = Heavy->StartRadius;
		NewAsset->HeavyEndRadius = Heavy->EndRadius;
		NewAsset->HeavyBaseDamageMultiplier = Heavy->BaseDamageMultiplier;
		NewAsset->HeavyFullChargeMultiplier = Heavy->FullChargeMultiplier;
	}
	if (UGA_Musket_SprintAttack* Sprint = Cast<UGA_Musket_SprintAttack>(Row->MusketCDO.Get()))
	{
		NewAsset->SprintDamageMultiplier = Sprint->DamageMultiplier;
		NewAsset->SprintHalfFanAngle = Sprint->HalfFanAngle;
	}

	FAssetRegistryModule::AssetCreated(NewAsset);

	const FScopedTransaction Transaction(LOCTEXT("CreateMusketTuningTx", "Create Musket Tuning Data"));
	Row->MusketCDO->Modify();
	Row->MusketCDO->TuningData = NewAsset;
	if (Row->BlueprintAsset.IsValid())
	{
		Row->BlueprintAsset->Modify();
		Row->BlueprintAsset->MarkPackageDirty();
	}
	NewAsset->MarkPackageDirty();

	StatusText = FText::Format(LOCTEXT("CreatedMusketTuning", "Created {0} and assigned it to {1}. Save assets when ready."),
		FText::FromString(AssetName), FText::FromString(SourceName));
	RefreshData(StatusText);
}

#undef LOCTEXT_NAMESPACE
