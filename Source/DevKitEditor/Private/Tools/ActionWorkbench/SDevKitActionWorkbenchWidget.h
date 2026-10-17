#pragma once

#include "CoreMinimal.h"
#include "Tools/ActionWorkbench/DevKitActionWorkbenchModel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

class IDetailsView;
class SBox;
class SWidgetSwitcher;
class SDockTab;
class FSpawnTabArgs;
struct FAssetData;

/** Native authoring UI. Opening or inspecting it never edits the live conflict asset. */
class SDevKitActionWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitActionWorkbenchWidget) : _InitialAsset(nullptr) {}
		SLATE_ARGUMENT(UStateConflictDataAsset*, InitialAsset)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual ~SDevKitActionWorkbenchWidget() override;
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	bool CanClose();
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);
	static void RegisterTab();
	static void UnregisterTab();
	static void OpenTab();

private:
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildCatalog();
	TSharedRef<SWidget> BuildMatrix();
	TSharedRef<SWidget> BuildInspector();
	TSharedRef<SWidget> BuildDiagnostics();
	TSharedRef<SWidget> BuildValidation();
	TSharedRef<SWidget> BuildSourcePage();
	void Refresh();
	void LoadAsset(UStateConflictDataAsset* Asset);
	void OnAssetPicked(const FAssetData& Asset);
	void SelectPair(FGameplayTag Request, FGameplayTag Current);
	void RefreshActors();
	void RefreshDiagnosticText();
	FText ActionName(FGameplayTag ActionTag) const;
	FText SessionText() const;
	FText SourcePathText() const;
	FReply SetCell(bool bCancel, bool bEnabled);
	FReply ValidateDraft();
	FReply ApplyDraft();
	FReply SaveSource();
	FReply ReloadDraft();
	FReply ExportDraft();
	FReply LocateSource();
	bool CanEdit() const;
	bool CanApply() const;
	bool ConfirmDiscard(const FText& Purpose) const;

	FDevKitActionWorkbenchModel Model;
	TArray<FDevKitActionWorkbenchAction> Actions;
	FGameplayTag RequestTag;
	FGameplayTag CurrentTag;
	FString SearchText;
	FString LastDraftSnapshot;
	FText Status;
	FText Diagnostics;
	bool bAppliedUnsaved = false;
	bool bSourceStale = false;
	bool bObservedPackageSave = false;
	FDelegateHandle PackageSavedHandle;
	int32 Page = 0;
	double NextRefresh = 0;
	TSharedPtr<SBox> CatalogBox;
	TSharedPtr<SBox> MatrixBox;
	TSharedPtr<SBox> InspectorBox;
	TSharedPtr<SBox> ValidationBox;
	TSharedPtr<SWidgetSwitcher> Pages;
	TSharedPtr<IDetailsView> DraftDetails;
	using FActorOption = TSharedPtr<TWeakObjectPtr<AActor>>;
	TArray<FActorOption> ActorOptions;
	FActorOption SelectedActor;
	TSharedPtr<SComboBox<FActorOption>> ActorPicker;
};
