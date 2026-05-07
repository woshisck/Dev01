#include "UI/CurrentRoomBuffWidgetSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UI/CurrentRoomBuffWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace CurrentRoomBuffWBPSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_CurrentRoomBuffPanel");
	const FString ReportFileName = TEXT("CurrentRoomBuffWidgetSetupReport.md");

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
	TWidget* ConstructNamedWidget(UWidgetTree* WidgetTree, const FName WidgetName)
	{
		TWidget* Widget = WidgetTree ? WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), WidgetName) : nullptr;
		if (Widget)
		{
			Widget->bIsVariable = true;
		}
		return Widget;
	}

	void ConfigureText(UTextBlock* TextBlock, const FString& Text, const FLinearColor& Color, const int32 Size, const bool bWrap)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetAutoWrapText(bWrap);

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
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

		UCanvasPanel* RootCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("RootCanvas"));
		USizeBox* PanelSizeBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("PanelSizeBox"));
		UBorder* OuterBorder = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("OuterBorder"));
		UVerticalBox* PanelStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("PanelStack"));
		UTextBlock* TitleText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TitleText"));
		UTextBlock* RoomNameText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("RoomNameText"));
		UVerticalBox* BuffListBox = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("BuffListBox"));
		UTextBlock* EmptyText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("EmptyText"));

		if (!RootCanvas || !PanelSizeBox || !OuterBorder || !PanelStack || !TitleText || !RoomNameText || !BuffListBox || !EmptyText)
		{
			return;
		}

		WidgetTree->RootWidget = RootCanvas;

		PanelSizeBox->SetWidthOverride(340.f);
		if (UCanvasPanelSlot* SizeSlot = RootCanvas->AddChildToCanvas(PanelSizeBox))
		{
			SizeSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
			SizeSlot->SetAlignment(FVector2D(0.f, 0.f));
			SizeSlot->SetPosition(FVector2D(24.f, 96.f));
			SizeSlot->SetAutoSize(true);
		}

		OuterBorder->SetBrushColor(FLinearColor(0.03f, 0.035f, 0.045f, 0.82f));
		OuterBorder->SetPadding(FMargin(12.f, 10.f));
		PanelSizeBox->AddChild(OuterBorder);
		OuterBorder->SetContent(PanelStack);

		ConfigureText(TitleText, TEXT("Current Enemy Runes"), FLinearColor(0.95f, 0.78f, 0.38f, 1.f), 18, false);
		ConfigureText(RoomNameText, TEXT("Room"), FLinearColor(0.72f, 0.78f, 0.86f, 1.f), 13, true);
		ConfigureText(EmptyText, TEXT("No enemy rune in this room"), FLinearColor(0.62f, 0.66f, 0.72f, 1.f), 12, true);

		if (UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
		if (UVerticalBoxSlot* RoomSlot = PanelStack->AddChildToVerticalBox(RoomNameText))
		{
			RoomSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}
		if (UVerticalBoxSlot* ListSlot = PanelStack->AddChildToVerticalBox(BuffListBox))
		{
			ListSlot->SetPadding(FMargin(0.f));
		}
		if (UVerticalBoxSlot* EmptySlot = PanelStack->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
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
			TEXT("- %s `%s` with parent `CurrentRoomBuffWidget`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*WidgetPackagePath));

		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = UCurrentRoomBuffWidget::StaticClass();

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
}

UCurrentRoomBuffWidgetSetupCommandlet::UCurrentRoomBuffWidgetSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UCurrentRoomBuffWidgetSetupCommandlet::Main(const FString& Params)
{
	using namespace CurrentRoomBuffWBPSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Current Room Buff Widget Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Target WBP: `%s`"), *WidgetPackagePath));
	ReportLines.Add(TEXT(""));

	bool bCreated = false;
	UWidgetBlueprint* WidgetBlueprint = CreateWidgetBlueprint(bDryRun, ReportLines, bCreated);
	if (WidgetBlueprint && !bDryRun)
	{
		const bool bMissingDesignerBindings = !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BuffListBox"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("TitleText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("RoomNameText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("EmptyText"));

		if (bCreated || bForceLayout || bMissingDesignerBindings)
		{
			BuildDesignerTree(WidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
			ReportLines.Add(TEXT("- Designer tree refreshed: RootCanvas -> PanelSizeBox -> OuterBorder -> PanelStack -> TitleText/RoomNameText/BuffListBox/EmptyText."));
		}
		else
		{
			ReportLines.Add(TEXT("- Existing designer tree kept unchanged."));
		}
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), CurrentRoomBuffWBPSetup::ReportFileName);
	FFileHelper::SaveStringToFile(
		FString::Join(ReportLines, LINE_TERMINATOR),
		*ReportPath,
		FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Display, TEXT("Current room buff widget setup finished. Report: %s"), *ReportPath);
	return 0;
}
