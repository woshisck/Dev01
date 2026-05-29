#include "UI/EnemyHealthMaterialSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/Package.h"

namespace
{
	constexpr const TCHAR* EnemyHealthMaterialPath = TEXT("/Game/UI/Health_NiagaraUI/M_NiagaraUI_Health_Direct");

	FString ToObjectPath(const FString& PackagePath)
	{
		FString AssetName;
		FString PackageName;
		PackagePath.Split(TEXT("/"), &PackageName, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
	}

	template<typename AssetT>
	AssetT* LoadAssetByPackagePath(const FString& PackagePath)
	{
		return Cast<AssetT>(StaticLoadObject(AssetT::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	template<typename ExpressionT>
	ExpressionT* AddExpression(UMaterial* Material, int32 NodeX, int32 NodeY)
	{
		if (!Material || !Material->GetEditorOnlyData())
		{
			return nullptr;
		}

		ExpressionT* Expression = NewObject<ExpressionT>(Material);
		Expression->MaterialExpressionEditorX = NodeX;
		Expression->MaterialExpressionEditorY = NodeY;
		Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
		return Expression;
	}

	UMaterialExpressionScalarParameter* AddScalarParameter(
		UMaterial* Material,
		const FName ParameterName,
		float DefaultValue,
		int32 NodeX,
		int32 NodeY)
	{
		UMaterialExpressionScalarParameter* Parameter =
			AddExpression<UMaterialExpressionScalarParameter>(Material, NodeX, NodeY);
		if (Parameter)
		{
			Parameter->ParameterName = ParameterName;
			Parameter->DefaultValue = DefaultValue;
			Parameter->Group = TEXT("Enemy Health");
		}
		return Parameter;
	}

	UMaterialExpressionVectorParameter* AddVectorParameter(
		UMaterial* Material,
		const FName ParameterName,
		const FLinearColor& DefaultValue,
		int32 NodeX,
		int32 NodeY)
	{
		UMaterialExpressionVectorParameter* Parameter =
			AddExpression<UMaterialExpressionVectorParameter>(Material, NodeX, NodeY);
		if (Parameter)
		{
			Parameter->ParameterName = ParameterName;
			Parameter->DefaultValue = DefaultValue;
			Parameter->Group = TEXT("Enemy Health");
		}
		return Parameter;
	}

	void AddCustomInput(
		UMaterialExpressionCustom* CustomNode,
		const FName InputName,
		UMaterialExpression* InputExpression,
		int32 OutputIndex = 0)
	{
		if (!CustomNode)
		{
			return;
		}

		FCustomInput& Input = CustomNode->Inputs.AddDefaulted_GetRef();
		Input.InputName = InputName;
		if (InputExpression)
		{
			Input.Input.Connect(OutputIndex, InputExpression);
		}
	}

	UMaterialExpressionComponentMask* AddMask(
		UMaterial* Material,
		UMaterialExpression* Source,
		bool bR,
		bool bG,
		bool bB,
		bool bA,
		int32 NodeX,
		int32 NodeY)
	{
		UMaterialExpressionComponentMask* Mask = AddExpression<UMaterialExpressionComponentMask>(Material, NodeX, NodeY);
		if (Mask)
		{
			Mask->R = bR;
			Mask->G = bG;
			Mask->B = bB;
			Mask->A = bA;
			if (Source)
			{
				Mask->Input.Connect(0, Source);
			}
		}
		return Mask;
	}

	UMaterial* CreateOrResetMaterial()
	{
		const FString PackagePath = EnemyHealthMaterialPath;
		const FString AssetName = FPackageName::GetShortName(PackagePath);
		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		UMaterial* Material = LoadAssetByPackagePath<UMaterial>(PackagePath);
		if (!Material)
		{
			Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
			FAssetRegistryModule::AssetCreated(Material);
		}

		if (UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData())
		{
			Data->ExpressionCollection.Expressions.Empty();
			Data->EmissiveColor.Expression = nullptr;
			Data->Opacity.Expression = nullptr;
			Data->BaseColor.Expression = nullptr;
		}

		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bUsedWithNiagaraSprites = true;

		return Material;
	}

	void BuildMaterialGraph(UMaterial* Material)
	{
		if (!Material || !Material->GetEditorOnlyData())
		{
			return;
		}

		UMaterialExpressionTextureCoordinate* UV = AddExpression<UMaterialExpressionTextureCoordinate>(Material, -1040, -160);
		UMaterialExpressionDynamicParameter* DynamicParameters =
			AddExpression<UMaterialExpressionDynamicParameter>(Material, -1040, 20);
		if (DynamicParameters)
		{
			DynamicParameters->ParamNames.SetNum(4);
			DynamicParameters->ParamNames[0] = TEXT("new");
			DynamicParameters->ParamNames[1] = TEXT("old");
			DynamicParameters->ParamNames[2] = TEXT("armor");
			DynamicParameters->ParamNames[3] = TEXT("Param4");
			DynamicParameters->DefaultValue = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
			DynamicParameters->ParameterIndex = 0;
		}

		UMaterialExpressionScalarParameter* HealthPercent =
			AddScalarParameter(Material, TEXT("HealthPercent"), 1.0f, -760, -80);
		UMaterialExpressionScalarParameter* OldHealthPercent =
			AddScalarParameter(Material, TEXT("OldHealthPercent"), 1.0f, -760, 60);
		UMaterialExpressionScalarParameter* Armor =
			AddScalarParameter(Material, TEXT("Armor"), 0.0f, -760, 200);
		UMaterialExpressionScalarParameter* UseDynamicParameters =
			AddScalarParameter(Material, TEXT("UseDynamicParameters"), 1.0f, -760, 340);
		UMaterialExpressionVectorParameter* RedColor =
			AddVectorParameter(Material, TEXT("RedColor"), FLinearColor(0.853f, 0.0f, 0.0f, 1.0f), -1040, 520);
		UMaterialExpressionVectorParameter* YellowColor =
			AddVectorParameter(Material, TEXT("YellowColor"), FLinearColor(0.9175f, 1.0f, 0.01f, 1.0f), -780, 520);
		UMaterialExpressionVectorParameter* GreenColor =
			AddVectorParameter(Material, TEXT("GreenColor"), FLinearColor(0.11f, 1.0f, 0.11f, 1.0f), -520, 520);
		UMaterialExpressionVectorParameter* BackgroundColor =
			AddVectorParameter(Material, TEXT("BackgroundColor"), FLinearColor(0.135417f, 0.135417f, 0.135417f, 1.0f), -1040, 700);
		UMaterialExpressionVectorParameter* ArmorSilver =
			AddVectorParameter(Material, TEXT("ArmorSilver"), FLinearColor(0.85f, 0.88f, 0.92f, 1.0f), -780, 700);
		UMaterialExpressionScalarParameter* EdgeSoftness =
			AddScalarParameter(Material, TEXT("EdgeSoftness"), 0.005f, -520, 700);

		UMaterialExpressionCustom* CustomNode = AddExpression<UMaterialExpressionCustom>(Material, 40, 120);
		if (CustomNode)
		{
			CustomNode->OutputType = CMOT_Float4;
			CustomNode->IncludeFilePaths.Add(TEXT("/Project/NiagaraEnemyHealthBar.ush"));
			CustomNode->Code = TEXT("return NiagaraEnemyHealthBarOldNew(UV, DynamicNew, DynamicOld, DynamicArmor, HealthPercent, OldHealthPercent, Armor, UseDynamicParameters, RedColor.rgb, YellowColor.rgb, GreenColor.rgb, BackgroundColor.rgb, ArmorSilver.rgb, EdgeSoftness);");
			AddCustomInput(CustomNode, TEXT("UV"), UV);
			AddCustomInput(CustomNode, TEXT("DynamicNew"), DynamicParameters, 0);
			AddCustomInput(CustomNode, TEXT("DynamicOld"), DynamicParameters, 1);
			AddCustomInput(CustomNode, TEXT("DynamicArmor"), DynamicParameters, 2);
			AddCustomInput(CustomNode, TEXT("HealthPercent"), HealthPercent);
			AddCustomInput(CustomNode, TEXT("OldHealthPercent"), OldHealthPercent);
			AddCustomInput(CustomNode, TEXT("Armor"), Armor);
			AddCustomInput(CustomNode, TEXT("UseDynamicParameters"), UseDynamicParameters);
			AddCustomInput(CustomNode, TEXT("RedColor"), RedColor);
			AddCustomInput(CustomNode, TEXT("YellowColor"), YellowColor);
			AddCustomInput(CustomNode, TEXT("GreenColor"), GreenColor);
			AddCustomInput(CustomNode, TEXT("BackgroundColor"), BackgroundColor);
			AddCustomInput(CustomNode, TEXT("ArmorSilver"), ArmorSilver);
			AddCustomInput(CustomNode, TEXT("EdgeSoftness"), EdgeSoftness);
		}

		UMaterialExpressionComponentMask* RGB = AddMask(Material, CustomNode, true, true, true, false, 360, 80);
		UMaterialExpressionComponentMask* Alpha = AddMask(Material, CustomNode, false, false, false, true, 360, 220);

		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		Data->EmissiveColor.Connect(0, RGB);
		Data->Opacity.Connect(0, Alpha);
	}
}

UEnemyHealthMaterialSetupCommandlet::UEnemyHealthMaterialSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UEnemyHealthMaterialSetupCommandlet::Main(const FString& Params)
{
	UMaterial* Material = CreateOrResetMaterial();
	if (!Material)
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyHealthMaterialSetup] Failed to create material %s"), EnemyHealthMaterialPath);
		return 1;
	}

	BuildMaterialGraph(Material);

	Material->PostEditChange();
	Material->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Material->GetPackage());
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	UE_LOG(LogTemp, Display, TEXT("[EnemyHealthMaterialSetup] Created %s"), EnemyHealthMaterialPath);
	return 0;
}
