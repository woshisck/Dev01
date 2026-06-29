#pragma once

#include "CoreMinimal.h"

struct FMaterialBatchMaterialTextureParameter
{
	FString ChannelName = TEXT("Unknown");
	FString ParameterName;
	FString TexturePath;
	FString TextureClass;
	bool bFoundTexture = false;
};

struct FMaterialBatchMaterialScalarParameter
{
	FString ParameterName;
	double Value = 0.0;
	bool bFoundValue = false;
};

struct FMaterialBatchMaterialVectorParameter
{
	FString ParameterName;
	FString Value;
	bool bFoundValue = false;
};

struct FMaterialBatchMaterialStaticSwitchParameter
{
	FString ParameterName;
	bool bValue = false;
	bool bFoundValue = false;
};

struct FMaterialBatchMaterialAuditResult
{
	FString MaterialPath;
	FString MaterialName;
	FString MaterialClass;
	FString ParentMaterialPath;
	bool bLoaded = false;
	FString LoadFailureReason;
	FString BlendMode;
	FString ShadingModel;
	bool bTwoSided = false;
	bool bMasked = false;
	bool bUsesWorldPositionOffset = false;
	TArray<FMaterialBatchMaterialTextureParameter> TextureParameters;
	TArray<FMaterialBatchMaterialScalarParameter> ScalarParameters;
	TArray<FMaterialBatchMaterialVectorParameter> VectorParameters;
	TArray<FMaterialBatchMaterialStaticSwitchParameter> StaticSwitchParameters;
};

class FMaterialBatchMaterialAuditBuilder
{
public:
	static FMaterialBatchMaterialAuditResult AuditMaterial(const FString& MaterialPath);
	static TArray<FString> BuildMarkdownReport(const FMaterialBatchMaterialAuditResult& Result);
};
