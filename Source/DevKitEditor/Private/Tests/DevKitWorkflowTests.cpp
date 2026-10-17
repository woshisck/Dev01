#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tools/Workflow/SDevKitWorkflowWidget.h"
#include "Tools/Workflow/SDevKitGraphicsQualityWorkbenchWidget.h"
#include "Tools/Workflow/SDevKitRenderStyleWorkbenchWidget.h"
#include "Scalability.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/StyleColors.h"
#include "StylizedLightingSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Text/STextBlock.h"
#include "IDetailsView.h"
#include "PropertyPath.h"
#include "StylizedCharacterLighting.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitWorkflowCatalogTest,
    "DevKit.Workflow.Catalog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitWorkflowCatalogTest::RunTest(const FString& Parameters)
{
    FString Json;
    TSharedPtr<FJsonObject> Root;
    if (!TestTrue(TEXT("Catalog exists"), FFileHelper::LoadFileToString(Json,
        *(FPaths::ProjectDir() / TEXT("Build/Dev01WorkflowCatalog.json"))))) return false;
    if (!TestTrue(TEXT("Catalog is valid JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root)
        && Root.IsValid())) return false;
    const TArray<TSharedPtr<FJsonValue>>* Workflows = nullptr;
    if (!TestTrue(TEXT("Workflows array exists"), Root->TryGetArrayField(TEXT("workflows"), Workflows))) return false;
    TestTrue(TEXT("At least one workflow"), Workflows->Num() > 0);
    TSet<FString> Ids;
    bool bFoundCharacterLookDev = false;
    bool bFoundGraphicsQuality = false;
    bool bFoundStylizedLighting = false;
    bool bFoundSceneAssets = false;
    bool bFoundGlobalLighting = false;
    for (const auto& Workflow : *Workflows)
    {
        const TSharedPtr<FJsonObject> Object = Workflow->AsObject();
        const TArray<TSharedPtr<FJsonValue>>* Steps = nullptr;
        if (!Object || !Object->TryGetArrayField(TEXT("steps"), Steps)) { AddError(TEXT("Invalid workflow")); continue; }
        for (const auto& StepValue : *Steps)
        {
            const TSharedPtr<FJsonObject> Step = StepValue->AsObject();
            if (!Step) { AddError(TEXT("Invalid step")); continue; }
            FString Id, Tab;
            Step->TryGetStringField(TEXT("id"), Id);
            Step->TryGetStringField(TEXT("tab"), Tab);
            TestFalse(TEXT("Step ID is nonempty"), Id.IsEmpty());
            TestFalse(TEXT("Step IDs are unique"), Ids.Contains(Id));
            Ids.Add(Id);
            if (Id == TEXT("scene_assets"))
            {
                bFoundSceneAssets = true;
                TestEqual(TEXT("Unified level explorer uses existing scene workbench tab"), Tab, FString(TEXT("DevKitSceneWorkbench")));
                FString Group; Step->TryGetStringField(TEXT("group"), Group);
                TestEqual(TEXT("Maps are separated from objects"), Group, FString(TEXT("场景（关卡）")));
            }
            if (Id == TEXT("character_lookdev"))
            {
                bFoundCharacterLookDev = true;
                FString WorkflowId;
                FString Group;
                Object->TryGetStringField(TEXT("id"), WorkflowId);
                Step->TryGetStringField(TEXT("group"), Group);
                TestEqual(TEXT("LookDev belongs to material and lighting authoring"), WorkflowId, FString(TEXT("materials")));
                TestEqual(TEXT("LookDev is discoverable under Rendering / Lighting"), Group, FString(TEXT("灯光")));
                TestEqual(TEXT("LookDev retains its standalone tab contract"), Tab, FString(TEXT("DevKitCharacterLookDev")));
                bool bExternal = false;
                Step->TryGetBoolField(TEXT("external"), bExternal);
                TestFalse(TEXT("LookDev is embedded in the native workflow"), bExternal);
            }
            if (Id == TEXT("emissive"))
            {
                bFoundStylizedLighting = true;
                FString WorkflowId;
                Object->TryGetStringField(TEXT("id"), WorkflowId);
                TestEqual(TEXT("Stylized lighting belongs to the rendering workbench"), WorkflowId, FString(TEXT("materials")));
                TestEqual(TEXT("Stylized lighting keeps its registered tool tab"), Tab, FString(TEXT("DevKitStylizedEmissiveLibrary")));
            }
            if (Id == TEXT("stylized_lighting"))
            {
                bFoundGlobalLighting = true;
                FString Group, Title;
                Step->TryGetStringField(TEXT("group"), Group);
                Step->TryGetStringField(TEXT("title"), Title);
                TestEqual(TEXT("Global lighting stays under rendering lights"), Group, FString(TEXT("灯光")));
                TestEqual(TEXT("Lighting title identifies the two-tone authoring workflow"), Title, FString(TEXT("二分光影设置")));
                TestEqual(TEXT("Global lighting retains its legacy tab contract"), Tab, FString(TEXT("DevKitStylizedLightingSettings")));
            }
            if (Id == TEXT("graphics_quality"))
            {
                bFoundGraphicsQuality = true;
                FString WorkflowId;
                Object->TryGetStringField(TEXT("id"), WorkflowId);
                TestEqual(TEXT("Graphics quality preview belongs to its own workbench category"), WorkflowId, FString(TEXT("performance")));
                TestEqual(TEXT("Graphics quality belongs to its dedicated performance workbench"), Tab, FString(TEXT("DevKitPerformanceWorkbench")));
                bool bExternal = false;
                Step->TryGetBoolField(TEXT("external"), bExternal);
                TestFalse(TEXT("Graphics quality preview is a native embedded panel"), bExternal);
            }
            TestTrue(*FString::Printf(TEXT("Registered tab: %s"), *Tab), FGlobalTabmanager::Get()->HasTabSpawner(FName(*Tab)));
            for (const TCHAR* Field : {TEXT("title"), TEXT("input"), TEXT("output")})
            {
                FString Value;
                TestTrue(TEXT("Step documents its inputs and outputs"), Step->TryGetStringField(Field, Value) && !Value.IsEmpty());
            }
        }
    }
    TestTrue(TEXT("Character LookDev is discoverable in the workflow catalog"), bFoundCharacterLookDev);
    TestTrue(TEXT("Stylized lighting is discoverable in the rendering workbench"), bFoundStylizedLighting);
    TestTrue(TEXT("Graphics quality preview is discoverable in the workflow catalog"), bFoundGraphicsQuality);
    TestTrue(TEXT("Unified level explorer is discoverable"), bFoundSceneAssets);
    TestTrue(TEXT("Global native rendering information is discoverable"), bFoundGlobalLighting);
    TestFalse(TEXT("Map creation is no longer a separate scene step"), Ids.Contains(TEXT("map_creator")));
    TestFalse(TEXT("Room configuration is no longer a separate scene step"), Ids.Contains(TEXT("level_data")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitGlobalLightingPanelTest,
    "DevKit.Workflow.GlobalLightingPanel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitGlobalLightingPanelTest::RunTest(const FString& Parameters)
{
    TMap<FString, FString> CVarValues;
    for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.DirectBlend"),
        TEXT("r.StylizedLumenLighting.IndirectBlend"), TEXT("r.StylizedLumenLighting.BandCount"),
        TEXT("r.StylizedLumenLighting.BandSoftness"), TEXT("r.StylizedLumenLighting.GlossInfluence"),
        TEXT("r.StylizedLumenLighting.SpecularIntensity"), TEXT("r.StylizedLumenLighting.SpecularOffset"), TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Shadow.Virtual.Enable"),
        TEXT("r.Yog.MaterialLighting.Enable"), TEXT("r.Yog.MaterialLighting.DiffuseContrast"), TEXT("r.Yog.MaterialLighting.IndirectAmount")})
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name)) CVarValues.Add(Name, Variable->GetString());
    const TSharedRef<SDevKitWorkflowWidget> Workbench = SNew(SDevKitWorkflowWidget).WorkflowId(TEXT("materials"));
    TestTrue(TEXT("Global lighting is selectable by stable step ID"), Workbench->SelectStepById(TEXT("stylized_lighting")));
    const TSharedRef<SWidget> Panel = SDevKitWorkflowWidget::MakeStylizedLightingSettingsPanel();
    Panel->SlatePrepass();
    FString Labels;
    TFunction<void(TSharedRef<SWidget>)> CollectLabels;
    CollectLabels = [&Labels, &CollectLabels](TSharedRef<SWidget> Widget)
    {
        if (Widget->GetType() == FName(TEXT("STextBlock"))) Labels += StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() + TEXT("\n");
        FChildren* Children = Widget->GetAllChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index) CollectLabels(Children->GetChildAt(Index));
    };
    CollectLabels(Panel);
    for (const TCHAR* Field : {TEXT("启用统一颜色分层"), TEXT("参考层数"), TEXT("分层边界柔和度"),
        TEXT("分层影响角色"), TEXT("分层影响场景")})
        TestFalse(*FString::Printf(TEXT("Retired global layer control is absent: %s"), Field), Labels.Contains(Field));
    TestFalse(TEXT("Cancelled character exposure is not editable"), Labels.Contains(TEXT("角色亮度补偿 / EV")));
    TestFalse(TEXT("Cancelled dark exposure range is not editable"), Labels.Contains(TEXT("分层暗部范围 / EV")));
    TestFalse(TEXT("Cancelled bright exposure range is not editable"), Labels.Contains(TEXT("分层亮部范围 / EV")));
    TestTrue(TEXT("Global panel identifies two-tone lighting controls"), Labels.Contains(TEXT("二分光影设置")));
    TestTrue(TEXT("Global panel identifies the original cloud source baseline"), Labels.Contains(TEXT("CL149 / Engine76")));
    for (const TCHAR* Field : {TEXT("启用二分光影（暗部两档 / 亮面连续）"), TEXT("暗部边界柔度"),
        TEXT("场景高光强度"), TEXT("场景高光偏移"), TEXT("Lumen Cache 二分混合"), TEXT("直接光二分混合")})
        TestTrue(*FString::Printf(TEXT("Active two-tone lighting control is visible: %s"), Field), Labels.Contains(Field));
    TestFalse(TEXT("Variable band-count control is removed"), Labels.Contains(TEXT("光照层数")));
    TestFalse(TEXT("Gloss-dependent boundary control is removed"), Labels.Contains(TEXT("光泽度影响")));
    for (const TCHAR* Field : {TEXT("角色连续受光"), TEXT("直接光 / 漫反射响应"), TEXT("阴影 / 材质可见性与颜色"),
        TEXT("可选后处理调色 / 与受光管线独立"), TEXT("全局对比度"), TEXT("角色二分 / 阴影整理")})
        TestFalse(*FString::Printf(TEXT("Retired pipeline control is absent: %s"), Field), Labels.Contains(Field));
    TestTrue(TEXT("Cache convergence is documented"), Labels.Contains(TEXT("缓存会渐进刷新")));
    TestTrue(TEXT("LookDev remains accessible"), Labels.Contains(TEXT("LookDev 关卡工作台")));
    TestTrue(TEXT("Scene resources remain accessible"), Labels.Contains(TEXT("场景资源与子关卡")));
    TestTrue(TEXT("Performance remains accessible"), Labels.Contains(TEXT("性能工作台 / 画质与阴影")));
    TestTrue(TEXT("Default save is explicit"), Labels.Contains(TEXT("保存项目全局默认")));
    TestTrue(TEXT("Two-tone Default is distinct from disabled-scene comparison"), Labels.Contains(TEXT("Default · 二分光影默认"))
        && Labels.Contains(TEXT("关闭场景分层对照")));
    TestTrue(TEXT("Live parameter editing is documented"), Labels.Contains(TEXT("实时预览 · 修改即生效")));
    TestFalse(TEXT("Live editing does not require an Apply button"), Labels.Contains(TEXT("Apply · 应用草稿")));
    TestTrue(TEXT("Platform acceptance is not confused with measured performance"), Labels.Contains(TEXT("05 平台与性能验收")));
    TestTrue(TEXT("Cloud direct and Lumen cache scope is explicit"), Labels.Contains(TEXT("既有直接光 + Lumen Cache 受光入口")));
    TestTrue(TEXT("Dark bands and continuous lit side are explicit"), Labels.Contains(TEXT("暗部两档")) && Labels.Contains(TEXT("亮面连续")));
    TestTrue(TEXT("Final GI quantization is not claimed"), Labels.Contains(TEXT("不是 GI 最终合成量化")));
    TestTrue(TEXT("Fixed VSM baseline is explicit and UI offers no shadow-map tuning"), Labels.Contains(TEXT("Lumen + VSM")));
    TestFalse(TEXT("SM-only sharpening control is absent"), Labels.Contains(TEXT("方向光投影边缘增量")));
    TestFalse(TEXT("Strict four-parameter Strength control is absent"), Labels.Contains(TEXT("受光分层强度")));
    TestFalse(TEXT("Retired CallInEditor actions remain hidden"), Labels.Contains(TEXT("ApplyToConsoleVariables")));
    TestNotNull(TEXT("Global panel has a direct console entry"), IConsoleManager::Get().FindConsoleObject(TEXT("DevKit.Rendering.GlobalEnvironment.Open")));
    for (const auto& Pair : CVarValues) TestEqual(*FString::Printf(TEXT("Constructing global UI does not change %s"), *Pair.Key),
        IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->GetString(), Pair.Value);
    if (FParse::Param(FCommandLine::Get(), TEXT("WorkflowRender")))
    {
        constexpr int32 Width = 1450, Height = 940;
        FWidgetRenderer Renderer(false);
        UTextureRenderTarget2D* Target = nullptr;
        for (int32 Frame = 0; Frame < 3; ++Frame)
        {
            Workbench->SlatePrepass();
            Target = Renderer.DrawWidget(Workbench, FVector2D(Width, Height));
            FlushRenderingCommands();
        }
        if (TestNotNull(TEXT("Global workbench render target"), Target))
        {
            TArray<FColor> Pixels;
            FReadSurfaceDataFlags Flags(RCM_UNorm); Flags.SetLinearToGamma(false);
            if (TestTrue(TEXT("Global panel native pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags)))
            {
                for (FColor& Pixel : Pixels) Pixel = Pixel.ReinterpretAsLinear().ToFColor(true);
                TArray64<uint8> Png;
                FImageUtils::PNGCompressImageArray(Width, Height, Pixels, Png);
                const FString Directory = FPaths::ProjectSavedDir() / TEXT("WorkflowHub/Reports");
                IFileManager::Get().MakeDirectory(*Directory, true);
                TestTrue(TEXT("Global workbench native screenshot"), FFileHelper::SaveArrayToFile(Png, *(Directory / TEXT("GlobalLightingWorkbench.png"))));
            }
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitRenderStyleDraftTest,
    "DevKit.Workflow.RenderStyleDraft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRenderStyleDraftTest::RunTest(const FString& Parameters)
{
    UYogRenderStyleSettings* Global = GetMutableDefault<UYogRenderStyleSettings>();
    UStylizedLightingSettings* Settings = GetMutableDefault<UStylizedLightingSettings>();
    struct FRestoreSession
    {
        UYogRenderStyleSettings* Global;
        UStylizedLightingSettings* Settings;
        FYogNativeRenderStyleSettings Style;
        FYogMaterialLightingStyle Lighting;
        TArray<FYogRenderStylePreset> Presets;
        bool bEnabled;
        float Bands, Softness, Gloss, Specular, Offset, Indirect, Direct;
        TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
        uint32 Count = 0;
        TMap<IConsoleVariable*, TPair<FString, EConsoleVariableFlags>> CVars;
        ~FRestoreSession()
        {
            Global->Style = Style; Global->Lighting = Lighting; Global->Presets = Presets;
            Settings->bEnableStylizedLumenLighting = bEnabled;
            Settings->BandCount = Bands; Settings->BandSoftness = Softness; Settings->GlossInfluence = Gloss;
            Settings->SpecularIntensity = Specular; Settings->SpecularOffset = Offset;
            Settings->IndirectBlend = Indirect; Settings->DirectBlend = Direct;
            Settings->ApplyToConsoleVariables();
            SetStylizedCharacterLightingProfiles(MakeArrayView(Packed.GetData(), Packed.Num()), Count);
            for (const auto& Pair : CVars)
            {
                Pair.Key->ClearFlags(ECVF_SetByMask);
                Pair.Key->Set(*Pair.Value.Key, static_cast<EConsoleVariableFlags>(Pair.Value.Value & ECVF_SetByMask));
                Pair.Key->SetFlags(Pair.Value.Value);
            }
        }
    } Restore{Global, Settings, Global->Style, Global->Lighting, Global->Presets,
        Settings->bEnableStylizedLumenLighting, Settings->BandCount, Settings->BandSoftness, Settings->GlossInfluence,
        Settings->SpecularIntensity, Settings->SpecularOffset, Settings->IndirectBlend, Settings->DirectBlend};
    GetStylizedCharacterLightingProfiles(Restore.Packed, Restore.Count);
    // The cloud uploader includes unchanged role settings; restore all its CVar writes after this test.
    for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.BandCount"),
        TEXT("r.StylizedLumenLighting.BandSoftness"), TEXT("r.StylizedLumenLighting.GlossInfluence"),
        TEXT("r.StylizedLumenLighting.SpecularIntensity"), TEXT("r.StylizedLumenLighting.SpecularOffset"),
        TEXT("r.StylizedLumenLighting.IndirectBlend"), TEXT("r.StylizedLumenLighting.DirectBlend"),
        TEXT("r.StylizedLumenLighting.IndirectMaxLuminance"), TEXT("r.StylizedReflection.Kuwahara.Enable"),
        TEXT("r.StylizedReflection.Kuwahara.Strength"), TEXT("r.StylizedCharacter.SelfShadow.Enable"),
        TEXT("r.StylizedCharacter.SelfShadow.HalfViewBlend"), TEXT("r.StylizedCharacter.SelfShadow.Strength"),
        TEXT("r.StylizedCharacter.SelfShadow.MaxTraceDistance"), TEXT("r.StylizedCharacterLighting.DirectColorInfluence"),
        TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"), TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence")})
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            Restore.CVars.Add(CVar, {CVar->GetString(), CVar->GetFlags()});
            CVar->ClearFlags(ECVF_SetByMask);
        }
    TMap<FString, TArray<uint8>> ConfigBefore;
    for (const FString& Path : {FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"), GGameIni})
    {
        TArray<uint8> Bytes;
        if (FFileHelper::LoadFileToArray(Bytes, *Path)) ConfigBefore.Add(Path, MoveTemp(Bytes));
    }
    TMap<UPackage*, bool> PackageDirtyBefore;
    for (TObjectIterator<UPackage> Package; Package; ++Package)
        if (!Package->HasAnyFlags(RF_Transient)) PackageDirtyBefore.Add(*Package, Package->IsDirty());
    TMap<FString, FString> NativeCVarBefore;
    for (const TCHAR* Name : {TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Lumen.DiffuseIndirect.Allow"),
        TEXT("r.ReflectionMethod"), TEXT("r.Lumen.Reflections.Allow"), TEXT("r.Shadow.Virtual.Enable"),
        TEXT("r.DefaultFeature.AutoExposure"), TEXT("r.EyeAdaptationQuality")})
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name)) NativeCVarBefore.Add(Name, CVar->GetString());
    const bool bContinuousBefore = Settings->bEnableContinuousCharacterLighting;
    const bool bToneBefore = Settings->bEnableCharacterTone;
    const float BeforeIndirectMax = Settings->IndirectMaxLuminance;
    FString ProfilesBefore;
    const FArrayProperty* ProfilesProperty = FindFProperty<FArrayProperty>(UStylizedLightingSettings::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterLightingProfiles));
    ProfilesProperty->ExportTextItem_Direct(ProfilesBefore, &Settings->CharacterLightingProfiles, nullptr, Settings, PPF_None);
    const TSharedRef<SDevKitRenderStyleWorkbenchWidget> Widget = SNew(SDevKitRenderStyleWorkbenchWidget);
    Widget->LoadBuiltInPreset(false);
    TestTrue(TEXT("Cloud default is enabled independently of retired Style"), Settings->bEnableStylizedLumenLighting);
    TestEqual(TEXT("Default fixes dark-side band count at two"), Settings->BandCount, 2.f);
    TestEqual(TEXT("Authoring draft fixes dark-side band count at two"), Widget->DraftBands, 2.f);
    TestEqual(TEXT("Cloud default softness"), Settings->BandSoftness, .18f);
    TestEqual(TEXT("Cloud default gloss"), Settings->GlossInfluence, .65f);
    TestEqual(TEXT("Cloud default specular"), Settings->SpecularIntensity, 1.f);
    TestEqual(TEXT("Cloud default specular offset"), Settings->SpecularOffset, 0.f);
    TestEqual(TEXT("Cloud default indirect blend"), Settings->IndirectBlend, 1.f);
    TestEqual(TEXT("Cloud default direct blend"), Settings->DirectBlend, 1.f);

    TArray<TSharedRef<SSpinBox<float>>> SpinBoxes;
    TSharedPtr<SCheckBox> EnableBox;
    TFunction<void(TSharedRef<SWidget>)> CollectControls;
    CollectControls = [&SpinBoxes, &EnableBox, &CollectControls](TSharedRef<SWidget> Node)
    {
        if (Node->GetType() == FName(TEXT("SSpinBox<float>"))) SpinBoxes.Add(StaticCastSharedRef<SSpinBox<float>>(Node));
        if (Node->GetType() == FName(TEXT("SCheckBox"))) EnableBox = StaticCastSharedRef<SCheckBox>(Node);
        FChildren* Children = Node->GetAllChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index) CollectControls(Children->GetChildAt(Index));
    };
    CollectControls(Widget->MakeLightingControls());
    TestEqual(TEXT("Five live numeric inputs exclude band count, gloss, roles and exposure"), SpinBoxes.Num(), 5);
    if (SpinBoxes.Num() == 5)
    {
        const float Values[] = {.27f, .64f, .84f, 1.6f, -.12f};
        const TCHAR* Names[] = {TEXT("r.StylizedLumenLighting.BandSoftness"), TEXT("r.StylizedLumenLighting.IndirectBlend"),
            TEXT("r.StylizedLumenLighting.DirectBlend"), TEXT("r.StylizedLumenLighting.SpecularIntensity"),
            TEXT("r.StylizedLumenLighting.SpecularOffset")};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Values); ++Index)
        {
            SpinBoxes[Index]->SetValue(Values[Index], false);
            IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Names[Index]);
            if (TestNotNull(*FString::Printf(TEXT("Cloud CVar exists: %s"), Names[Index]), CVar))
                TestEqual(*FString::Printf(TEXT("Real interactive callback reaches renderer: %s"), Names[Index]), CVar->GetFloat(), Values[Index]);
        }
        TestEqual(TEXT("Editing unrelated controls cannot revive multi-band authoring"), Settings->BandCount, 2.f);
        TestEqual(TEXT("Removed gloss control retains compatibility value"), Settings->GlossInfluence, .65f);
        TestEqual(TEXT("Cloud indirect mix is its original parameter, not Strength"), Settings->IndirectBlend, .64f);
        SpinBoxes[2]->SetValue(1.4f, false);
        TestEqual(TEXT("Direct two-tone mix cannot amplify beyond one"), Settings->DirectBlend, 1.f);
        TestFalse(TEXT("Manual edits clear stale preset labels"), Widget->SelectedPreset.IsValid());
    }
    if (TestTrue(TEXT("Enable has a real live checkbox"), EnableBox.IsValid()))
    {
        EnableBox->ToggleCheckedState();
        TestFalse(TEXT("Checkbox disables scene layers immediately"), Settings->bEnableStylizedLumenLighting);
        EnableBox->ToggleCheckedState();
        TestTrue(TEXT("Checkbox enables scene layers immediately"), Settings->bEnableStylizedLumenLighting);
    }
    Widget->ResetDraft();
    TestTrue(TEXT("Default restores enabled cloud lighting, not disabled native comparison"), Settings->bEnableStylizedLumenLighting);
    TestEqual(TEXT("Default restores cloud indirect blend one"), Settings->IndirectBlend, 1.f);
    Widget->LoadBuiltInPreset(true);
    TestFalse(TEXT("Independent comparison only disables scene layering"), Settings->bEnableStylizedLumenLighting);
    Global->Style.bEnabled = true;
    Global->ApplyToRenderer();
    TestFalse(TEXT("Retired Style cannot re-enable disabled-scene comparison"), Settings->bEnableStylizedLumenLighting);
    Global->Style.bEnabled = false;
    Widget->ResetDraft();
    TestTrue(TEXT("Cloud Default does not require retired Style"), Settings->bEnableStylizedLumenLighting);
    Global->Style = Restore.Style;

    FYogRenderStylePreset& Custom = Global->Presets.AddDefaulted_GetRef();
    Custom.Name = FName(*(TEXT("Cloud-Light-Test-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    Custom.bIncludesCloudLighting = true; Custom.CloudLightingVersion = 1;
    Custom.bEnableCloudLighting = true; Custom.CloudLightingBands = 4.5f;
    Custom.CloudLightingSoftness = .12f; Custom.CloudLightingGlossInfluence = .36f;
    Custom.CloudLightingSpecularIntensity = 1.3f; Custom.CloudLightingSpecularOffset = -.23f;
    Custom.CloudLightingIndirectBlend = .75f; Custom.CloudLightingDirectBlend = 1.5f;
    Custom.Style.GlobalContrast = 1.49f; Custom.Style.bEnablePostProcessGrading = true;
    Custom.Lighting.DiffuseContrast = 1.9f;
    Widget->OnPresetSelected(MakeShared<FString>(Custom.Name.ToString()), ESelectInfo::OnMouseClick);
    TestEqual(TEXT("Legacy version-one preset ignores multi-band data"), Settings->BandCount, 2.f);
    TestEqual(TEXT("Legacy preset retains a fixed two-band draft"), Widget->DraftBands, 2.f);
    TestEqual(TEXT("Versioned preset applies cloud softness"), Settings->BandSoftness, .12f);
    TestEqual(TEXT("Versioned preset applies cloud gloss"), Settings->GlossInfluence, .36f);
    TestEqual(TEXT("Versioned preset applies cloud specular"), Settings->SpecularIntensity, 1.3f);
    TestEqual(TEXT("Versioned preset applies cloud specular offset"), Settings->SpecularOffset, -.23f);
    TestEqual(TEXT("Versioned preset applies cloud indirect"), Settings->IndirectBlend, .75f);
    TestEqual(TEXT("Version-one direct mix is clamped to a non-amplifying blend"), Settings->DirectBlend, 1.f);
    if (IConsoleVariable* ActualSpecular = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularIntensity")))
    {
        ActualSpecular->Set(2.25f, ECVF_SetByCode);
        TestTrue(TEXT("Status reads actual renderer CVar rather than stale draft"), Widget->GetAppliedText().ToString().Contains(TEXT("高光强度 2.25")));
        Settings->ApplyToConsoleVariables();
    }
    const FName CustomName = Custom.Name;
    Custom.CloudLightingVersion = 99;
    Custom.CloudLightingBands = 2.f;
    Custom.CloudLightingSoftness = .91f;
    Widget->OnPresetSelected(MakeShared<FString>(CustomName.ToString()), ESelectInfo::OnMouseClick);
    TestEqual(TEXT("Unsupported cloud preset version cannot alter fixed bands"), Settings->BandCount, 2.f);
    TestEqual(TEXT("Unsupported preset does not apply its authored softness"), Settings->BandSoftness, .12f);
    TestTrue(TEXT("Unsupported version is reported"), Widget->Status.Contains(TEXT("不支持的版本")));
    FYogRenderStylePreset& Legacy = Global->Presets.AddDefaulted_GetRef();
    Legacy.Name = FName(*(TEXT("Legacy-Test-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    Legacy.bIncludesLumenLayers = true; Legacy.LumenLayerStrength = .05f;
    Legacy.LumenLayerBands = 2.f; Legacy.Style.GlobalContrast = 1.5f; Legacy.Lighting.DiffuseContrast = 2.f;
    Widget->OnPresetSelected(MakeShared<FString>(Legacy.Name.ToString()), ESelectInfo::OnMouseClick);
    TestEqual(TEXT("Old strict Strength cannot migrate to cloud IndirectBlend"), Settings->IndirectBlend, .75f);
    TestEqual(TEXT("Old strict preset does not alter fixed two-tone bands"), Settings->BandCount, 2.f);
    TestTrue(TEXT("Old compatibility is explicitly reported"), Widget->Status.Contains(TEXT("旧兼容预设")));
    FYogRenderStylePreset& TwoTone = Global->Presets.AddDefaulted_GetRef();
    TwoTone.Name = FName(*(TEXT("TwoTone-Test-") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    TwoTone.bIncludesCloudLighting = true; TwoTone.CloudLightingVersion = 2;
    TwoTone.bEnableCloudLighting = true; TwoTone.CloudLightingBands = 7.5f;
    TwoTone.CloudLightingSoftness = .26f; TwoTone.CloudLightingGlossInfluence = .36f;
    TwoTone.CloudLightingSpecularIntensity = 1.22f; TwoTone.CloudLightingSpecularOffset = -.2f;
    TwoTone.CloudLightingIndirectBlend = .42f; TwoTone.CloudLightingDirectBlend = .88f;
    Widget->OnPresetSelected(MakeShared<FString>(TwoTone.Name.ToString()), ESelectInfo::OnMouseClick);
    TestEqual(TEXT("Version-two presets retain fixed dark-side bands even with malformed legacy data"), Settings->BandCount, 2.f);
    TestEqual(TEXT("Version-two preset reaches live boundary softness"), Settings->BandSoftness, .26f);
    TestEqual(TEXT("Version-two preset reaches live direct mix"), Settings->DirectBlend, .88f);
    TestEqual(TEXT("Version-two preset reaches live cache mix"), Settings->IndirectBlend, .42f);
    TestEqual(TEXT("Cloud UI does not change retired post-process contrast"), Global->Style.GlobalContrast, Restore.Style.GlobalContrast);
    TestEqual(TEXT("Cloud UI does not change retired display enable"), Global->Style.bEnablePostProcessGrading, Restore.Style.bEnablePostProcessGrading);
    TestEqual(TEXT("Cloud UI does not change retired diffuse response"), Global->Lighting.DiffuseContrast, Restore.Lighting.DiffuseContrast);
    TestEqual(TEXT("Cloud UI does not change role continuous compatibility data"), Settings->bEnableContinuousCharacterLighting, bContinuousBefore);
    TestEqual(TEXT("Cloud UI does not change role tone"), Settings->bEnableCharacterTone, bToneBefore);
    TestEqual(TEXT("Only eight exposed settings change; old indirect max remains"), Settings->IndirectMaxLuminance, BeforeIndirectMax);
    FString ProfilesAfter;
    ProfilesProperty->ExportTextItem_Direct(ProfilesAfter, &Settings->CharacterLightingProfiles, nullptr, Settings, PPF_None);
    TestEqual(TEXT("Cloud UI never changes role profile or exposure fields"), ProfilesAfter, ProfilesBefore);
    TestFalse(TEXT("Role controls are not restored as a giant Details list"), SDevKitRenderStyleWorkbenchWidget::IsCharacterPropertyVisible(TEXT("ContinuousDiffuseContrast")));
    const FProperty* StyleProperty = FindFProperty<FProperty>(UYogRenderStyleSettings::StaticClass(), GET_MEMBER_NAME_CHECKED(UYogRenderStyleSettings, Style));
    const FProperty* LightingProperty = FindFProperty<FProperty>(UYogRenderStyleSettings::StaticClass(), GET_MEMBER_NAME_CHECKED(UYogRenderStyleSettings, Lighting));
    TestTrue(TEXT("Retired Style is serialized but not editable in ProjectSettings"), StyleProperty && StyleProperty->HasAnyPropertyFlags(CPF_Config) && !StyleProperty->HasAnyPropertyFlags(CPF_Edit));
    TestTrue(TEXT("Retired Lighting is serialized but not editable in ProjectSettings"), LightingProperty && LightingProperty->HasAnyPropertyFlags(CPF_Config) && !LightingProperty->HasAnyPropertyFlags(CPF_Edit));
    TestTrue(TEXT("Live edits are not represented as saved"), Widget->bHasPendingChanges);
    for (const auto& Pair : ConfigBefore)
    {
        TArray<uint8> After;
        TestTrue(*FString::Printf(TEXT("Realtime editing retains config: %s"), *Pair.Key), FFileHelper::LoadFileToArray(After, *Pair.Key));
        TestTrue(*FString::Printf(TEXT("Realtime editing does not write config bytes: %s"), *Pair.Key), After == Pair.Value);
    }
    for (const auto& Pair : PackageDirtyBefore)
        TestEqual(*FString::Printf(TEXT("Realtime editing does not dirty asset: %s"), *Pair.Key->GetName()), Pair.Key->IsDirty(), Pair.Value);
    for (const auto& Pair : NativeCVarBefore)
        TestEqual(*FString::Printf(TEXT("Native renderer CVar remains unchanged: %s"), *Pair.Key), IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->GetString(), Pair.Value);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitGlobalLightingSaveTest,
    "DevKit.Workflow.RetiredGlobalLightingSaveNoOp", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitGlobalLightingSaveTest::RunTest(const FString& Parameters)
{
    const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("WorkflowHub/Reports"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Path = Directory / (TEXT("GlobalDefaults-Test-") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".ini"));
    FConfigFile Before;
    const FString Section = UStylizedLightingSettings::StaticClass()->GetPathName();
    Before.SetString(*Section, TEXT("IndirectBlend"), TEXT("0.12345"));
    Before.SetString(TEXT("UnrelatedSection"), TEXT("Sentinel"), TEXT("Preserve Me"));
    if (!TestTrue(TEXT("Generated temporary config written"), Before.Write(Path))) return false;
    FString Status;
    TestFalse(TEXT("Retired UI helper cannot write rendering configuration"), SDevKitWorkflowWidget::SaveGlobalEnvironmentDefaults(Path, Status));
    AddInfo(Status);
    FConfigFile After; After.Read(Path);
    FString Actual;
    TestFalse(TEXT("Global rendering config is not serialized"), After.GetString(*Section, TEXT("GlobalEnvironment"), Actual));
    TestTrue(TEXT("Unrelated section preserved"), After.GetString(TEXT("UnrelatedSection"), TEXT("Sentinel"), Actual));
    TestEqual(TEXT("Unrelated value unchanged"), Actual, FString(TEXT("Preserve Me")));
    TestTrue(TEXT("Legacy property preserved"), After.GetString(*Section, TEXT("IndirectBlend"), Actual));
    TestEqual(TEXT("Legacy indirect mix not overwritten"), Actual, FString(TEXT("0.12345")));
    // Delete only the exact GUID-named generated test file. Never touch project configuration or assets.
    TestTrue(TEXT("Temporary config removed"), IFileManager::Get().Delete(*Path));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitRenderStylePresetSaveTest,
    "DevKit.Workflow.RenderStylePresetSave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRenderStylePresetSaveTest::RunTest(const FString& Parameters)
{
    const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("WorkflowHub/Reports"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Path = Directory / (TEXT("RenderStylePresets-Test-") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".ini"));
    const FString Section = UYogRenderStyleSettings::StaticClass()->GetPathName();
    FConfigFile File;
    const FString SavedStyle = TEXT("(bEnabled=True,GlobalContrast=1.01,GlobalSaturation=1.0,ShadowsContrast=1.0)");
    const FString SavedLighting = TEXT("(bEnabled=True,DiffuseContrast=1.19,RampAmount=0.23,IndirectAmount=0.64)");
    File.SetString(*Section, TEXT("Style"), *SavedStyle);
    File.SetString(*Section, TEXT("Lighting"), *SavedLighting);
    File.SetString(TEXT("UnrelatedSection"), TEXT("Sentinel"), TEXT("Preserve Me"));
    if (!TestTrue(TEXT("Create GUID-scoped preset-save fixture"), File.Write(Path))) return false;

    UYogRenderStyleSettings* Settings = GetMutableDefault<UYogRenderStyleSettings>();
    const FYogNativeRenderStyleSettings BeforeStyle = Settings->Style;
    const FYogMaterialLightingStyle BeforeLighting = Settings->Lighting;
    const TArray<FYogRenderStylePreset> BeforePresets = Settings->Presets;
    Settings->Style.GlobalContrast = 1.43f;
    Settings->Lighting.DiffuseContrast = 1.75f;
    Settings->Presets.Reset();
    FYogRenderStylePreset& Preset = Settings->Presets.AddDefaulted_GetRef();
    Preset.Name = TEXT("Cloud Custom Test");
    Preset.bIncludesCloudLighting = true; Preset.CloudLightingVersion = 2;
    Preset.bEnableCloudLighting = false; Preset.CloudLightingBands = 2.f;
    Preset.CloudLightingSoftness = .24f; Preset.CloudLightingGlossInfluence = .51f;
    Preset.CloudLightingSpecularIntensity = 1.42f; Preset.CloudLightingSpecularOffset = -.13f;
    Preset.CloudLightingIndirectBlend = .61f; Preset.CloudLightingDirectBlend = .84f;
    // Preserve all older serialized fields, but this UI never applies them.
    Preset.bIncludesLumenLayers = true; Preset.LumenLayerStrength = .05f;
    Preset.Style.GlobalContrast = 1.07f;
    Preset.Lighting.DiffuseContrast = 1.51f;
    Preset.Lighting.IndirectAmount = .28f;
    Preset.Lighting.ShadowTint = FLinearColor(.6f, .7f, .8f, 1.f);
    FConfigCacheIni LocalConfig(EConfigCacheType::Temporary);
    FConfigBranch* Branch = LocalConfig.FindBranch(FName(TEXT("Game")), FString());
    if (Branch) Branch->StaticLayers.Add(Path);
    TestTrue(TEXT("Single-property preset persistence is verified by readback"),
        Branch && SDevKitRenderStyleWorkbenchWidget::SavePresetTableToConfig(Settings, Path, &LocalConfig));
    FConfigFile After; After.Combine(Path);
    FString Actual;
    TestTrue(TEXT("Saving a preset retains previously saved Style"), After.GetString(*Section, TEXT("Style"), Actual));
    TestEqual(TEXT("Preset save does not persist a session-only display value"), Actual, SavedStyle);
    TestTrue(TEXT("Saving a preset retains saved generic material Lighting"), After.GetString(*Section, TEXT("Lighting"), Actual));
    TestEqual(TEXT("Preset save cannot implicitly persist generic response"), Actual, SavedLighting);
    TArray<FString> SerializedPresets;
    After.GetArray(*Section, TEXT("Presets"), SerializedPresets);
    if (TestEqual(TEXT("Preset table contains the saved preset"), SerializedPresets.Num(), 1))
    {
        FYogRenderStylePreset RoundTrip;
        const TCHAR* End = FYogRenderStylePreset::StaticStruct()->ImportText(*SerializedPresets[0], &RoundTrip, nullptr,
            PPF_SerializedAsImportText, GWarn, TEXT("CloudLightingPreset"));
        if (TestNotNull(TEXT("Native struct import reads saved cloud preset"), End))
        {
            TestEqual(TEXT("Saved preset keeps its name"), RoundTrip.Name, Preset.Name);
            TestTrue(TEXT("Saved preset explicitly includes cloud contract"), RoundTrip.bIncludesCloudLighting);
            TestEqual(TEXT("Saved two-tone preset has explicit schema version two"), RoundTrip.CloudLightingVersion, 2);
            TestFalse(TEXT("Saved preset retains disabled scene comparison"), RoundTrip.bEnableCloudLighting);
            TestEqual(TEXT("Saved two-tone preset fixes dark-side band count"), RoundTrip.CloudLightingBands, 2.f);
            TestEqual(TEXT("Saved cloud softness"), RoundTrip.CloudLightingSoftness, .24f);
            TestEqual(TEXT("Saved cloud gloss"), RoundTrip.CloudLightingGlossInfluence, .51f);
            TestEqual(TEXT("Saved cloud specular intensity"), RoundTrip.CloudLightingSpecularIntensity, 1.42f);
            TestEqual(TEXT("Saved cloud specular offset"), RoundTrip.CloudLightingSpecularOffset, -.13f);
            TestEqual(TEXT("Saved cloud indirect blend"), RoundTrip.CloudLightingIndirectBlend, .61f);
            TestEqual(TEXT("Saved cloud direct blend"), RoundTrip.CloudLightingDirectBlend, .84f);
            TestTrue(TEXT("Old strict Includes flag remains load-compatible"), RoundTrip.bIncludesLumenLayers);
            TestEqual(TEXT("Old strict Strength survives without conversion"), RoundTrip.LumenLayerStrength, .05f);
            TestEqual(TEXT("Old grading data remains load-compatible but inactive"), RoundTrip.Style.GlobalContrast, 1.07f);
            TestEqual(TEXT("Old generic material data remains load-compatible but inactive"), RoundTrip.Lighting.DiffuseContrast, 1.51f);
        }
    }
    TestTrue(TEXT("Preset saving preserves unrelated section"), After.GetString(TEXT("UnrelatedSection"), TEXT("Sentinel"), Actual));
    TestEqual(TEXT("Unrelated preset-save sentinel is unchanged"), Actual, FString(TEXT("Preserve Me")));
    Settings->Style = BeforeStyle; Settings->Lighting = BeforeLighting; Settings->Presets = BeforePresets;

    // Exact eight-key defaults writer, never full SaveConfig or role-profile serialization.
    UStylizedLightingSettings* Lumen = GetMutableDefault<UStylizedLightingSettings>();
    const bool bBeforeLumen = Lumen->bEnableStylizedLumenLighting;
    const float BeforeValues[] = {Lumen->BandCount, Lumen->BandSoftness, Lumen->GlossInfluence,
        Lumen->SpecularIntensity, Lumen->SpecularOffset, Lumen->IndirectBlend, Lumen->DirectBlend};
    Lumen->bEnableStylizedLumenLighting = true;
    Lumen->BandCount = 7.5f; Lumen->BandSoftness = .32f; Lumen->GlossInfluence = .56f;
    Lumen->SpecularIntensity = 1.38f; Lumen->SpecularOffset = -.2f;
    Lumen->IndirectBlend = .86f; Lumen->DirectBlend = 1.21f;
    TestTrue(TEXT("Eight cloud defaults are saved by narrow static-layer writes"),
        Branch && SDevKitRenderStyleWorkbenchWidget::SaveCloudLightingDefaultsToConfig(Lumen, Path, &LocalConfig));
    TestTrue(TEXT("Saving identical cloud defaults remains successful"),
        Branch && SDevKitRenderStyleWorkbenchWidget::SaveCloudLightingDefaultsToConfig(Lumen, Path, &LocalConfig));
    FConfigFile DefaultsAfter; DefaultsAfter.Combine(Path);
    const FString LumenSection = UStylizedLightingSettings::StaticClass()->GetPathName();
    bool bEnabled = false;
    TestTrue(TEXT("Cloud enable key exists"), DefaultsAfter.GetBool(*LumenSection, TEXT("bEnableStylizedLumenLighting"), bEnabled));
    TestTrue(TEXT("Cloud enable is saved"), bEnabled);
    const TCHAR* Keys[] = {TEXT("BandCount"), TEXT("BandSoftness"), TEXT("GlossInfluence"), TEXT("SpecularIntensity"),
        TEXT("SpecularOffset"), TEXT("IndirectBlend"), TEXT("DirectBlend")};
    const float Expected[] = {2.f, .32f, .56f, 1.38f, -.2f, .86f, 1.f};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Keys); ++Index)
    {
        float Value = 0.f;
        TestTrue(*FString::Printf(TEXT("Original cloud default key exists: %s"), Keys[Index]), DefaultsAfter.GetFloat(*LumenSection, Keys[Index], Value));
        TestEqual(*FString::Printf(TEXT("Cloud default saved with two-tone safety contract: %s"), Keys[Index]), Value, Expected[Index]);
    }
    const FConfigSection* SavedSection = DefaultsAfter.FindSection(LumenSection);
    TestTrue(TEXT("Cloud defaults section contains exactly eight keys"), SavedSection && SavedSection->Num() == 8);
    for (const TCHAR* Retired : {TEXT("CharacterLightingProfiles"), TEXT("GlobalEnvironment"), TEXT("bEnableCharacterTone"),
        TEXT("IndirectMaxLuminance"), TEXT("ReflectionKuwaharaMode"), TEXT("Strength")})
        TestFalse(*FString::Printf(TEXT("Unrelated defaults are never written: %s"), Retired), DefaultsAfter.GetString(*LumenSection, Retired, Actual));
    TestTrue(TEXT("Eight-key saving preserves unrelated section"), DefaultsAfter.GetString(TEXT("UnrelatedSection"), TEXT("Sentinel"), Actual));
    TestEqual(TEXT("Eight-key saving preserves unrelated value"), Actual, FString(TEXT("Preserve Me")));
    TestTrue(TEXT("Eight-key saving preserves old Lighting serialization"), DefaultsAfter.GetString(*Section, TEXT("Lighting"), Actual));
    TestEqual(TEXT("Eight-key saving never writes generic material lighting"), Actual, SavedLighting);
    Lumen->bEnableStylizedLumenLighting = bBeforeLumen;
    Lumen->BandCount = BeforeValues[0]; Lumen->BandSoftness = BeforeValues[1]; Lumen->GlossInfluence = BeforeValues[2];
    Lumen->SpecularIntensity = BeforeValues[3]; Lumen->SpecularOffset = BeforeValues[4];
    Lumen->IndirectBlend = BeforeValues[5]; Lumen->DirectBlend = BeforeValues[6];
    // Delete only the exact GUID-scoped fixture, never project configuration or assets.
    TestTrue(TEXT("Preset-save fixture removed"), IFileManager::Get().Delete(*Path));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitGraphicsQualityWorkbenchConstructionTest,
    "DevKit.Workflow.GraphicsQualityWorkbenchConstruction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitGraphicsQualityWorkbenchConstructionTest::RunTest(const FString& Parameters)
{
    const Scalability::FQualityLevels Before = Scalability::GetQualityLevels();
    const TSharedRef<SDevKitGraphicsQualityWorkbenchWidget> Widget = SNew(SDevKitGraphicsQualityWorkbenchWidget);
    Widget->SlatePrepass();
    TestTrue(TEXT("Graphics quality panel has a usable desired width"), Widget->GetDesiredSize().X > 0.f);
    TestTrue(TEXT("Graphics quality panel has a usable desired height"), Widget->GetDesiredSize().Y > 0.f);
    TestTrue(TEXT("Constructing the panel does not change active scalability"), Before == Scalability::GetQualityLevels());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitPerformanceWorkbenchConstructionTest,
    "DevKit.Workflow.PerformanceWorkbenchConstruction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitPerformanceWorkbenchConstructionTest::RunTest(const FString& Parameters)
{
    const TSharedRef<SDevKitWorkflowWidget> Widget = SNew(SDevKitWorkflowWidget).WorkflowId(TEXT("performance"));
    Widget->SlatePrepass();
    TestTrue(TEXT("Performance workbench opens with usable content"), Widget->GetDesiredSize().X > 0.f && Widget->GetDesiredSize().Y > 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitWorkflowNativeBootTest,
    "DevKit.Workflow.NativeWidgetBoot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitWorkflowNativeBootTest::RunTest(const FString& Parameters)
{
    const TSharedRef<SDevKitWorkflowWidget> Widget = SNew(SDevKitWorkflowWidget);
    Widget->SlatePrepass();
    TestTrue(TEXT("Workbench has a usable desired width"), Widget->GetDesiredSize().X > 0.f);
    TestTrue(TEXT("Workbench has a usable desired height"), Widget->GetDesiredSize().Y > 0.f);
    for (int32 Page = 0; Page < 3; ++Page)
    {
        if (Page > 0) Widget->ShowOperations();
        Widget->SlatePrepass();
        TestTrue(TEXT("Native page has visible content"), Widget->GetDesiredSize().Y > 0.f);
        if (!FParse::Param(FCommandLine::Get(), TEXT("WorkflowRender"))) continue;
        // Component rendering only: no desktop capture or external window interaction.
        const int32 Width = Page == 2 ? 1000 : 1400;
        const int32 Height = Page == 2 ? 720 : 900;
        FWidgetRenderer Renderer(false);
        UTextureRenderTarget2D* Target = nullptr;
        // AutoWrapText caches its arranged width after the first paint.
        for (int32 Frame = 0; Frame < 3; ++Frame)
        {
            Widget->Invalidate(EInvalidateWidgetReason::Layout);
            Widget->SlatePrepass();
            Target = Renderer.DrawWidget(Widget, FVector2D(Width, Height));
            FlushRenderingCommands();
        }
        if (!TestNotNull(TEXT("Offscreen widget render target"), Target)) return false;
        TArray<FColor> Pixels;
        FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
        ReadFlags.SetLinearToGamma(false);
        if (!TestTrue(TEXT("Rendered widget pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags))) return false;
        // Renderer(false) returns linear pixels. Encode sRGB once for a PNG viewer.
        for (FColor& Pixel : Pixels) Pixel = Pixel.ReinterpretAsLinear().ToFColor(true);
        const FColor ThemeBackground = FStyleColors::Background.GetSpecifiedColor().ToFColor(true);
        const FColor Corner = Pixels[Width - 1];
        TestTrue(TEXT("Component export preserves theme luminance"), FMath::Abs(int32(ThemeBackground.R) - int32(Corner.R)) <= 2);
        AddInfo(FString::Printf(TEXT("Theme background=%s; rendered corner=%s"),
            *ThemeBackground.ToHex(), *Corner.ToHex()));
        TArray64<uint8> Png;
        FImageUtils::PNGCompressImageArray(Width, Height, Pixels, Png);
        const FString Directory = FPaths::ProjectSavedDir() / TEXT("WorkflowHub/Reports");
        IFileManager::Get().MakeDirectory(*Directory, true);
        TestTrue(TEXT("Saved component render"), FFileHelper::SaveArrayToFile(Png,
            *(Directory / (Page == 0 ? TEXT("NativeWorkbench.png") : (Page == 1 ? TEXT("NativeOperations.png") : TEXT("NativeOperationsCompact.png"))))));
    }
    return true;
}

class FDevKitWorkflowWaitCheck : public IAutomationLatentCommand
{
public:
    FDevKitWorkflowWaitCheck(TSharedPtr<SDevKitWorkflowWidget> InWidget, FAutomationTestBase* InTest)
        : Widget(InWidget), Test(InTest), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        if (Widget->bOperationBusy && FPlatformTime::Seconds() - Started < 45.0) return false;
        Test->TestFalse(TEXT("Native check completed without blocking the game thread"), Widget->bOperationBusy);
        Test->TestTrue(TEXT("Native check reports the restored host module set"), Widget->bOperationPassed);
        Test->TestFalse(TEXT("Native check retains inspectable output"), Widget->OperationDetails.IsEmpty());
        Test->AddInfo(Widget->OperationSummary);
        return true;
    }
private:
    TSharedPtr<SDevKitWorkflowWidget> Widget;
    FAutomationTestBase* Test;
    double Started;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitWorkflowNativeCheckTest,
    "DevKit.Workflow.HostModuleCheck", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitWorkflowNativeCheckTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("WorkflowCheck")))
    {
        AddInfo(TEXT("Use -WorkflowCheck for the local Python / host PCB integration test."));
        return true;
    }
    const TSharedRef<SDevKitWorkflowWidget> Widget = SNew(SDevKitWorkflowWidget);
    Widget->ShowOperations();
    Widget->RunReadOnlyCheck(TEXT("buildids"));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDevKitWorkflowWaitCheck>(Widget, this));
    return true;
}

#endif
