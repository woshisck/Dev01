#include "UI/GraphicsSettingsWidgetSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UI/YogGraphicsSettingsWidgetBase.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace GraphicsSettingsWidgetSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Frontend/WBP_GraphicsSettingsWidget");
	const FString ReportFileName = TEXT("GraphicsSettingsWidgetSetupReport.md");

	const FLinearColor PageBgColor(0.009f, 0.011f, 0.016f, 0.94f);
	const FLinearColor PanelColor(0.030f, 0.034f, 0.043f, 0.94f);
	const FLinearColor PanelBorderColor(0.50f, 0.55f, 0.58f, 0.72f);
	const FLinearColor ButtonColor(0.045f, 0.052f, 0.062f, 0.92f);
	const FLinearColor ButtonHoverColor(0.12f, 0.19f, 0.24f, 1.f);
	const FLinearColor MainTextColor(0.91f, 0.90f, 0.84f, 1.f);
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
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.62f));

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
	}

	void ConfigureButton(UButton* Button)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		Style.Normal.TintColor = FSlateColor(ButtonColor);
		Style.Hovered.TintColor = FSlateColor(ButtonHoverColor);
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.09f, 0.14f, 0.18f, 1.f));
		Style.Disabled.TintColor = FSlateColor(FLinearColor(0.04f, 0.04f, 0.05f, 0.36f));
		Button->SetStyle(Style);
	}

	UTextBlock* MakeText(UWidgetTree* WidgetTree, const FName Name, const FString& Text, int32 Size, const FLinearColor& Color = MainTextColor, ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* TextBlock = ConstructNamedWidget<UTextBlock>(WidgetTree, Name, false);
		ConfigureText(TextBlock, Text, Color, Size, Justification, true);
		return TextBlock;
	}

	UButton* MakeButton(UWidgetTree* WidgetTree, const FName ButtonName, const FString& Label)
	{
		UButton* Button = ConstructNamedWidget<UButton>(WidgetTree, ButtonName);
		if (Button)
		{
			ConfigureButton(Button);
			Button->SetContent(MakeText(WidgetTree, FName(*(ButtonName.ToString() + TEXT("_Label"))), Label, 16, MainTextColor, ETextJustify::Center));
		}
		return Button;
	}

	USlider* MakeQualitySlider(UWidgetTree* WidgetTree, const FName SliderName, float Value)
	{
		USlider* Slider = ConstructNamedWidget<USlider>(WidgetTree, SliderName);
		if (Slider)
		{
			Slider->SetMinValue(0.f);
			Slider->SetMaxValue(3.f);
			Slider->SetStepSize(1.f / 3.f);
			Slider->SetValue(Value);
		}
		return Slider;
	}

	void AddVertical(UVerticalBox* Parent, UWidget* Widget, const FMargin& Padding, ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
	{
		if (!Parent || !Widget)
		{
			return;
		}

		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Widget))
		{
			Slot->SetPadding(Padding);
			Slot->SetSize(FSlateChildSize(SizeRule));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
	}

	void AddHorizontal(UHorizontalBox* Parent, UWidget* Widget, const FMargin& Padding = FMargin(0.f, 0.f, 8.f, 0.f))
	{
		if (!Parent || !Widget)
		{
			return;
		}

		if (UHorizontalBoxSlot* Slot = Parent->AddChildToHorizontalBox(Widget))
		{
			Slot->SetPadding(Padding);
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	UHorizontalBox* MakeButtonRow(UWidgetTree* WidgetTree, const FName RowName, const TArray<TPair<FName, FString>>& Buttons)
	{
		UHorizontalBox* Row = ConstructNamedWidget<UHorizontalBox>(WidgetTree, RowName, false);
		for (const TPair<FName, FString>& ButtonInfo : Buttons)
		{
			AddHorizontal(Row, MakeButton(WidgetTree, ButtonInfo.Key, ButtonInfo.Value));
		}
		return Row;
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
					*FString::Printf(TEXT("GraphicsSettings_Old_%s_%s"), *ExistingWidget->GetName(), *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
					GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}

		WidgetTree->RootWidget = nullptr;
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
			TEXT("- %s `%s` with parent `UYogGraphicsSettingsWidgetBase`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*WidgetPackagePath));

		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = UYogGraphicsSettingsWidgetBase::StaticClass();

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
		USizeBox* PanelSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("PanelSize"), false);
		UBorder* Panel = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("Panel"), false);
		UScrollBox* RootScroll = ConstructNamedWidget<UScrollBox>(WidgetTree, TEXT("RootScroll"), false);
		UVerticalBox* RootStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("RootStack"), false);

		if (!RootCanvas || !Background || !PanelSize || !Panel || !RootScroll || !RootStack)
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

		PanelSize->SetWidthOverride(760.f);
		PanelSize->SetHeightOverride(700.f);
		PanelSize->AddChild(Panel);
		if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSize))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D::ZeroVector);
			PanelSlot->SetSize(FVector2D(760.f, 700.f));
		}

		Panel->SetBrush(FSlateRoundedBoxBrush(PanelColor, 6.f, PanelBorderColor, 1.2f));
		Panel->SetPadding(FMargin(28.f, 26.f, 28.f, 24.f));
		Panel->SetContent(RootScroll);
		RootScroll->AddChild(RootStack);

		AddVertical(RootStack, MakeText(WidgetTree, TEXT("HeaderText"), TEXT("Graphics"), 34, MainTextColor, ETextJustify::Center), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("HintText"), TEXT("Select a profile, then tune runtime options for handheld or PC targets."), 16, MutedTextColor, ETextJustify::Center), FMargin(0.f, 0.f, 0.f, 20.f));

		AddVertical(RootStack, ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("CurrentProfileText")), FMargin(0.f, 0.f, 0.f, 10.f));
		ConfigureText(Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CurrentProfileText"))), TEXT("Current: Medium"), MainTextColor, 20, ETextJustify::Center, false);

		AddVertical(RootStack, MakeButtonRow(WidgetTree, TEXT("TargetTierRow"), {
			{ TEXT("BtnTargetPCUltra"), TEXT("PC Ultra") },
			{ TEXT("BtnTargetSteamDeck15W"), TEXT("Deck 15W") },
			{ TEXT("BtnTargetSwitch2Candidate"), TEXT("Switch 2") },
			{ TEXT("BtnTargetSteamDeck5W"), TEXT("Deck 5W") },
			{ TEXT("BtnTargetFallbackLow"), TEXT("Fallback") }
		}), FMargin(0.f, 0.f, 0.f, 14.f));

		AddVertical(RootStack, MakeButtonRow(WidgetTree, TEXT("ProfileRow"), {
			{ TEXT("BtnProfileLow"), TEXT("Low") },
			{ TEXT("BtnProfileMedium"), TEXT("Medium") },
			{ TEXT("BtnProfileHigh"), TEXT("High") },
			{ TEXT("BtnProfileUltra"), TEXT("Ultra") }
		}), FMargin(0.f, 0.f, 0.f, 18.f));

		AddVertical(RootStack, ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("ResolutionScaleText")), FMargin(0.f, 0.f, 0.f, 6.f));
		ConfigureText(Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ResolutionScaleText"))), TEXT("Resolution Scale: 80%"), MainTextColor, 17, ETextJustify::Left, false);

		USlider* ResolutionSlider = ConstructNamedWidget<USlider>(WidgetTree, TEXT("ResolutionScaleSlider"));
		if (ResolutionSlider)
		{
			ResolutionSlider->SetMinValue(25.f);
			ResolutionSlider->SetMaxValue(100.f);
			ResolutionSlider->SetValue(80.f);
		}
		AddVertical(RootStack, ResolutionSlider, FMargin(0.f, 0.f, 0.f, 18.f));

		AddVertical(RootStack, ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("FrameRateText")), FMargin(0.f, 0.f, 0.f, 8.f));
		ConfigureText(Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("FrameRateText"))), TEXT("Frame Limit: 60"), MainTextColor, 17, ETextJustify::Left, false);
		AddVertical(RootStack, MakeButtonRow(WidgetTree, TEXT("FrameRow"), {
			{ TEXT("BtnFrame30"), TEXT("30") },
			{ TEXT("BtnFrame40"), TEXT("40") },
			{ TEXT("BtnFrame60"), TEXT("60") },
			{ TEXT("BtnFrame120"), TEXT("120") },
			{ TEXT("BtnFrameUnlimited"), TEXT("Unlimited") }
		}), FMargin(0.f, 0.f, 0.f, 18.f));

		UHorizontalBox* ToggleRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("ToggleRow"), false);
		UCheckBox* LumenToggle = ConstructNamedWidget<UCheckBox>(WidgetTree, TEXT("LumenLiteCheckBox"));
		UCheckBox* BatchToggle = ConstructNamedWidget<UCheckBox>(WidgetTree, TEXT("BatchProxiesCheckBox"));
		if (LumenToggle)
		{
			LumenToggle->SetContent(MakeText(WidgetTree, TEXT("LumenLiteCheckBox_Label"), TEXT("Lumen Lite"), 16, MainTextColor, ETextJustify::Left));
		}
		if (BatchToggle)
		{
			BatchToggle->SetContent(MakeText(WidgetTree, TEXT("BatchProxiesCheckBox_Label"), TEXT("Batch Proxies"), 16, MainTextColor, ETextJustify::Left));
		}
		AddHorizontal(ToggleRow, LumenToggle, FMargin(0.f, 0.f, 24.f, 0.f));
		AddHorizontal(ToggleRow, BatchToggle, FMargin(0.f));
		AddVertical(RootStack, ToggleRow, FMargin(0.f, 0.f, 0.f, 22.f));

		AddVertical(RootStack, MakeText(WidgetTree, TEXT("DetailedQualitySection"), TEXT("Detailed Quality"), 20, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 8.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("ModelQualityText"), TEXT("Model Quality: 1"), 16, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeQualitySlider(WidgetTree, TEXT("ModelQualitySlider"), 1.f), FMargin(0.f, 0.f, 0.f, 10.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("ShadowQualityText"), TEXT("Shadow Quality: 1"), 16, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeQualitySlider(WidgetTree, TEXT("ShadowQualitySlider"), 1.f), FMargin(0.f, 0.f, 0.f, 10.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("ReflectionQualityText"), TEXT("Reflection Quality: 1"), 16, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeQualitySlider(WidgetTree, TEXT("ReflectionQualitySlider"), 1.f), FMargin(0.f, 0.f, 0.f, 10.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("TextureQualityText"), TEXT("Texture Quality: 2"), 16, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeQualitySlider(WidgetTree, TEXT("TextureQualitySlider"), 2.f), FMargin(0.f, 0.f, 0.f, 10.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("MaterialQualityText"), TEXT("Material Quality: 1"), 16, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeQualitySlider(WidgetTree, TEXT("MaterialQualitySlider"), 1.f), FMargin(0.f, 0.f, 0.f, 10.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("DynamicLightQualityText"), TEXT("Dynamic Light Quality: 1"), 16, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeQualitySlider(WidgetTree, TEXT("DynamicLightQualitySlider"), 1.f), FMargin(0.f, 0.f, 0.f, 10.f));
		AddVertical(RootStack, MakeText(WidgetTree, TEXT("MaterialLightQualityText"), TEXT("Material Light Quality: 1 (1 lights)"), 16, MainTextColor, ETextJustify::Left), FMargin(0.f, 0.f, 0.f, 4.f));
		AddVertical(RootStack, MakeQualitySlider(WidgetTree, TEXT("MaterialLightQualitySlider"), 1.f), FMargin(0.f, 0.f, 0.f, 18.f));

		AddVertical(RootStack, MakeButtonRow(WidgetTree, TEXT("ActionRow"), {
			{ TEXT("BtnApplyCustom"), TEXT("Apply Custom") },
			{ TEXT("BtnBack"), TEXT("Back") }
		}), FMargin(0.f), ESlateSizeRule::Automatic);
	}

	bool NeedsRefresh(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return true;
		}

		for (const FName WidgetName : UYogGraphicsSettingsWidgetBase::GetRequiredDesignerWidgetNames())
		{
			if (!WidgetBlueprint->WidgetTree->FindWidget(WidgetName))
			{
				return true;
			}
		}

		return false;
	}
}

UGraphicsSettingsWidgetSetupCommandlet::UGraphicsSettingsWidgetSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

FString UGraphicsSettingsWidgetSetupCommandlet::GetTargetWidgetPackagePath()
{
	return GraphicsSettingsWidgetSetup::WidgetPackagePath;
}

FString UGraphicsSettingsWidgetSetupCommandlet::GetReportFileName()
{
	return GraphicsSettingsWidgetSetup::ReportFileName;
}

int32 UGraphicsSettingsWidgetSetupCommandlet::Main(const FString& Params)
{
	using namespace GraphicsSettingsWidgetSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Graphics Settings Widget Setup Report"));
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
			ReportLines.Add(TEXT("- Designer tree refreshed with target tier, graphics profile, resolution, frame-rate, detailed quality, Lumen Lite, batch proxy, apply, and back controls."));
			ReportLines.Add(FString::Printf(TEXT("- Default controller focus target: `%s`."), *UYogGraphicsSettingsWidgetBase::GetDefaultFocusWidgetName().ToString()));
		}
		else
		{
			ReportLines.Add(TEXT("- Existing graphics settings designer tree kept unchanged."));
			ReportLines.Add(FString::Printf(TEXT("- Default controller focus target: `%s`."), *UYogGraphicsSettingsWidgetBase::GetDefaultFocusWidgetName().ToString()));
		}
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Graphics settings widget setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
