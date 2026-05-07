#include "Level/ShopRoomSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "Data/ShopDataAsset.h"
#include "FileHelpers.h"
#include "GameplayTagContainer.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UI/ShopSelectionWidget.h"
#include "UObject/Package.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace ShopRoomSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Playtest_UI/Event/WBP_ShopSelection");
	const FString ShopDataPackagePath = TEXT("/Game/Docs/Map/Shop/DA_ShopInventory_512_Default");
	const FString ShopRoomPackagePath = TEXT("/Game/Docs/Map/Shop/DA_Room_512_Shop");
	const FString CampaignPackagePath = TEXT("/Game/Docs/Map/DA_Campaign_MainRun");
	const FString ReportFileName = TEXT("ShopRoomSetupReport.md");

	struct FDefaultStockSpec
	{
		FString PackagePath;
		int32 Cost = -1;
		FString Note;
	};

	const TArray<FDefaultStockSpec> DefaultStock = {
		{ TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack"), 40, TEXT("Damage baseline") },
		{ TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Burn"), 55, TEXT("Damage over time") },
		{ TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Shield"), 50, TEXT("Defensive option") },
		{ TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Poison"), 45, TEXT("Poison control") }
	};

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

	template <typename TAsset>
	TAsset* CreateOrLoadDataAsset(const FString& PackagePath, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (TAsset* Existing = LoadAssetByPackagePath<TAsset>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found asset `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s asset `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create package `%s`."), *PackagePath));
			return nullptr;
		}

		TAsset* NewAsset = NewObject<TAsset>(
			Package,
			TAsset::StaticClass(),
			*FPackageName::GetLongPackageAssetName(PackagePath),
			RF_Public | RF_Standalone | RF_Transactional);

		if (NewAsset)
		{
			FAssetRegistryModule::AssetCreated(NewAsset);
			NewAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(NewAsset->GetPackage());
		}
		else
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to allocate asset `%s`."), *PackagePath));
		}
		return NewAsset;
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
		ConfigureText(Label, Text, FLinearColor(0.92f, 0.90f, 0.84f, 1.0f), 16, true);
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
		UTextBlock* GoldText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("GoldText"));
		UTextBlock* StatusText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("StatusText"));
		UVerticalBox* ItemBox = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("ItemBox"));

		if (!RootCanvas || !PanelSizeBox || !OuterBorder || !PanelStack || !TitleText || !GoldText || !StatusText || !ItemBox)
		{
			return;
		}

		WidgetTree->RootWidget = RootCanvas;
		PanelSizeBox->SetWidthOverride(520.0f);
		OuterBorder->SetBrushColor(FLinearColor(0.025f, 0.028f, 0.034f, 0.94f));
		OuterBorder->SetPadding(FMargin(18.0f, 16.0f));

		if (UCanvasPanelSlot* SizeSlot = RootCanvas->AddChildToCanvas(PanelSizeBox))
		{
			SizeSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			SizeSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			SizeSlot->SetAutoSize(true);
		}

		PanelSizeBox->AddChild(OuterBorder);
		OuterBorder->SetContent(PanelStack);

		ConfigureText(TitleText, TEXT("\u7b26\u6587\u5546\u5e97"), FLinearColor(0.95f, 0.78f, 0.38f, 1.0f), 24, false);
		ConfigureText(GoldText, TEXT("\u5f53\u524d\u91d1\u5e01"), FLinearColor(0.82f, 0.86f, 0.92f, 1.0f), 16, false);
		ConfigureText(StatusText, TEXT(""), FLinearColor(0.70f, 0.74f, 0.82f, 1.0f), 15, true);

		if (UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
		if (UVerticalBoxSlot* GoldSlot = PanelStack->AddChildToVerticalBox(GoldText))
		{
			GoldSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
		if (UVerticalBoxSlot* StatusSlot = PanelStack->AddChildToVerticalBox(StatusText))
		{
			StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		if (UVerticalBoxSlot* ItemSlot = PanelStack->AddChildToVerticalBox(ItemBox))
		{
			ItemSlot->SetPadding(FMargin(0.0f));
		}

		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FName ButtonName(*FString::Printf(TEXT("ItemButton%d"), Index));
			AddButtonToBox(ItemBox, MakeNamedButton(WidgetTree, ButtonName, FString::Printf(TEXT("\u5546\u54c1 %d"), Index + 1)), FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		if (UVerticalBoxSlot* CloseSlot = PanelStack->AddChildToVerticalBox(MakeNamedButton(WidgetTree, TEXT("BtnClose"), TEXT("\u5173\u95ed"))))
		{
			CloseSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
		}
	}

	UWidgetBlueprint* CreateWidgetBlueprint(bool bDryRun, TArray<FString>& ReportLines, bool& bCreated)
	{
		bCreated = false;
		if (UWidgetBlueprint* Existing = LoadAssetByPackagePath<UWidgetBlueprint>(WidgetPackagePath))
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
		Factory->ParentClass = UShopSelectionWidget::StaticClass();

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
			ReportLines.Add(TEXT("- Failed to create shop selection widget."));
		}
		return NewWidgetBlueprint;
	}

	bool ConfigureShopData(UShopDataAsset* ShopData, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (!ShopData)
		{
			return false;
		}

		ShopData->Modify();
		ShopData->StockPool.Reset();
		ShopData->MaxVisibleItems = 4;
		ShopData->bShuffleStockOnOpen = true;

		for (const FDefaultStockSpec& Spec : DefaultStock)
		{
			URuneDataAsset* Rune = LoadAssetByPackagePath<URuneDataAsset>(Spec.PackagePath);
			if (!Rune)
			{
				ReportLines.Add(FString::Printf(TEXT("- Missing stock rune `%s`."), *Spec.PackagePath));
				continue;
			}

			FShopRuneEntry Entry;
			Entry.RuneAsset = Rune;
			Entry.OverrideGoldCost = Spec.Cost;
			Entry.bSoldOutAfterPurchase = true;
			Entry.Note = FText::FromString(Spec.Note);
			ShopData->StockPool.Add(Entry);
		}

		ShopData->MarkPackageDirty();
		DirtyPackages.AddUnique(ShopData->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Configured shop stock entries: %d."), ShopData->StockPool.Num()));
		return ShopData->StockPool.Num() > 0;
	}

	void ConfigureShopRoomTags(URoomDataAsset* Room, TArray<FString>& ReportLines)
	{
		if (!Room)
		{
			return;
		}

		const TArray<FName> RemoveTypeTags = {
			FName(TEXT("Room.Type.Normal")),
			FName(TEXT("Room.Type.Elite")),
			FName(TEXT("Room.Type.Event"))
		};
		for (const FName TagName : RemoveTypeTags)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
			if (Tag.IsValid())
			{
				Room->RoomTags.RemoveTag(Tag);
			}
		}

		const FGameplayTag ShopTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Shop")), false);
		const FGameplayTag LayerTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Layer.L1")), false);
		if (ShopTag.IsValid())
		{
			Room->RoomTags.AddTag(ShopTag);
		}
		else
		{
			ReportLines.Add(TEXT("- Missing gameplay tag `Room.Type.Shop`."));
		}
		if (LayerTag.IsValid())
		{
			Room->RoomTags.AddTag(LayerTag);
		}
		else
		{
			ReportLines.Add(TEXT("- Missing gameplay tag `Room.Layer.L1`."));
		}
	}

	void EnsureDefaultPortals(URoomDataAsset* Room)
	{
		if (!Room || !Room->PortalDestinations.IsEmpty())
		{
			return;
		}

		for (int32 Index = 0; Index < 3; ++Index)
		{
			FPortalDestConfig Config;
			Config.PortalIndex = Index;
			Room->PortalDestinations.Add(Config);
		}
	}

	bool ConfigureShopRoom(
		URoomDataAsset* Room,
		UShopDataAsset* ShopData,
		UWidgetBlueprint* WidgetBlueprint,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (!Room || !ShopData)
		{
			return false;
		}

		Room->Modify();
		Room->RoomName = FName(TEXT("ShopRoom"));
		Room->DisplayName = FText::FromString(TEXT("\u7b26\u6587\u5546\u5e97"));
		ConfigureShopRoomTags(Room, ReportLines);
		Room->EnemyPool.Reset();
		Room->BuffPool.Reset();
		Room->LootPool.Reset();
		Room->bEnableTimedClearObjective = false;
		Room->SacrificeEventAltarData = nullptr;
		Room->SacrificeEventAltarClass = nullptr;
		Room->SacrificeEventWidgetClass = nullptr;
		Room->bIsHubRoom = false;
		Room->ShopData = ShopData;
		Room->ShopActorClass = nullptr;

		TSubclassOf<UShopSelectionWidget> WidgetClass = UShopSelectionWidget::StaticClass();
		if (WidgetBlueprint && WidgetBlueprint->GeneratedClass && WidgetBlueprint->GeneratedClass->IsChildOf(UShopSelectionWidget::StaticClass()))
		{
			WidgetClass = WidgetBlueprint->GeneratedClass;
		}
		Room->ShopWidgetClass = WidgetClass;
		Room->ShopSpawnOffset = FVector(260.0f, 0.0f, 0.0f);
		EnsureDefaultPortals(Room);

		Room->MarkPackageDirty();
		DirtyPackages.AddUnique(Room->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Configured shop room `%s`."), *ShopRoomPackagePath));
		return true;
	}

	void AddShopRoomToCampaign(URoomDataAsset* Room, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (!Room)
		{
			return;
		}

		UCampaignDataAsset* Campaign = LoadAssetByPackagePath<UCampaignDataAsset>(CampaignPackagePath);
		if (!Campaign)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing campaign `%s`; shop room not inserted into RoomPool."), *CampaignPackagePath));
			return;
		}

		const bool bAlreadyInPool = Campaign->RoomPool.ContainsByPredicate([Room](const TObjectPtr<URoomDataAsset>& Entry)
		{
			return Entry.Get() == Room;
		});
		if (bAlreadyInPool)
		{
			ReportLines.Add(FString::Printf(TEXT("- Campaign already contains `%s`."), *ShopRoomPackagePath));
			return;
		}

		Campaign->Modify();
		Campaign->RoomPool.Add(Room);
		Campaign->MarkPackageDirty();
		DirtyPackages.AddUnique(Campaign->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Added shop room to campaign `%s` RoomPool."), *CampaignPackagePath));
	}
}

UShopRoomSetupCommandlet::UShopRoomSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UShopRoomSetupCommandlet::Main(const FString& Params)
{
	using namespace ShopRoomSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Shop Room Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Shop data: `%s`"), *ShopDataPackagePath));
	ReportLines.Add(FString::Printf(TEXT("- Shop room: `%s`"), *ShopRoomPackagePath));
	ReportLines.Add(FString::Printf(TEXT("- Widget: `%s`"), *WidgetPackagePath));
	ReportLines.Add(TEXT(""));

	bool bCreatedWidget = false;
	UWidgetBlueprint* WidgetBlueprint = CreateWidgetBlueprint(bDryRun, ReportLines, bCreatedWidget);
	if (WidgetBlueprint && !bDryRun)
	{
		const bool bMissingDesignerBindings = !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("TitleText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("GoldText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("StatusText"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("ItemBox"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("ItemButton0"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnClose"));

		if (bCreatedWidget || bForceLayout || bMissingDesignerBindings)
		{
			BuildDesignerTree(WidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
			ReportLines.Add(TEXT("- Designer tree refreshed for shop selection WBP."));
		}
		else
		{
			ReportLines.Add(TEXT("- Existing shop selection WBP designer tree kept unchanged."));
		}
	}

	UShopDataAsset* ShopData = CreateOrLoadDataAsset<UShopDataAsset>(ShopDataPackagePath, bDryRun, ReportLines, DirtyPackages);
	bool bHasStock = false;
	if (!bDryRun && ShopData)
	{
		bHasStock = ConfigureShopData(ShopData, ReportLines, DirtyPackages);
	}
	else if (bDryRun)
	{
		ReportLines.Add(TEXT("- Would configure default 512 rune shop stock."));
		bHasStock = true;
	}

	URoomDataAsset* ShopRoom = CreateOrLoadDataAsset<URoomDataAsset>(ShopRoomPackagePath, bDryRun, ReportLines, DirtyPackages);
	if (!bDryRun && ShopRoom && ShopData)
	{
		ConfigureShopRoom(ShopRoom, ShopData, WidgetBlueprint, ReportLines, DirtyPackages);
		AddShopRoomToCampaign(ShopRoom, ReportLines, DirtyPackages);
	}
	else if (bDryRun)
	{
		ReportLines.Add(TEXT("- Would configure shop room and add it to main campaign RoomPool."));
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ShopRoomSetup::ReportFileName, ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Shop room setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return bHasStock ? 0 : 1;
}
