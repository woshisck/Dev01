#include "Water/WaterStyleAssetSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "Water/CheapWaterSurfaceActor.h"
#include "Water/InteractiveWaterVolume.h"
#include "Water/WaterCausticsDecalActor.h"

namespace
{
	const FString RootPath = TEXT("/Game/Water/ValveStyle");
	const FString MaterialRoot = RootPath + TEXT("/Materials");
	const FString BlueprintRoot = RootPath + TEXT("/Blueprints");

	const FString SurfaceMaterialPath = MaterialRoot + TEXT("/M_WaterValve_Surface");
	const FString CausticsMaterialPath = MaterialRoot + TEXT("/M_WaterValve_CausticsDecal");
	const FString StampMaterialPath = MaterialRoot + TEXT("/M_WaterValve_InteractionStamp");
	const FString DecayMaterialPath = MaterialRoot + TEXT("/M_WaterValve_InteractionDecay");
	const FString CheapCleanMIPath = MaterialRoot + TEXT("/MI_WaterValve_Cheap_Clean");
	const FString CheapMuddyMIPath = MaterialRoot + TEXT("/MI_WaterValve_Cheap_Muddy");
	const FString CheapOilMIPath = MaterialRoot + TEXT("/MI_WaterValve_Cheap_OilFilm");
	const FString InteractiveMIPath = MaterialRoot + TEXT("/MI_WaterValve_Interactive_Default");
	const FString CausticsMIPath = MaterialRoot + TEXT("/MI_WaterValve_Caustics_Default");

	const FString CheapBlueprintPath = BlueprintRoot + TEXT("/BP_WaterValve_CheapSurface");
	const FString InteractiveBlueprintPath = BlueprintRoot + TEXT("/BP_WaterValve_InteractiveVolume");
	const FString CausticsBlueprintPath = BlueprintRoot + TEXT("/BP_WaterValve_CausticsDecal");

	const FString CausticsTexturePath = TEXT("/Game/Art/Material/WaterMaterial/Texture/T_Caustics");
	const FString DefaultWaterNormalPath = TEXT("/Game/Art/Material/WaterMaterial/Texture/T_Water_01_N");
	const FString DefaultNoisePath = TEXT("/Game/Art/Material/WaterMaterial/Texture/T_Noise_RGB_03");

	FString ToObjectPath(const FString& PackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		return PackagePath + TEXT(".") + AssetName;
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

	UMaterialExpressionScalarParameter* AddScalarParameter(UMaterial* Material, const FName ParameterName, float DefaultValue, int32 NodeX, int32 NodeY, const FName Group = TEXT("Valve Water"))
	{
		UMaterialExpressionScalarParameter* Parameter = AddExpression<UMaterialExpressionScalarParameter>(Material, NodeX, NodeY);
		if (Parameter)
		{
			Parameter->ParameterName = ParameterName;
			Parameter->DefaultValue = DefaultValue;
			Parameter->Group = Group;
		}
		return Parameter;
	}

	UMaterialExpressionVectorParameter* AddVectorParameter(UMaterial* Material, const FName ParameterName, const FLinearColor& DefaultValue, int32 NodeX, int32 NodeY, const FName Group = TEXT("Valve Water"))
	{
		UMaterialExpressionVectorParameter* Parameter = AddExpression<UMaterialExpressionVectorParameter>(Material, NodeX, NodeY);
		if (Parameter)
		{
			Parameter->ParameterName = ParameterName;
			Parameter->DefaultValue = DefaultValue;
			Parameter->Group = Group;
		}
		return Parameter;
	}

	UMaterialExpressionTextureObjectParameter* AddTextureObjectParameter(UMaterial* Material, const FName ParameterName, UTexture* DefaultTexture, int32 NodeX, int32 NodeY, const FName Group = TEXT("Valve Water"))
	{
		UMaterialExpressionTextureObjectParameter* Parameter = AddExpression<UMaterialExpressionTextureObjectParameter>(Material, NodeX, NodeY);
		if (Parameter)
		{
			Parameter->ParameterName = ParameterName;
			Parameter->Texture = DefaultTexture;
			Parameter->Group = Group;
		}
		return Parameter;
	}

	void AddCustomInput(UMaterialExpressionCustom* CustomNode, const FName InputName, UMaterialExpression* InputExpression, int32 OutputIndex = 0)
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

	UMaterialExpressionComponentMask* AddMask(UMaterial* Material, UMaterialExpression* Source, bool R, bool G, bool B, bool A, int32 NodeX, int32 NodeY)
	{
		UMaterialExpressionComponentMask* Mask = AddExpression<UMaterialExpressionComponentMask>(Material, NodeX, NodeY);
		if (Mask)
		{
			Mask->R = R;
			Mask->G = G;
			Mask->B = B;
			Mask->A = A;
			if (Source)
			{
				Mask->Input.Connect(0, Source);
			}
		}
		return Mask;
	}

	void FinalizeMaterial(UMaterial* Material, TArray<UPackage*>& DirtyPackages)
	{
		FAssetRegistryModule::AssetCreated(Material);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		DirtyPackages.AddUnique(Material->GetPackage());
	}

	UMaterial* CreateSurfaceMaterial(TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UMaterial* Existing = LoadAssetByPackagePath<UMaterial>(SurfaceMaterialPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *SurfaceMaterialPath));
			return Existing;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(SurfaceMaterialPath);
		UPackage* Package = CreatePackage(*SurfaceMaterialPath);
		UMaterial* Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Material || !Material->GetEditorOnlyData())
		{
			ReportLines.Add(TEXT("- Failed to create water surface material."));
			return nullptr;
		}

		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = true;

		UTexture* DefaultWaterNormal = LoadAssetByPackagePath<UTexture>(DefaultWaterNormalPath);
		UTexture* DefaultNoise = LoadAssetByPackagePath<UTexture>(DefaultNoisePath);

		UMaterialExpressionWorldPosition* WorldPos = AddExpression<UMaterialExpressionWorldPosition>(Material, -1180, -200);
		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(Material, -1180, 0);
		UMaterialExpressionVectorParameter* BaseColor = AddVectorParameter(Material, TEXT("BaseWaterColor"), FLinearColor(0.10f, 0.16f, 0.14f, 1.0f), -1180, -520);
		UMaterialExpressionVectorParameter* DepthColor = AddVectorParameter(Material, TEXT("DepthColor"), FLinearColor(0.05f, 0.08f, 0.06f, 1.0f), -1180, -400);
		UMaterialExpressionVectorParameter* FoamColor = AddVectorParameter(Material, TEXT("FoamColor"), FLinearColor(0.82f, 0.82f, 0.74f, 1.0f), -1180, -760);
		UMaterialExpressionVectorParameter* TurbidityColor = AddVectorParameter(Material, TEXT("TurbidityColor"), FLinearColor(0.32f, 0.22f, 0.12f, 1.0f), -1180, -640);
		UMaterialExpressionVectorParameter* Bounds = AddVectorParameter(Material, TEXT("WaterInteractionWorldBounds"), FLinearColor(0, 0, 1000, 1000), -1180, 140);
		UMaterialExpressionTextureObjectParameter* InteractionRT = AddTextureObjectParameter(Material, TEXT("WaterInteractionRT"), DefaultNoise, -1180, 280);
		UMaterialExpressionTextureObjectParameter* Layer1Texture = AddTextureObjectParameter(Material, TEXT("AttachmentLayer1Texture"), DefaultNoise, -1180, 430, TEXT("Valve Water Attachments"));
		UMaterialExpressionTextureObjectParameter* Layer2Texture = AddTextureObjectParameter(Material, TEXT("AttachmentLayer2Texture"), DefaultNoise, -1180, 560, TEXT("Valve Water Attachments"));
		UMaterialExpressionTextureObjectParameter* NormalTexture = AddTextureObjectParameter(Material, TEXT("NormalA"), DefaultWaterNormal, -1180, 690);

		UMaterialExpressionScalarParameter* Opacity = AddScalarParameter(Material, TEXT("Opacity"), 0.65f, -880, -520);
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(Material, TEXT("Roughness"), 0.18f, -880, -400);
		UMaterialExpressionScalarParameter* SpecularIntensity = AddScalarParameter(Material, TEXT("SpecularIntensity"), 1.0f, -880, -280);
		UMaterialExpressionScalarParameter* FoamStrength = AddScalarParameter(Material, TEXT("FoamStrength"), 1.0f, -880, -160);
		UMaterialExpressionScalarParameter* TurbidityStrength = AddScalarParameter(Material, TEXT("TurbidityStrength"), 1.0f, -880, -40);
		UMaterialExpressionScalarParameter* GlintIntensity = AddScalarParameter(Material, TEXT("GlintIntensity"), 1.0f, -880, 80);
		UMaterialExpressionScalarParameter* GlintThreshold = AddScalarParameter(Material, TEXT("GlintThreshold"), 0.72f, -880, 200);
		UMaterialExpressionScalarParameter* Layer1Intensity = AddScalarParameter(Material, TEXT("AttachmentLayer1Intensity"), 0.35f, -880, 430, TEXT("Valve Water Attachments"));
		UMaterialExpressionScalarParameter* Layer2Intensity = AddScalarParameter(Material, TEXT("AttachmentLayer2Intensity"), 0.0f, -880, 560, TEXT("Valve Water Attachments"));
		UMaterialExpressionScalarParameter* WaveScale = AddScalarParameter(Material, TEXT("WaveScale"), 1.0f, -880, 690);

		UMaterialExpressionCustom* ColorNode = AddExpression<UMaterialExpressionCustom>(Material, -450, -420);
		if (ColorNode)
		{
			ColorNode->Description = TEXT("Valve-style cheap/interactive water color");
			ColorNode->OutputType = CMOT_Float3;
			ColorNode->Code = TEXT(
				"float2 size = max(WaterInteractionWorldBounds.zw, float2(1.0, 1.0));\n"
				"float2 uv = saturate((WorldPosition.xy - WaterInteractionWorldBounds.xy) / size);\n"
				"float4 rt = Texture2DSample(WaterInteractionRT, WaterInteractionRTSampler, uv);\n"
				"float2 layerUV = WorldPosition.xy * 0.004 * max(WaveScale, 0.001);\n"
				"float l1 = Texture2DSample(AttachmentLayer1Texture, AttachmentLayer1TextureSampler, layerUV + Time.xx * float2(0.03, 0.018)).r * AttachmentLayer1Intensity;\n"
				"float l2 = Texture2DSample(AttachmentLayer2Texture, AttachmentLayer2TextureSampler, layerUV * 0.57 + Time.xx * float2(-0.012, 0.02)).g * AttachmentLayer2Intensity;\n"
				"float debrisOpen = saturate(rt.a);\n"
				"float attach = saturate((l1 + l2) * (1.0 - debrisOpen));\n"
				"float foam = saturate(rt.g * FoamStrength);\n"
				"float turbidity = saturate(rt.b * TurbidityStrength + attach * 0.35);\n"
				"float sparkle = smoothstep(GlintThreshold, 1.0, rt.a + rt.r * 0.35) * (0.55 + 0.45 * sin(Time * 42.0 + rt.r * 23.0));\n"
				"float3 color = lerp(BaseWaterColor.rgb, DepthColor.rgb, 0.25);\n"
				"color = lerp(color, TurbidityColor.rgb, turbidity);\n"
				"color = lerp(color, float3(0.72, 0.68, 0.50), attach * 0.45);\n"
				"color += FoamColor.rgb * foam * 0.8;\n"
				"color += float3(1.0, 0.86, 0.52) * sparkle * GlintIntensity;\n"
				"return saturate(color);");
			AddCustomInput(ColorNode, TEXT("WorldPosition"), WorldPos);
			AddCustomInput(ColorNode, TEXT("Time"), Time);
			AddCustomInput(ColorNode, TEXT("BaseWaterColor"), BaseColor);
			AddCustomInput(ColorNode, TEXT("DepthColor"), DepthColor);
			AddCustomInput(ColorNode, TEXT("FoamColor"), FoamColor);
			AddCustomInput(ColorNode, TEXT("TurbidityColor"), TurbidityColor);
			AddCustomInput(ColorNode, TEXT("WaterInteractionWorldBounds"), Bounds);
			AddCustomInput(ColorNode, TEXT("WaterInteractionRT"), InteractionRT);
			AddCustomInput(ColorNode, TEXT("AttachmentLayer1Texture"), Layer1Texture);
			AddCustomInput(ColorNode, TEXT("AttachmentLayer2Texture"), Layer2Texture);
			AddCustomInput(ColorNode, TEXT("Opacity"), Opacity);
			AddCustomInput(ColorNode, TEXT("FoamStrength"), FoamStrength);
			AddCustomInput(ColorNode, TEXT("TurbidityStrength"), TurbidityStrength);
			AddCustomInput(ColorNode, TEXT("GlintIntensity"), GlintIntensity);
			AddCustomInput(ColorNode, TEXT("GlintThreshold"), GlintThreshold);
			AddCustomInput(ColorNode, TEXT("AttachmentLayer1Intensity"), Layer1Intensity);
			AddCustomInput(ColorNode, TEXT("AttachmentLayer2Intensity"), Layer2Intensity);
			AddCustomInput(ColorNode, TEXT("WaveScale"), WaveScale);
		}

		UMaterialExpressionCustom* NormalNode = AddExpression<UMaterialExpressionCustom>(Material, -450, 140);
		if (NormalNode)
		{
			NormalNode->Description = TEXT("Water normal with ripple RT");
			NormalNode->OutputType = CMOT_Float3;
			NormalNode->Code = TEXT(
				"float2 size = max(WaterInteractionWorldBounds.zw, float2(1.0, 1.0));\n"
				"float2 uv = saturate((WorldPosition.xy - WaterInteractionWorldBounds.xy) / size);\n"
				"float2 texel = 1.0 / max(size, float2(1.0, 1.0));\n"
				"float h = Texture2DSample(WaterInteractionRT, WaterInteractionRTSampler, uv).r;\n"
				"float hx = Texture2DSample(WaterInteractionRT, WaterInteractionRTSampler, uv + float2(texel.x, 0)).r;\n"
				"float hy = Texture2DSample(WaterInteractionRT, WaterInteractionRTSampler, uv + float2(0, texel.y)).r;\n"
				"float2 layerUV = WorldPosition.xy * 0.006 * max(WaveScale, 0.001) + Time.xx * float2(0.025, 0.015);\n"
				"float3 n = Texture2DSample(NormalA, NormalASampler, layerUV).xyz * 2.0 - 1.0;\n"
				"n.xy += float2(h - hx, h - hy) * 3.0;\n"
				"return normalize(float3(n.xy, max(0.25, n.z)));");
			AddCustomInput(NormalNode, TEXT("WorldPosition"), WorldPos);
			AddCustomInput(NormalNode, TEXT("Time"), Time);
			AddCustomInput(NormalNode, TEXT("WaterInteractionWorldBounds"), Bounds);
			AddCustomInput(NormalNode, TEXT("WaterInteractionRT"), InteractionRT);
			AddCustomInput(NormalNode, TEXT("NormalA"), NormalTexture);
			AddCustomInput(NormalNode, TEXT("WaveScale"), WaveScale);
		}

		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		Data->BaseColor.Connect(0, ColorNode);
		Data->EmissiveColor.Connect(0, ColorNode);
		Data->Opacity.Connect(0, Opacity);
		Data->Roughness.Connect(0, Roughness);
		Data->Specular.Connect(0, SpecularIntensity);
		Data->Normal.Connect(0, NormalNode);

		FinalizeMaterial(Material, DirtyPackages);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *SurfaceMaterialPath));
		return Material;
	}

	UMaterial* CreateRenderTargetMaterial(const FString& MaterialPath, bool bDecay, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UMaterial* Existing = LoadAssetByPackagePath<UMaterial>(MaterialPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *MaterialPath));
			return Existing;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(MaterialPath);
		UPackage* Package = CreatePackage(*MaterialPath);
		UMaterial* Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Material || !Material->GetEditorOnlyData())
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create `%s`."), *MaterialPath));
			return nullptr;
		}

		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UTexture* DefaultNoise = LoadAssetByPackagePath<UTexture>(DefaultNoisePath);
		UMaterialExpressionTextureCoordinate* UV = AddExpression<UMaterialExpressionTextureCoordinate>(Material, -1100, -200);
		UMaterialExpressionTextureObjectParameter* Previous = AddTextureObjectParameter(Material, TEXT("PreviousInteractionTexture"), DefaultNoise, -1100, -20);

		UMaterialExpressionCustom* Output = AddExpression<UMaterialExpressionCustom>(Material, -560, -120);
		if (Output)
		{
			Output->Description = bDecay ? TEXT("Water interaction decay") : TEXT("Water interaction stamp");
			Output->OutputType = CMOT_Float4;
			if (bDecay)
			{
				UMaterialExpressionScalarParameter* DeltaSeconds = AddScalarParameter(Material, TEXT("DeltaSeconds"), 0.016f, -1100, 140, TEXT("Valve Water RT"));
				UMaterialExpressionScalarParameter* FoamDecay = AddScalarParameter(Material, TEXT("FoamDecay"), 1.2f, -1100, 260, TEXT("Valve Water RT"));
				UMaterialExpressionScalarParameter* TurbidityClearTime = AddScalarParameter(Material, TEXT("TurbidityClearTime"), 4.0f, -1100, 380, TEXT("Valve Water RT"));
				UMaterialExpressionScalarParameter* GlintLifetime = AddScalarParameter(Material, TEXT("GlintLifetime"), 0.8f, -1100, 500, TEXT("Valve Water RT"));
				Output->Code = TEXT(
					"float4 p = Texture2DSample(PreviousInteractionTexture, PreviousInteractionTextureSampler, UV);\n"
					"float foamDecay = exp(-DeltaSeconds * max(FoamDecay, 0.001));\n"
					"float mudDecay = exp(-DeltaSeconds / max(TurbidityClearTime, 0.001));\n"
					"float glintDecay = exp(-DeltaSeconds / max(GlintLifetime, 0.001));\n"
					"p.r *= exp(-DeltaSeconds * 1.8);\n"
					"p.g *= foamDecay;\n"
					"p.b *= mudDecay;\n"
					"p.a *= glintDecay;\n"
					"return saturate(p);");
				AddCustomInput(Output, TEXT("UV"), UV);
				AddCustomInput(Output, TEXT("PreviousInteractionTexture"), Previous);
				AddCustomInput(Output, TEXT("DeltaSeconds"), DeltaSeconds);
				AddCustomInput(Output, TEXT("FoamDecay"), FoamDecay);
				AddCustomInput(Output, TEXT("TurbidityClearTime"), TurbidityClearTime);
				AddCustomInput(Output, TEXT("GlintLifetime"), GlintLifetime);
			}
			else
			{
				UMaterialExpressionVectorParameter* Bounds = AddVectorParameter(Material, TEXT("WaterInteractionWorldBounds"), FLinearColor(0, 0, 1000, 1000), -1100, 140, TEXT("Valve Water RT"));
				UMaterialExpressionVectorParameter* LocationRadius = AddVectorParameter(Material, TEXT("ImpulseWorldLocationRadius"), FLinearColor(0, 0, 0, 100), -1100, 260, TEXT("Valve Water RT"));
				UMaterialExpressionVectorParameter* DirectionStrength = AddVectorParameter(Material, TEXT("ImpulseDirectionStrength"), FLinearColor(1, 0, 1, 1), -1100, 380, TEXT("Valve Water RT"));
				UMaterialExpressionVectorParameter* Channels = AddVectorParameter(Material, TEXT("ImpulseChannels"), FLinearColor(1, 0.4, 0.2, 1), -1100, 500, TEXT("Valve Water RT"));
				Output->Code = TEXT(
					"float4 p = Texture2DSample(PreviousInteractionTexture, PreviousInteractionTextureSampler, UV);\n"
					"float2 size = max(WaterInteractionWorldBounds.zw, float2(1.0, 1.0));\n"
					"float2 center = (ImpulseWorldLocationRadius.xy - WaterInteractionWorldBounds.xy) / size;\n"
					"float radius = max(ImpulseWorldLocationRadius.w / max(size.x, size.y), 0.001);\n"
					"float d = length(UV - center) / radius;\n"
					"float ring = exp(-pow((d - 0.55) * 5.5, 2.0));\n"
					"float core = saturate(1.0 - d);\n"
					"float stamp = saturate(core * 0.65 + ring * 0.75);\n"
					"float dir = saturate(dot(normalize((UV - center) + 0.0001), normalize(ImpulseDirectionStrength.xy + 0.0001)) * 0.5 + 0.5);\n"
					"float strength = max(ImpulseDirectionStrength.z, 0.0) * max(ImpulseDirectionStrength.w, 0.1);\n"
					"float4 add = float4(Channels.r * ring, Channels.g * stamp, Channels.b * core, Channels.a * stamp * dir) * strength;\n"
					"return saturate(max(p, add));");
				AddCustomInput(Output, TEXT("UV"), UV);
				AddCustomInput(Output, TEXT("PreviousInteractionTexture"), Previous);
				AddCustomInput(Output, TEXT("WaterInteractionWorldBounds"), Bounds);
				AddCustomInput(Output, TEXT("ImpulseWorldLocationRadius"), LocationRadius);
				AddCustomInput(Output, TEXT("ImpulseDirectionStrength"), DirectionStrength);
				AddCustomInput(Output, TEXT("ImpulseChannels"), Channels);
			}
		}

		UMaterialExpressionComponentMask* RGB = AddMask(Material, Output, true, true, true, false, -240, -160);
		UMaterialExpressionComponentMask* Alpha = AddMask(Material, Output, false, false, false, true, -240, 40);
		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		Data->EmissiveColor.Connect(0, RGB);
		Data->Opacity.Connect(0, Alpha);

		FinalizeMaterial(Material, DirtyPackages);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *MaterialPath));
		return Material;
	}

	UMaterial* CreateCausticsMaterial(TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UMaterial* Existing = LoadAssetByPackagePath<UMaterial>(CausticsMaterialPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *CausticsMaterialPath));
			return Existing;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(CausticsMaterialPath);
		UPackage* Package = CreatePackage(*CausticsMaterialPath);
		UMaterial* Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Material || !Material->GetEditorOnlyData())
		{
			ReportLines.Add(TEXT("- Failed to create caustics material."));
			return nullptr;
		}

		Material->MaterialDomain = MD_DeferredDecal;
		Material->BlendMode = BLEND_Translucent;
		Material->DecalBlendMode = DBM_Translucent;
		Material->SetShadingModel(MSM_Unlit);

		UTexture* CausticsTexture = LoadAssetByPackagePath<UTexture>(CausticsTexturePath);
		UMaterialExpressionTextureCoordinate* UV = AddExpression<UMaterialExpressionTextureCoordinate>(Material, -900, -120);
		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(Material, -900, 40);
		UMaterialExpressionTextureObjectParameter* Texture = AddTextureObjectParameter(Material, TEXT("CausticsTexture"), CausticsTexture, -900, -300);
		UMaterialExpressionScalarParameter* Intensity = AddScalarParameter(Material, TEXT("CausticsIntensity"), 0.35f, -900, 190, TEXT("Valve Water Caustics"));
		UMaterialExpressionScalarParameter* Scale = AddScalarParameter(Material, TEXT("CausticsScale"), 1.0f, -900, 320, TEXT("Valve Water Caustics"));
		UMaterialExpressionVectorParameter* Scroll = AddVectorParameter(Material, TEXT("CausticsScrollSpeed"), FLinearColor(0.04f, 0.025f, 0, 0), -900, 450, TEXT("Valve Water Caustics"));

		UMaterialExpressionCustom* Output = AddExpression<UMaterialExpressionCustom>(Material, -440, -100);
		if (Output)
		{
			Output->Description = TEXT("Cheap moving caustics decal");
			Output->OutputType = CMOT_Float4;
			Output->Code = TEXT(
				"float2 uv1 = UV * max(CausticsScale, 0.001) + Time * CausticsScrollSpeed.xy;\n"
				"float2 uv2 = UV * max(CausticsScale * 0.73, 0.001) - Time * CausticsScrollSpeed.yx * 0.7;\n"
				"float c = Texture2DSample(CausticsTexture, CausticsTextureSampler, uv1).r;\n"
				"c *= Texture2DSample(CausticsTexture, CausticsTextureSampler, uv2).g;\n"
				"c = smoothstep(0.18, 0.82, c) * CausticsIntensity;\n"
				"return float4(c.xxx, c);");
			AddCustomInput(Output, TEXT("UV"), UV);
			AddCustomInput(Output, TEXT("Time"), Time);
			AddCustomInput(Output, TEXT("CausticsTexture"), Texture);
			AddCustomInput(Output, TEXT("CausticsIntensity"), Intensity);
			AddCustomInput(Output, TEXT("CausticsScale"), Scale);
			AddCustomInput(Output, TEXT("CausticsScrollSpeed"), Scroll);
		}

		UMaterialExpressionComponentMask* RGB = AddMask(Material, Output, true, true, true, false, -200, -160);
		UMaterialExpressionComponentMask* Alpha = AddMask(Material, Output, false, false, false, true, -200, 40);
		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		Data->EmissiveColor.Connect(0, RGB);
		Data->Opacity.Connect(0, Alpha);

		FinalizeMaterial(Material, DirtyPackages);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *CausticsMaterialPath));
		return Material;
	}

	UMaterialInstanceConstant* CreateMaterialInstance(const FString& InstancePath, UMaterialInterface* Parent, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UMaterialInstanceConstant* Existing = LoadAssetByPackagePath<UMaterialInstanceConstant>(InstancePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *InstancePath));
			return Existing;
		}
		if (!Parent)
		{
			ReportLines.Add(FString::Printf(TEXT("- Skipped `%s`: parent material is missing."), *InstancePath));
			return nullptr;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(InstancePath);
		UPackage* Package = CreatePackage(*InstancePath);
		UMaterialInstanceConstant* Instance = NewObject<UMaterialInstanceConstant>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Instance)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create `%s`."), *InstancePath));
			return nullptr;
		}

		Instance->SetParentEditorOnly(Parent);
		FAssetRegistryModule::AssetCreated(Instance);
		Instance->PostEditChange();
		Instance->MarkPackageDirty();
		DirtyPackages.AddUnique(Instance->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *InstancePath));
		return Instance;
	}

	void SetScalar(UMaterialInstanceConstant* Instance, const FName ParameterName, float Value)
	{
		if (Instance)
		{
			Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(ParameterName), Value);
			Instance->MarkPackageDirty();
		}
	}

	void SetVector(UMaterialInstanceConstant* Instance, const FName ParameterName, const FLinearColor& Value)
	{
		if (Instance)
		{
			Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(ParameterName), Value);
			Instance->MarkPackageDirty();
		}
	}

	void ConfigureMaterialInstances(
		UMaterialInstanceConstant* CheapCleanMI,
		UMaterialInstanceConstant* CheapMuddyMI,
		UMaterialInstanceConstant* CheapOilMI,
		UMaterialInstanceConstant* InteractiveMI,
		UMaterialInstanceConstant* CausticsMI,
		TArray<UPackage*>& DirtyPackages)
	{
		SetVector(CheapCleanMI, TEXT("BaseWaterColor"), FLinearColor(0.08f, 0.14f, 0.13f, 1.0f));
		SetVector(CheapCleanMI, TEXT("DepthColor"), FLinearColor(0.03f, 0.07f, 0.06f, 1.0f));
		SetScalar(CheapCleanMI, TEXT("Opacity"), 0.52f);
		SetScalar(CheapCleanMI, TEXT("AttachmentLayer1Intensity"), 0.10f);
		SetScalar(CheapCleanMI, TEXT("AttachmentLayer2Intensity"), 0.0f);

		SetVector(CheapMuddyMI, TEXT("BaseWaterColor"), FLinearColor(0.20f, 0.14f, 0.08f, 1.0f));
		SetVector(CheapMuddyMI, TEXT("DepthColor"), FLinearColor(0.09f, 0.06f, 0.035f, 1.0f));
		SetVector(CheapMuddyMI, TEXT("TurbidityColor"), FLinearColor(0.36f, 0.25f, 0.12f, 1.0f));
		SetScalar(CheapMuddyMI, TEXT("Opacity"), 0.78f);
		SetScalar(CheapMuddyMI, TEXT("AttachmentLayer1Intensity"), 0.42f);
		SetScalar(CheapMuddyMI, TEXT("AttachmentLayer2Intensity"), 0.24f);

		SetVector(CheapOilMI, TEXT("BaseWaterColor"), FLinearColor(0.07f, 0.08f, 0.07f, 1.0f));
		SetVector(CheapOilMI, TEXT("DepthColor"), FLinearColor(0.025f, 0.028f, 0.024f, 1.0f));
		SetScalar(CheapOilMI, TEXT("Opacity"), 0.64f);
		SetScalar(CheapOilMI, TEXT("SpecularIntensity"), 1.35f);
		SetScalar(CheapOilMI, TEXT("Roughness"), 0.08f);
		SetScalar(CheapOilMI, TEXT("AttachmentLayer1Intensity"), 0.58f);
		SetScalar(CheapOilMI, TEXT("AttachmentLayer2Intensity"), 0.20f);

		SetVector(InteractiveMI, TEXT("BaseWaterColor"), FLinearColor(0.12f, 0.15f, 0.11f, 1.0f));
		SetVector(InteractiveMI, TEXT("DepthColor"), FLinearColor(0.045f, 0.060f, 0.040f, 1.0f));
		SetVector(InteractiveMI, TEXT("FoamColor"), FLinearColor(0.82f, 0.78f, 0.62f, 1.0f));
		SetVector(InteractiveMI, TEXT("TurbidityColor"), FLinearColor(0.34f, 0.23f, 0.11f, 1.0f));
		SetScalar(InteractiveMI, TEXT("Opacity"), 0.70f);
		SetScalar(InteractiveMI, TEXT("FoamStrength"), 1.15f);
		SetScalar(InteractiveMI, TEXT("TurbidityStrength"), 1.1f);
		SetScalar(InteractiveMI, TEXT("GlintIntensity"), 1.4f);
		SetScalar(InteractiveMI, TEXT("GlintThreshold"), 0.64f);
		SetScalar(InteractiveMI, TEXT("AttachmentLayer1Intensity"), 0.38f);
		SetScalar(InteractiveMI, TEXT("AttachmentLayer2Intensity"), 0.18f);

		SetScalar(CausticsMI, TEXT("CausticsIntensity"), 0.32f);
		SetScalar(CausticsMI, TEXT("CausticsScale"), 1.0f);
		SetVector(CausticsMI, TEXT("CausticsScrollSpeed"), FLinearColor(0.04f, 0.025f, 0.0f, 0.0f));

		for (UMaterialInstanceConstant* Instance : { CheapCleanMI, CheapMuddyMI, CheapOilMI, InteractiveMI, CausticsMI })
		{
			if (Instance)
			{
				Instance->PostEditChange();
				DirtyPackages.AddUnique(Instance->GetPackage());
			}
		}
	}

	UBlueprint* CreateBlueprintAsset(const FString& PackagePath, UClass* ParentClass, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UBlueprint* Existing = LoadAssetByPackagePath<UBlueprint>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = ParentClass;

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(PackagePath),
			FPackageName::GetLongPackagePath(PackagePath),
			UBlueprint::StaticClass(),
			Factory));
		if (Blueprint)
		{
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			FAssetRegistryModule::AssetCreated(Blueprint);
			Blueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(Blueprint->GetPackage());
			ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *PackagePath));
		}
		return Blueprint;
	}

	void SetObjectProperty(UObject* Object, const FName PropertyName, UObject* Value)
	{
		if (!Object)
		{
			return;
		}
		if (FObjectProperty* Property = FindFProperty<FObjectProperty>(Object->GetClass(), PropertyName))
		{
			Property->SetObjectPropertyValue_InContainer(Object, Value);
			Object->MarkPackageDirty();
		}
	}

	void ConfigureBlueprintDefaults(
		UBlueprint* CheapBP,
		UBlueprint* InteractiveBP,
		UBlueprint* CausticsBP,
		UMaterialInterface* CheapMI,
		UMaterialInterface* InteractiveMI,
		UMaterialInterface* CausticsMI,
		UMaterialInterface* StampMaterial,
		UMaterialInterface* DecayMaterial,
		TArray<UPackage*>& DirtyPackages)
	{
		if (CheapBP && CheapBP->GeneratedClass)
		{
			UObject* CDO = CheapBP->GeneratedClass->GetDefaultObject();
			SetObjectProperty(CDO, TEXT("WaterMaterial"), CheapMI);
			FKismetEditorUtilities::CompileBlueprint(CheapBP);
			DirtyPackages.AddUnique(CheapBP->GetPackage());
		}

		if (InteractiveBP && InteractiveBP->GeneratedClass)
		{
			UObject* CDO = InteractiveBP->GeneratedClass->GetDefaultObject();
			SetObjectProperty(CDO, TEXT("WaterMaterial"), InteractiveMI);
			SetObjectProperty(CDO, TEXT("CausticsMaterial"), CausticsMI);
			SetObjectProperty(CDO, TEXT("InteractionStampMaterial"), StampMaterial);
			SetObjectProperty(CDO, TEXT("InteractionDecayMaterial"), DecayMaterial);
			FKismetEditorUtilities::CompileBlueprint(InteractiveBP);
			DirtyPackages.AddUnique(InteractiveBP->GetPackage());
		}

		if (CausticsBP && CausticsBP->GeneratedClass)
		{
			UObject* CDO = CausticsBP->GeneratedClass->GetDefaultObject();
			SetObjectProperty(CDO, TEXT("CausticsMaterial"), CausticsMI);
			FKismetEditorUtilities::CompileBlueprint(CausticsBP);
			DirtyPackages.AddUnique(CausticsBP->GetPackage());
		}
	}
}

UWaterStyleAssetSetupCommandlet::UWaterStyleAssetSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWaterStyleAssetSetupCommandlet::Main(const FString& Params)
{
	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Valve-style Water Asset Setup"));

	UMaterial* SurfaceMaterial = CreateSurfaceMaterial(ReportLines, DirtyPackages);
	UMaterial* CausticsMaterial = CreateCausticsMaterial(ReportLines, DirtyPackages);
	UMaterial* StampMaterial = CreateRenderTargetMaterial(StampMaterialPath, false, ReportLines, DirtyPackages);
	UMaterial* DecayMaterial = CreateRenderTargetMaterial(DecayMaterialPath, true, ReportLines, DirtyPackages);

	UMaterialInstanceConstant* CheapCleanMI = CreateMaterialInstance(CheapCleanMIPath, SurfaceMaterial, ReportLines, DirtyPackages);
	UMaterialInstanceConstant* CheapMuddyMI = CreateMaterialInstance(CheapMuddyMIPath, SurfaceMaterial, ReportLines, DirtyPackages);
	UMaterialInstanceConstant* CheapOilMI = CreateMaterialInstance(CheapOilMIPath, SurfaceMaterial, ReportLines, DirtyPackages);
	UMaterialInstanceConstant* InteractiveMI = CreateMaterialInstance(InteractiveMIPath, SurfaceMaterial, ReportLines, DirtyPackages);
	UMaterialInstanceConstant* CausticsMI = CreateMaterialInstance(CausticsMIPath, CausticsMaterial, ReportLines, DirtyPackages);

	ConfigureMaterialInstances(CheapCleanMI, CheapMuddyMI, CheapOilMI, InteractiveMI, CausticsMI, DirtyPackages);

	UBlueprint* CheapBP = CreateBlueprintAsset(CheapBlueprintPath, ACheapWaterSurfaceActor::StaticClass(), ReportLines, DirtyPackages);
	UBlueprint* InteractiveBP = CreateBlueprintAsset(InteractiveBlueprintPath, AInteractiveWaterVolume::StaticClass(), ReportLines, DirtyPackages);
	UBlueprint* CausticsBP = CreateBlueprintAsset(CausticsBlueprintPath, AWaterCausticsDecalActor::StaticClass(), ReportLines, DirtyPackages);

	ConfigureBlueprintDefaults(
		CheapBP,
		InteractiveBP,
		CausticsBP,
		CheapCleanMI,
		InteractiveMI,
		CausticsMI,
		StampMaterial,
		DecayMaterial,
		DirtyPackages);

	if (DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	for (const FString& Line : ReportLines)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
	}

	return 0;
}
