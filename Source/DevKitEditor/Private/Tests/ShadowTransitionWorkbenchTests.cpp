#if WITH_DEV_AUTOMATION_TESTS

#include "Tools/Workflow/SDevKitRenderStyleWorkbenchWidget.h"
#include "Tools/Workflow/DevKitCloudLightingFinishTrial.h"

#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StylizedCharacterLighting.h"
#include "StylizedLightingSettings.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    struct FScopedRetiredTransitionState
    {
        UStylizedLightingSettings* Settings = GetMutableDefault<UStylizedLightingSettings>();
        FYogShadowTransitionSettings Scene = Settings->SceneShadowTransition;
        TArray<FStylizedCharacterLightingProfile> Profiles = Settings->CharacterLightingProfiles;
        TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
        uint32 Count = 0;

        FScopedRetiredTransitionState() { GetStylizedCharacterLightingProfiles(Packed, Count); }
        ~FScopedRetiredTransitionState()
        {
            Settings->SceneShadowTransition = Scene;
            Settings->CharacterLightingProfiles = Profiles;
            SetStylizedCharacterLightingProfiles(MakeArrayView(Packed.GetData(), Packed.Num()), Count);
        }
    };

    void CollectRetirementLabels(TSharedRef<SWidget> Widget, FString& Labels, int32& ColorPickers)
    {
        if (Widget->GetType() == FName(TEXT("STextBlock")))
            Labels += StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() + TEXT("\n");
        if (Widget->GetType() == FName(TEXT("SDevKitWorkbenchColorPicker"))) ++ColorPickers;
        FChildren* Children = Widget->GetAllChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
            CollectRetirementLabels(Children->GetChildAt(Index), Labels, ColorPickers);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitShadowTransitionRetirementTest,
    "DevKit.Workflow.ShadowTransition.RetiredControlsAndNeutralUpload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitShadowTransitionRetirementTest::RunTest(const FString&)
{
    if (FDevKitCloudLightingFinishTrial::IsActive())
    {
        AddWarning(TEXT("End the existing finishing trial before testing retired transition isolation."));
        return true;
    }
    FScopedRetiredTransitionState Restore;
    auto* Settings = Restore.Settings;
    Settings->SceneShadowTransition.bEnabled = true;
    Settings->SceneShadowTransition.Tint = FLinearColor(.1f, .8f, .2f, 1.f);
    Settings->SceneShadowTransition.Strength = 1.f;
    Settings->CharacterLightingProfiles.SetNum(1);
    auto& Profile = Settings->CharacterLightingProfiles[0];
    Profile.bOverrideShadowTransition = true;
    Profile.ShadowTransition.bEnabled = true;
    Profile.ShadowTransition.Tint = FLinearColor(.9f, .1f, .1f, 1.f);
    Profile.ShadowTransition.Strength = 1.f;

    TMap<IConsoleVariable*, TPair<FString, EConsoleVariableFlags>> ProtectedCVars;
    for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.IndirectBlend"),
        TEXT("r.StylizedLumenLighting.DirectBlend"), TEXT("r.Shadow.Virtual.Enable"),
        TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"), TEXT("r.DefaultFeature.AutoExposure")})
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
            ProtectedCVars.Add(CVar, TPair<FString, EConsoleVariableFlags>(CVar->GetString(), CVar->GetFlags()));
    TMap<FString, TArray<uint8>> ConfigBefore;
    for (const FString& Path : {FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"), GGameIni})
    {
        TArray<uint8> Bytes;
        if (FFileHelper::LoadFileToArray(Bytes, *Path)) ConfigBefore.Add(Path, MoveTemp(Bytes));
    }
    TMap<TWeakObjectPtr<UPackage>, bool> PackageDirty;
    for (TObjectIterator<UPackage> It; It; ++It)
        if (!It->HasAnyFlags(RF_Transient)) PackageDirty.Add(TWeakObjectPtr<UPackage>(*It), It->IsDirty());

    Settings->ApplyShadowTransitionParameters();
    TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
    uint32 Count = 0;
    GetStylizedCharacterLightingProfiles(Packed, Count);
    const auto BeforeLegacyChange = Packed;
    Settings->SceneShadowTransition.Tint = FLinearColor::Green;
    Profile.ShadowTransition.Tint = FLinearColor::Blue;
    Settings->ApplyShadowTransitionParameters();
    GetStylizedCharacterLightingProfiles(Packed,Count);
    for (int32 Index=0;Index<4;++Index)
        TestEqual(TEXT("Legacy shadow tint cannot affect new metal controls"),Packed[Index],BeforeLegacyChange[Index]);

    TSharedPtr<SDevKitRenderStyleWorkbenchWidget> Widget = SNew(SDevKitRenderStyleWorkbenchWidget);
    FString Labels;
    int32 ColorPickers = 0;
    CollectRetirementLabels(Widget.ToSharedRef(), Labels, ColorPickers);
    TestFalse(TEXT("Retired transition section is absent"), Labels.Contains(TEXT("暗部交界颜色")));
    TestFalse(TEXT("Retired transition override action is absent"), Labels.Contains(TEXT("覆写通用过渡")));
    TestFalse(TEXT("Retired transition persistence action is absent"), Labels.Contains(TEXT("保存过渡及角色")));
    TestEqual(TEXT("Only active metal highlight contributes a color picker"), ColorPickers, 1);
    TestTrue(TEXT("Scene two-band controls remain available"), Labels.Contains(TEXT("01 二分光影设置")));
    TestTrue(TEXT("Remaining character section is renumbered"), Labels.Contains(TEXT("02 角色受光开关")));
    Widget.Reset();
    for (const auto& Pair : ProtectedCVars)
    {
        TestEqual(TEXT("Retired transition upload and UI do not change native or scene CVar values"), Pair.Key->GetString(), Pair.Value.Key);
        TestEqual(TEXT("Retired transition upload and UI preserve CVar priority"), Pair.Key->GetFlags(), Pair.Value.Value);
    }
    for (const auto& Pair : ConfigBefore)
    {
        TArray<uint8> After;
        TestTrue(TEXT("Retirement UI does not save project configuration"), FFileHelper::LoadFileToArray(After, *Pair.Key) && After == Pair.Value);
    }
    for (const auto& Pair : PackageDirty)
        if (const UPackage* Package = Pair.Key.Get())
            TestEqual(TEXT("Retirement UI does not dirty loaded assets"), Package->IsDirty(), Pair.Value);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitShadowTransitionMetadataTest,
    "DevKit.Workflow.ShadowTransition.LegacyDataNotEditable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitShadowTransitionMetadataTest::RunTest(const FString&)
{
    const FProperty* Scene = FindFProperty<FProperty>(UStylizedLightingSettings::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SceneShadowTransition));
    const FProperty* Override = FindFProperty<FProperty>(FStylizedCharacterLightingProfile::StaticStruct(),
        GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, bOverrideShadowTransition));
    const FProperty* Transition = FindFProperty<FProperty>(FStylizedCharacterLightingProfile::StaticStruct(),
        GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ShadowTransition));
    for (const FProperty* Property : {Scene, Override, Transition})
    {
        if (!TestNotNull(TEXT("Legacy transition data remains serializable for compatibility"), Property)) continue;
        TestFalse(TEXT("Legacy transition data is not editable or Blueprint-visible"),
            Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible));
    }
    return true;
}
#endif
