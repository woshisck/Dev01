#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudRootWeaponComboListBlueprintBindingTest,
	"DevKitEditor.UI.HUD.WeaponComboListRightAligned",
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

	UWidget* TopLeftRegion = WidgetTree->FindWidget(TEXT("TopLeftPlayerInfoRegion"));
	UWidget* RootCanvas = WidgetTree->FindWidget(TEXT("RootCanvas"));
	UWidget* ComboPanel = WidgetTree->FindWidget(TEXT("WeaponComboListPanel"));
	UWidget* ComboTitle = WidgetTree->FindWidget(TEXT("WeaponComboListTitle"));
	UWidget* ComboText = WidgetTree->FindWidget(TEXT("WeaponComboListText"));

	bool bValid = true;
	bValid &= TestNotNull(TEXT("HUD root contains RootCanvas"), RootCanvas);
	bValid &= TestNotNull(TEXT("HUD root contains TopLeftPlayerInfoRegion"), TopLeftRegion);
	bValid &= TestNotNull(TEXT("HUD root contains WeaponComboListPanel"), ComboPanel);
	bValid &= TestNotNull(TEXT("HUD root contains WeaponComboListTitle"), ComboTitle);
	bValid &= TestNotNull(TEXT("HUD root contains WeaponComboListText"), ComboText);

	if (ComboTitle)
	{
		bValid &= TestEqual(TEXT("WeaponComboListTitle text is right aligned"),
			ReadTextJustification(ComboTitle).Get(ETextJustify::Left),
			ETextJustify::Right);
	}

	if (ComboText)
	{
		bValid &= TestTrue(TEXT("WeaponComboListText uses rich text so input icons can render"),
			ComboText->IsA<UYogCommonRichTextBlock>());
		bValid &= TestNotEqual(TEXT("WeaponComboListText does not clip combo lines"),
			ComboText->GetClipping(),
			EWidgetClipping::ClipToBounds);
		bValid &= TestEqual(TEXT("WeaponComboListText text is right aligned"),
			ReadTextJustification(ComboText).Get(ETextJustify::Left),
			ETextJustify::Right);
	}

	if (UOverlay* TopLeftOverlay = Cast<UOverlay>(TopLeftRegion))
	{
		bValid &= TestTrue(TEXT("WeaponComboListPanel is mounted in the top-left weapon cluster"),
			ComboPanel && TopLeftOverlay->GetChildIndex(ComboPanel) != INDEX_NONE);
	}

	if (ComboPanel)
	{
		bValid &= TestNotEqual(TEXT("WeaponComboListPanel does not clip combo lines"),
			ComboPanel->GetClipping(),
			EWidgetClipping::ClipToBounds);

		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ComboPanel->Slot))
		{
			bValid &= TestEqual(TEXT("WeaponComboListPanel aligns with the top-left weapon cluster"),
				OverlaySlot->GetHorizontalAlignment(),
				HAlign_Left);
			bValid &= TestEqual(TEXT("WeaponComboListPanel sits near the top of the weapon cluster"),
				OverlaySlot->GetVerticalAlignment(),
				VAlign_Top);
			bValid &= TestEqual(TEXT("WeaponComboListPanel leaves room for weapon slots"),
				OverlaySlot->GetPadding().Left,
				216.0f,
				0.001f);
		}
	}

	return bValid;
}


#endif
