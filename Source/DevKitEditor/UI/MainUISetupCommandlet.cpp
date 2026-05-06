#include "UI/MainUISetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Styling/SlateBrush.h"
#include "UI/CurrentRoomBuffWidget.h"
#include "UI/LiquidHealthBarWidget.h"
#include "UI/PauseMenuWidget.h"
#include "UI/PlayerCommonInfoWidget.h"
#include "UI/YogHUD.h"
#include "UI/YogHUDRootWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace MainUISetup
{
	const FString HudWidgetPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_HUDRoot");
	const FString PlayerCommonInfoWidgetPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_PlayerCommonInfoHud");
	const FString PauseWidgetPath = TEXT("/Game/UI/Playtest_UI/Pause/WBP_PauseMenu");
	const FString PauseTextureRoot = TEXT("/Game/UI/Playtest_UI/UI_Tex/Pause");
	const FString HudTextureRoot = TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD");
	const FString FrontendTextureRoot = TEXT("/Game/UI/Playtest_UI/UI_Tex/Frontend");
	const FString ReportFileName = TEXT("MainUISetupReport.md");

	const TCHAR* PlayerHealthClassPath = TEXT("/Game/UI/WB_PlayerHealthBar.WB_PlayerHealthBar_C");
	const TCHAR* EnemyArrowClassPath = TEXT("/Game/UI/Playtest_UI/CombatInfo/WBP_EnemyArrow.WBP_EnemyArrow_C");
	const TCHAR* WeaponGlassClassPath = TEXT("/Game/UI/Playtest_UI/WeaponInfo/WBP_WeaponGlassIcon.WBP_WeaponGlassIcon_C");
	const TCHAR* InfoPopupClassPath = TEXT("/Game/UI/Playtest_UI/Tutorial/WBP_InfoPopup.WBP_InfoPopup_C");
	const TCHAR* CombatDeckClassPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_CombatDeckBar.WBP_CombatDeckBar_C");
	const TCHAR* CurrentRoomBuffClassPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_CurrentRoomBuffPanel.WBP_CurrentRoomBuffPanel_C");
	const TCHAR* PlayerCommonInfoClassPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_PlayerCommonInfoHud.WBP_PlayerCommonInfoHud_C");

	const TCHAR* PanelFrameTexturePath = TEXT("/Game/UI/Playtest_UI/UI_Tex/Pause/T_PausePanel_OrnateFrame.T_PausePanel_OrnateFrame");
	const TCHAR* DividerTexturePath = TEXT("/Game/UI/Playtest_UI/UI_Tex/Pause/T_PauseDivider_Ornate.T_PauseDivider_Ornate");
	const TCHAR* FocusTexturePath = TEXT("/Game/UI/Playtest_UI/UI_Tex/Pause/T_PauseFocusGlow.T_PauseFocusGlow");
	const TCHAR* GoldIconTexturePath = TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon.T_GoldCoinIcon");

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

	UTexture2D* LoadTexture(const TCHAR* ObjectPath)
	{
		return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, ObjectPath));
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

	UWidget* ConstructWidgetFromPath(UWidgetTree* WidgetTree, const TCHAR* ClassPath, const FName WidgetName, TArray<FString>& ReportLines, UClass* FallbackClass = nullptr)
	{
		UClass* WidgetClass = LoadClass<UWidget>(nullptr, ClassPath);
		if (!WidgetClass)
		{
			WidgetClass = FallbackClass;
			ReportLines.Add(FString::Printf(TEXT("- Missing widget class `%s`; using fallback for `%s`."), ClassPath, *WidgetName.ToString()));
		}

		if (!WidgetTree || !WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create `%s`."), *WidgetName.ToString()));
			return nullptr;
		}

		UWidget* Widget = WidgetTree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
		if (Widget)
		{
			Widget->bIsVariable = true;
		}
		return Widget;
	}

	void ConfigureCanvasSlot(UCanvasPanelSlot* Slot, const FAnchors& Anchors, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment, int32 ZOrder)
	{
		if (!Slot)
		{
			return;
		}

		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetAutoSize(false);
		Slot->SetZOrder(ZOrder);
	}

	void ConfigureText(UTextBlock* TextBlock, const FString& Text, const FLinearColor& Color, int32 Size, bool bWrap = false)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetAutoWrapText(bWrap);
		TextBlock->SetJustification(ETextJustify::Center);

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
	}

	void AddWidgetToOverlay(UOverlay* Overlay, UWidget* Child, EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& Padding = FMargin())
	{
		if (!Overlay || !Child)
		{
			return;
		}

		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Child))
		{
			Slot->SetHorizontalAlignment(HAlign);
			Slot->SetVerticalAlignment(VAlign);
			Slot->SetPadding(Padding);
		}
	}

	UOverlay* AddHudRegion(UWidgetTree* WidgetTree, UCanvasPanel* RootCanvas, const FName Name, const FAnchors& Anchors, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment, int32 ZOrder)
	{
		UOverlay* Region = ConstructNamedWidget<UOverlay>(WidgetTree, Name);
		if (!Region || !RootCanvas)
		{
			return nullptr;
		}

		Region->SetVisibility(ESlateVisibility::HitTestInvisible);
		ConfigureCanvasSlot(RootCanvas->AddChildToCanvas(Region), Anchors, Position, Size, Alignment, ZOrder);
		return Region;
	}

	void ResetWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		if (WidgetBlueprint->WidgetTree->RootWidget)
		{
			WidgetBlueprint->WidgetTree->RemoveWidget(WidgetBlueprint->WidgetTree->RootWidget);
			WidgetBlueprint->WidgetTree->RootWidget = nullptr;
		}
	}

	void BuildPlayerCommonInfoTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		USizeBox* RootSizeBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSizeBox"), false);
		UVerticalBox* CommonInfoList = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("CommonInfoList"));
		UHorizontalBox* GoldRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("GoldRow"));
		USizeBox* GoldIconBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("GoldIconBox"), false);
		UImage* GoldIcon = ConstructNamedWidget<UImage>(WidgetTree, TEXT("GoldIcon"));
		UTextBlock* GoldText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("GoldText"));

		if (!RootSizeBox || !CommonInfoList || !GoldRow || !GoldIconBox || !GoldIcon || !GoldText)
		{
			return;
		}

		RootSizeBox->SetWidthOverride(180.0f);
		RootSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->AddChild(CommonInfoList);

		GoldRow->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UVerticalBoxSlot* GoldRowSlot = CommonInfoList->AddChildToVerticalBox(GoldRow))
		{
			GoldRowSlot->SetHorizontalAlignment(HAlign_Right);
			GoldRowSlot->SetPadding(FMargin(0.0f, 2.0f));
		}

		GoldIconBox->SetWidthOverride(24.0f);
		GoldIconBox->SetHeightOverride(24.0f);
		GoldIconBox->AddChild(GoldIcon);
		if (UHorizontalBoxSlot* IconSlot = GoldRow->AddChildToHorizontalBox(GoldIconBox))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		}

		if (UTexture2D* GoldTexture = LoadTexture(GoldIconTexturePath))
		{
			FSlateBrush GoldBrush;
			GoldBrush.SetResourceObject(GoldTexture);
			GoldBrush.ImageSize = FVector2D(24.0f, 24.0f);
			GoldBrush.DrawAs = ESlateBrushDrawType::Image;
			GoldIcon->SetBrush(GoldBrush);
		}
		GoldIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

		ConfigureText(GoldText, TEXT("0"), FLinearColor(0.95f, 0.83f, 0.42f, 1.0f), 18, false);
		GoldText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		GoldText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		if (UHorizontalBoxSlot* TextSlot = GoldRow->AddChildToHorizontalBox(GoldText))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		ReportLines.Add(TEXT("- Player common info HUD designer tree refreshed with gold row and extensible entry list."));
	}

	void BuildHudTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UCanvasPanel* RootCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("RootCanvas"));
		if (!RootCanvas)
		{
			return;
		}

		RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = RootCanvas;

		if (UWidget* EnemyArrow = ConstructWidgetFromPath(WidgetTree, EnemyArrowClassPath, TEXT("EnemyArrow"), ReportLines))
		{
			EnemyArrow->SetVisibility(ESlateVisibility::HitTestInvisible);
			ConfigureCanvasSlot(RootCanvas->AddChildToCanvas(EnemyArrow), FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, 0);
		}

		UOverlay* TopLeft = AddHudRegion(WidgetTree, RootCanvas, TEXT("TopLeftPlayerInfoRegion"), FAnchors(0.f, 0.f), FVector2D(16.f, 16.f), FVector2D(380.f, 128.f), FVector2D(0.f, 0.f), 5);
		UOverlay* TopRight = AddHudRegion(WidgetTree, RootCanvas, TEXT("TopRightPlayerInfoRegion"), FAnchors(1.f, 0.f), FVector2D(-16.f, 16.f), FVector2D(460.f, 132.f), FVector2D(1.f, 0.f), 5);
		UOverlay* Boss = AddHudRegion(WidgetTree, RootCanvas, TEXT("BossInfoRegion"), FAnchors(0.5f, 0.f), FVector2D(0.f, 16.f), FVector2D(980.f, 118.f), FVector2D(0.5f, 0.f), 6);
		UOverlay* LeftLevel = AddHudRegion(WidgetTree, RootCanvas, TEXT("LeftLevelInfoRegion"), FAnchors(0.f, 0.5f), FVector2D(24.f, 0.f), FVector2D(360.f, 560.f), FVector2D(0.f, 0.5f), 5);
		UOverlay* RightLevel = AddHudRegion(WidgetTree, RootCanvas, TEXT("RightLevelInfoRegion"), FAnchors(1.f, 0.5f), FVector2D(-24.f, 0.f), FVector2D(360.f, 560.f), FVector2D(1.f, 0.5f), 5);
		UOverlay* BottomLeft = AddHudRegion(WidgetTree, RootCanvas, TEXT("BottomLeftPlayerInfoRegion"), FAnchors(0.f, 1.f), FVector2D(24.f, -24.f), FVector2D(440.f, 150.f), FVector2D(0.f, 1.f), 8);
		UOverlay* BottomCenter = AddHudRegion(WidgetTree, RootCanvas, TEXT("BottomCenterCombatRegion"), FAnchors(0.5f, 1.f), FVector2D(0.f, -24.f), FVector2D(900.f, 180.f), FVector2D(0.5f, 1.f), 8);
		UOverlay* BottomRight = AddHudRegion(WidgetTree, RootCanvas, TEXT("BottomRightPlayerInfoRegion"), FAnchors(1.f, 1.f), FVector2D(-24.f, -24.f), FVector2D(420.f, 150.f), FVector2D(1.f, 1.f), 8);

		if (Boss)
		{
			Boss->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (TopLeft)
		{
			TopLeft->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (TopRight)
		{
			TopRight->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (RightLevel)
		{
			RightLevel->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (BottomRight)
		{
			BottomRight->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		UWidget* PlayerCommonInfo = ConstructWidgetFromPath(
			WidgetTree,
			PlayerCommonInfoClassPath,
			TEXT("PlayerCommonInfoHud"),
			ReportLines,
			UPlayerCommonInfoWidget::StaticClass());
		AddWidgetToOverlay(BottomRight, PlayerCommonInfo, HAlign_Right, VAlign_Top, FMargin(0.0f, 8.0f, 8.0f, 0.0f));

		USizeBox* PlayerHealthHost = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("PlayerHealthHost"), false);
		PlayerHealthHost->SetWidthOverride(420.f);
		PlayerHealthHost->SetHeightOverride(82.f);
		if (UWidget* PlayerHealth = ConstructWidgetFromPath(WidgetTree, PlayerHealthClassPath, TEXT("PlayerHealthBar"), ReportLines, ULiquidHealthBarWidget::StaticClass()))
		{
			PlayerHealthHost->AddChild(PlayerHealth);
		}
		AddWidgetToOverlay(BottomLeft, PlayerHealthHost, HAlign_Left, VAlign_Bottom);

		UWidget* WeaponGlass = ConstructWidgetFromPath(WidgetTree, WeaponGlassClassPath, TEXT("WeaponGlassIcon"), ReportLines);
		AddWidgetToOverlay(BottomLeft, WeaponGlass, HAlign_Left, VAlign_Bottom, FMargin(0.f, 0.f, 0.f, 28.f));

		USizeBox* CombatDeckHost = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("CombatDeckHost"), false);
		CombatDeckHost->SetWidthOverride(900.f);
		CombatDeckHost->SetHeightOverride(180.f);
		if (UWidget* CombatDeck = ConstructWidgetFromPath(WidgetTree, CombatDeckClassPath, TEXT("CombatDeckBar"), ReportLines))
		{
			CombatDeckHost->AddChild(CombatDeck);
		}
		AddWidgetToOverlay(BottomCenter, CombatDeckHost, HAlign_Center, VAlign_Bottom);

		USizeBox* CurrentRoomBuffHost = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("CurrentRoomBuffPanelHost"), false);
		CurrentRoomBuffHost->SetWidthOverride(360.f);
		CurrentRoomBuffHost->SetHeightOverride(560.f);
		if (UWidget* CurrentRoomBuff = ConstructWidgetFromPath(WidgetTree, CurrentRoomBuffClassPath, TEXT("CurrentRoomBuffPanel"), ReportLines, UCurrentRoomBuffWidget::StaticClass()))
		{
			CurrentRoomBuffHost->AddChild(CurrentRoomBuff);
		}
		AddWidgetToOverlay(LeftLevel, CurrentRoomBuffHost, HAlign_Left, VAlign_Center);

		if (UWidget* InfoPopup = ConstructWidgetFromPath(WidgetTree, InfoPopupClassPath, TEXT("InfoPopup"), ReportLines))
		{
			InfoPopup->SetVisibility(ESlateVisibility::HitTestInvisible);
			ConfigureCanvasSlot(RootCanvas->AddChildToCanvas(InfoPopup), FAnchors(0.5f, 0.5f), FVector2D(0.f, 360.f), FVector2D(600.f, 50.f), FVector2D(0.5f, 0.5f), 20);
		}

		ReportLines.Add(TEXT("- HUD designer tree refreshed with named regions and existing HUD widgets."));
	}

	void ConfigureButton(UButton* Button, UTexture2D* FocusTexture)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		Style.Normal.TintColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.25f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.18f, 0.35f, 0.55f, 0.78f));
		Style.Hovered.TintColor = FSlateColor(FLinearColor(0.12f, 0.30f, 0.54f, 0.78f));
		if (FocusTexture)
		{
			Style.Hovered.SetResourceObject(FocusTexture);
			Style.Hovered.ImageSize = FVector2D(560.f, 54.f);
			Style.Hovered.DrawAs = ESlateBrushDrawType::Image;
			Style.Pressed.SetResourceObject(FocusTexture);
			Style.Pressed.ImageSize = FVector2D(560.f, 54.f);
			Style.Pressed.DrawAs = ESlateBrushDrawType::Image;
		}
		Button->SetStyle(Style);
	}

	UButton* AddPauseButton(UWidgetTree* WidgetTree, UVerticalBox* ButtonList, const FName ButtonName, const FName TextName, const FString& Label, UTexture2D* FocusTexture)
	{
		UButton* Button = ConstructNamedWidget<UButton>(WidgetTree, ButtonName);
		UTextBlock* LabelText = ConstructNamedWidget<UTextBlock>(WidgetTree, TextName);
		if (!Button || !LabelText || !ButtonList)
		{
			return Button;
		}

		ConfigureButton(Button, FocusTexture);
		ConfigureText(LabelText, Label, FLinearColor(0.78f, 0.80f, 0.82f, 1.f), 28, false);
		Button->SetContent(LabelText);

		if (UVerticalBoxSlot* Slot = ButtonList->AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(0.f, 4.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}

		return Button;
	}

	void AddDivider(UWidgetTree* WidgetTree, UVerticalBox* Stack, const FName Name, UTexture2D* DividerTexture)
	{
		if (!Stack)
		{
			return;
		}

		UWidget* DividerWidget = nullptr;
		if (DividerTexture)
		{
			UImage* DividerImage = ConstructNamedWidget<UImage>(WidgetTree, Name);
			FSlateBrush Brush;
			Brush.SetResourceObject(DividerTexture);
			Brush.ImageSize = FVector2D(620.f, 24.f);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			DividerImage->SetBrush(Brush);
			DividerWidget = DividerImage;
		}
		else
		{
			USizeBox* DividerBox = ConstructNamedWidget<USizeBox>(WidgetTree, Name);
			DividerBox->SetHeightOverride(2.f);
			UBorder* DividerBorder = ConstructNamedWidget<UBorder>(WidgetTree, FName(*(Name.ToString() + TEXT("Fill"))), false);
			DividerBorder->SetBrushColor(FLinearColor(0.8f, 0.78f, 0.68f, 0.32f));
			DividerBox->AddChild(DividerBorder);
			DividerWidget = DividerBox;
		}

		if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(DividerWidget))
		{
			Slot->SetPadding(FMargin(0.f, 8.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
	}

	void BuildPauseTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UTexture2D* PanelFrameTexture = LoadTexture(PanelFrameTexturePath);
		UTexture2D* DividerTexture = LoadTexture(DividerTexturePath);
		UTexture2D* FocusTexture = LoadTexture(FocusTexturePath);

		UOverlay* RootOverlay = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("RootOverlay"));
		UBorder* BackgroundDim = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("BackgroundDim"));
		USizeBox* PanelSizeBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("PanelSizeBox"));
		UOverlay* PanelOverlay = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("PanelOverlay"));
		UImage* PanelFrameImage = ConstructNamedWidget<UImage>(WidgetTree, TEXT("PanelFrameImage"));
		UBorder* PanelBorder = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("PanelBorder"));
		UVerticalBox* MenuStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("MenuStack"));
		UTextBlock* TitleText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("TitleText"));
		UVerticalBox* ButtonList = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("ButtonList"));
		UTextBlock* DescriptionText = ConstructNamedWidget<UTextBlock>(WidgetTree, TEXT("DescriptionText"));

		if (!RootOverlay || !BackgroundDim || !PanelSizeBox || !PanelOverlay || !PanelFrameImage || !PanelBorder || !MenuStack || !TitleText || !ButtonList || !DescriptionText)
		{
			return;
		}

		WidgetTree->RootWidget = RootOverlay;
		RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		BackgroundDim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.36f));
		AddWidgetToOverlay(RootOverlay, BackgroundDim, HAlign_Fill, VAlign_Fill);

		PanelSizeBox->SetWidthOverride(760.f);
		PanelSizeBox->SetHeightOverride(560.f);
		AddWidgetToOverlay(RootOverlay, PanelSizeBox, HAlign_Center, VAlign_Center);
		PanelSizeBox->AddChild(PanelOverlay);

		if (PanelFrameTexture)
		{
			FSlateBrush FrameBrush;
			FrameBrush.SetResourceObject(PanelFrameTexture);
			FrameBrush.ImageSize = FVector2D(760.f, 560.f);
			FrameBrush.DrawAs = ESlateBrushDrawType::Image;
			PanelFrameImage->SetBrush(FrameBrush);
			AddWidgetToOverlay(PanelOverlay, PanelFrameImage, HAlign_Fill, VAlign_Fill);
		}

		PanelBorder->SetBrushColor(FLinearColor(0.015f, 0.015f, 0.018f, 0.76f));
		PanelBorder->SetPadding(FMargin(92.f, 54.f, 92.f, 44.f));
		AddWidgetToOverlay(PanelOverlay, PanelBorder, HAlign_Fill, VAlign_Fill);
		PanelBorder->SetContent(MenuStack);

		ConfigureText(TitleText, TEXT("Destiny Paused"), FLinearColor(0.72f, 0.73f, 0.76f, 1.f), 34, false);
		if (UVerticalBoxSlot* TitleSlot = MenuStack->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		AddDivider(WidgetTree, MenuStack, TEXT("TopDivider"), DividerTexture);

		if (UVerticalBoxSlot* ButtonListSlot = MenuStack->AddChildToVerticalBox(ButtonList))
		{
			ButtonListSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonListSlot->SetPadding(FMargin(58.f, 0.f));
		}

		AddPauseButton(WidgetTree, ButtonList, TEXT("BtnControl"), TEXT("BtnControlText"), TEXT("Control"), FocusTexture);
		AddPauseButton(WidgetTree, ButtonList, TEXT("BtnDisplay"), TEXT("BtnDisplayText"), TEXT("Display"), FocusTexture);
		AddPauseButton(WidgetTree, ButtonList, TEXT("BtnSound"), TEXT("BtnSoundText"), TEXT("Sound"), FocusTexture);
		AddPauseButton(WidgetTree, ButtonList, TEXT("BtnSave"), TEXT("BtnSaveText"), TEXT("Save"), FocusTexture);
		AddPauseButton(WidgetTree, ButtonList, TEXT("BtnQuit"), TEXT("BtnQuitText"), TEXT("Quit"), FocusTexture);

		AddDivider(WidgetTree, MenuStack, TEXT("BottomDivider"), DividerTexture);

		ConfigureText(DescriptionText, TEXT("Quit the game and back to the Main Menu."), FLinearColor(0.62f, 0.64f, 0.68f, 1.f), 20, true);
		if (UVerticalBoxSlot* DescSlot = MenuStack->AddChildToVerticalBox(DescriptionText))
		{
			DescSlot->SetHorizontalAlignment(HAlign_Center);
			DescSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		}

		ReportLines.Add(TEXT("- Pause menu designer tree refreshed with controller-focusable buttons."));
	}

	UWidgetBlueprint* CreateWidgetBlueprint(const FString& PackagePath, UClass* ParentClass, bool bDryRun, TArray<FString>& ReportLines, bool& bCreated)
	{
		bCreated = false;
		if (UWidgetBlueprint* Existing = LoadWidgetBlueprint(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found existing `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = ParentClass;

		UObject* NewAsset = AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(PackagePath),
			FPackageName::GetLongPackagePath(PackagePath),
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
			ReportLines.Add(FString::Printf(TEXT("- Failed to create `%s`."), *PackagePath));
		}

		return WidgetBlueprint;
	}

	bool HudNeedsRefresh(UWidgetBlueprint* WidgetBlueprint)
	{
		return !WidgetBlueprint || !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("TopLeftPlayerInfoRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("TopRightPlayerInfoRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BossInfoRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("LeftLevelInfoRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("RightLevelInfoRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BottomLeftPlayerInfoRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BottomCenterCombatRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BottomRightPlayerInfoRegion"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("PlayerHealthBar"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("CombatDeckBar"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("PlayerCommonInfoHud"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("CurrentRoomBuffPanel"));
	}

	bool PlayerCommonInfoNeedsRefresh(UWidgetBlueprint* WidgetBlueprint)
	{
		return !WidgetBlueprint || !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("CommonInfoList"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("GoldRow"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("GoldIcon"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("GoldText"));
	}

	bool PauseNeedsRefresh(UWidgetBlueprint* WidgetBlueprint)
	{
		return !WidgetBlueprint || !WidgetBlueprint->WidgetTree
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnControl"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnDisplay"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnSound"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnSave"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("BtnQuit"))
			|| !WidgetBlueprint->WidgetTree->FindWidget(TEXT("DescriptionText"));
	}

	void ImportTextures(const FString& SectionTitle, const FString& SourceSubdir, const FString& DestinationPath, const TArray<FString>& FileNames, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		const FString SourceDir = FPaths::Combine(FPaths::ProjectDir(), SourceSubdir);

		ReportLines.Add(SectionTitle);
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (const FString& FileName : FileNames)
		{
			const FString SourceFile = FPaths::Combine(SourceDir, FileName);
			if (!FPaths::FileExists(SourceFile))
			{
				ReportLines.Add(FString::Printf(TEXT("- Missing source PNG `%s`."), *SourceFile));
				continue;
			}

			ReportLines.Add(FString::Printf(TEXT("- %s `%s` -> `%s`."), bDryRun ? TEXT("Would import/reimport") : TEXT("Imported/reimported"), *SourceFile, *DestinationPath));
			if (bDryRun)
			{
				continue;
			}

			UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
			ImportData->DestinationPath = DestinationPath;
			ImportData->Filenames.Add(SourceFile);
			ImportData->bReplaceExisting = true;

			const TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);
			for (UObject* ImportedAsset : ImportedAssets)
			{
				if (ImportedAsset)
				{
					if (UTexture2D* Texture = Cast<UTexture2D>(ImportedAsset))
					{
						Texture->Modify();
						Texture->CompressionSettings = TC_EditorIcon;
						Texture->LODGroup = TEXTUREGROUP_UI;
						Texture->MipGenSettings = TMGS_NoMipmaps;
						Texture->NeverStream = true;
						Texture->SRGB = true;
						Texture->PostEditChange();
					}
					ImportedAsset->MarkPackageDirty();
					DirtyPackages.AddUnique(ImportedAsset->GetPackage());
				}
			}
		}
	}

	void ImportPauseTextures(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ImportTextures(
			TEXT("## Pause texture import"),
			TEXT("SourceArt/UI/Pause"),
			PauseTextureRoot,
			{
				TEXT("T_PausePanel_OrnateFrame.png"),
				TEXT("T_PauseDivider_Ornate.png"),
				TEXT("T_PauseFocusGlow.png")
			},
			bDryRun,
			ReportLines,
			DirtyPackages);
	}

	void ImportHudTextures(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ImportTextures(
			TEXT("## HUD texture import"),
			TEXT("SourceArt/UI/HUD"),
			HudTextureRoot,
			{
				TEXT("T_GoldCoinIcon.png")
			},
			bDryRun,
			ReportLines,
			DirtyPackages);
	}

	void ImportFrontendTextures(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ImportTextures(
			TEXT("## Frontend texture import"),
			TEXT("SourceArt/UI/Frontend"),
			FrontendTextureRoot,
			{
				TEXT("T_MainMenu_Dungeon.png")
			},
			bDryRun,
			ReportLines,
			DirtyPackages);
	}

	void AssignHudBlueprintDefaults(UWidgetBlueprint* HudWidget, UWidgetBlueprint* PauseWidget, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
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

			ReportLines.Add(FString::Printf(TEXT("- %s defaults on `%s`."), bDryRun ? TEXT("Would update") : TEXT("Updated"), *CandidatePath));
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
			if (HudWidget && HudWidget->GeneratedClass)
			{
				HudCDO->MainHUDClass = TSubclassOf<UYogHUDRootWidget>(HudWidget->GeneratedClass);
			}
			if (PauseWidget && PauseWidget->GeneratedClass)
			{
				HudCDO->PauseMenuClass = TSubclassOf<UPauseMenuWidget>(PauseWidget->GeneratedClass);
			}
			HudBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(HudBlueprint->GetPackage());
			return;
		}

		ReportLines.Add(TEXT("- No AYogHUD blueprint found to update."));
	}
}

UMainUISetupCommandlet::UMainUISetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMainUISetupCommandlet::Main(const FString& Params)
{
	using namespace MainUISetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bForceLayout = Params.Contains(TEXT("ForceLayout"), ESearchCase::IgnoreCase);
	const bool bFrontendOnly = Params.Contains(TEXT("FrontendOnly"), ESearchCase::IgnoreCase);
	const bool bHudOnly = Params.Contains(TEXT("HudOnly"), ESearchCase::IgnoreCase);
	const bool bHudRootOnly = Params.Contains(TEXT("HudRootOnly"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Main UI Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- ForceLayout: %s"), bForceLayout ? TEXT("true") : TEXT("false")));
	ReportLines.Add(FString::Printf(TEXT("- FrontendOnly: %s"), bFrontendOnly ? TEXT("true") : TEXT("false")));
	ReportLines.Add(FString::Printf(TEXT("- HudOnly: %s"), bHudOnly ? TEXT("true") : TEXT("false")));
	ReportLines.Add(FString::Printf(TEXT("- HudRootOnly: %s"), bHudRootOnly ? TEXT("true") : TEXT("false")));
	ReportLines.Add(TEXT(""));

	if (!bFrontendOnly && !bHudOnly && !bHudRootOnly)
	{
		ImportPauseTextures(bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}
	if (!bFrontendOnly && !bHudRootOnly)
	{
		ImportHudTextures(bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}
	if (!bHudOnly && !bHudRootOnly)
	{
		ImportFrontendTextures(bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}

	if (bFrontendOnly)
	{
		if (!bDryRun && DirtyPackages.Num() > 0)
		{
			UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
		}

		const FString TextureOnlyReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), ReportFileName);
		FFileHelper::SaveStringToFile(
			FString::Join(ReportLines, LINE_TERMINATOR),
			*TextureOnlyReportPath,
			FFileHelper::EEncodingOptions::ForceUTF8);

		UE_LOG(LogTemp, Display, TEXT("Main UI setup finished. Report: %s"), *TextureOnlyReportPath);
		return 0;
	}

	if (!bHudRootOnly)
	{
		bool bPlayerCommonInfoCreated = false;
		UWidgetBlueprint* PlayerCommonInfoWidget = CreateWidgetBlueprint(
			PlayerCommonInfoWidgetPath,
			UPlayerCommonInfoWidget::StaticClass(),
			bDryRun,
			ReportLines,
			bPlayerCommonInfoCreated);
		if (PlayerCommonInfoWidget && !bDryRun && (bPlayerCommonInfoCreated || bForceLayout || PlayerCommonInfoNeedsRefresh(PlayerCommonInfoWidget)))
		{
			BuildPlayerCommonInfoTree(PlayerCommonInfoWidget, ReportLines);
			FKismetEditorUtilities::CompileBlueprint(PlayerCommonInfoWidget);
			PlayerCommonInfoWidget->MarkPackageDirty();
			DirtyPackages.AddUnique(PlayerCommonInfoWidget->GetPackage());
		}
		else if (PlayerCommonInfoWidget && !bDryRun)
		{
			ReportLines.Add(TEXT("- Existing player common info HUD designer tree kept unchanged."));
		}
	}

	bool bHudCreated = false;
	UWidgetBlueprint* HudWidget = CreateWidgetBlueprint(HudWidgetPath, UYogHUDRootWidget::StaticClass(), bDryRun, ReportLines, bHudCreated);
	if (HudWidget && !bDryRun && (bHudCreated || bForceLayout || HudNeedsRefresh(HudWidget)))
	{
		BuildHudTree(HudWidget, ReportLines);
		FKismetEditorUtilities::CompileBlueprint(HudWidget);
		HudWidget->MarkPackageDirty();
		DirtyPackages.AddUnique(HudWidget->GetPackage());
	}
	else if (HudWidget && !bDryRun)
	{
		ReportLines.Add(TEXT("- Existing HUD designer tree kept unchanged."));
	}

	UWidgetBlueprint* PauseWidget = nullptr;
	if (!bHudOnly && !bHudRootOnly)
	{
		bool bPauseCreated = false;
		PauseWidget = CreateWidgetBlueprint(PauseWidgetPath, UPauseMenuWidget::StaticClass(), bDryRun, ReportLines, bPauseCreated);
		if (PauseWidget && !bDryRun && (bPauseCreated || bForceLayout || PauseNeedsRefresh(PauseWidget)))
		{
			BuildPauseTree(PauseWidget, ReportLines);
			FKismetEditorUtilities::CompileBlueprint(PauseWidget);
			PauseWidget->MarkPackageDirty();
			DirtyPackages.AddUnique(PauseWidget->GetPackage());
		}
		else if (PauseWidget && !bDryRun)
		{
			ReportLines.Add(TEXT("- Existing pause designer tree kept unchanged."));
		}
	}

	if (bHudRootOnly)
	{
		ReportLines.Add(TEXT("## HUD blueprint defaults"));
		ReportLines.Add(TEXT("- Skipped because HudRootOnly was requested."));
	}
	else if (!bDryRun)
	{
		AssignHudBlueprintDefaults(HudWidget, PauseWidget, bDryRun, ReportLines, DirtyPackages);
	}
	else
	{
		AssignHudBlueprintDefaults(HudWidget, PauseWidget, bDryRun, ReportLines, DirtyPackages);
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

	UE_LOG(LogTemp, Display, TEXT("Main UI setup finished. Report: %s"), *ReportPath);
	return 0;
}
