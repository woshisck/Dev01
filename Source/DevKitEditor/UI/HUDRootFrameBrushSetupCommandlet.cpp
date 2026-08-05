#include "UI/HUDRootFrameBrushSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Slate/SlateBrushAsset.h"
#include "Styling/SlateBrush.h"
#include "WidgetBlueprint.h"

namespace HUDRootFrameBrushSetup
{
	const FString HudRootPackagePath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_HUDRoot");
	const FString BrushAssetRootPath = TEXT("/Game/UI/Playtest_UI/HUD/FrameBrushes");
	const FString ReportFileName = TEXT("HUDRootFrameBrushSetupReport.md");

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString ExistingPackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &ExistingPackageFile);
	}

	UWidgetBlueprint* LoadWidgetBlueprint(const FString& PackagePath)
	{
		if (UWidgetBlueprint* Existing = FindObject<UWidgetBlueprint>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}

		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}

		return Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	bool IsFrameLikeImageWidget(const UWidget* Widget)
	{
		if (!Widget || !Widget->IsA<UImage>())
		{
			return false;
		}

		const FString WidgetName = Widget->GetName();
		return WidgetName.Contains(TEXT("Frame"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Border"), ESearchCase::IgnoreCase);
	}

	bool ShouldCreateFrameBrush(const UWidget* Widget)
	{
		return Widget && (Widget->IsA<UBorder>() || IsFrameLikeImageWidget(Widget));
	}

	void CollectWidgets(UWidget* Widget, TArray<UWidget*>& OutWidgets)
	{
		if (!Widget)
		{
			return;
		}

		OutWidgets.Add(Widget);

		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			const int32 ChildCount = Panel->GetChildrenCount();
			for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
			{
				CollectWidgets(Panel->GetChildAt(ChildIndex), OutWidgets);
			}
		}
	}

	FString MakeBrushAssetPackagePath(const UWidget& Widget)
	{
		FString SanitizedName = Widget.GetName();
		SanitizedName.ReplaceInline(TEXT(" "), TEXT("_"));
		return BrushAssetRootPath / FString::Printf(TEXT("DA_%s_Brush"), *SanitizedName);
	}

	USlateBrushAsset* CreateOrLoadBrushAsset(const FString& PackagePath, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (USlateBrushAsset* Existing = Cast<USlateBrushAsset>(StaticLoadObject(USlateBrushAsset::StaticClass(), nullptr, *ToObjectPath(PackagePath))))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		const FName AssetName(*FPackageName::GetLongPackageAssetName(PackagePath));
		USlateBrushAsset* BrushAsset = NewObject<USlateBrushAsset>(
			Package,
			USlateBrushAsset::StaticClass(),
			AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!BrushAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create `%s`."), *PackagePath));
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(BrushAsset);
		BrushAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *PackagePath));
		return BrushAsset;
	}

	bool SetBrushDrawAsBox(UWidget& Widget, FSlateBrush& Brush)
	{
		const bool bChanged = Brush.DrawAs != ESlateBrushDrawType::Box;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		if (Brush.ImageSize.IsNearlyZero())
		{
			Brush.ImageSize = FVector2D(64.0f, 64.0f);
		}
		if (Brush.Margin == FMargin())
		{
			Brush.Margin = FMargin(0.25f);
		}
		Widget.Modify();
		return bChanged;
	}

	bool ProcessFrameWidget(UWidget& Widget, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		FSlateBrush Brush;
		bool bHasBrush = false;

		if (UBorder* Border = Cast<UBorder>(&Widget))
		{
			Brush = Border->Background;
			SetBrushDrawAsBox(Widget, Brush);
			Border->SetBrush(Brush);
			bHasBrush = true;
		}
		else if (UImage* Image = Cast<UImage>(&Widget))
		{
			Brush = Image->GetBrush();
			SetBrushDrawAsBox(Widget, Brush);
			Image->SetBrush(Brush);
			bHasBrush = true;
		}

		if (!bHasBrush)
		{
			return false;
		}

		const FString BrushAssetPackagePath = MakeBrushAssetPackagePath(Widget);
		if (USlateBrushAsset* BrushAsset = CreateOrLoadBrushAsset(BrushAssetPackagePath, ReportLines, DirtyPackages))
		{
			BrushAsset->Modify();
			BrushAsset->Brush = Brush;
			BrushAsset->Brush.DrawAs = ESlateBrushDrawType::Box;
			BrushAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(BrushAsset->GetPackage());
		}

		ReportLines.Add(FString::Printf(
			TEXT("- `%s` `%s`: Brush.DrawAs set to Box; data asset `%s`."),
			*Widget.GetClass()->GetName(),
			*Widget.GetName(),
			*BrushAssetPackagePath));
		return true;
	}
}

UHUDRootFrameBrushSetupCommandlet::UHUDRootFrameBrushSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UHUDRootFrameBrushSetupCommandlet::Main(const FString& Params)
{
	using namespace HUDRootFrameBrushSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# HUD Root Frame Brush Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Widget: `%s`"), *HudRootPackagePath));
	ReportLines.Add(FString::Printf(TEXT("- Brush asset root: `%s`"), *BrushAssetRootPath));
	ReportLines.Add(TEXT("- Selection rule: every `Border` widget, plus `Image` widgets whose names contain `Frame` or `Border`."));
	ReportLines.Add(TEXT(""));

	UWidgetBlueprint* HudRootWidget = LoadWidgetBlueprint(HudRootPackagePath);
	if (!HudRootWidget || !HudRootWidget->WidgetTree || !HudRootWidget->WidgetTree->RootWidget)
	{
		ReportLines.Add(TEXT("- Failed to load HUD root Widget Blueprint or widget tree."));
		FString ReportPath;
		FString SharedReportPath;
		DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath);
		UE_LOG(LogTemp, Error, TEXT("HUD root frame brush setup failed. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
		return 1;
	}

	TArray<UWidget*> Widgets;
	CollectWidgets(HudRootWidget->WidgetTree->RootWidget, Widgets);

	int32 ProcessedCount = 0;
	for (UWidget* Widget : Widgets)
	{
		if (!ShouldCreateFrameBrush(Widget))
		{
			continue;
		}

		if (Widget->IsA<UBorder>() || Widget->IsA<UImage>())
		{
			if (bDryRun)
			{
				ReportLines.Add(FString::Printf(
					TEXT("- Would update `%s` `%s` and create/update `%s`."),
					*Widget->GetClass()->GetName(),
					*Widget->GetName(),
					*MakeBrushAssetPackagePath(*Widget)));
				++ProcessedCount;
			}
			else if (ProcessFrameWidget(*Widget, ReportLines, DirtyPackages))
			{
				++ProcessedCount;
			}
		}
	}

	ReportLines.Add(TEXT(""));
	ReportLines.Add(FString::Printf(TEXT("- Processed frame brush count: %d"), ProcessedCount));

	if (!bDryRun)
	{
		HudRootWidget->Modify();
		FKismetEditorUtilities::CompileBlueprint(HudRootWidget);
		HudRootWidget->MarkPackageDirty();
		DirtyPackages.AddUnique(HudRootWidget->GetPackage());

		if (DirtyPackages.Num() > 0)
		{
			UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
		}
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath);
	UE_LOG(LogTemp, Display, TEXT("HUD root frame brush setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
