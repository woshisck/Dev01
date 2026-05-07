#include "UI/PortalPreviewWidgetSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "CommonTextBlock.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/RichTextBlockDecorator.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UI/InputActionRichTextDecorator.h"
#include "UI/PortalPreviewWidget.h"
#include "UI/YogCommonRichTextBlock.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace PortalPreviewWBPSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Playtest_UI/Portal/WBP_PortalPreview");
	const FString ReportFileName = TEXT("PortalPreviewWidgetSetupReport.md");
	const FString InputActionDecoratorClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InputActionDecorator.BP_InputActionDecorator_C");
	const FString InfoPopupTextStyleClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InfoPopupTextStyle.BP_InfoPopupTextStyle_C");

	const FLinearColor PanelFillColor(0.025f, 0.030f, 0.038f, 0.86f);
	const FLinearColor PanelBorderColor(0.74f, 0.70f, 0.58f, 0.90f);
	const FLinearColor MainTextColor(0.94f, 0.94f, 0.92f, 1.f);
	const FLinearColor MutedTextColor(0.76f, 0.77f, 0.78f, 1.f);
	const FLinearColor LootTextColor(0.92f, 0.90f, 0.82f, 1.f);
	const FLinearColor BadgeColor(0.58f, 0.58f, 0.54f, 0.92f);
	const FLinearColor HintTextColor(0.92f, 0.92f, 0.90f, 1.f);

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
			Widget->SetClipping(EWidgetClipping::ClipToBounds);
		}
		return Widget;
	}

	void ConfigureText(UTextBlock* TextBlock, const FString& Text, const FLinearColor& Color, const int32 Size, const ETextJustify::Type Justification = ETextJustify::Left, const bool bWrap = false)
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
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
	}

	void SetClassArrayProperty(UObject* Object, const FName PropertyName, const TArray<UClass*>& Values)
	{
		if (!Object)
		{
			return;
		}

		if (FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName))
		{
			if (FClassProperty* ClassProperty = CastField<FClassProperty>(ArrayProperty->Inner))
			{
				Object->Modify();
				FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
				Helper.EmptyValues();
				for (UClass* Value : Values)
				{
					if (!Value)
					{
						continue;
					}

					const int32 Index = Helper.AddValue();
					ClassProperty->SetPropertyValue(Helper.GetRawPtr(Index), Value);
				}
			}
		}
	}

	void ConfigureRichText(UYogCommonRichTextBlock* RichTextBlock, const FString& Text, const FLinearColor& Color, const int32 Size)
	{
		if (!RichTextBlock)
		{
			return;
		}

		RichTextBlock->SetText(FText::FromString(Text));
		RichTextBlock->SetAutoWrapText(true);
		RichTextBlock->SetJustification(ETextJustify::Left);
		RichTextBlock->FontStyleClass = LoadClass<UCommonTextStyle>(nullptr, *InfoPopupTextStyleClassPath);
		RichTextBlock->OverrideFontSize = Size;
		RichTextBlock->OverrideColor = Color;

		TArray<UClass*> DecoratorClasses;
		if (UClass* InputActionDecoratorClass = LoadClass<URichTextBlockDecorator>(nullptr, *InputActionDecoratorClassPath))
		{
			DecoratorClasses.Add(InputActionDecoratorClass);
		}
		SetClassArrayProperty(RichTextBlock, TEXT("DecoratorClasses"), DecoratorClasses);
	}

	void ConfigurePanelBorder(UBorder* Border)
	{
		if (!Border)
		{
			return;
		}

		Border->SetBrush(FSlateRoundedBoxBrush(
			PanelFillColor,
			3.f,
			PanelBorderColor,
			2.f));
		Border->SetPadding(FMargin(16.f, 14.f));
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

		TArray<UWidget*> ExistingWidgets;
		TArray<UObject*> ExistingObjects;
		GetObjectsWithOuter(WidgetTree, ExistingObjects, true);
		for (UObject* ExistingObject : ExistingObjects)
		{
			if (UWidget* ExistingWidget = Cast<UWidget>(ExistingObject))
			{
				ExistingWidgets.AddUnique(ExistingWidget);
			}
		}

		if (WidgetTree->RootWidget)
		{
			WidgetTree->RemoveWidget(WidgetTree->RootWidget);
			WidgetTree->RootWidget = nullptr;
		}

		for (UWidget* ExistingWidget : ExistingWidgets)
		{
			if (!ExistingWidget)
			{
				continue;
			}

			WidgetTree->RemoveWidget(ExistingWidget);
			const FString OldName = FString::Printf(
				TEXT("PortalPreview_Old_%s_%s"),
				*ExistingWidget->GetName(),
				*FGuid::NewGuid().ToString(EGuidFormats::Digits));
			ExistingWidget->Rename(*OldName, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
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

		UCanvasPanel* RootCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("RootCanvas"));
		USizeBox* Root = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("Root"));
		UBorder* BG = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("BG"));
		UVerticalBox* VStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("VStack"), false);
		UHorizontalBox* HeaderBox = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("HeaderBox"), false);
		UBorder* RoomTypeBadge = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("RoomTypeBadge"));
		UTextBlock* RoomTypeText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("RoomTypeText"));
		UTextBlock* RoomNameText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("RoomNameText"));
		UTextBlock* MarkerTitleText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("MarkerTitleText"), false);
		UVerticalBox* BuffListBox = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("BuffListBox"));
		UTextBlock* LootSummaryText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("LootSummaryText"));
		UYogCommonRichTextBlock* InteractHintRoot = ConstructNamedWidget<UYogCommonRichTextBlock>(WidgetTree, TEXT("InteractHintRoot"));

		if (!RootCanvas || !Root || !BG || !VStack || !HeaderBox || !RoomTypeBadge || !RoomTypeText || !RoomNameText || !MarkerTitleText || !BuffListBox || !LootSummaryText || !InteractHintRoot)
		{
			return;
		}

		WidgetTree->RootWidget = RootCanvas;

		Root->SetWidthOverride(430.f);
		Root->SetMinDesiredWidth(360.f);
		if (UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(Root))
		{
			RootSlot->SetAnchors(FAnchors(0.f, 0.f));
			RootSlot->SetAlignment(FVector2D(0.f, 0.f));
			RootSlot->SetPosition(FVector2D::ZeroVector);
			RootSlot->SetSize(FVector2D(430.f, 260.f));
			RootSlot->SetAutoSize(true);
		}

		ConfigurePanelBorder(BG);
		Root->AddChild(BG);
		BG->SetContent(VStack);

		if (UVerticalBoxSlot* HeaderSlot = VStack->AddChildToVerticalBox(HeaderBox))
		{
			HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		RoomTypeBadge->SetBrushColor(BadgeColor);
		RoomTypeBadge->SetPadding(FMargin(10.f, 3.f));
		RoomTypeBadge->SetContent(RoomTypeText);
		ConfigureText(RoomTypeText, TEXT("普通"), MainTextColor, 20, ETextJustify::Center, false);
		if (UHorizontalBoxSlot* BadgeSlot = HeaderBox->AddChildToHorizontalBox(RoomTypeBadge))
		{
			BadgeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			BadgeSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureText(RoomNameText, TEXT("下一关"), MainTextColor, 22, ETextJustify::Left, true);
		if (UHorizontalBoxSlot* RoomSlot = HeaderBox->AddChildToHorizontalBox(RoomNameText))
		{
			RoomSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			RoomSlot->SetVerticalAlignment(VAlign_Center);
			RoomSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
		}

		ConfigureText(MarkerTitleText, TEXT("\"敌人将携带印记\""), MainTextColor, 22, ETextJustify::Left, true);
		if (UVerticalBoxSlot* MarkerSlot = VStack->AddChildToVerticalBox(MarkerTitleText))
		{
			MarkerSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		if (UVerticalBoxSlot* BuffSlot = VStack->AddChildToVerticalBox(BuffListBox))
		{
			BuffSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		ConfigureText(LootSummaryText, TEXT("战利品：符文、金币"), LootTextColor, 22, ETextJustify::Left, true);
		if (UVerticalBoxSlot* LootSlot = VStack->AddChildToVerticalBox(LootSummaryText))
		{
			LootSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}

		ConfigureRichText(InteractHintRoot, TEXT("<input action=\"Interact\"/> 进入"), HintTextColor, 14);
		InteractHintRoot->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* HintSlot = VStack->AddChildToVerticalBox(InteractHintRoot))
		{
			HintSlot->SetPadding(FMargin(0.f));
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
			TEXT("- %s `%s` with parent `PortalPreviewWidget`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*WidgetPackagePath));

		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = UPortalPreviewWidget::StaticClass();

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

UPortalPreviewWidgetSetupCommandlet::UPortalPreviewWidgetSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UPortalPreviewWidgetSetupCommandlet::Main(const FString& Params)
{
	using namespace PortalPreviewWBPSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Portal Preview Widget Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Target WBP: `%s`"), *WidgetPackagePath));
	ReportLines.Add(TEXT(""));

	bool bCreated = false;
	UWidgetBlueprint* WidgetBlueprint = CreateWidgetBlueprint(bDryRun, ReportLines, bCreated);
	if (WidgetBlueprint && !bDryRun)
	{
		const bool bMissingDesignerBindings = !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BG"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("RoomNameText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("RoomTypeBadge"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("RoomTypeText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BuffListBox"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("LootSummaryText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("InteractHintRoot"));

		if (bCreated || bForceLayout || bMissingDesignerBindings)
		{
			BuildDesignerTree(WidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
			ReportLines.Add(TEXT("- Designer tree refreshed with BG border, header, buff list, loot summary, and input hint."));
		}
		else
		{
			if (UBorder* BG = Cast<UBorder>(WidgetBlueprint->WidgetTree->FindWidget(TEXT("BG"))))
			{
				BG->Modify();
				ConfigurePanelBorder(BG);
				FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
				WidgetBlueprint->MarkPackageDirty();
				DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
				ReportLines.Add(TEXT("- Existing designer tree kept; BG border style refreshed."));
			}
			else
			{
				ReportLines.Add(TEXT("- Existing designer tree kept unchanged."));
			}
		}
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(PortalPreviewWBPSetup::ReportFileName, ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Portal preview widget setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
