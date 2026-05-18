#include "UI/MetaProgressionWidgetTreeSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "WidgetBlueprint.h"

namespace MetaProgressionWidgetTreeSetup
{
	const FString TreeWidgetPath = TEXT("/Game/UI/MetaProgression/WBP_MetaUpgradeTree");
	const FString CardWidgetPath = TEXT("/Game/UI/MetaProgression/WBP_MetaNodeCard");
	const FString SummaryWidgetPath = TEXT("/Game/UI/MetaProgression/WBP_RunSummary");

	const FLinearColor PageBg(0.012f, 0.014f, 0.018f, 0.94f);
	const FLinearColor PanelBg(0.030f, 0.034f, 0.044f, 0.96f);
	const FLinearColor PanelBorder(0.48f, 0.52f, 0.62f, 0.72f);
	const FLinearColor MainText(0.92f, 0.92f, 0.88f, 1.f);
	const FLinearColor MutedText(0.66f, 0.69f, 0.72f, 1.f);
	const FLinearColor AccentFlesh(0.48f, 0.18f, 0.16f, 1.f);
	const FLinearColor AccentMystic(0.22f, 0.25f, 0.58f, 1.f);
	const FLinearColor AccentAction(0.20f, 0.42f, 0.30f, 1.f);

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString PackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &PackageFile);
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
	TWidget* ConstructNamedWidget(UWidgetTree* WidgetTree, FName WidgetName, bool bVariable = true)
	{
		TWidget* Widget = WidgetTree ? WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), WidgetName) : nullptr;
		if (Widget)
		{
			Widget->bIsVariable = bVariable;
			Widget->SetClipping(EWidgetClipping::ClipToBounds);
		}
		return Widget;
	}

	void ConfigureText(UTextBlock* TextBlock, const FString& Text, int32 Size, const FLinearColor& Color = MainText, ETextJustify::Type Justification = ETextJustify::Left)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);
		TextBlock->SetAutoWrapText(true);

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
	}

	UTextBlock* MakeButtonLabel(UWidgetTree* WidgetTree, const FName LabelName, const FString& Text)
	{
		UTextBlock* Label = ConstructNamedWidget<UTextBlock>(WidgetTree, LabelName, false);
		ConfigureText(Label, Text, 16, MainText, ETextJustify::Center);
		return Label;
	}

	void ConfigureButton(UButton* Button, const FLinearColor& Accent)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		Style.Normal.TintColor = FSlateColor(FLinearColor(0.055f, 0.060f, 0.072f, 0.96f));
		Style.Hovered.TintColor = FSlateColor(Accent);
		Style.Pressed.TintColor = FSlateColor(FLinearColor(Accent.R * 0.82f, Accent.G * 0.82f, Accent.B * 0.82f, 1.f));
		Style.Disabled.TintColor = FSlateColor(FLinearColor(0.04f, 0.04f, 0.045f, 0.55f));
		Button->SetStyle(Style);
	}

	UButton* MakeButton(UWidgetTree* WidgetTree, const FName ButtonName, const FString& LabelText, const FLinearColor& Accent)
	{
		UButton* Button = ConstructNamedWidget<UButton>(WidgetTree, ButtonName);
		if (Button)
		{
			ConfigureButton(Button, Accent);
			Button->SetContent(MakeButtonLabel(WidgetTree, FName(*(ButtonName.ToString() + TEXT("_Label"))), LabelText));
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
					*FString::Printf(TEXT("MetaProgression_Old_%s_%s"), *ExistingWidget->GetName(), *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
					GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}

		WidgetTree->RootWidget = nullptr;
	}

	UBorder* MakePanel(UWidgetTree* WidgetTree, FName Name, const FMargin& Padding)
	{
		UBorder* Panel = ConstructNamedWidget<UBorder>(WidgetTree, Name, false);
		if (Panel)
		{
			Panel->SetBrush(FSlateRoundedBoxBrush(PanelBg, 6.f, PanelBorder, 1.2f));
			Panel->SetPadding(Padding);
		}
		return Panel;
	}

	void SetCanvasFill(UCanvasPanelSlot* Slot)
	{
		if (!Slot)
		{
			return;
		}
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f));
	}

	bool BuildUpgradeTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UCanvasPanel* RootCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("RootCanvas"), false);
		UBorder* Background = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("Background"), false);
		USizeBox* RootSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSize"), false);
		UBorder* Panel = MakePanel(WidgetTree, TEXT("UpgradePanel"), FMargin(20.f));
		UVerticalBox* RootStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("RootStack"), false);
		UHorizontalBox* TopBar = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("TopBar"), false);
		UTextBlock* Title = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtTitle"), false);
		UTextBlock* CurrencyLabel = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtCurrencyLabel"), false);
		UTextBlock* CurrencyAmount = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtCurrencyAmount"));
		UButton* FleshButton = MakeButton(WidgetTree, TEXT("BtnFleshSide"), TEXT("Flesh"), AccentFlesh);
		UButton* MysticButton = MakeButton(WidgetTree, TEXT("BtnMysticSide"), TEXT("Mystic"), AccentMystic);
		UButton* CloseButton = MakeButton(WidgetTree, TEXT("BtnClose"), TEXT("Close"), FLinearColor(0.32f, 0.32f, 0.36f, 1.f));
		UScrollBox* NodeList = ConstructNamedWidget<UScrollBox>(WidgetTree, TEXT("NodeList"));

		if (!RootCanvas || !Background || !RootSize || !Panel || !RootStack || !TopBar || !Title || !CurrencyLabel || !CurrencyAmount || !FleshButton || !MysticButton || !CloseButton || !NodeList)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;
		Background->SetBrushColor(PageBg);
		SetCanvasFill(RootCanvas->AddChildToCanvas(Background));

		RootSize->SetWidthOverride(900.f);
		RootSize->SetHeightOverride(720.f);
		RootSize->AddChild(Panel);
		Panel->SetContent(RootStack);
		if (UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(RootSize))
		{
			RootSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			RootSlot->SetPosition(FVector2D::ZeroVector);
			RootSlot->SetSize(FVector2D(900.f, 720.f));
		}

		ConfigureText(Title, TEXT("Meta Upgrade Tree"), 30, MainText, ETextJustify::Left);
		ConfigureText(CurrencyLabel, TEXT("Currency"), 15, MutedText, ETextJustify::Left);
		ConfigureText(CurrencyAmount, TEXT("0"), 22, MainText, ETextJustify::Left);

		if (UVerticalBoxSlot* TitleSlot = RootStack->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		}
		if (UVerticalBoxSlot* TopBarSlot = RootStack->AddChildToVerticalBox(TopBar))
		{
			TopBarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
		}

		auto AddTopBarChild = [TopBar](UWidget* Widget, float Fill, const FMargin& Padding)
		{
			if (UHorizontalBoxSlot* Slot = TopBar->AddChildToHorizontalBox(Widget))
			{
				Slot->SetSize(FSlateChildSize(Fill > 0.f ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
				Slot->SetPadding(Padding);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddTopBarChild(CurrencyLabel, 0.f, FMargin(0.f, 0.f, 8.f, 0.f));
		AddTopBarChild(CurrencyAmount, 1.f, FMargin(0.f, 0.f, 16.f, 0.f));
		AddTopBarChild(FleshButton, 0.f, FMargin(0.f, 0.f, 8.f, 0.f));
		AddTopBarChild(MysticButton, 0.f, FMargin(0.f, 0.f, 8.f, 0.f));
		AddTopBarChild(CloseButton, 0.f, FMargin(0.f));

		if (UVerticalBoxSlot* ListSlot = RootStack->AddChildToVerticalBox(NodeList))
		{
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		return true;
	}

	bool BuildNodeCard(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UBorder* Card = MakePanel(WidgetTree, TEXT("CardRoot"), FMargin(14.f));
		UVerticalBox* Stack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("CardStack"), false);
		UTextBlock* NodeName = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtNodeName"));
		UTextBlock* LevelProgress = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtLevelProgress"));
		UProgressBar* Progress = ConstructNamedWidget<UProgressBar>(WidgetTree, TEXT("ProgressLevel"));
		UTextBlock* Cost = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtCost"));
		UButton* Purchase = MakeButton(WidgetTree, TEXT("BtnPurchase"), TEXT("Purchase"), AccentAction);

		if (!Card || !Stack || !NodeName || !LevelProgress || !Progress || !Cost || !Purchase)
		{
			return false;
		}

		WidgetTree->RootWidget = Card;
		Card->SetContent(Stack);
		ConfigureText(NodeName, TEXT("Node Name"), 20, MainText, ETextJustify::Left);
		ConfigureText(LevelProgress, TEXT("0/3"), 14, MutedText, ETextJustify::Left);
		ConfigureText(Cost, TEXT("Cost: 10"), 15, MutedText, ETextJustify::Left);
		Progress->SetPercent(0.f);

		for (UWidget* Widget : { static_cast<UWidget*>(NodeName), static_cast<UWidget*>(LevelProgress), static_cast<UWidget*>(Progress), static_cast<UWidget*>(Cost), static_cast<UWidget*>(Purchase) })
		{
			if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Widget))
			{
				Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			}
		}

		return true;
	}

	bool BuildRunSummary(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UCanvasPanel* RootCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("RootCanvas"), false);
		UBorder* Background = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("Background"), false);
		USizeBox* RootSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSize"), false);
		UBorder* Panel = MakePanel(WidgetTree, TEXT("SummaryPanel"), FMargin(24.f));
		UVerticalBox* Stack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("SummaryStack"), false);
		UTextBlock* Title = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtSummaryTitle"), false);
		UTextBlock* FloorLabel = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtFloorLabel"), false);
		UTextBlock* FloorReached = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtFloorReached"));
		UTextBlock* KillLabel = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtKillLabel"), false);
		UTextBlock* EnemiesKilled = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TxtEnemiesKilled"));
		UButton* ReturnButton = MakeButton(WidgetTree, TEXT("BtnReturnToHub"), TEXT("Return to Hub"), AccentAction);

		if (!RootCanvas || !Background || !RootSize || !Panel || !Stack || !Title || !FloorLabel || !FloorReached || !KillLabel || !EnemiesKilled || !ReturnButton)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;
		Background->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.58f));
		SetCanvasFill(RootCanvas->AddChildToCanvas(Background));

		RootSize->SetWidthOverride(520.f);
		RootSize->SetHeightOverride(360.f);
		RootSize->AddChild(Panel);
		Panel->SetContent(Stack);
		if (UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(RootSize))
		{
			RootSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			RootSlot->SetPosition(FVector2D::ZeroVector);
			RootSlot->SetSize(FVector2D(520.f, 360.f));
		}

		ConfigureText(Title, TEXT("Run Summary"), 30, MainText, ETextJustify::Center);
		ConfigureText(FloorLabel, TEXT("Floor Reached"), 15, MutedText, ETextJustify::Center);
		ConfigureText(FloorReached, TEXT("0"), 34, MainText, ETextJustify::Center);
		ConfigureText(KillLabel, TEXT("Enemies Killed"), 15, MutedText, ETextJustify::Center);
		ConfigureText(EnemiesKilled, TEXT("0"), 34, MainText, ETextJustify::Center);

		for (UWidget* Widget : { static_cast<UWidget*>(Title), static_cast<UWidget*>(FloorLabel), static_cast<UWidget*>(FloorReached), static_cast<UWidget*>(KillLabel), static_cast<UWidget*>(EnemiesKilled), static_cast<UWidget*>(ReturnButton) })
		{
			if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Widget))
			{
				Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
				Slot->SetHorizontalAlignment(HAlign_Fill);
			}
		}

		return true;
	}

	bool HasRequiredWidgets(UWidgetBlueprint* WidgetBlueprint, const TArray<FName>& RequiredNames)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}
		for (const FName& Name : RequiredNames)
		{
			if (!WidgetBlueprint->WidgetTree->FindWidget(Name))
			{
				return false;
			}
		}
		return true;
	}
}

UMetaProgressionWidgetTreeSetupCommandlet::UMetaProgressionWidgetTreeSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMetaProgressionWidgetTreeSetupCommandlet::Main(const FString& Params)
{
	using namespace MetaProgressionWidgetTreeSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);
	TArray<UPackage*> DirtyPackages;

	struct FTarget
	{
		FString PackagePath;
		TArray<FName> RequiredNames;
		TFunction<bool(UWidgetBlueprint*)> Builder;
	};

	const TArray<FTarget> Targets =
	{
		{
			TreeWidgetPath,
			{ TEXT("TxtCurrencyAmount"), TEXT("BtnFleshSide"), TEXT("BtnMysticSide"), TEXT("BtnClose"), TEXT("NodeList") },
			[](UWidgetBlueprint* WidgetBlueprint) { return BuildUpgradeTree(WidgetBlueprint); }
		},
		{
			CardWidgetPath,
			{ TEXT("TxtNodeName"), TEXT("TxtLevelProgress"), TEXT("TxtCost"), TEXT("ProgressLevel"), TEXT("BtnPurchase") },
			[](UWidgetBlueprint* WidgetBlueprint) { return BuildNodeCard(WidgetBlueprint); }
		},
		{
			SummaryWidgetPath,
			{ TEXT("TxtFloorReached"), TEXT("TxtEnemiesKilled"), TEXT("BtnReturnToHub") },
			[](UWidgetBlueprint* WidgetBlueprint) { return BuildRunSummary(WidgetBlueprint); }
		}
	};

	for (const FTarget& Target : Targets)
	{
		UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(Target.PackagePath);
		if (!WidgetBlueprint)
		{
			UE_LOG(LogTemp, Error, TEXT("[MetaProgressionWidgetTreeSetup] Missing widget blueprint: %s"), *Target.PackagePath);
			return 1;
		}

		const bool bNeedsRefresh = bForceLayout || !HasRequiredWidgets(WidgetBlueprint, Target.RequiredNames);
		UE_LOG(LogTemp, Display, TEXT("[MetaProgressionWidgetTreeSetup] %s needs refresh: %s"), *Target.PackagePath, bNeedsRefresh ? TEXT("true") : TEXT("false"));
		if (!bApply || !bNeedsRefresh)
		{
			continue;
		}

		if (!Target.Builder(WidgetBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("[MetaProgressionWidgetTreeSetup] Failed to build widget tree for %s"), *Target.PackagePath);
			return 1;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
		FAssetRegistryModule::AssetCreated(WidgetBlueprint);
	}

	if (bApply && !DirtyPackages.IsEmpty())
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	return 0;
}
