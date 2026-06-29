#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/YogGraphicsSettingsWidgetBase.h"
#include "System/YogPerformanceSettingsLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogPerformanceSettingsCustomClampTest,
	"DevKit.Performance.Settings.MakesClampedCustomSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogPerformanceSettingsCustomClampTest::RunTest(const FString& Parameters)
{
	FYogGraphicsSettings BaseSettings = UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Ultra);

	const FYogGraphicsSettings CustomSettings =
		UYogPerformanceSettingsLibrary::MakeCustomGraphicsSettings(BaseSettings, 999.f, 500, false, true);

	TestEqual(TEXT("Custom settings are marked as Custom"), CustomSettings.PerformanceProfile, EYogPerformanceProfile::Custom);
	TestEqual(TEXT("Custom settings clear the target tier"), CustomSettings.SelectedTargetTier, EYogPerformanceTargetTier::Custom);
	TestEqual(TEXT("Resolution scale is clamped to the runtime max"), CustomSettings.ResolutionScalePercent, 100.f);
	TestEqual(TEXT("Frame limit is clamped to the save max"), CustomSettings.FrameRateLimit, 240);
	TestFalse(TEXT("Lumen Lite toggle is applied"), CustomSettings.bUseLumenLite);
	TestTrue(TEXT("Batch proxy preference toggle is applied"), CustomSettings.bPreferBatchedGeometryProxies);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogPerformanceTargetTierTest,
	"DevKit.Performance.Settings.TargetTierMappings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogPerformanceTargetTierTest::RunTest(const FString& Parameters)
{
	const FYogGraphicsSettings PCUltra =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::PCUltra);
	TestEqual(TEXT("PC Ultra records the selected target tier"), PCUltra.SelectedTargetTier, EYogPerformanceTargetTier::PCUltra);
	TestEqual(TEXT("PC Ultra uses the Ultra base profile"), PCUltra.PerformanceProfile, EYogPerformanceProfile::Ultra);
	TestTrue(TEXT("PC Ultra keeps Lumen enabled"), PCUltra.bUseLumenLite);
	TestFalse(TEXT("PC Ultra keeps Nanite disabled"), PCUltra.bUseNanite);
	TestFalse(TEXT("PC Ultra keeps VSM disabled"), PCUltra.bUseVirtualShadowMaps);

	const FYogGraphicsSettings SteamDeck15W =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::SteamDeck15W);
	TestEqual(TEXT("Steam Deck 15W records the selected target tier"), SteamDeck15W.SelectedTargetTier, EYogPerformanceTargetTier::SteamDeck15W);
	TestEqual(TEXT("Steam Deck 15W uses the Medium base profile"), SteamDeck15W.PerformanceProfile, EYogPerformanceProfile::Medium);
	TestTrue(TEXT("Steam Deck 15W enables Lumen Lite for testing"), SteamDeck15W.bUseLumenLite);
	TestTrue(TEXT("Steam Deck 15W prefers batch proxies"), SteamDeck15W.bPreferBatchedGeometryProxies);
	TestFalse(TEXT("Steam Deck 15W keeps Nanite disabled"), SteamDeck15W.bUseNanite);
	TestFalse(TEXT("Steam Deck 15W keeps VSM disabled"), SteamDeck15W.bUseVirtualShadowMaps);

	const FYogGraphicsSettings Switch2Candidate =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::Switch2Candidate);
	TestEqual(TEXT("Switch 2 candidate records the selected target tier"), Switch2Candidate.SelectedTargetTier, EYogPerformanceTargetTier::Switch2Candidate);
	TestEqual(TEXT("Switch 2 candidate uses the Medium base profile"), Switch2Candidate.PerformanceProfile, EYogPerformanceProfile::Medium);
	TestTrue(TEXT("Switch 2 candidate enables Lumen Lite for testing"), Switch2Candidate.bUseLumenLite);
	TestTrue(TEXT("Switch 2 candidate prefers batch proxies"), Switch2Candidate.bPreferBatchedGeometryProxies);
	TestFalse(TEXT("Switch 2 candidate keeps Nanite disabled"), Switch2Candidate.bUseNanite);
	TestFalse(TEXT("Switch 2 candidate keeps VSM disabled"), Switch2Candidate.bUseVirtualShadowMaps);

	const FYogGraphicsSettings SteamDeck5W =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::SteamDeck5W);
	TestEqual(TEXT("Steam Deck 5W records the selected target tier"), SteamDeck5W.SelectedTargetTier, EYogPerformanceTargetTier::SteamDeck5W);
	TestEqual(TEXT("Steam Deck 5W uses the Low base profile"), SteamDeck5W.PerformanceProfile, EYogPerformanceProfile::Low);
	TestFalse(TEXT("Steam Deck 5W disables Lumen"), SteamDeck5W.bUseLumenLite);
	TestTrue(TEXT("Steam Deck 5W prefers batch proxies"), SteamDeck5W.bPreferBatchedGeometryProxies);

	const FYogGraphicsSettings FallbackLow =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::FallbackLow);
	TestEqual(TEXT("Fallback low records the selected target tier"), FallbackLow.SelectedTargetTier, EYogPerformanceTargetTier::FallbackLow);
	TestEqual(TEXT("Fallback low uses the Low base profile"), FallbackLow.PerformanceProfile, EYogPerformanceProfile::Low);
	TestFalse(TEXT("Fallback low disables Lumen"), FallbackLow.bUseLumenLite);
	TestEqual(TEXT("Fallback low uses the lowest texture quality"), FallbackLow.TextureQuality, 0);

	const TArray<EYogPerformanceTargetTier> TargetTiers = UYogPerformanceSettingsLibrary::GetSelectablePerformanceTargetTiers();
	TestTrue(TEXT("Selectable target tiers include PC Ultra"), TargetTiers.Contains(EYogPerformanceTargetTier::PCUltra));
	TestTrue(TEXT("Selectable target tiers include Steam Deck 15W"), TargetTiers.Contains(EYogPerformanceTargetTier::SteamDeck15W));
	TestTrue(TEXT("Selectable target tiers include Switch 2 candidate"), TargetTiers.Contains(EYogPerformanceTargetTier::Switch2Candidate));
	TestTrue(TEXT("Selectable target tiers include Steam Deck 5W"), TargetTiers.Contains(EYogPerformanceTargetTier::SteamDeck5W));
	TestTrue(TEXT("Selectable target tiers include fallback low"), TargetTiers.Contains(EYogPerformanceTargetTier::FallbackLow));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogGraphicsSettingsWidgetContractTest,
	"DevKit.Performance.Settings.GraphicsSettingsWidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogGraphicsSettingsWidgetContractTest::RunTest(const FString& Parameters)
{
	const TArray<FName> RequiredWidgets = UYogGraphicsSettingsWidgetBase::GetRequiredDesignerWidgetNames();

	TestTrue(TEXT("Graphics settings widget requires a Low preset button"), RequiredWidgets.Contains(TEXT("BtnProfileLow")));
	TestTrue(TEXT("Graphics settings widget requires a Medium preset button"), RequiredWidgets.Contains(TEXT("BtnProfileMedium")));
	TestTrue(TEXT("Graphics settings widget requires a High preset button"), RequiredWidgets.Contains(TEXT("BtnProfileHigh")));
	TestTrue(TEXT("Graphics settings widget requires an Ultra preset button"), RequiredWidgets.Contains(TEXT("BtnProfileUltra")));
	TestTrue(TEXT("Graphics settings widget requires a PC Ultra target button"), RequiredWidgets.Contains(TEXT("BtnTargetPCUltra")));
	TestTrue(TEXT("Graphics settings widget requires a Steam Deck 15W target button"), RequiredWidgets.Contains(TEXT("BtnTargetSteamDeck15W")));
	TestTrue(TEXT("Graphics settings widget requires a Switch 2 target button"), RequiredWidgets.Contains(TEXT("BtnTargetSwitch2Candidate")));
	TestTrue(TEXT("Graphics settings widget requires a Steam Deck 5W target button"), RequiredWidgets.Contains(TEXT("BtnTargetSteamDeck5W")));
	TestTrue(TEXT("Graphics settings widget requires a fallback target button"), RequiredWidgets.Contains(TEXT("BtnTargetFallbackLow")));
	TestTrue(TEXT("Graphics settings widget requires an Apply Custom button"), RequiredWidgets.Contains(TEXT("BtnApplyCustom")));
	TestTrue(TEXT("Graphics settings widget requires a Back button"), RequiredWidgets.Contains(TEXT("BtnBack")));
	TestTrue(TEXT("Graphics settings widget requires a resolution scale text field"), RequiredWidgets.Contains(TEXT("ResolutionScaleText")));
	TestTrue(TEXT("Graphics settings widget requires a model quality slider"), RequiredWidgets.Contains(TEXT("ModelQualitySlider")));
	TestTrue(TEXT("Graphics settings widget requires a shadow quality slider"), RequiredWidgets.Contains(TEXT("ShadowQualitySlider")));
	TestTrue(TEXT("Graphics settings widget requires a reflection quality slider"), RequiredWidgets.Contains(TEXT("ReflectionQualitySlider")));
	TestTrue(TEXT("Graphics settings widget requires a texture quality slider"), RequiredWidgets.Contains(TEXT("TextureQualitySlider")));
	TestTrue(TEXT("Graphics settings widget requires a material quality slider"), RequiredWidgets.Contains(TEXT("MaterialQualitySlider")));
	TestTrue(TEXT("Graphics settings widget requires a dynamic light quality slider"), RequiredWidgets.Contains(TEXT("DynamicLightQualitySlider")));
	TestTrue(TEXT("Graphics settings widget requires a material light quality slider"), RequiredWidgets.Contains(TEXT("MaterialLightQualitySlider")));
	TestEqual(TEXT("Graphics settings widget routes default controller focus to Apply Custom"),
		UYogGraphicsSettingsWidgetBase::GetDefaultFocusWidgetName(),
		FName(TEXT("BtnApplyCustom")));
	TestTrue(TEXT("Default focus target is a required designer widget"),
		RequiredWidgets.Contains(UYogGraphicsSettingsWidgetBase::GetDefaultFocusWidgetName()));

	FYogGraphicsSettings SavedSettings = UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Medium);
	SavedSettings.SelectedTargetTier = EYogPerformanceTargetTier::Switch2Candidate;
	SavedSettings.ResolutionScalePercent = 73.f;
	SavedSettings.FrameRateLimit = 40;
	SavedSettings.bUseLumenLite = false;

	UYogGraphicsSettingsWidgetBase* Widget = NewObject<UYogGraphicsSettingsWidgetBase>();
	TestNotNull(TEXT("Graphics settings widget base is constructible"), Widget);
	Widget->SetPendingSettings(SavedSettings);
	const FYogGraphicsSettings InitialPendingSettings = Widget->GetPendingSettings();
	TestEqual(TEXT("Widget stores pending target tier before customization"), InitialPendingSettings.SelectedTargetTier, EYogPerformanceTargetTier::Switch2Candidate);

	Widget->SetPendingModelQuality(3);
	Widget->SetPendingShadowQuality(2);
	Widget->SetPendingReflectionQuality(1);
	Widget->SetPendingTextureQuality(0);
	Widget->SetPendingMaterialQuality(2);
	Widget->SetPendingDynamicLightQuality(1);
	Widget->SetPendingMaterialLightQuality(2);

	const FYogGraphicsSettings PendingSettings = Widget->GetPendingSettings();
	TestEqual(TEXT("Widget stores pending resolution scale"), PendingSettings.ResolutionScalePercent, 73.f);
	TestEqual(TEXT("Widget stores pending frame limit"), PendingSettings.FrameRateLimit, 40);
	TestFalse(TEXT("Widget stores pending Lumen Lite toggle"), PendingSettings.bUseLumenLite);
	TestEqual(TEXT("Widget stores pending model quality as view distance"), PendingSettings.ViewDistanceQuality, 3);
	TestEqual(TEXT("Widget mirrors model quality to foliage"), PendingSettings.FoliageQuality, 3);
	TestEqual(TEXT("Widget stores pending shadow quality"), PendingSettings.ShadowQuality, 2);
	TestEqual(TEXT("Widget stores pending reflection quality"), PendingSettings.ReflectionQuality, 1);
	TestEqual(TEXT("Widget stores pending texture quality"), PendingSettings.TextureQuality, 0);
	TestEqual(TEXT("Widget stores pending material quality as shading"), PendingSettings.ShadingQuality, 2);
	TestEqual(TEXT("Widget mirrors material quality to effects"), PendingSettings.EffectsQuality, 2);
	TestEqual(TEXT("Widget stores pending dynamic light quality"), PendingSettings.DynamicLightQuality, 1);
	TestEqual(TEXT("Widget stores pending material light quality"), PendingSettings.MaterialLightQuality, 2);
	TestEqual(TEXT("Widget maps material light quality to the max LightInfo count"), PendingSettings.MaterialLightMaxLightInfoCount, 2);
	TestEqual(TEXT("Quality sliders mark settings as custom"), PendingSettings.PerformanceProfile, EYogPerformanceProfile::Custom);
	TestEqual(TEXT("Quality sliders clear the target tier"), PendingSettings.SelectedTargetTier, EYogPerformanceTargetTier::Custom);

	return true;
}

#endif
