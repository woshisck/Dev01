#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "UI/LiquidHealthBarWidget.h"
#include "UI/YogCommonRichTextBlock.h"
#include "WidgetBlueprint.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudRootWeaponComboListBlueprintBindingTest,
	"DevKitEditor.UI.HUD.WeaponComboListInTopRight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHudRootWeaponComboListBlueprintBindingTest::RunTest(const FString& Parameters)
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

	UWidget* TopRightRegion = WidgetTree->FindWidget(TEXT("TopRightPlayerInfoRegion"));
	UWidget* ComboPanel = WidgetTree->FindWidget(TEXT("WeaponComboListPanel"));
	UWidget* ComboText = WidgetTree->FindWidget(TEXT("WeaponComboListText"));

	bool bValid = true;
	bValid &= TestNotNull(TEXT("HUD root contains TopRightPlayerInfoRegion"), TopRightRegion);
	bValid &= TestNotNull(TEXT("HUD root contains WeaponComboListPanel"), ComboPanel);
	bValid &= TestNotNull(TEXT("HUD root contains WeaponComboListText"), ComboText);

	if (ComboText)
	{
		bValid &= TestTrue(TEXT("WeaponComboListText uses rich text so input icons can render"),
			ComboText->IsA<UYogCommonRichTextBlock>());
	}

	if (UPanelWidget* TopRightPanel = Cast<UPanelWidget>(TopRightRegion))
	{
		bValid &= TestTrue(TEXT("WeaponComboListPanel is placed inside TopRightPlayerInfoRegion"),
			ComboPanel && TopRightPanel->GetChildIndex(ComboPanel) != INDEX_NONE);
	}

	return bValid;
}

#endif
