#pragma once

#include "CoreMinimal.h"
#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class SWrapBox;

/** Lighting comparisons on authored plugin maps. Construction never opens or edits a map. */
class YOGLOOKDEVEDITOR_API SDevKitCharacterLookDevWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitCharacterLookDevWidget) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	void Activate();
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
	using FReviewOption = TSharedPtr<FDevKitLookDevLevelEntry>;
	void RefreshEntries();
	void RebuildLightingButtons();
	void RefreshLightingTargets(bool bForce = false);
	void SetStatus(bool bSuccess, const FText& Message);
	void OnReviewSelected(FReviewOption Option, ESelectInfo::Type SelectInfo);
	FReply LoadSelectedReview();
	FReply SwitchLighting(FString PackagePath);
	FReply RefreshPanel();
	bool CanLoadReview() const;
	bool CanEditLighting() const;
	FText GetSelectedReviewText() const;
	FText GetCurrentLevelText() const;
	FText GetDirectionalHeading() const;
	FText GetSkyHeading() const;

	TArray<FReviewOption> ReviewOptions;
	TArray<FDevKitLookDevLevelEntry> LightingEntries;
	FReviewOption SelectedReview;
	TSharedPtr<SComboBox<FReviewOption>> ReviewCombo;
	TSharedPtr<SWrapBox> LightingButtons;
	TSharedPtr<IDetailsView> EnvironmentDetails;
	TSharedPtr<IDetailsView> DirectionalDetails;
	TSharedPtr<IDetailsView> SkyDetails;
	TArray<TWeakObjectPtr<UObject>> BoundDirectionalObjects;
	TArray<TWeakObjectPtr<UObject>> BoundSkyObjects;
	TWeakObjectPtr<UObject> BoundRig;
	FString BoundLightingPath;
	FText LightingTargetHint;
	FText StatusText;
	double NextRefreshTime = 0.0;
	int32 BoundRigCount = 0;
	bool bBoundRigComplete = false;
	bool bStatusIsError = false;
	bool bNativeComponentsEditable = true;
};
