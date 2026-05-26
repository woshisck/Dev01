#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
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

	UWidget* TopRightRegion = WidgetTree->FindWidget(TEXT("TopRightPlayerInfoRegion"));
	UWidget* RootCanvas = WidgetTree->FindWidget(TEXT("RootCanvas"));
	UWidget* ComboPanel = WidgetTree->FindWidget(TEXT("WeaponComboListPanel"));
	UWidget* ComboTitle = WidgetTree->FindWidget(TEXT("WeaponComboListTitle"));
	UWidget* ComboText = WidgetTree->FindWidget(TEXT("WeaponComboListText"));

	bool bValid = true;
	bValid &= TestNotNull(TEXT("HUD root contains RootCanvas"), RootCanvas);
	bValid &= TestNotNull(TEXT("HUD root contains TopRightPlayerInfoRegion"), TopRightRegion);
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

	if (UCanvasPanel* RootCanvasPanel = Cast<UCanvasPanel>(RootCanvas))
	{
		bValid &= TestTrue(TEXT("WeaponComboListPanel is mounted directly on RootCanvas so it cannot resize or shift level float panels"),
			ComboPanel && RootCanvasPanel->GetChildIndex(ComboPanel) != INDEX_NONE);
	}

	if (ComboPanel)
	{
		bValid &= TestNotEqual(TEXT("WeaponComboListPanel does not clip combo lines"),
			ComboPanel->GetClipping(),
			EWidgetClipping::ClipToBounds);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ComboPanel->Slot))
		{
			const FAnchors Anchors = CanvasSlot->GetAnchors();
			bValid &= TestEqual(TEXT("WeaponComboListPanel anchors to the top-right corner"),
				Anchors.Minimum,
				FVector2D(1.f, 0.f));
			bValid &= TestEqual(TEXT("WeaponComboListPanel max anchor stays top-right"),
				Anchors.Maximum,
				FVector2D(1.f, 0.f));
			bValid &= TestEqual(TEXT("WeaponComboListPanel aligns its right edge to the anchor"),
				CanvasSlot->GetAlignment(),
				FVector2D(1.f, 0.f));
			bValid &= TestEqual(TEXT("WeaponComboListPanel right edge sits near the screen edge"),
				CanvasSlot->GetPosition().X,
				-16.0,
				0.001);
			bValid &= TestEqual(TEXT("WeaponComboListPanel uses the requested compact width"),
				CanvasSlot->GetSize().X,
				460.0,
				0.001);
			bValid &= TestTrue(TEXT("WeaponComboListPanel has enough vertical room for long weapon combo lists"),
				CanvasSlot->GetSize().Y >= 300.f);
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
