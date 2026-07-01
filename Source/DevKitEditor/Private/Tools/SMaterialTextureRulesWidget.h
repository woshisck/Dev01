#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;
class UTexture2D;
class UYogMaterialTextureNamingConvention;

enum class EMaterialComplianceStatus : uint8
{
	Pass,
	Warning,
	Blocked
};

struct FMaterialComplianceResult
{
	FAssetData AssetData;
	FString AssetName;
	FString PackagePath;
	FString AssetType;
	EMaterialComplianceStatus Status = EMaterialComplianceStatus::Pass;
	TArray<FString> Messages;
};

class SMaterialTextureRulesWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialTextureRulesWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply WriteDefaultTextureNamingConventionAsset();
	FReply OpenDefaultTextureNamingConventionAsset();
	FReply CheckSelectedTextureNaming();
	FReply CheckSelectedMaterialCompliance();
	FReply ScanArtMaterialCompliance();
	void SetStatus(const FText& InStatus) const;
	FText GetUsageText() const;
	FText GetResultsRichText() const;

	TArray<UTexture2D*> CollectSelectedTextures() const;
	TArray<FAssetData> CollectSelectedComplianceAssets() const;
	UYogMaterialTextureNamingConvention* LoadOrCreateDefaultConvention(bool bFillDefaultRules) const;
	void EvaluateTextureAsset(const FAssetData& AssetData, const UYogMaterialTextureNamingConvention* Convention, TArray<FMaterialComplianceResult>& OutResults) const;
	void EvaluateMaterialAsset(const FAssetData& AssetData, const UYogMaterialTextureNamingConvention* Convention, TArray<FMaterialComplianceResult>& OutResults) const;
	void UpdateResults(TArray<FMaterialComplianceResult>&& InResults, const FText& Summary);

	TSharedPtr<STextBlock> StatusTextBlock;
	TArray<FMaterialComplianceResult> LastResults;
};
