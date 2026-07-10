#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/YogGraphicsSettingsWidgetBase.h"
#include "System/YogPerformanceSettingsLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogPerformanceSettingsCustomClampTest,
	"DevKit.Performance.Settings.MakesClampedCustomSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogPerformanceSettingsCustomClampTest::RunTest(const FString& Parameters)
{
	FYogGraphicsSettings BaseSettings = UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Epic);

	const FYogGraphicsSettings CustomSettings =
		UYogPerformanceSettingsLibrary::MakeCustomGraphicsSettings(BaseSettings, 999.f, 500, false, true);

	TestEqual(TEXT("Custom settings are marked as Custom"), CustomSettings.PerformanceProfile, EYogPerformanceProfile::Custom);
	TestEqual(TEXT("Custom settings clear the target tier"), CustomSettings.SelectedTargetTier, EYogPerformanceTargetTier::Custom);
	TestEqual(TEXT("Resolution scale is clamped to the runtime max"), CustomSettings.ResolutionScalePercent, 100.f);
	TestEqual(TEXT("Frame limit is clamped to the save max"), CustomSettings.FrameRateLimit, 240);
	TestFalse(TEXT("Lumen Lite toggle is applied"), CustomSettings.bUseLumenLite);
	TestTrue(TEXT("Batch proxy preference toggle is applied"), CustomSettings.bPreferBatchedGeometryProxies);
	TestEqual(TEXT("Custom settings preserve material quality"), CustomSettings.MaterialQuality, BaseSettings.MaterialQuality);
	TestEqual(TEXT("Custom settings preserve texture collection quality"), CustomSettings.TextureCollectionQuality, BaseSettings.TextureCollectionQuality);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogPerformanceTargetTierTest,
	"DevKit.Performance.Settings.TargetTierMappings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogPerformanceTargetTierTest::RunTest(const FString& Parameters)
{
	const FYogGraphicsSettings Epic =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::Epic);
	TestEqual(TEXT("Epic records the selected target tier"), Epic.SelectedTargetTier, EYogPerformanceTargetTier::Epic);
	TestEqual(TEXT("Epic uses the Epic base profile"), Epic.PerformanceProfile, EYogPerformanceProfile::Epic);
	TestTrue(TEXT("Epic keeps Lumen enabled"), Epic.bUseLumenLite);
	TestFalse(TEXT("Epic keeps Nanite disabled"), Epic.bUseNanite);
	TestFalse(TEXT("Epic keeps VSM disabled"), Epic.bUseVirtualShadowMaps);
	TestFalse(TEXT("Epic is source-biased"), Epic.bPreferBatchedGeometryProxies);
	TestEqual(TEXT("Epic uses the highest material quality"), Epic.MaterialQuality, 3);
	TestEqual(TEXT("Epic uses the highest texture collection quality"), Epic.TextureCollectionQuality, 3);
	TestEqual(TEXT("Epic maps to UE material quality Epic"), UYogPerformanceSettingsLibrary::GetNativeMaterialQualityLevelForProjectMaterialQuality(Epic.MaterialQuality), 3);

	const FYogGraphicsSettings High =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::High);
	TestEqual(TEXT("High records the selected target tier"), High.SelectedTargetTier, EYogPerformanceTargetTier::High);
	TestEqual(TEXT("High uses the High base profile"), High.PerformanceProfile, EYogPerformanceProfile::High);
	TestTrue(TEXT("High keeps Lumen enabled"), High.bUseLumenLite);
	TestFalse(TEXT("High is still source-biased by default"), High.bPreferBatchedGeometryProxies);
	TestEqual(TEXT("High uses high material quality"), High.MaterialQuality, 2);
	TestEqual(TEXT("High uses high texture collection quality"), High.TextureCollectionQuality, 2);
	TestEqual(TEXT("High maps to UE material quality High"), UYogPerformanceSettingsLibrary::GetNativeMaterialQualityLevelForProjectMaterialQuality(High.MaterialQuality), 1);

	const FYogGraphicsSettings Mid =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::Mid);
	TestEqual(TEXT("Mid records the selected target tier"), Mid.SelectedTargetTier, EYogPerformanceTargetTier::Mid);
	TestEqual(TEXT("Mid uses the Mid base profile"), Mid.PerformanceProfile, EYogPerformanceProfile::Mid);
	TestTrue(TEXT("Mid enables Lumen Lite for validation"), Mid.bUseLumenLite);
	TestEqual(TEXT("Mid uses the standard UE5.8 Lumen Lite GI tier"), Mid.GlobalIlluminationQuality, 1);
	TestEqual(TEXT("Mid keeps local exposure available through the High post process tier"), Mid.PostProcessQuality, 2);
	TestTrue(TEXT("Mid prefers batch proxies"), Mid.bPreferBatchedGeometryProxies);
	TestEqual(TEXT("Mid uses standard material quality"), Mid.MaterialQuality, 1);
	TestEqual(TEXT("Mid uses standard texture collection quality"), Mid.TextureCollectionQuality, 1);
	TestEqual(TEXT("Mid maps to UE material quality Medium"), UYogPerformanceSettingsLibrary::GetNativeMaterialQualityLevelForProjectMaterialQuality(Mid.MaterialQuality), 2);

	const FYogGraphicsSettings Low =
		UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier::Low);
	TestEqual(TEXT("Low records the selected target tier"), Low.SelectedTargetTier, EYogPerformanceTargetTier::Low);
	TestEqual(TEXT("Low uses the Low base profile"), Low.PerformanceProfile, EYogPerformanceProfile::Low);
	TestTrue(TEXT("Low uses the Lumen Lite floor by default"), Low.bUseLumenLite);
	TestEqual(TEXT("Low uses the standard UE5.8 Lumen Lite GI tier"), Low.GlobalIlluminationQuality, 1);
	TestEqual(TEXT("Low keeps local exposure available through the Low post process tier"), Low.PostProcessQuality, 1);
	TestTrue(TEXT("Low prefers batch proxies"), Low.bPreferBatchedGeometryProxies);
	TestEqual(TEXT("Low keeps one material-light entry for scene readability"), Low.MaterialLightMaxLightInfoCount, 1);
	TestEqual(TEXT("Low uses the lowest dynamic overlay quality"), Low.DynamicOverlayQuality, 0);
	TestEqual(TEXT("Low maps to UE material quality Low"), UYogPerformanceSettingsLibrary::GetNativeMaterialQualityLevelForProjectMaterialQuality(Low.MaterialQuality), 0);

	const TArray<EYogPerformanceTargetTier> TargetTiers = UYogPerformanceSettingsLibrary::GetSelectablePerformanceTargetTiers();
	TestEqual(TEXT("Selectable target tiers expose only the four formal tiers"), TargetTiers.Num(), 4);
	TestTrue(TEXT("Selectable target tiers include Epic"), TargetTiers.Contains(EYogPerformanceTargetTier::Epic));
	TestTrue(TEXT("Selectable target tiers include High"), TargetTiers.Contains(EYogPerformanceTargetTier::High));
	TestTrue(TEXT("Selectable target tiers include Mid"), TargetTiers.Contains(EYogPerformanceTargetTier::Mid));
	TestTrue(TEXT("Selectable target tiers include Low"), TargetTiers.Contains(EYogPerformanceTargetTier::Low));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogMaterialPerformanceInterfaceTest,
	"DevKit.Performance.Settings.MaterialTierInterface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogMaterialPerformanceInterfaceTest::RunTest(const FString& Parameters)
{
	const FYogMaterialPerformanceTierInterface Epic =
		UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier::Epic);
	TestEqual(TEXT("Epic material interface exposes native UE material quality"), Epic.NativeMaterialQualityLevel, 3);
	TestEqual(TEXT("Epic material interface keeps three unique texture sets"), Epic.MaxUniqueTextureSets, 3);
	TestEqual(TEXT("Epic material interface keeps three runtime blend layers"), Epic.MaxRuntimeBlendLayers, 3);
	TestEqual(TEXT("Epic material interface keeps two runtime overlay layers"), Epic.MaxRuntimeOverlayLayers, 2);
	TestTrue(TEXT("Epic material interface prefers source master material"), Epic.bPreferSourceMasterMaterial);
	TestFalse(TEXT("Epic material interface does not prefer baked material"), Epic.bPreferBakedMaterial);
	TestTrue(TEXT("Epic material interface uses texture collection"), Epic.bUseTextureCollection);
	TestFalse(TEXT("Epic material interface does not use legacy VT atlas"), Epic.bUseVTAtlas);

	const FYogMaterialPerformanceTierInterface High =
		UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier::High);
	TestEqual(TEXT("High material interface exposes native UE material quality"), High.NativeMaterialQualityLevel, 1);
	TestEqual(TEXT("High material interface keeps three unique texture sets"), High.MaxUniqueTextureSets, 3);
	TestEqual(TEXT("High material interface keeps one runtime overlay layer"), High.MaxRuntimeOverlayLayers, 1);
	TestTrue(TEXT("High material interface still prefers source master material"), High.bPreferSourceMasterMaterial);

	const FYogMaterialPerformanceTierInterface Mid =
		UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier::Mid);
	TestEqual(TEXT("Mid material interface exposes native UE material quality"), Mid.NativeMaterialQualityLevel, 2);
	TestEqual(TEXT("Mid material interface reduces to two unique texture sets"), Mid.MaxUniqueTextureSets, 2);
	TestEqual(TEXT("Mid material interface reduces to two runtime blend layers"), Mid.MaxRuntimeBlendLayers, 2);
	TestEqual(TEXT("Mid material interface keeps one runtime overlay layer"), Mid.MaxRuntimeOverlayLayers, 1);
	TestFalse(TEXT("Mid material interface does not prefer source master material"), Mid.bPreferSourceMasterMaterial);
	TestTrue(TEXT("Mid material interface prefers baked material"), Mid.bPreferBakedMaterial);

	const FYogMaterialPerformanceTierInterface Low =
		UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier::Low);
	TestEqual(TEXT("Low material interface exposes native UE material quality"), Low.NativeMaterialQualityLevel, 0);
	TestEqual(TEXT("Low material interface reduces to one unique texture set"), Low.MaxUniqueTextureSets, 1);
	TestEqual(TEXT("Low material interface reduces to one runtime blend layer"), Low.MaxRuntimeBlendLayers, 1);
	TestEqual(TEXT("Low material interface disables runtime overlay layers"), Low.MaxRuntimeOverlayLayers, 0);
	TestFalse(TEXT("Low material interface disables ground RVT overlay"), Low.bAllowGroundRuntimeRVTOverlay);
	TestFalse(TEXT("Wall RVT overlay is not enabled by default"), Low.bAllowWallRuntimeRVTOverlay);
	TestTrue(TEXT("Low material interface prefers baked material"), Low.bPreferBakedMaterial);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogGraphicsSettingsWidgetContractTest,
	"DevKit.Performance.Settings.GraphicsSettingsWidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogGraphicsSettingsWidgetContractTest::RunTest(const FString& Parameters)
{
	const TArray<FName> RequiredWidgets = UYogGraphicsSettingsWidgetBase::GetRequiredDesignerWidgetNames();

	TestTrue(TEXT("Graphics settings widget requires an Epic tier button"), RequiredWidgets.Contains(TEXT("BtnTierEpic")));
	TestTrue(TEXT("Graphics settings widget requires a High tier button"), RequiredWidgets.Contains(TEXT("BtnTierHigh")));
	TestTrue(TEXT("Graphics settings widget requires a Mid tier button"), RequiredWidgets.Contains(TEXT("BtnTierMid")));
	TestTrue(TEXT("Graphics settings widget requires a Low tier button"), RequiredWidgets.Contains(TEXT("BtnTierLow")));
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

	FYogGraphicsSettings SavedSettings = UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Mid);
	SavedSettings.SelectedTargetTier = EYogPerformanceTargetTier::Mid;
	SavedSettings.ResolutionScalePercent = 73.f;
	SavedSettings.FrameRateLimit = 40;
	SavedSettings.bUseLumenLite = false;

	UYogGraphicsSettingsWidgetBase* Widget = NewObject<UYogGraphicsSettingsWidgetBase>();
	TestNotNull(TEXT("Graphics settings widget base is constructible"), Widget);
	Widget->SetPendingSettings(SavedSettings);
	const FYogGraphicsSettings InitialPendingSettings = Widget->GetPendingSettings();
	TestEqual(TEXT("Widget stores pending target tier before customization"), InitialPendingSettings.SelectedTargetTier, EYogPerformanceTargetTier::Mid);

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
	TestEqual(TEXT("Widget stores pending material quality"), PendingSettings.MaterialQuality, 2);
	TestEqual(TEXT("Widget mirrors material quality to shading"), PendingSettings.ShadingQuality, 2);
	TestEqual(TEXT("Widget mirrors material quality to effects"), PendingSettings.EffectsQuality, 2);
	TestEqual(TEXT("Widget stores pending dynamic light quality"), PendingSettings.DynamicLightQuality, 1);
	TestEqual(TEXT("Widget stores pending material light quality"), PendingSettings.MaterialLightQuality, 2);
	TestEqual(TEXT("Widget maps material light quality to the max LightInfo count"), PendingSettings.MaterialLightMaxLightInfoCount, 2);
	TestEqual(TEXT("Quality sliders mark settings as custom"), PendingSettings.PerformanceProfile, EYogPerformanceProfile::Custom);
	TestEqual(TEXT("Quality sliders clear the target tier"), PendingSettings.SelectedTargetTier, EYogPerformanceTargetTier::Custom);

	return true;
}

#endif
