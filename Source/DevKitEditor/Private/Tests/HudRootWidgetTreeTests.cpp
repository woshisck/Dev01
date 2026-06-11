#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/LiquidHealthBarWidget.h"
#include "UI/YogCommonRichTextBlock.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace
{
	TOptional<ETextJustify::Type> ReadTextJustification(const UWidget* Widget)
	{
		if (!Widget)
		{
			return {};
		}

		if (const FByteProperty* ByteProperty = FindFProperty<FByteProperty>(Widget->GetClass(), TEXT("Justification")))
		{
			return static_cast<ETextJustify::Type>(ByteProperty->GetPropertyValue_InContainer(Widget));
		}

		if (const FEnumProperty* EnumProperty = FindFProperty<FEnumProperty>(Widget->GetClass(), TEXT("Justification")))
		{
			if (const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
			{
				return static_cast<ETextJustify::Type>(UnderlyingProperty->GetUnsignedIntPropertyValue(
					EnumProperty->ContainerPtrToValuePtr<void>(Widget)));
			}
		}

		return {};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudRootPlayerHealthBarBlueprintBindingTest,
	"DevKitEditor.UI.HUD.PlayerHealthBarUsesDesignerBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHudRootPlayerHealthBarBlueprintBindingTest::RunTest(const FString& Parameters)
{
	const TCHAR* PlayerHealthClassPath = TEXT("/Game/UI/Widget/WB_PlayerHealthBar.WB_PlayerHealthBar_C");
	const TCHAR* PlayerHealthBlueprintPath = TEXT("/Game/UI/Widget/WB_PlayerHealthBar.WB_PlayerHealthBar");
	const TCHAR* HudBlueprintPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_HUDRoot.WBP_HUDRoot");

	UClass* ExpectedHealthClass = LoadClass<ULiquidHealthBarWidget>(nullptr, PlayerHealthClassPath);
	if (!TestNotNull(TEXT("Designer player health bar blueprint class loads"), ExpectedHealthClass))
	{
		return false;
	}

	UWidgetBlueprint* PlayerHealthBlueprint = LoadObject<UWidgetBlueprint>(nullptr, PlayerHealthBlueprintPath);
	if (!TestNotNull(TEXT("Designer player health bar blueprint loads"), PlayerHealthBlueprint))
	{
		return false;
	}

	UWidget* LiquidFillImage = PlayerHealthBlueprint->WidgetTree
		? PlayerHealthBlueprint->WidgetTree->FindWidget(TEXT("LiquidFillImage"))
		: nullptr;
	if (!TestNotNull(TEXT("Designer player health bar contains LiquidFillImage"), LiquidFillImage))
	{
		return false;
	}
	const bool bLiquidFillImageIsImage = LiquidFillImage->IsA<UImage>();
	TestTrue(TEXT("LiquidFillImage is an image widget so the native health bar can drive its brush material"),
		bLiquidFillImageIsImage);

	UWidgetBlueprint* HudBlueprint = LoadObject<UWidgetBlueprint>(nullptr, HudBlueprintPath);
	if (!TestNotNull(TEXT("HUD root widget blueprint loads"), HudBlueprint))
	{
		return false;
	}

	UWidgetTree* WidgetTree = HudBlueprint->WidgetTree;
	if (!TestNotNull(TEXT("HUD root has a designer widget tree"), WidgetTree))
	{
		return false;
	}

	UWidget* PlayerHealthBar = WidgetTree->FindWidget(TEXT("PlayerHealthBar"));
	if (!TestNotNull(TEXT("HUD root contains PlayerHealthBar"), PlayerHealthBar))
	{
		return false;
	}

	const bool bUsesDesignerBlueprint = PlayerHealthBar->GetClass()->IsChildOf(ExpectedHealthClass);
	if (!bUsesDesignerBlueprint)
	{
		AddError(FString::Printf(
			TEXT("PlayerHealthBar is `%s`, expected `%s` so the LiquidFillImage binding exists at runtime."),
			*GetNameSafe(PlayerHealthBar->GetClass()),
			*GetNameSafe(ExpectedHealthClass)));
	}

	TestTrue(TEXT("PlayerHealthBar uses the designer blueprint with bound LiquidFillImage"), bUsesDesignerBlueprint);
	return bLiquidFillImageIsImage && bUsesDesignerBlueprint;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudRootCombatDeckMountedTopLeftTest,
	"DevKitEditor.UI.HUD.CombatDeckMountedTopLeft",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHudRootCombatDeckMountedTopLeftTest::RunTest(const FString& Parameters)
{
	const TCHAR* HudBlueprintPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_HUDRoot.WBP_HUDRoot");

	UWidgetBlueprint* HudBlueprint = LoadObject<UWidgetBlueprint>(nullptr, HudBlueprintPath);
	if (!TestNotNull(TEXT("HUD root widget blueprint loads"), HudBlueprint))
	{
		return false;
	}

	UWidgetTree* WidgetTree = HudBlueprint->WidgetTree;
	if (!TestNotNull(TEXT("HUD root has a designer widget tree"), WidgetTree))
	{
		return false;
	}

	UWidget* TopLeftRegion = WidgetTree->FindWidget(TEXT("TopLeftPlayerInfoRegion"));
	UWidget* BottomCenterRegion = WidgetTree->FindWidget(TEXT("BottomCenterCombatRegion"));
	UWidget* CombatDeckHost = WidgetTree->FindWidget(TEXT("CombatDeckHost"));
	UWidget* CombatDeckBar = WidgetTree->FindWidget(TEXT("CombatDeckBar"));
	UWidget* ComboPanel = WidgetTree->FindWidget(TEXT("WeaponComboListPanel"));

	bool bValid = true;
	bValid &= TestNotNull(TEXT("HUD root contains TopLeftPlayerInfoRegion"), TopLeftRegion);
	bValid &= TestNotNull(TEXT("HUD root contains BottomCenterCombatRegion as an empty reserved region"), BottomCenterRegion);
	bValid &= TestNotNull(TEXT("HUD root contains CombatDeckHost"), CombatDeckHost);
	bValid &= TestNotNull(TEXT("HUD root contains CombatDeckBar"), CombatDeckBar);
	bValid &= TestNull(TEXT("Legacy WeaponComboListPanel is not generated for the current no-combo combat design"), ComboPanel);

	if (UOverlay* TopLeftOverlay = Cast<UOverlay>(TopLeftRegion))
	{
		bValid &= TestTrue(TEXT("CombatDeckHost is mounted in the top-left HUD region"),
			CombatDeckHost && TopLeftOverlay->GetChildIndex(CombatDeckHost) != INDEX_NONE);
	}

	if (UOverlay* BottomCenterOverlay = Cast<UOverlay>(BottomCenterRegion))
	{
		bValid &= TestEqual(TEXT("BottomCenterCombatRegion no longer hosts the combat deck during normal play"),
			BottomCenterOverlay->GetChildIndex(CombatDeckHost),
			INDEX_NONE);
	}

	if (USizeBox* DeckHostSize = Cast<USizeBox>(CombatDeckHost))
	{
		bValid &= TestEqual(TEXT("CombatDeckHost uses the compact top-left width"),
			DeckHostSize->GetWidthOverride(),
			520.0f,
			0.001f);
		bValid &= TestEqual(TEXT("CombatDeckHost uses the compact top-left height"),
			DeckHostSize->GetHeightOverride(),
			96.0f,
			0.001f);
	}

	return bValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudRootPlayerBuffBarPlacementTest,
	"DevKitEditor.UI.HUD.PlayerBuffBarSitsAboveHealthBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHudRootPlayerBuffBarPlacementTest::RunTest(const FString& Parameters)
{
	const TCHAR* HudBlueprintPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_HUDRoot.WBP_HUDRoot");

	UWidgetBlueprint* HudBlueprint = LoadObject<UWidgetBlueprint>(nullptr, HudBlueprintPath);
	if (!TestNotNull(TEXT("HUD root widget blueprint loads"), HudBlueprint))
	{
		return false;
	}

	UWidgetTree* WidgetTree = HudBlueprint->WidgetTree;
	if (!TestNotNull(TEXT("HUD root has a designer widget tree"), WidgetTree))
	{
		return false;
	}

	UWidget* BottomLeftRegion = WidgetTree->FindWidget(TEXT("BottomLeftPlayerInfoRegion"));
	UWidget* PlayerHealthHost = WidgetTree->FindWidget(TEXT("PlayerHealthHost"));
	UWidget* PlayerBuffBarHost = WidgetTree->FindWidget(TEXT("PlayerBuffBarHost"));
	UWidget* PlayerBuffBar = WidgetTree->FindWidget(TEXT("PlayerBuffBar"));

	bool bValid = true;
	bValid &= TestNotNull(TEXT("HUD root contains BottomLeftPlayerInfoRegion"), BottomLeftRegion);
	bValid &= TestNotNull(TEXT("HUD root contains PlayerHealthHost"), PlayerHealthHost);
	bValid &= TestNotNull(TEXT("HUD root contains PlayerBuffBarHost"), PlayerBuffBarHost);
	bValid &= TestNotNull(TEXT("HUD root contains PlayerBuffBar"), PlayerBuffBar);

	if (PlayerBuffBarHost)
	{
		bValid &= TestTrue(TEXT("PlayerBuffBarHost uses a fixed size box so buff placeholders do not resize the health cluster"),
			PlayerBuffBarHost->IsA<USizeBox>());

		if (UOverlaySlot* BuffSlot = Cast<UOverlaySlot>(PlayerBuffBarHost->Slot))
		{
			bValid &= TestEqual(TEXT("PlayerBuffBarHost aligns with the left edge of the bottom-left HUD region"),
				BuffSlot->GetHorizontalAlignment(),
				HAlign_Left);
			bValid &= TestEqual(TEXT("PlayerBuffBarHost is anchored from the bottom near the health bar"),
				BuffSlot->GetVerticalAlignment(),
				VAlign_Bottom);
			bValid &= TestTrue(TEXT("PlayerBuffBarHost sits above PlayerHealthHost with positive bottom padding"),
				BuffSlot->GetPadding().Bottom >= 82.f);
		}
		else
		{
			AddError(TEXT("PlayerBuffBarHost must be mounted in BottomLeftPlayerInfoRegion as an overlay child."));
			bValid = false;
		}
	}

	if (PlayerHealthHost)
	{
		if (UOverlaySlot* HealthSlot = Cast<UOverlaySlot>(PlayerHealthHost->Slot))
		{
			bValid &= TestEqual(TEXT("PlayerHealthHost remains bottom aligned"),
				HealthSlot->GetVerticalAlignment(),
				VAlign_Bottom);
		}
	}

	return bValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewRewardIconBoxBlueprintBindingTest,
	"DevKitEditor.UI.PortalPreview.RewardIconBoxBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewRewardIconBoxBlueprintBindingTest::RunTest(const FString& Parameters)
{
	const TCHAR* PortalPreviewBlueprintPath = TEXT("/Game/UI/Playtest_UI/Portal/WBP_PortalPreview.WBP_PortalPreview");

	UWidgetBlueprint* PortalPreviewBlueprint = LoadObject<UWidgetBlueprint>(nullptr, PortalPreviewBlueprintPath);
	if (!TestNotNull(TEXT("Portal preview widget blueprint loads"), PortalPreviewBlueprint))
	{
		return false;
	}

	UWidgetTree* WidgetTree = PortalPreviewBlueprint->WidgetTree;
	if (!TestNotNull(TEXT("Portal preview has a designer widget tree"), WidgetTree))
	{
		return false;
	}

	UWidget* LootIconBox = WidgetTree->FindWidget(TEXT("LootIconBox"));
	UWidget* LootSummaryText = WidgetTree->FindWidget(TEXT("LootSummaryText"));

	bool bValid = true;
	bValid &= TestNotNull(TEXT("Portal preview contains LootIconBox"), LootIconBox);
	bValid &= TestNotNull(TEXT("Portal preview keeps LootSummaryText as native fallback"), LootSummaryText);

	if (LootIconBox)
	{
		bValid &= TestTrue(TEXT("LootIconBox is a horizontal reward icon row"),
			LootIconBox->IsA<UHorizontalBox>());
	}

	if (LootSummaryText)
	{
		bValid &= TestTrue(TEXT("LootSummaryText is hidden by default because rewards render as icons"),
			LootSummaryText->GetVisibility() == ESlateVisibility::Collapsed);
		bValid &= TestTrue(TEXT("LootSummaryText remains a text block fallback"),
			LootSummaryText->IsA<UTextBlock>());
	}

	return bValid;
}

#endif
