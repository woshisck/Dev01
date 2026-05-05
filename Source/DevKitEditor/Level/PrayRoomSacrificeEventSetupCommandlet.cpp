#include "Level/PrayRoomSacrificeEventSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
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
#include "Data/AltarDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "FileHelpers.h"
#include "GameplayTagContainer.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UI/SacrificeSelectionWidget.h"
#include "UObject/Package.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace PrayRoomSacrificeEventSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Playtest_UI/Event/WBP_SacrificeSelection");
	const FString AltarDataPackagePath = TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_Altar_PrayRoom_SacrificeEvent");
	const FString SacrificeRunePackagePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_MoonlightShadow");
	const TArray<FString> PrayRoomPackagePaths = {
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom")
	};
	const FString ReportFileName = TEXT("PrayRoomSacrificeEventSetupReport.md");

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString ExistingPackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &ExistingPackageFile);
	}

	template <typename T>
	T* LoadAssetByPackagePath(const FString& PackagePath)
	{
		if (PackagePath.IsEmpty())
		{
			return nullptr;
		}
		if (T* Existing = FindObject<T>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}
		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}
		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
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

	UTextBlock* MakeButtonLabel(UWidgetTree* WidgetTree, UButton* Button, const FName ButtonName, const FString& Text)
	{
		if (!WidgetTree || !Button)
		{
			return nullptr;
		}

		const FName LabelName(*FString::Printf(TEXT("%s_Label"), *ButtonName.ToString()));
		UTextBlock* Label = ConstructNamedWidget<UTextBlock>(WidgetTree, LabelName);
		ConfigureText(Label, Text, FLinearColor(0.92f, 0.88f, 0.80f, 1.0f), 16, true);
		if (Label)
		{
			Label->SetJustification(ETextJustify::Center);
			Button->AddChild(Label);
		}
		return Label;
	}

	UButton* MakeNamedButton(UWidgetTree* WidgetTree, const FName ButtonName, const FString& Text)
	{
		UButton* Button = ConstructNamedWidget<UButton>(WidgetTree, ButtonName);
		MakeButtonLabel(WidgetTree, Button, ButtonName, Text);
		return Button;
	}

	void AddButtonToBox(UVerticalBox* Box, UButton* Button, const FMargin& Padding)
	{
		if (Box && Button)
		{
			if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Button))
			{
				Slot->SetPadding(Padding);
			}
		}
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
		UTextBlock* DescriptionText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("DescriptionText"));
		UTextBlock* CostText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("CostText"));
		UVerticalBox* OptionBox = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("OptionBox"));
		UVerticalBox* DeckCardBox = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("DeckCardBox"));
		UHorizontalBox* ActionRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("ActionRow"));

		if (!RootCanvas || !PanelSizeBox || !OuterBorder || !PanelStack || !TitleText || !DescriptionText || !CostText || !OptionBox || !DeckCardBox || !ActionRow)
		{
			return;
		}

		WidgetTree->RootWidget = RootCanvas;
		PanelSizeBox->SetWidthOverride(560.0f);
		OuterBorder->SetBrushColor(FLinearColor(0.025f, 0.025f, 0.032f, 0.92f));
		OuterBorder->SetPadding(FMargin(18.0f, 16.0f));

		if (UCanvasPanelSlot* SizeSlot = RootCanvas->AddChildToCanvas(PanelSizeBox))
		{
			SizeSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			SizeSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			SizeSlot->SetAutoSize(true);
		}

		PanelSizeBox->AddChild(OuterBorder);
		OuterBorder->SetContent(PanelStack);

		ConfigureText(TitleText, TEXT("\u732e\u796d\u5723\u575b"), FLinearColor(0.95f, 0.72f, 0.34f, 1.0f), 24, false);
		ConfigureText(DescriptionText, TEXT("\u9009\u62e9\u4ee3\u4ef7\uff0c\u83b7\u5f97\u732e\u796d\u7b26\u6587"), FLinearColor(0.80f, 0.82f, 0.86f, 1.0f), 16, true);
		ConfigureText(CostText, TEXT("\u4ee3\u4ef7"), FLinearColor(0.64f, 0.68f, 0.76f, 1.0f), 15, true);

		if (UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
		if (UVerticalBoxSlot* DescSlot = PanelStack->AddChildToVerticalBox(DescriptionText))
		{
			DescSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
		if (UVerticalBoxSlot* CostSlot = PanelStack->AddChildToVerticalBox(CostText))
		{
			CostSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		AddButtonToBox(OptionBox, MakeNamedButton(WidgetTree, TEXT("OptionButton0"), TEXT("\u4ee3\u4ef7 1")), FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		AddButtonToBox(OptionBox, MakeNamedButton(WidgetTree, TEXT("OptionButton1"), TEXT("\u4ee3\u4ef7 2")), FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		AddButtonToBox(OptionBox, MakeNamedButton(WidgetTree, TEXT("OptionButton2"), TEXT("\u4ee3\u4ef7 3")), FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		if (UVerticalBoxSlot* OptionSlot = PanelStack->AddChildToVerticalBox(OptionBox))
		{
			OptionSlot->SetPadding(FMargin(0.0f));
		}

		for (int32 Index = 0; Index < 8; ++Index)
		{
			const FName ButtonName(*FString::Printf(TEXT("DeckButton%d"), Index));
			AddButtonToBox(DeckCardBox, MakeNamedButton(WidgetTree, ButtonName, FString::Printf(TEXT("\u5361\u724c %d"), Index + 1)), FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
		if (UVerticalBoxSlot* DeckSlot = PanelStack->AddChildToVerticalBox(DeckCardBox))
		{
			DeckSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 8.0f));
		}

		UButton* BtnConfirm = MakeNamedButton(WidgetTree, TEXT("BtnConfirm"), TEXT("\u786e\u8ba4\u732e\u796d"));
		UButton* BtnCancel = MakeNamedButton(WidgetTree, TEXT("BtnCancel"), TEXT("\u53d6\u6d88"));
		if (BtnConfirm && BtnCancel)
		{
			if (UHorizontalBoxSlot* ConfirmSlot = ActionRow->AddChildToHorizontalBox(BtnConfirm))
			{
				ConfirmSlot->SetPadding(FMargin(0.0f, 8.0f, 8.0f, 0.0f));
			}
			if (UHorizontalBoxSlot* CancelSlot = ActionRow->AddChildToHorizontalBox(BtnCancel))
			{
				CancelSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			}
		}
		PanelStack->AddChildToVerticalBox(ActionRow);
	}

	UWidgetBlueprint* LoadWidgetBlueprint()
	{
		return LoadAssetByPackagePath<UWidgetBlueprint>(WidgetPackagePath);
	}

	UWidgetBlueprint* CreateWidgetBlueprint(bool bDryRun, TArray<FString>& ReportLines, bool& bCreated)
	{
		bCreated = false;
		if (UWidgetBlueprint* Existing = LoadWidgetBlueprint())
		{
			ReportLines.Add(FString::Printf(TEXT("- Found widget `%s`."), *WidgetPackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s widget `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *WidgetPackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = USacrificeSelectionWidget::StaticClass();

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
			ReportLines.Add(TEXT("- Failed to create widget blueprint."));
		}
		return NewWidgetBlueprint;
	}

	UAltarDataAsset* CreateAltarDataAsset(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UAltarDataAsset* Existing = LoadAssetByPackagePath<UAltarDataAsset>(AltarDataPackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found altar data `%s`."), *AltarDataPackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s altar data `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *AltarDataPackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*AltarDataPackagePath);
		if (!Package)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create altar package `%s`."), *AltarDataPackagePath));
			return nullptr;
		}

		UAltarDataAsset* NewAsset = NewObject<UAltarDataAsset>(
			Package,
			UAltarDataAsset::StaticClass(),
			*FPackageName::GetLongPackageAssetName(AltarDataPackagePath),
			RF_Public | RF_Standalone | RF_Transactional);

		if (NewAsset)
		{
			FAssetRegistryModule::AssetCreated(NewAsset);
			NewAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(NewAsset->GetPackage());
		}
		else
		{
			ReportLines.Add(TEXT("- Failed to allocate altar data asset."));
		}
		return NewAsset;
	}

	FAltarSacrificeEntry MakeCostEntry(
		URuneDataAsset* Rune,
		const ESacrificeOfferingCostType CostType,
		const FString& CostText,
		const float PrimaryMagnitude,
		const float SecondaryMagnitude)
	{
		FAltarSacrificeEntry Entry;
		Entry.GrantedRune = Rune;
		Entry.CostType = CostType;
		Entry.CostDescription = FText::FromString(CostText);
		Entry.PrimaryMagnitude = PrimaryMagnitude;
		Entry.SecondaryMagnitude = SecondaryMagnitude;
		return Entry;
	}

	bool ConfigureAltarData(UAltarDataAsset* AltarData, URuneDataAsset* SacrificeRune, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (!AltarData || !SacrificeRune)
		{
			return false;
		}

		AltarData->Modify();
		AltarData->EventSacrificeRune = SacrificeRune;
		AltarData->bPurificationEnabled = false;
		AltarData->bSacrificeEnabled = true;
		AltarData->SacrificeRunePool.Reset();
		AltarData->SacrificeRunePool.Add(MakeCostEntry(
			SacrificeRune,
			ESacrificeOfferingCostType::SacrificeDeckCard,
			TEXT("\u732e\u796d\u5361\u7ec4\u4e2d\u7684\u4e00\u5f20\u5361\u724c"),
			0.0f,
			0.0f));
		AltarData->SacrificeRunePool.Add(MakeCostEntry(
			SacrificeRune,
			ESacrificeOfferingCostType::AttackUpDamageTakenUp,
			TEXT("\u653b\u51fb\u529b\u589e\u52a0 15%\uff0c\u53d7\u5230\u7684\u4f24\u5bb3\u589e\u52a0 20%"),
			0.15f,
			0.20f));
		AltarData->SacrificeRunePool.Add(MakeCostEntry(
			SacrificeRune,
			ESacrificeOfferingCostType::CritRateDownCritDamageUp,
			TEXT("\u66b4\u51fb\u6982\u7387\u964d\u4f4e 50%\uff0c\u66b4\u51fb\u4f24\u5bb3\u589e\u52a0 50%"),
			0.50f,
			0.50f));

		AltarData->MarkPackageDirty();
		DirtyPackages.AddUnique(AltarData->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Configured altar data with rune `%s` and three fixed costs."), *SacrificeRunePackagePath));
		return true;
	}

	FGameplayTag RequestTagOrNone(const FName TagName, TArray<FString>& ReportLines)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (!Tag.IsValid())
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing gameplay tag `%s`; room tag not applied."), *TagName.ToString()));
		}
		return Tag;
	}

	void ConfigureRoomTags(URoomDataAsset* Room, TArray<FString>& ReportLines)
	{
		if (!Room)
		{
			return;
		}

		const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Normal")), false);
		const FGameplayTag EliteTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Elite")), false);
		const FGameplayTag ShopTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Shop")), false);
		if (NormalTag.IsValid())
		{
			Room->RoomTags.RemoveTag(NormalTag);
		}
		if (EliteTag.IsValid())
		{
			Room->RoomTags.RemoveTag(EliteTag);
		}
		if (ShopTag.IsValid())
		{
			Room->RoomTags.RemoveTag(ShopTag);
		}

		const FGameplayTag EventTag = RequestTagOrNone(FName(TEXT("Room.Type.Event")), ReportLines);
		const FGameplayTag LayerTag = RequestTagOrNone(FName(TEXT("Room.Layer.L1")), ReportLines);
		if (EventTag.IsValid())
		{
			Room->RoomTags.AddTag(EventTag);
		}
		if (LayerTag.IsValid())
		{
			Room->RoomTags.AddTag(LayerTag);
		}
	}

	void ConfigureRoomAsset(
		const FString& RoomPackagePath,
		UAltarDataAsset* AltarData,
		UWidgetBlueprint* WidgetBlueprint,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		URoomDataAsset* Room = LoadAssetByPackagePath<URoomDataAsset>(RoomPackagePath);
		if (!Room)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing room asset `%s`."), *RoomPackagePath));
			return;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s room `%s` as PrayRoom sacrifice event room."), bDryRun ? TEXT("Would configure") : TEXT("Configured"), *RoomPackagePath));
		if (bDryRun)
		{
			return;
		}

		Room->Modify();
		Room->RoomName = FName(TEXT("PrayRoom"));
		Room->DisplayName = FText::FromString(TEXT("\u7948\u7977\u5ba4"));
		ConfigureRoomTags(Room, ReportLines);
		Room->bEnableTimedClearObjective = true;
		Room->TimedClearSeconds = 90.0f;
		Room->SacrificeEventAltarData = AltarData;
		Room->SacrificeEventAltarClass = nullptr;
		TSubclassOf<USacrificeSelectionWidget> WidgetClass = USacrificeSelectionWidget::StaticClass();
		if (WidgetBlueprint && WidgetBlueprint->GeneratedClass)
		{
			WidgetClass = WidgetBlueprint->GeneratedClass;
		}
		Room->SacrificeEventWidgetClass = WidgetClass;
		Room->SacrificeEventAltarSpawnOffset = FVector(250.0f, 0.0f, 0.0f);
		Room->MarkPackageDirty();
		DirtyPackages.AddUnique(Room->GetPackage());
	}
}

UPrayRoomSacrificeEventSetupCommandlet::UPrayRoomSacrificeEventSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UPrayRoomSacrificeEventSetupCommandlet::Main(const FString& Params)
{
	using namespace PrayRoomSacrificeEventSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# PrayRoom Sacrifice Event Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Sacrifice rune: `%s`"), *SacrificeRunePackagePath));
	ReportLines.Add(FString::Printf(TEXT("- Altar data: `%s`"), *AltarDataPackagePath));
	ReportLines.Add(FString::Printf(TEXT("- Widget: `%s`"), *WidgetPackagePath));
	ReportLines.Add(TEXT(""));

	URuneDataAsset* SacrificeRune = LoadAssetByPackagePath<URuneDataAsset>(SacrificeRunePackagePath);
	if (!SacrificeRune)
	{
		ReportLines.Add(FString::Printf(TEXT("- Missing sacrifice rune `%s`."), *SacrificeRunePackagePath));
	}

	bool bCreatedWidget = false;
	UWidgetBlueprint* WidgetBlueprint = CreateWidgetBlueprint(bDryRun, ReportLines, bCreatedWidget);
	if (WidgetBlueprint && !bDryRun)
	{
		const bool bMissingDesignerBindings = !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("TitleText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("DescriptionText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("CostText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("OptionBox"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("DeckCardBox"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("OptionButton0"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnConfirm"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnCancel"));

		if (bCreatedWidget || bForceLayout || bMissingDesignerBindings)
		{
			BuildDesignerTree(WidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
			ReportLines.Add(TEXT("- Designer tree refreshed for sacrifice selection WBP."));
		}
		else
		{
			ReportLines.Add(TEXT("- Existing sacrifice selection WBP designer tree kept unchanged."));
		}
	}

	UAltarDataAsset* AltarData = CreateAltarDataAsset(bDryRun, ReportLines, DirtyPackages);
	if (!bDryRun && AltarData && SacrificeRune)
	{
		ConfigureAltarData(AltarData, SacrificeRune, ReportLines, DirtyPackages);
	}
	else if (bDryRun)
	{
		ReportLines.Add(TEXT("- Would configure altar data with three fixed costs."));
	}

	for (const FString& RoomPackagePath : PrayRoomPackagePaths)
	{
		ConfigureRoomAsset(RoomPackagePath, AltarData, WidgetBlueprint, bDryRun, ReportLines, DirtyPackages);
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), ReportFileName);
	FFileHelper::SaveStringToFile(
		FString::Join(ReportLines, LINE_TERMINATOR),
		*ReportPath,
		FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Display, TEXT("PrayRoom sacrifice event setup finished. Report: %s"), *ReportPath);
	return SacrificeRune ? 0 : 1;
}
