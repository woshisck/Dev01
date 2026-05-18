#include "UI/SlotSelectWidgetSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UI/YogSlotSelectWidgetBase.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace SlotSelectWidgetSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Frontend/WBP_SlotSelectWidget");
	const FString ReportFileName = TEXT("SlotSelectWidgetSetupReport.md");

	const FLinearColor PageBgColor(0.008f, 0.010f, 0.014f, 0.92f);
	const FLinearColor CardFillColor(0.028f, 0.032f, 0.040f, 0.92f);
	const FLinearColor CardBorderColor(0.60f, 0.58f, 0.48f, 0.82f);
	const FLinearColor MainTextColor(0.92f, 0.91f, 0.86f, 1.f);
	const FLinearColor MutedTextColor(0.68f, 0.70f, 0.72f, 1.f);

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
	TWidget* ConstructNamedWidget(UWidgetTree* WidgetTree, const FName WidgetName, bool bVariable = true)
	{
		TWidget* Widget = WidgetTree ? WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), WidgetName) : nullptr;
		if (Widget)
		{
			Widget->bIsVariable = bVariable;
			Widget->SetClipping(EWidgetClipping::ClipToBounds);
		}
		return Widget;
	}

	void ConfigureText(UTextBlock* TextBlock, const FString& Text, const FLinearColor& Color, int32 Size, ETextJustify::Type Justification = ETextJustify::Left, bool bWrap = false)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);
		TextBlock->SetAutoWrapText(bWrap);
		TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
	}

	void ConfigureButton(UButton* Button, const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		Style.Normal.TintColor = FSlateColor(Normal);
		Style.Hovered.TintColor = FSlateColor(Hovered);
		Style.Pressed.TintColor = FSlateColor(Pressed);
		Style.Disabled.TintColor = FSlateColor(FLinearColor(0.05f, 0.05f, 0.055f, 0.32f));
		Button->SetStyle(Style);
	}

	UTextBlock* MakeButtonLabel(UWidgetTree* WidgetTree, const FName LabelName, const FString& Text)
	{
		UTextBlock* Label = ConstructNamedWidget<UTextBlock>(WidgetTree, LabelName, false);
		ConfigureText(Label, Text, MainTextColor, 17, ETextJustify::Center, false);
		return Label;
	}

	UButton* AddButton(UWidgetTree* WidgetTree, UVerticalBox* ButtonList, const FName ButtonName, const FString& LabelText, const FLinearColor& AccentColor)
	{
		UButton* Button = ConstructNamedWidget<UButton>(WidgetTree, ButtonName);
		if (!Button || !ButtonList)
		{
			return Button;
		}

		ConfigureButton(
			Button,
			FLinearColor(0.04f, 0.045f, 0.055f, 0.84f),
			AccentColor,
			FLinearColor(AccentColor.R * 0.82f, AccentColor.G * 0.82f, AccentColor.B * 0.82f, 1.f));

		Button->SetContent(MakeButtonLabel(WidgetTree, FName(*(ButtonName.ToString() + TEXT("_Label"))), LabelText));
		if (UVerticalBoxSlot* ButtonSlot = ButtonList->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.f, 4.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		return Button;
	}

	void ResetWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		WidgetBlueprint->Modify();
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		WidgetTree->Modify();

		TArray<UObject*> ExistingObjects;
		GetObjectsWithOuter(WidgetTree, ExistingObjects, true);
		for (UObject* ExistingObject : ExistingObjects)
		{
			if (UWidget* ExistingWidget = Cast<UWidget>(ExistingObject))
			{
				WidgetTree->RemoveWidget(ExistingWidget);
				ExistingWidget->Rename(
					*FString::Printf(TEXT("SlotSelect_Old_%s_%s"), *ExistingWidget->GetName(), *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
					GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}

		WidgetTree->RootWidget = nullptr;
	}

	void AddSlotCard(UWidgetTree* WidgetTree, UHorizontalBox* CardRow, int32 SlotIndex)
	{
		const FString Suffix = FString::FromInt(SlotIndex);

		USizeBox* CardSize = ConstructNamedWidget<USizeBox>(WidgetTree, FName(*FString::Printf(TEXT("SlotCardSize_%s"), *Suffix)), false);
		UBorder* SlotCard = ConstructNamedWidget<UBorder>(WidgetTree, FName(*FString::Printf(TEXT("SlotCard_%s"), *Suffix)));
		UVerticalBox* CardStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, FName(*FString::Printf(TEXT("SlotCardStack_%s"), *Suffix)), false);
		UTextBlock* TitleText = ConstructNamedWidget<UTextBlock>(WidgetTree, FName(*FString::Printf(TEXT("SlotTitleText_%s"), *Suffix)));
		UTextBlock* PreviewText = ConstructNamedWidget<UTextBlock>(WidgetTree, FName(*FString::Printf(TEXT("SlotPreviewText_%s"), *Suffix)));
		UVerticalBox* ButtonList = ConstructNamedWidget<UVerticalBox>(WidgetTree, FName(*FString::Printf(TEXT("SlotButtonList_%s"), *Suffix)), false);

		if (!CardSize || !SlotCard || !CardStack || !TitleText || !PreviewText || !ButtonList || !CardRow)
		{
			return;
		}

		CardSize->SetWidthOverride(330.f);
		CardSize->SetHeightOverride(430.f);
		CardSize->AddChild(SlotCard);

		SlotCard->SetBrush(FSlateRoundedBoxBrush(CardFillColor, 6.f, CardBorderColor, 1.5f));
		SlotCard->SetPadding(FMargin(18.f, 18.f, 18.f, 16.f));
		SlotCard->SetContent(CardStack);

		ConfigureText(TitleText, FString::Printf(TEXT("Slot %d"), SlotIndex + 1), MainTextColor, 26, ETextJustify::Center, false);
		if (UVerticalBoxSlot* TitleSlot = CardStack->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		ConfigureText(PreviewText, TEXT("Loading..."), MutedTextColor, 18, ETextJustify::Left, true);
		if (UVerticalBoxSlot* PreviewSlot = CardStack->AddChildToVerticalBox(PreviewText))
		{
			PreviewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			PreviewSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
			PreviewSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		if (UVerticalBoxSlot* ButtonListSlot = CardStack->AddChildToVerticalBox(ButtonList))
		{
			ButtonListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			ButtonListSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		AddButton(
			WidgetTree,
			ButtonList,
			FName(*FString::Printf(TEXT("BtnContinue_%s"), *Suffix)),
			TEXT("Continue"),
			FLinearColor(0.16f, 0.34f, 0.58f, 1.f));
		AddButton(
			WidgetTree,
			ButtonList,
			FName(*FString::Printf(TEXT("BtnNewGame_%s"), *Suffix)),
			TEXT("New Game"),
			FLinearColor(0.22f, 0.42f, 0.28f, 1.f));
		AddButton(
			WidgetTree,
			ButtonList,
			FName(*FString::Printf(TEXT("BtnDelete_%s"), *Suffix)),
			TEXT("Delete"),
			FLinearColor(0.48f, 0.18f, 0.16f, 1.f));

		if (UHorizontalBoxSlot* CardSlot = CardRow->AddChildToHorizontalBox(CardSize))
		{
			CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			CardSlot->SetPadding(FMargin(10.f, 0.f));
			CardSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	void BuildDesignerTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UCanvasPanel* RootCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("RootCanvas"), false);
		UBorder* Background = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("Background"), false);
		USizeBox* RootSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSize"), false);
		UVerticalBox* RootStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("RootStack"), false);
		UTextBlock* HeaderText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("HeaderText"), false);
		UTextBlock* HintText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("HintText"), false);
		UHorizontalBox* CardRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("CardRow"), false);

		if (!RootCanvas || !Background || !RootSize || !RootStack || !HeaderText || !HintText || !CardRow)
		{
			return;
		}

		WidgetTree->RootWidget = RootCanvas;

		Background->SetBrushColor(PageBgColor);
		if (UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(Background))
		{
			BgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			BgSlot->SetOffsets(FMargin(0.f));
		}

		RootSize->SetWidthOverride(1120.f);
		RootSize->SetHeightOverride(620.f);
		RootSize->AddChild(RootStack);
		if (UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(RootSize))
		{
			RootSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			RootSlot->SetPosition(FVector2D::ZeroVector);
			RootSlot->SetSize(FVector2D(1120.f, 620.f));
		}

		ConfigureText(HeaderText, TEXT("Select Save Slot"), MainTextColor, 38, ETextJustify::Center, false);
		if (UVerticalBoxSlot* HeaderSlot = RootStack->AddChildToVerticalBox(HeaderText))
		{
			HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
			HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		ConfigureText(HintText, TEXT("Continue resumes a saved checkpoint. New Game resets only the selected slot progression."), MutedTextColor, 18, ETextJustify::Center, true);
		if (UVerticalBoxSlot* HintSlot = RootStack->AddChildToVerticalBox(HintText))
		{
			HintSlot->SetPadding(FMargin(120.f, 0.f, 120.f, 22.f));
			HintSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		if (UVerticalBoxSlot* CardRowSlot = RootStack->AddChildToVerticalBox(CardRow))
		{
			CardRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			CardRowSlot->SetHorizontalAlignment(HAlign_Center);
			CardRowSlot->SetVerticalAlignment(VAlign_Fill);
		}

		AddSlotCard(WidgetTree, CardRow, 0);
		AddSlotCard(WidgetTree, CardRow, 1);
		AddSlotCard(WidgetTree, CardRow, 2);
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
			TEXT("- %s `%s` with parent `UYogSlotSelectWidgetBase`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*WidgetPackagePath));

		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = UYogSlotSelectWidgetBase::StaticClass();

		UObject* NewAsset = AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(WidgetPackagePath),
			FPackageName::GetLongPackagePath(WidgetPackagePath),
			UWidgetBlueprint::StaticClass(),
			Factory);

		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewAsset);
		if (WidgetBlueprint)
		{
			bCreated = true;
			FAssetRegistryModule::AssetCreated(WidgetBlueprint);
		}
		else
		{
			ReportLines.Add(TEXT("- Failed to create widget blueprint asset."));
		}

		return WidgetBlueprint;
	}

	bool NeedsRefresh(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return true;
		}

		const TCHAR* RequiredWidgets[] =
		{
			TEXT("SlotCard_0"), TEXT("SlotTitleText_0"), TEXT("SlotPreviewText_0"), TEXT("BtnContinue_0"), TEXT("BtnNewGame_0"), TEXT("BtnDelete_0"),
			TEXT("SlotCard_1"), TEXT("SlotTitleText_1"), TEXT("SlotPreviewText_1"), TEXT("BtnContinue_1"), TEXT("BtnNewGame_1"), TEXT("BtnDelete_1"),
			TEXT("SlotCard_2"), TEXT("SlotTitleText_2"), TEXT("SlotPreviewText_2"), TEXT("BtnContinue_2"), TEXT("BtnNewGame_2"), TEXT("BtnDelete_2")
		};

		for (const TCHAR* WidgetName : RequiredWidgets)
		{
			if (!WidgetBlueprint->WidgetTree->FindWidget(WidgetName))
			{
				return true;
			}
		}

		return false;
	}
}

USlotSelectWidgetSetupCommandlet::USlotSelectWidgetSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 USlotSelectWidgetSetupCommandlet::Main(const FString& Params)
{
	using namespace SlotSelectWidgetSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Slot Select Widget Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- ForceLayout: %s"), bForceLayout ? TEXT("true") : TEXT("false")));
	ReportLines.Add(FString::Printf(TEXT("- Target WBP: `%s`"), *WidgetPackagePath));
	ReportLines.Add(TEXT(""));

	bool bCreated = false;
	UWidgetBlueprint* WidgetBlueprint = CreateWidgetBlueprint(bDryRun, ReportLines, bCreated);
	if (WidgetBlueprint && !bDryRun)
	{
		if (bCreated || bForceLayout || NeedsRefresh(WidgetBlueprint))
		{
			BuildDesignerTree(WidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
			ReportLines.Add(TEXT("- Designer tree refreshed with 3 slot cards, 18 bindings, and 9 action buttons."));
		}
		else
		{
			ReportLines.Add(TEXT("- Existing slot select designer tree kept unchanged."));
		}
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Slot select widget setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
