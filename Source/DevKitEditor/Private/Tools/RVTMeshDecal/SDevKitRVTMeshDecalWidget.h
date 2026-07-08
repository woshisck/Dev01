#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SCheckBox;
class SEditableTextBox;

class SDevKitRVTMeshDecalWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitRVTMeshDecalWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString GetCurrentWorldPackagePath() const;
	void RefreshDefaultPaths();
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	void SetStatus(const FText& InStatus, bool bInIsError);

	FString GetPlaneMeshObjectPath() const;
	FString GetMaterialObjectPath() const;
	FString GetRuntimeVirtualTextureObjectPath() const;
	void OnPlaneMeshChanged(const FAssetData& AssetData);
	void OnMaterialChanged(const FAssetData& AssetData);
	void OnRuntimeVirtualTextureChanged(const FAssetData& AssetData);

	FReply UseCurrentLevelPaths();
	FReply CreateOrUpdateFoliageType();

	TSharedPtr<SEditableTextBox> FoliageTypeFolderTextBox;
	TSharedPtr<SEditableTextBox> FoliageTypeNameOverrideTextBox;
	FString PlaneMeshObjectPath;
	FString MaterialObjectPath;
	FString RuntimeVirtualTextureObjectPath;
	int32 TranslucencySortPriority = 0;
	float MinScale = 1.0f;
	float MaxScale = 1.0f;
	bool bAlignToNormal = true;
	bool bRandomYaw = true;
	FText StatusText;
	bool bStatusIsError = false;
};
