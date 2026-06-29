#include "MaterialBatch/MaterialBatchMaterialAudit.h"

#include "MaterialBatch/MaterialBatchBuildPlan.h"
#include "MaterialShaderType.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"

namespace
{
FString GetObjectPath(const UObject* Object)
{
	return Object ? Object->GetPathName() : FString();
}

FString GetObjectClassName(const UObject* Object)
{
	return Object && Object->GetClass() ? Object->GetClass()->GetName() : FString();
}

FString GetBlendModeName(const UMaterialInterface* Material)
{
	if (!Material)
	{
		return FString();
	}

	const UEnum* Enum = StaticEnum<EBlendMode>();
	return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Material->GetBlendMode())) : FString::FromInt(static_cast<int32>(Material->GetBlendMode()));
}

FString FormatLinearColor(const FLinearColor& Value)
{
	return FString::Printf(TEXT("(R=%.3f,G=%.3f,B=%.3f,A=%.3f)"), Value.R, Value.G, Value.B, Value.A);
}

FString FormatYesNo(const bool bValue)
{
	return bValue ? TEXT("Yes") : TEXT("No");
}

FString FormatFound(const bool bValue)
{
	return bValue ? TEXT("Yes") : TEXT("No");
}

FString GetParentMaterialPath(const UMaterialInterface* Material)
{
	const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
	return MaterialInstance && MaterialInstance->Parent ? MaterialInstance->Parent->GetPathName() : FString();
}
}

FMaterialBatchMaterialAuditResult FMaterialBatchMaterialAuditBuilder::AuditMaterial(const FString& MaterialPath)
{
	FMaterialBatchMaterialAuditResult Result;
	Result.MaterialPath = MaterialPath;

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
	if (!Material)
	{
		Result.LoadFailureReason = TEXT("Could not load material as UMaterialInterface.");
		return Result;
	}

	Result.bLoaded = true;
	Result.MaterialName = Material->GetName();
	Result.MaterialClass = GetObjectClassName(Material);
	Result.ParentMaterialPath = GetParentMaterialPath(Material);
	Result.BlendMode = GetBlendModeName(Material);
	Result.ShadingModel = GetShadingModelFieldString(Material->GetShadingModels());
	Result.bTwoSided = Material->IsTwoSided();
	Result.bMasked = Material->IsMasked();
	Result.bUsesWorldPositionOffset = false;

	TArray<FMaterialParameterInfo> TextureParameterInfos;
	TArray<FGuid> TextureParameterIds;
	Material->GetAllTextureParameterInfo(TextureParameterInfos, TextureParameterIds);
	TextureParameterInfos.Sort([](const FMaterialParameterInfo& Left, const FMaterialParameterInfo& Right)
	{
		return Left.ToString() < Right.ToString();
	});
	for (const FMaterialParameterInfo& ParameterInfo : TextureParameterInfos)
	{
		UTexture* Texture = nullptr;
		const bool bFoundTexture = Material->GetTextureParameterValue(FHashedMaterialParameterInfo(ParameterInfo), Texture);

		FMaterialBatchMaterialTextureParameter Row;
		Row.ChannelName = FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(ParameterInfo.Name.ToString());
		Row.ParameterName = ParameterInfo.Name.ToString();
		Row.TexturePath = GetObjectPath(Texture);
		Row.TextureClass = GetObjectClassName(Texture);
		Row.bFoundTexture = bFoundTexture && Texture != nullptr;
		Result.TextureParameters.Add(Row);
	}

	TArray<FMaterialParameterInfo> ScalarParameterInfos;
	TArray<FGuid> ScalarParameterIds;
	Material->GetAllScalarParameterInfo(ScalarParameterInfos, ScalarParameterIds);
	ScalarParameterInfos.Sort([](const FMaterialParameterInfo& Left, const FMaterialParameterInfo& Right)
	{
		return Left.ToString() < Right.ToString();
	});
	for (const FMaterialParameterInfo& ParameterInfo : ScalarParameterInfos)
	{
		float Value = 0.0f;
		const bool bFoundValue = Material->GetScalarParameterValue(FHashedMaterialParameterInfo(ParameterInfo), Value);

		FMaterialBatchMaterialScalarParameter Row;
		Row.ParameterName = ParameterInfo.Name.ToString();
		Row.Value = Value;
		Row.bFoundValue = bFoundValue;
		Result.ScalarParameters.Add(Row);
	}

	TArray<FMaterialParameterInfo> VectorParameterInfos;
	TArray<FGuid> VectorParameterIds;
	Material->GetAllVectorParameterInfo(VectorParameterInfos, VectorParameterIds);
	VectorParameterInfos.Sort([](const FMaterialParameterInfo& Left, const FMaterialParameterInfo& Right)
	{
		return Left.ToString() < Right.ToString();
	});
	for (const FMaterialParameterInfo& ParameterInfo : VectorParameterInfos)
	{
		FLinearColor Value = FLinearColor::Black;
		const bool bFoundValue = Material->GetVectorParameterValue(FHashedMaterialParameterInfo(ParameterInfo), Value);

		FMaterialBatchMaterialVectorParameter Row;
		Row.ParameterName = ParameterInfo.Name.ToString();
		Row.Value = FormatLinearColor(Value);
		Row.bFoundValue = bFoundValue;
		Result.VectorParameters.Add(Row);
	}

#if WITH_EDITORONLY_DATA
	TArray<FMaterialParameterInfo> StaticSwitchParameterInfos;
	TArray<FGuid> StaticSwitchParameterIds;
	Material->GetAllStaticSwitchParameterInfo(StaticSwitchParameterInfos, StaticSwitchParameterIds);
	StaticSwitchParameterInfos.Sort([](const FMaterialParameterInfo& Left, const FMaterialParameterInfo& Right)
	{
		return Left.ToString() < Right.ToString();
	});
	for (const FMaterialParameterInfo& ParameterInfo : StaticSwitchParameterInfos)
	{
		bool bValue = false;
		FGuid ExpressionGuid;
		const bool bFoundValue = Material->GetStaticSwitchParameterValue(FHashedMaterialParameterInfo(ParameterInfo), bValue, ExpressionGuid);

		FMaterialBatchMaterialStaticSwitchParameter Row;
		Row.ParameterName = ParameterInfo.Name.ToString();
		Row.bValue = bValue;
		Row.bFoundValue = bFoundValue;
		Result.StaticSwitchParameters.Add(Row);
	}
#endif

	return Result;
}

TArray<FString> FMaterialBatchMaterialAuditBuilder::BuildMarkdownReport(const FMaterialBatchMaterialAuditResult& Result)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("# Material Batch Material Audit"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Material: `%s`"), Result.MaterialPath.IsEmpty() ? TEXT("(not set)") : *Result.MaterialPath));
	Lines.Add(FString::Printf(TEXT("- Loaded: %s"), *FormatYesNo(Result.bLoaded)));
	if (!Result.bLoaded)
	{
		Lines.Add(FString::Printf(TEXT("- Failure: %s"), Result.LoadFailureReason.IsEmpty() ? TEXT("Unknown") : *Result.LoadFailureReason));
	}
	Lines.Add(FString::Printf(TEXT("- Name: `%s`"), Result.MaterialName.IsEmpty() ? TEXT("") : *Result.MaterialName));
	Lines.Add(FString::Printf(TEXT("- Class: `%s`"), Result.MaterialClass.IsEmpty() ? TEXT("") : *Result.MaterialClass));
	Lines.Add(FString::Printf(TEXT("- Parent: `%s`"), Result.ParentMaterialPath.IsEmpty() ? TEXT("") : *Result.ParentMaterialPath));
	Lines.Add(FString::Printf(TEXT("- BlendMode: %s"), Result.BlendMode.IsEmpty() ? TEXT("(unknown)") : *Result.BlendMode));
	Lines.Add(FString::Printf(TEXT("- ShadingModel: %s"), Result.ShadingModel.IsEmpty() ? TEXT("(unknown)") : *Result.ShadingModel));
	Lines.Add(FString::Printf(TEXT("- TwoSided: %s"), *FormatYesNo(Result.bTwoSided)));
	Lines.Add(FString::Printf(TEXT("- Masked: %s"), *FormatYesNo(Result.bMasked)));
	Lines.Add(FString::Printf(TEXT("- UsesWorldPositionOffset: %s"), *FormatYesNo(Result.bUsesWorldPositionOffset)));
	Lines.Add(TEXT(""));

	Lines.Add(TEXT("## Texture Parameters"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Channel | Parameter | Texture | TextureClass | Found |"));
	Lines.Add(TEXT("| --- | --- | --- | --- | --- |"));
	if (Result.TextureParameters.IsEmpty())
	{
		Lines.Add(TEXT("| Unknown | `` | `` |  | No |"));
	}
	else
	{
		for (const FMaterialBatchMaterialTextureParameter& Row : Result.TextureParameters)
		{
			Lines.Add(FString::Printf(
				TEXT("| %s | `%s` | `%s` | %s | %s |"),
				Row.ChannelName.IsEmpty() ? TEXT("Unknown") : *Row.ChannelName,
				Row.ParameterName.IsEmpty() ? TEXT("") : *Row.ParameterName,
				Row.TexturePath.IsEmpty() ? TEXT("") : *Row.TexturePath,
				Row.TextureClass.IsEmpty() ? TEXT("") : *Row.TextureClass,
				*FormatFound(Row.bFoundTexture)));
		}
	}
	Lines.Add(TEXT(""));

	Lines.Add(TEXT("## Scalar Parameters"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Parameter | Value | Found |"));
	Lines.Add(TEXT("| --- | ---: | --- |"));
	if (Result.ScalarParameters.IsEmpty())
	{
		Lines.Add(TEXT("| `` | 0.000 | No |"));
	}
	else
	{
		for (const FMaterialBatchMaterialScalarParameter& Row : Result.ScalarParameters)
		{
			Lines.Add(FString::Printf(
				TEXT("| `%s` | %.3f | %s |"),
				Row.ParameterName.IsEmpty() ? TEXT("") : *Row.ParameterName,
				Row.Value,
				*FormatFound(Row.bFoundValue)));
		}
	}
	Lines.Add(TEXT(""));

	Lines.Add(TEXT("## Vector Parameters"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Parameter | Value | Found |"));
	Lines.Add(TEXT("| --- | --- | --- |"));
	if (Result.VectorParameters.IsEmpty())
	{
		Lines.Add(TEXT("| `` |  | No |"));
	}
	else
	{
		for (const FMaterialBatchMaterialVectorParameter& Row : Result.VectorParameters)
		{
			Lines.Add(FString::Printf(
				TEXT("| `%s` | `%s` | %s |"),
				Row.ParameterName.IsEmpty() ? TEXT("") : *Row.ParameterName,
				Row.Value.IsEmpty() ? TEXT("") : *Row.Value,
				*FormatFound(Row.bFoundValue)));
		}
	}
	Lines.Add(TEXT(""));

	Lines.Add(TEXT("## Static Switch Parameters"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Parameter | Value | Found |"));
	Lines.Add(TEXT("| --- | --- | --- |"));
	if (Result.StaticSwitchParameters.IsEmpty())
	{
		Lines.Add(TEXT("| `` | false | No |"));
	}
	else
	{
		for (const FMaterialBatchMaterialStaticSwitchParameter& Row : Result.StaticSwitchParameters)
		{
			Lines.Add(FString::Printf(
				TEXT("| `%s` | %s | %s |"),
				Row.ParameterName.IsEmpty() ? TEXT("") : *Row.ParameterName,
				Row.bValue ? TEXT("true") : TEXT("false"),
				*FormatFound(Row.bFoundValue)));
		}
	}
	Lines.Add(TEXT(""));

	Lines.Add(TEXT("## Batch Compatibility Notes"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Texture parameters are classified for Texture2DArray channel planning only; this report does not generate arrays."));
	Lines.Add(TEXT("- Scalar/vector parameters are candidates for the future property texture row layout."));
	Lines.Add(TEXT("- Static switch differences still split batches unless converted into runtime parameters in the batch material."));
	Lines.Add(TEXT("- Blend mode, shading model, two-sided state, masked state, and WPO usage must remain compatible inside one batch."));

	return Lines;
}
