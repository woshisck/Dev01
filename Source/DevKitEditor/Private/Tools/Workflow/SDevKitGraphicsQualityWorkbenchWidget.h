#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Scalability.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

struct FDevKitQualityPreset
{
	FString Name;
	TArray<float> Values;
};

struct FDevKitQualityCVarSnapshot
{
	FString Name;
	FString Value;
	EConsoleVariableFlags Flags = ECVF_Default;
};

/** Editor-only preview of the project's overall scalability tiers. */
class SDevKitGraphicsQualityWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitGraphicsQualityWorkbenchWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	friend class FDevKitGraphicsQualityCloudContractTest;
	void CaptureOpeningState();
	FReply SelectTier(int32 Quality);
	FReply ApplySelectedTier();
	FReply RestoreDefaults();
	FReply SavePreset();
	void LoadPreset(const FString& PresetName);
	void LoadPresets();
	void SavePresets() const;
	void BuildDefaultValues();
	void EditParameter(int32 Row, int32 Tier, float Value);
	bool PreviewParameter(int32 Row);
	TSharedRef<SWidget> MakeParameterMatrix();
	TSharedRef<SWidget> MakeLightingDiagnostics();
	TSharedRef<SWidget> MakeParameterRow(int32 RowIndex);
	TSharedRef<SWidget> MakeTierButton(int32 Quality, const TCHAR* Label);
	void OnPresetSelected(TSharedPtr<FString> Preset, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> MakePresetOption(TSharedPtr<FString> Preset) const;
	FReply RestoreProjectDefault();
	FText GetCurrentTierText() const;
	FText GetQualityBreakdownText() const;
	FText GetRenderingPathText() const;
	FText GetLastActionText() const;
	static FText GetTierName(int32 Quality);

	TArray<TSharedPtr<FString>> PresetOptions;
	TArray<FDevKitQualityPreset> Presets;
	TArray<float> CurrentValues;
	TArray<float> LastAppliedValues;
	Scalability::FQualityLevels OpeningQualityLevels;
	TArray<FDevKitQualityCVarSnapshot> OpeningCVars;
	TSharedPtr<FString> SelectedPreset;
	TSharedPtr<class SEditableTextBox> PresetNameBox;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> PresetCombo;
	int32 SelectedTier = 1;
	FString LastAction = TEXT("尚未更改当前编辑器画质。");
};
