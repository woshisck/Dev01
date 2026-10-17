#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/IConsoleManager.h"
#include "Data/RoomDataAsset.h"
#include "Tools/Workflow/DevKitWorkbenchUI.h"
#include "Tools/Workflow/SDevKitWorkflowWidget.h"
#include "Tools/Workflow/SDevKitGraphicsQualityWorkbenchWidget.h"
#include "Tools/SceneWorkbench/SDevKitSceneRoomConfigWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include <limits>

namespace
{
    void GatherAreas(const TSharedRef<SWidget>& Widget, TArray<TSharedRef<SExpandableArea>>& Areas)
    {
        if (Widget->GetType() == FName(TEXT("SExpandableArea")))
            Areas.Add(StaticCastSharedRef<SExpandableArea>(Widget));
        FChildren* Children = Widget->GetAllChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
            GatherAreas(Children->GetChildAt(Index), Areas);
    }

    int32 CountType(const TSharedRef<SWidget>& Widget, FName Type)
    {
        // Template controls report types such as SSpinBox<float>, not SSpinBox.
        int32 Count = Widget->GetType().ToString().StartsWith(Type.ToString()) ? 1 : 0;
        FChildren* Children = Widget->GetAllChildren();
        for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
            Count += CountType(Children->GetChildAt(Index), Type);
        return Count;
    }

    struct FScopedWorkflowStep
    {
        FString Section = TEXT("DevKit.Workflow.performance");
        int32 Value = 0;
        bool bExisted = GConfig->GetInt(*Section, TEXT("Step"), Value, GEditorPerProjectIni);
        ~FScopedWorkflowStep()
        {
            if (bExisted) GConfig->SetInt(*Section, TEXT("Step"), Value, GEditorPerProjectIni);
            else GConfig->RemoveKey(*Section, TEXT("Step"), GEditorPerProjectIni);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitWorkbenchColorPickerTest,
    "DevKit.Workflow.Presentation.NativeColorPickerPreviewAndCancel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitWorkbenchColorPickerTest::RunTest(const FString& Parameters)
{
    FLinearColor Value(0.13f, 0.24f, 0.36f, 0.42f);
    const FLinearColor Original = Value;
    int32 Changes = 0;
    bool bReportedOpen = false;
    const TSharedRef<SDevKitWorkbenchColorPicker> Widget = SNew(SDevKitWorkbenchColorPicker)
        .Color_Lambda([&Value]() { return Value; }).ClampValue(true)
        .OnColorChanged_Lambda([&Value, &Changes](FLinearColor Color) { Value = Color; ++Changes; })
        .OnPickerOpenChanged_Lambda([&bReportedOpen](bool bOpen) { bReportedOpen = bOpen; });
    TestEqual(TEXT("Constructing a swatch does not apply or save a color"), Changes, 0);
    TestEqual(TEXT("Swatch contains a native color block"), CountType(Widget, TEXT("SColorBlock")), 1);
    TestEqual(TEXT("Swatch does not expose raw RGB float boxes"), CountType(Widget, TEXT("SSpinBox")), 0);
    FColorPickerArgs Args = Widget->MakePickerArgs();
    TestTrue(TEXT("Picker open state is available to lock target selection"), bReportedOpen && Widget->IsPickerOpen());
    TestFalse(TEXT("Realtime preview is not deferred to OK"), Args.bOnlyRefreshOnOk);
    TestFalse(TEXT("Realtime preview is not deferred to mouse up"), Args.bOnlyRefreshOnMouseUp);
    TestFalse(TEXT("Unused alpha has no editing control"), Args.bUseAlpha);
    TestTrue(TEXT("Tint range uses native 0..1 color picker policy"), Args.bClampValue);
    const FLinearColor Preview(0.2f, 0.4f, 0.6f, 0.95f);
    Args.OnColorCommitted.Execute(Preview);
    TestTrue(TEXT("Linear RGB values reach the live update delegate unchanged"),
        Value.Equals(FLinearColor(0.2f, 0.4f, 0.6f, Original.A)));
    TestEqual(TEXT("Hidden alpha is preserved"), Value.A, Original.A);
    Args.OnColorCommitted.Execute(FLinearColor(std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), -2.f, 0.f));
    TestTrue(TEXT("Nonfinite channels fall back and bounded tint remains finite"),
        FMath::IsFinite(Value.R) && FMath::IsFinite(Value.G) && FMath::IsFinite(Value.B)
        && Value.R == Original.R && Value.G == Original.G && Value.B == 0.f);
    Args.OnColorPickerCancelled.Execute(FLinearColor::Black);
    TestTrue(TEXT("Cancel restores opening snapshot, not the latest preview or callback argument"), Value.Equals(Original));
    Args.OnColorPickerWindowClosed.Execute(SNew(SWindow));
    TestFalse(TEXT("Closing clears picker session state"), Widget->IsPickerOpen() || bReportedOpen);

    FLinearColor HDR(5.5f, 2.f, 0.4f, 0.65f);
    const FLinearColor HDROriginal = HDR;
    const TSharedRef<SDevKitWorkbenchColorPicker> HDRWidget = SNew(SDevKitWorkbenchColorPicker)
        .Color_Lambda([&HDR]() { return HDR; })
        .OnColorChanged_Lambda([&HDR](FLinearColor Color) { HDR = Color; });
    FColorPickerArgs HDRArgs = HDRWidget->MakePickerArgs();
    TestFalse(TEXT("Existing HDR color semantic is not clamped to SDR by default"), HDRArgs.bClampValue);
    TestTrue(TEXT("Native picker initializes with HDR values"), HDRArgs.InitialColor.Equals(HDROriginal));
    HDRArgs.OnColorCommitted.Execute(FLinearColor(8.f, 3.f, 0.1f, 0.f));
    TestEqual(TEXT("HDR realtime values above one are retained"), HDR.R, 8.f);
    TestEqual(TEXT("HDR hidden alpha is retained"), HDR.A, HDROriginal.A);
    HDRArgs.OnColorPickerCancelled.Execute(FLinearColor::Black);
    TestTrue(TEXT("HDR cancel restores full original data"), HDR.Equals(HDROriginal));
    HDRArgs.OnColorPickerWindowClosed.Execute(SNew(SWindow));

    FLinearColor Alpha(0.1f, 0.2f, 0.3f, 0.4f);
    const TSharedRef<SDevKitWorkbenchColorPicker> AlphaWidget = SNew(SDevKitWorkbenchColorPicker)
        .Color_Lambda([&Alpha]() { return Alpha; }).UseAlpha(true)
        .OnColorChanged_Lambda([&Alpha](FLinearColor Color) { Alpha = Color; });
    FColorPickerArgs AlphaArgs = AlphaWidget->MakePickerArgs();
    AlphaArgs.OnColorCommitted.Execute(FLinearColor(0.5f, 0.6f, 0.7f, 0.8f));
    TestEqual(TEXT("Alpha changes only when explicitly editable"), Alpha.A, 0.8f);
    AlphaArgs.OnColorPickerWindowClosed.Execute(SNew(SWindow));
    TestEqual(TEXT("OK/window close retains latest preview"), Alpha.A, 0.8f);

    // Owner shutdown is an explicit Cancel while its live setter is still valid,
    // not a late helper-destructor callback into an already destroyed owner.
    const TSharedPtr<SColorPicker> UnrelatedNativePicker = GetColorPicker();
    const int32 ChangesBeforeShutdown = Changes;
    FColorPickerArgs ShutdownArgs = Widget->MakePickerArgs();
    ShutdownArgs.OnColorCommitted.Execute(FLinearColor(.8f, .7f, .6f, 1.f));
    Widget->CancelForOwnerShutdown();
    TestTrue(TEXT("Owner shutdown cancels live preview to the opening value"), Value.Equals(Original));
    TestFalse(TEXT("Owner shutdown immediately releases the session lock"), Widget->IsPickerOpen() || bReportedOpen);
    TestTrue(TEXT("A helper without an owned native instance never closes another picker"), GetColorPicker() == UnrelatedNativePicker);
    TestEqual(TEXT("One preview and one Cancel are dispatched"), Changes, ChangesBeforeShutdown + 2);
    Widget->CancelForOwnerShutdown();
    ShutdownArgs.OnColorCommitted.Execute(FLinearColor::Black);
    ShutdownArgs.OnColorPickerCancelled.Execute(FLinearColor::Black);
    ShutdownArgs.OnColorPickerWindowClosed.Execute(SNew(SWindow));
    Widget->OpenPicker();
    TestTrue(TEXT("Late native callbacks after shutdown cannot write or reopen a dead owner's state"), Value.Equals(Original));
    TestEqual(TEXT("Shutdown and delayed window close are idempotent"), Changes, ChangesBeforeShutdown + 2);
    TestFalse(TEXT("Shutdown helper retains no callback capturing its owner"), Widget->ColorChanged.IsBound() || Widget->PickerOpenChanged.IsBound());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitWorkbenchGroupTest,
    "DevKit.Workflow.Presentation.CollapsibleGroupsPreserveControls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitWorkbenchGroupTest::RunTest(const FString& Parameters)
{
    int32 ExpansionChanges = 0;
    const TSharedRef<SWidget> RetainedBody = SNew(STextBlock).Text(FText::FromString(TEXT("Retained body")));
    const TSharedRef<SExpandableArea> Section = DevKitWorkbenchUI::MakeSection(TEXT("Test group"), RetainedBody,
        false, TEXT("Presentation only"), FSlateColor::UseForeground(),
        FOnBooleanValueChanged::CreateLambda([&ExpansionChanges](bool) { ++ExpansionChanges; }));
    TestTrue(TEXT("Main groups are initially expanded"), Section->IsExpanded());
    Section->SetExpanded(false);
    TestFalse(TEXT("Header collapses the body"), Section->IsExpanded());
    Section->SetExpanded(true);
    TestEqual(TEXT("Expansion callback tracks both changes"), ExpansionChanges, 2);
    TestEqual(TEXT("Collapsing retains body widgets rather than rebuilding their values"), CountType(Section, TEXT("STextBlock")), 3);

    TMap<FString, TPair<FString, EConsoleVariableFlags>> ProtectedCVars;
    for (const TCHAR* Name : {TEXT("r.Shadow.Virtual.Enable"), TEXT("r.DynamicGlobalIlluminationMethod"),
        TEXT("r.Lumen.ScreenProbeGather.RadianceCache.NumProbesToTraceBudget"), TEXT("r.Streaming.MipBias")})
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
            ProtectedCVars.Add(Name, TPair<FString, EConsoleVariableFlags>(CVar->GetString(), CVar->GetFlags()));

    FScopedWorkflowStep RestoreWorkflowStep;
    const TSharedRef<SDevKitWorkflowWidget> Workbench = SNew(SDevKitWorkflowWidget).WorkflowId(TEXT("performance"));
    TArray<TSharedRef<SExpandableArea>> NavigationAreas;
    GatherAreas(Workbench->Navigation.ToSharedRef(), NavigationAreas);
    if (!TestEqual(TEXT("Ungrouped performance workflow receives a collapsible navigation category"), NavigationAreas.Num(), 1)) return false;
    NavigationAreas[0]->SetExpanded(false);
    TestTrue(TEXT("Navigation collapse is remembered using stable workflow/group key"),
        Workbench->CollapsedNavigationGroups.FindRef(TEXT("performance:性能")));
    const TSharedPtr<SWidget> CachedPanel = Workbench->ToolCache.FindRef(TEXT("graphics_quality"));
    TestTrue(TEXT("Stable step ID remains selectable"), Workbench->SelectStepById(TEXT("graphics_quality")));
    TestTrue(TEXT("Step selection retains the original tool and draft"),
        Workbench->ToolCache.FindRef(TEXT("graphics_quality")) == CachedPanel);
    NavigationAreas.Reset();
    GatherAreas(Workbench->Navigation.ToSharedRef(), NavigationAreas);
    TestFalse(TEXT("Step change retains category collapse"), NavigationAreas[0]->IsExpanded());
    Workbench->SetSearchText(FText::FromString(TEXT("画质")));
    NavigationAreas.Reset(); GatherAreas(Workbench->Navigation.ToSharedRef(), NavigationAreas);
    TestTrue(TEXT("Search temporarily expands matching navigation"), NavigationAreas.Num() == 1 && NavigationAreas[0]->IsExpanded());
    Workbench->SetSearchText(FText::GetEmpty());
    NavigationAreas.Reset(); GatherAreas(Workbench->Navigation.ToSharedRef(), NavigationAreas);
    TestFalse(TEXT("Clearing search restores remembered collapse"), NavigationAreas[0]->IsExpanded());

    // Exercise the common navigation for EVERY top-level workbench without creating
    // its legacy embedded tool or scanning/changing the tool's authored assets.
    for (const TCHAR* WorkflowId : {TEXT("materials"), TEXT("surface"), TEXT("scene"), TEXT("gameplay"), TEXT("performance")})
    {
        Workbench->WorkflowId = WorkflowId;
        Workbench->Workflows.Reset();
        Workbench->LoadCatalog();
        Workbench->ActiveWorkflow = 0;
        Workbench->ActiveStep = 0;
        Workbench->RebuildNavigation();
        NavigationAreas.Reset(); GatherAreas(Workbench->Navigation.ToSharedRef(), NavigationAreas);
        TestTrue(*FString::Printf(TEXT("%s workbench categories support folding"), WorkflowId), NavigationAreas.Num() > 0);
        if (NavigationAreas.Num())
        {
            NavigationAreas[0]->SetExpanded(false);
            Workbench->RebuildNavigation();
            NavigationAreas.Reset(); GatherAreas(Workbench->Navigation.ToSharedRef(), NavigationAreas);
            TestTrue(*FString::Printf(TEXT("%s folding survives navigation rebuild"), WorkflowId),
                NavigationAreas.Num() > 0 && !NavigationAreas[0]->IsExpanded());
        }
    }
    Workbench->WorkflowId = TEXT("performance");
    Workbench->Workflows.Reset(); Workbench->LoadCatalog();
    Workbench->RebuildNavigation();

    TArray<TSharedRef<SExpandableArea>> QualityAreas;
    GatherAreas(CachedPanel.ToSharedRef(), QualityAreas);
    TestEqual(TEXT("Performance exposes state, presets, five native categories and diagnostics"), QualityAreas.Num(), 8);
    TestEqual(TEXT("All four tiers retain twenty-one editable native parameters"), CountType(CachedPanel.ToSharedRef(), TEXT("SSpinBox")), 84);
    for (const TSharedRef<SExpandableArea>& Area : QualityAreas) Area->SetExpanded(false);
    TestEqual(TEXT("Collapsing categories does not discard editable tier controls"), CountType(CachedPanel.ToSharedRef(), TEXT("SSpinBox")), 84);
    Workbench->ShowOperations();
    TArray<TSharedRef<SExpandableArea>> OperationAreas;
    GatherAreas(Workbench->Content.ToSharedRef(), OperationAreas);
    TestEqual(TEXT("All five release stages collapse independently, plus detailed output"), OperationAreas.Num(), 6);

    const TStrongObjectPtr<URoomDataAsset> Room(NewObject<URoomDataAsset>(GetTransientPackage(), NAME_None, RF_Transactional));
    Room->RoomName = TEXT("L1_CommonLevel_CollapseTest");
    const TSharedRef<SDevKitSceneRoomConfigWidget> RoomForm = SNew(SDevKitSceneRoomConfigWidget);
    RoomForm->SetRoom(Room.Get());
    TArray<TSharedRef<SExpandableArea>> DefaultAreas, GameplayAreas;
    GatherAreas(RoomForm->MakeDefaultsPanel(), DefaultAreas);
    GatherAreas(RoomForm->MakeGameplayPanel(), GameplayAreas);
    TestTrue(TEXT("Room defaults are collapsible"), DefaultAreas.Num() > 0);
    TestTrue(TEXT("Gameplay main categories are independently collapsible"), GameplayAreas.Num() >= 8);
    if (GameplayAreas.Num()) GameplayAreas[0]->SetExpanded(false);
    RoomForm->SetRoom(Room.Get());
    GameplayAreas.Reset(); GatherAreas(RoomForm->MakeGameplayPanel(), GameplayAreas);
    TestTrue(TEXT("Gameplay collapse survives native property refresh"), GameplayAreas.Num() && !GameplayAreas[0]->IsExpanded());
    TestEqual(TEXT("Collapsing does not change authored room identity"), Room->RoomName, FName(TEXT("L1_CommonLevel_CollapseTest")));
    TestEqual(TEXT("Collapsing does not add or remove enemies"), Room->EnemyPool.Num(), 0);
    for (const auto& Pair : ProtectedCVars)
    {
        IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Pair.Key);
        TestEqual(*FString::Printf(TEXT("Presentation does not change %s"), *Pair.Key), CVar->GetString(), Pair.Value.Key);
        TestEqual(*FString::Printf(TEXT("Presentation does not change priority of %s"), *Pair.Key), CVar->GetFlags(), Pair.Value.Value);
    }
    return true;
}

#endif
