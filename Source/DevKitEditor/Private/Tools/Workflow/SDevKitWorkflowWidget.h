#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SVerticalBox;

/** Workflow navigation owns presentation only; existing tools retain their data models. */
class SDevKitWorkflowWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDevKitWorkflowWidget) {}
        SLATE_ARGUMENT(FString, WorkflowId)
        SLATE_EVENT(FSimpleDelegate, OnOpenDecalPlacement)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    ~SDevKitWorkflowWidget();
    static void OrganizeAssets();
    static TSharedRef<SWidget> MakeStylizedLightingSettingsPanel();
    static bool SaveGlobalEnvironmentDefaults(const FString& ConfigPath, FString& Status);
    bool SelectStepById(const FString& StepId);
    void ShowOperations();

private:
    friend class FDevKitWorkflowNativeCheckTest;
    friend class FDevKitWorkflowWaitCheck;
    friend class FDevKitWorkbenchGroupTest;
    struct FStep
    {
        FString Id, Group, Title, Input, Output;
        FName Tab;
        bool bExternal = false;
    };
    struct FWorkflow
    {
        FString Id, Title, Summary;
        TArray<FStep> Steps;
    };

    void LoadCatalog();
    void SelectWorkflow(int32 Index);
    void SelectStep(int32 Index);
    void RebuildNavigation();
    TSharedRef<SWidget> MakeTool(const FStep& Step);
    TSharedRef<SWidget> MakeOperationsPage();
    FReply RunReadOnlyCheck(FString Action);
    FString GetOperationsRoot() const;
    FReply OpenExternal(const FStep& Step);
    FText GetStepSummary() const;
    FText GetStepTitle() const;
    FText GetStepInput() const;
    FText GetStepOutput() const;
    void SetSearchText(const FText& Text);

    TArray<FWorkflow> Workflows;
    FString WorkflowId;
    TMap<FString, TSharedPtr<SWidget>> ToolCache;
    // Presentation state survives step/search navigation, but never enters project assets or runtime settings.
    TMap<FString, bool> CollapsedNavigationGroups;
    TSharedPtr<SVerticalBox> Navigation;
    TSharedPtr<SBox> Content;
    FSimpleDelegate OpenDecalPlacement;
    int32 ActiveWorkflow = 0;
    int32 ActiveStep = 0;
    FString LoadError;
    FString SearchText;
    FString OperationSummary = TEXT("尚未检查。先核对模块，再进行发布预检。所有检查结果都留在此面板。");
    FString OperationDetails;
    bool bOperationBusy = false;
    bool bOperationPassed = false;
};
