#include "UI/FinisherQTEWidgetSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UI/FinisherQTEWidget.h"
#include "UI/YogHUD.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace FinisherQTEWidgetSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_FinisherQTEPrompt");
	const FString ReportFileName = TEXT("FinisherQTEWidgetSetupReport.md");

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

	template <typename TWidget>
	TWidget* ConstructNamedWidget(UWidgetTree* WidgetTree, const FName WidgetName, const bool bVariable = true)
	{
		TWidget* Widget = WidgetTree ? WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), WidgetName) : nullptr;
		if (Widget)
		{
			Widget->bIsVariable = bVariable;
		}
		return Widget;
	}

	void ConfigureText(UTextBlock* TextBlock, const FString& Text, const FLinearColor& Color, const int32 Size, const ETextJustify::Type Justification)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
		TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	}

	void BuildDesignerTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		WidgetBlueprint->Modify();
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		WidgetTree->Modify();

		if (WidgetTree->RootWidget)
		{
			WidgetTree->RemoveWidget(WidgetTree->RootWidget);
			WidgetTree->RootWidget = nullptr;
		}

		UCanvasPanel* RootCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("FinisherQTERoot"), false);
		USizeBox* PanelSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("PromptPanelSize"), false);
		UBorder* PromptPanel = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("PromptPanel"));
		UVerticalBox* PromptStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("PromptStack"), false);
		UHorizontalBox* PromptRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("PromptRow"), false);
		UBorder* KeyBack = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("KeyBack"), false);
		UTextBlock* KeyText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("KeyText"));
		UTextBlock* PromptText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("PromptText"));
		UProgressBar* WindowProgressBar = ConstructNamedWidget<UProgressBar>(WidgetTree, TEXT("WindowProgressBar"));

		if (!RootCanvas || !PanelSize || !PromptPanel || !PromptStack || !PromptRow || !KeyBack || !KeyText || !PromptText || !WindowProgressBar)
		{
			return;
		}

		WidgetTree->RootWidget = RootCanvas;
		RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);

		PanelSize->SetWidthOverride(280.f);
		PanelSize->SetHeightOverride(96.f);
		if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSize))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D(0.f, 170.f));
			PanelSlot->SetSize(FVector2D(280.f, 96.f));
			PanelSlot->SetAutoSize(false);
		}

		PromptPanel->SetBrushColor(FLinearColor(0.015f, 0.016f, 0.020f, 0.82f));
		PromptPanel->SetPadding(FMargin(12.f, 10.f));
		PanelSize->AddChild(PromptPanel);
		PromptPanel->SetContent(PromptStack);

		if (UVerticalBoxSlot* RowSlot = PromptStack->AddChildToVerticalBox(PromptRow))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		KeyBack->SetBrushColor(FLinearColor(0.92f, 0.74f, 0.25f, 0.95f));
		KeyBack->SetPadding(FMargin(12.f, 5.f));
		KeyBack->SetContent(KeyText);
		if (UHorizontalBoxSlot* KeySlot = PromptRow->AddChildToHorizontalBox(KeyBack))
		{
			KeySlot->SetVerticalAlignment(VAlign_Center);
			KeySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}

		ConfigureText(KeyText, TEXT("H"), FLinearColor(0.02f, 0.018f, 0.012f, 1.f), 26, ETextJustify::Center);
		ConfigureText(PromptText, TEXT("FINISHER"), FLinearColor(0.96f, 0.96f, 0.92f, 1.f), 19, ETextJustify::Left);
		if (UHorizontalBoxSlot* PromptSlot = PromptRow->AddChildToHorizontalBox(PromptText))
		{
			PromptSlot->SetVerticalAlignment(VAlign_Center);
			PromptSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		WindowProgressBar->SetPercent(1.f);
		WindowProgressBar->SetFillColorAndOpacity(FLinearColor(0.94f, 0.76f, 0.26f, 1.f));
		if (UVerticalBoxSlot* BarSlot = PromptStack->AddChildToVerticalBox(WindowProgressBar))
		{
			BarSlot->SetPadding(FMargin(0.f));
		}
	}

	UWidgetBlueprint* CreateWidgetBlueprint(bool bDryRun, TArray<FString>& ReportLines, bool& bCreated)
	{
		bCreated = false;

		if (UWidgetBlueprint* Existing = LoadWidgetBlueprint(WidgetPackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found existing `%s`."), *WidgetPackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- %s `%s` with parent `FinisherQTEWidget`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*WidgetPackagePath));

		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = UFinisherQTEWidget::StaticClass();

		UObject* NewAsset = AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(WidgetPackagePath),
			FPackageName::GetLongPackagePath(WidgetPackagePath),
			UWidgetBlueprint::StaticClass(),
			Factory);

		UWidgetBlueprint* NewWidgetBlueprint = Cast<UWidgetBlueprint>(NewAsset);
		if (NewWidgetBlueprint)
		{
			bCreated = true;
			FAssetRegistryModule::AssetCreated(NewWidgetBlueprint);
		}
		else
		{
			ReportLines.Add(TEXT("- Failed to create widget blueprint asset."));
		}

		return NewWidgetBlueprint;
	}

	void AssignHudDefaults(UWidgetBlueprint* WidgetBlueprint, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		const TArray<FString> CandidateHudBlueprints = {
			TEXT("/Game/UI/B_HUD_Intro"),
			TEXT("/Game/UI/BP_YogHUD"),
			TEXT("/Game/UI/Playtest_UI/HUD/BP_YogHUD")
		};

		ReportLines.Add(TEXT("## HUD blueprint defaults"));
		for (const FString& CandidatePath : CandidateHudBlueprints)
		{
			UBlueprint* HudBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *ToObjectPath(CandidatePath)));
			if (!HudBlueprint || !HudBlueprint->GeneratedClass || !HudBlueprint->GeneratedClass->IsChildOf(AYogHUD::StaticClass()))
			{
				continue;
			}

			ReportLines.Add(FString::Printf(TEXT("- %s FinisherQTEWidgetClass on `%s`."), bDryRun ? TEXT("Would update") : TEXT("Updated"), *CandidatePath));
			if (bDryRun)
			{
				return;
			}

			AYogHUD* HudCDO = Cast<AYogHUD>(HudBlueprint->GeneratedClass->GetDefaultObject());
			if (!HudCDO)
			{
				continue;
			}

			HudCDO->Modify();
			if (WidgetBlueprint && WidgetBlueprint->GeneratedClass)
			{
				HudCDO->FinisherQTEWidgetClass = TSubclassOf<UFinisherQTEWidget>(WidgetBlueprint->GeneratedClass);
			}
			HudBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(HudBlueprint->GetPackage());
			return;
		}

		ReportLines.Add(TEXT("- No AYogHUD blueprint found to update."));
	}
}

UFinisherQTEWidgetSetupCommandlet::UFinisherQTEWidgetSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UFinisherQTEWidgetSetupCommandlet::Main(const FString& Params)
{
	using namespace FinisherQTEWidgetSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Finisher QTE Widget Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Target WBP: `%s`"), *WidgetPackagePath));
	ReportLines.Add(TEXT(""));

	bool bCreated = false;
	UWidgetBlueprint* WidgetBlueprint = CreateWidgetBlueprint(bDryRun, ReportLines, bCreated);
	if (WidgetBlueprint && !bDryRun)
	{
		const bool bMissingDesignerBindings = !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("PromptPanel"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("KeyText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("PromptText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("WindowProgressBar"));

		if (bCreated || bForceLayout || bMissingDesignerBindings)
		{
			BuildDesignerTree(WidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
			ReportLines.Add(TEXT("- Designer tree refreshed: centered prompt panel with heavy key label and QTE progress bar."));
		}
		else
		{
			ReportLines.Add(TEXT("- Existing designer tree kept unchanged."));
		}
	}

	AssignHudDefaults(WidgetBlueprint, bDryRun, ReportLines, DirtyPackages);

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(FinisherQTEWidgetSetup::ReportFileName, ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Finisher QTE widget setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
