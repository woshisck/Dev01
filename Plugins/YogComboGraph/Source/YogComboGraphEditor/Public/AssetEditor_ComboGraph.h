#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GenericGraphAssetEditor/AssetEditor_GenericGraph.h"

class IDetailsView;
class IAnimationEditor;
class IPersonaToolkit;
class SBox;
class UAnimMontage;
class UGameplayAbilityComboGraphNode;

/**
 * Custom asset editor for UGameplayAbilityComboGraph.
 * Extends the generic graph editor with PIE debug highlighting:
 * during Play-In-Editor the currently active combo node turns green,
 * mirroring the behaviour of AnimBP state machine debuggers.
 *
 * Active-node lookup goes through IYogComboGraphActiveInstance so this editor
 * has no dependency on any project-specific class.
 */
class YOGCOMBOGRAPHEDITOR_API FAssetEditor_ComboGraph : public FAssetEditor_GenericGraph
{
public:
	FAssetEditor_ComboGraph();
	virtual ~FAssetEditor_ComboGraph();

	void OpenSelectedMontageTab();

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

private:
	virtual void CreateEditorModeManager() override;
	virtual void CreateEdGraph() override;
	virtual void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection) override;

	TSharedRef<SDockTab> SpawnTab_SelectedMontage(const FSpawnTabArgs& Args);
	void UpdateSelectedMontage();
	void RefreshSelectedMontageWorkspace();
	void ResetSelectedMontageWorkspace();
	void ReleaseSelectedMontageWorkspace();
	void BuildSelectedMontageWorkspace(UAnimMontage* Montage);
	TSharedRef<SWidget> MakeMontageWorkspacePlaceholder() const;
	UGameplayAbilityComboGraphNode* GetSingleSelectedComboNode() const;
	FText GetSelectedMontageSummaryText() const;
	FReply OpenSelectedMontage();
	void HandleMontageObjectsSelected(const TArray<UObject*>& SelectedObjects);
	void HandleMontageSectionsChanged();
	void HandleMontageInvokeTab(const FTabId& TabId);

	void OnPIEStarted(bool bIsSimulating);
	void OnPIEEnded(bool bIsSimulating);

	bool TickDebugger(float DeltaTime);
	void ClearDebugState();

	TSharedPtr<IDetailsView> MontageDetailsView;
	TSharedPtr<SBox> MontageViewportHost;
	TSharedPtr<SBox> MontageTimelineHost;
	TSharedPtr<SBox> MontageSectionsHost;
	TSharedPtr<IPersonaToolkit> MontagePersonaToolkit;
	TSharedPtr<IAnimationEditor> MontageAnimationEditorHost;
	TWeakObjectPtr<UGameplayAbilityComboGraphNode> SelectedComboNode;
	TWeakObjectPtr<UAnimMontage> SelectedMontage;
	TWeakObjectPtr<UAnimMontage> WorkspaceMontage;
	FSimpleMulticastDelegate MontageSectionsChanged;

	FTSTicker::FDelegateHandle DebugTickHandle;
	FDelegateHandle PIEStartHandle;
	FDelegateHandle PIEEndHandle;
};
