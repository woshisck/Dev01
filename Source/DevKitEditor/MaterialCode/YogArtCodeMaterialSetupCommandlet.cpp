#include "YogArtCodeMaterialSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "CoreGlobals.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCollection.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionQualitySwitch.h"
#include "Materials/MaterialExpressionRuntimeVirtualTextureOutput.h"
#include "Materials/MaterialExpressionRuntimeVirtualTextureSampleParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionStaticBoolParameter.h"
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "Materials/MaterialExpressionTextureCollectionParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "MaterialShared.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ShaderCompiler.h"
#include "UnrealEngine.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "VT/RuntimeVirtualTexture.h"

DEFINE_LOG_CATEGORY_STATIC(LogYogArtCodeMaterialSetup, Log, All);

namespace
{
constexpr const TCHAR* SchemaName = TEXT("DevKit.YogArt.CodeMaterial.v1");
constexpr const TCHAR* BuildingKind = TEXT("Building");
constexpr const TCHAR* PropKind = TEXT("Prop");
constexpr const TCHAR* BuildingAssetPath = TEXT("/YogArt_Material/MasterMaterial/Env/M_Yog_Building_Source");
constexpr const TCHAR* PropAssetPath = TEXT("/YogArt_Material/MasterMaterial/Env/M_Yog_Prop_Source");
constexpr const TCHAR* BuildingJsonRelativePath = TEXT("YogArt_Material/Config/MaterialCode/YogBuildingMaterial.json");
constexpr const TCHAR* PropJsonRelativePath = TEXT("YogArt_Material/Config/MaterialCode/YogPropMaterial.json");
constexpr const TCHAR* CommonShaderRelativePath = TEXT("YogArt/Env/YogEnvMaterialCommon.ush");
constexpr const TCHAR* GroundMaterialAssetPath = TEXT("/YogArt_Material/MasterMaterial/Env/M_Yog_Ground_RVT_Source");
constexpr const TCHAR* GroundMaterialObjectPath = TEXT("/YogArt_Material/MasterMaterial/Env/M_Yog_Ground_RVT_Source.M_Yog_Ground_RVT_Source");
constexpr const TCHAR* GroundHeightRvtAssetPath = TEXT("/YogArt_Material/Data/RVT_Default_WorldHeight");
constexpr const TCHAR* GroundHeightRvtObjectPath = TEXT("/YogArt_Material/Data/RVT_Default_WorldHeight.RVT_Default_WorldHeight");
constexpr uint64 CodeMaterialGeneratorVersion = 4;

enum class ECodeMaterialTarget : uint8
{
	Building,
	Prop,
	All
};

enum class ECodeInputType : uint8
{
	Texture2D,
	Scalar,
	Vector
};

enum class ECollectionRole : uint8
{
	BaseColor,
	MRAH,
	NormalLight
};

struct FCodeInputSpec
{
	FString Name;
	ECodeInputType Type = ECodeInputType::Scalar;
	bool bCustomInput = false;
	FString DefaultAsset;
	FString Sampler;
	EMaterialSamplerType SamplerType = SAMPLERTYPE_Color;
	FString Group;
	int32 SortPriority = 0;
	double ScalarDefault = 0.0;
	FLinearColor VectorDefault = FLinearColor::Black;
	bool bHasMin = false;
	bool bHasMax = false;
	double Min = 0.0;
	double Max = 0.0;
	bool bHasPerInstanceDataIndex = false;
	int32 PerInstanceDataIndex = INDEX_NONE;
};

struct FQualityScalarSpec
{
	FString Name;
	double DefaultValue = 0.0;
	double Values[EMaterialQualityLevel::Num] = {};
};

struct FGroundRvtSpec
{
	FString StaticSwitch;
	bool bStaticDefault = false;
	FString SurfaceParameter;
	FString SurfaceDefaultAsset;
	FString HeightParameter;
	FString HeightDefaultAsset;
	bool QualityEnabled[EMaterialQualityLevel::Num] = {};
};

struct FCollectionBindingSpec
{
	FString TextureParameter;
	FString IndexParameter;
};

struct FCollectionSpec
{
	FString Parameter;
	ECollectionRole Role = ECollectionRole::BaseColor;
	FString DefaultAsset;
	FString Sampler;
	TArray<FCollectionBindingSpec> Bindings;
};

struct FOutputSpec
{
	FString Name;
	ECustomMaterialOutputType Type = CMOT_Float1;
	FString MaterialProperty;
};

struct FMaterialCodeSpec
{
	FString MaterialKind;
	FString AssetPath;
	FString ShaderInclude;
	FString EntryPoint;
	TArray<FCodeInputSpec> Inputs;
	FOutputSpec MainOutput;
	TArray<FOutputSpec> AdditionalOutputs;
	FString TextureCollectionSwitch;
	bool bTextureCollectionDefault = true;
	TArray<FCollectionSpec> Collections;
	TArray<FQualityScalarSpec> QualityInputs;
	FGroundRvtSpec GroundRvt;
	FString GenerationFingerprint;
};

void AddError(TArray<FString>& Errors, const FString& Location, const FString& Message)
{
	Errors.Add(FString::Printf(TEXT("%s: %s"), *Location, *Message));
}

bool ValidateAllowedFields(
	const TSharedPtr<FJsonObject>& Object,
	std::initializer_list<const TCHAR*> AllowedFields,
	const FString& Location,
	TArray<FString>& Errors)
{
	if (!Object.IsValid())
	{
		AddError(Errors, Location, TEXT("expected an object."));
		return false;
	}

	TSet<FString> Allowed;
	for (const TCHAR* Field : AllowedFields)
	{
		Allowed.Add(Field);
	}

	bool bValid = true;
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
	{
		if (!Allowed.Contains(Pair.Key))
		{
			AddError(Errors, Location, FString::Printf(TEXT("unknown field `%s`."), *Pair.Key));
			bValid = false;
		}
	}
	return bValid;
}

const TSharedPtr<FJsonValue>* FindRequiredValue(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	TArray<FString>& Errors)
{
	if (!Object.IsValid())
	{
		AddError(Errors, Location, TEXT("expected an object."));
		return nullptr;
	}

	const TSharedPtr<FJsonValue>* Value = Object->Values.Find(Field);
	if (!Value || !Value->IsValid())
	{
		AddError(Errors, Location, FString::Printf(TEXT("missing required field `%s`."), Field));
		return nullptr;
	}
	return Value;
}

bool GetRequiredString(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	FString& OutValue,
	TArray<FString>& Errors)
{
	const TSharedPtr<FJsonValue>* Value = FindRequiredValue(Object, Field, Location, Errors);
	if (!Value)
	{
		return false;
	}
	if ((*Value)->Type != EJson::String)
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` must be a string."), Field));
		return false;
	}

	OutValue = (*Value)->AsString();
	if (OutValue.IsEmpty())
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` cannot be empty."), Field));
		return false;
	}
	return true;
}

bool GetRequiredBool(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	bool& OutValue,
	TArray<FString>& Errors)
{
	const TSharedPtr<FJsonValue>* Value = FindRequiredValue(Object, Field, Location, Errors);
	if (!Value)
	{
		return false;
	}
	if ((*Value)->Type != EJson::Boolean)
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` must be a boolean."), Field));
		return false;
	}

	OutValue = (*Value)->AsBool();
	return true;
}

bool GetRequiredNumber(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	double& OutValue,
	TArray<FString>& Errors)
{
	const TSharedPtr<FJsonValue>* Value = FindRequiredValue(Object, Field, Location, Errors);
	if (!Value)
	{
		return false;
	}
	if ((*Value)->Type != EJson::Number)
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` must be a number."), Field));
		return false;
	}

	OutValue = (*Value)->AsNumber();
	if (!FMath::IsFinite(OutValue))
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` must be finite."), Field));
		return false;
	}
	return true;
}

bool GetRequiredInt32(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	int32& OutValue,
	TArray<FString>& Errors)
{
	double Number = 0.0;
	if (!GetRequiredNumber(Object, Field, Location, Number, Errors))
	{
		return false;
	}
	if (Number != FMath::FloorToDouble(Number) || Number < static_cast<double>(MIN_int32) || Number > static_cast<double>(MAX_int32))
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` must be a 32-bit integer."), Field));
		return false;
	}

	OutValue = static_cast<int32>(Number);
	return true;
}

bool GetRequiredObject(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	TSharedPtr<FJsonObject>& OutValue,
	TArray<FString>& Errors)
{
	const TSharedPtr<FJsonValue>* Value = FindRequiredValue(Object, Field, Location, Errors);
	if (!Value)
	{
		return false;
	}
	if ((*Value)->Type != EJson::Object)
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` must be an object."), Field));
		return false;
	}

	OutValue = (*Value)->AsObject();
	return OutValue.IsValid();
}

bool GetRequiredArray(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	const TArray<TSharedPtr<FJsonValue>>*& OutValue,
	TArray<FString>& Errors)
{
	const TSharedPtr<FJsonValue>* Value = FindRequiredValue(Object, Field, Location, Errors);
	if (!Value)
	{
		return false;
	}
	if ((*Value)->Type != EJson::Array)
	{
		AddError(Errors, Location, FString::Printf(TEXT("field `%s` must be an array."), Field));
		return false;
	}

	OutValue = &(*Value)->AsArray();
	return true;
}

bool GetOptionalNumber(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	bool& bOutPresent,
	double& OutValue,
	TArray<FString>& Errors)
{
	bOutPresent = Object.IsValid() && Object->Values.Contains(Field);
	if (!bOutPresent)
	{
		return true;
	}
	return GetRequiredNumber(Object, Field, Location, OutValue, Errors);
}

bool GetOptionalInt32(
	const TSharedPtr<FJsonObject>& Object,
	const TCHAR* Field,
	const FString& Location,
	bool& bOutPresent,
	int32& OutValue,
	TArray<FString>& Errors)
{
	bOutPresent = Object.IsValid() && Object->Values.Contains(Field);
	if (!bOutPresent)
	{
		return true;
	}
	return GetRequiredInt32(Object, Field, Location, OutValue, Errors);
}

bool IsValidHlslIdentifier(const FString& Identifier)
{
	if (Identifier.IsEmpty())
	{
		return false;
	}

	const TCHAR First = Identifier[0];
	if (!(First == TEXT('_') || FChar::IsAlpha(First)))
	{
		return false;
	}

	for (int32 Index = 1; Index < Identifier.Len(); ++Index)
	{
		const TCHAR Character = Identifier[Index];
		if (!(Character == TEXT('_') || FChar::IsAlnum(Character)))
		{
			return false;
		}
	}
	return true;
}

bool ValidateIdentifier(const FString& Identifier, const FString& Location, TArray<FString>& Errors)
{
	if (!IsValidHlslIdentifier(Identifier))
	{
		AddError(Errors, Location, FString::Printf(TEXT("`%s` is not a valid HLSL identifier."), *Identifier));
		return false;
	}
	return true;
}

bool TryAddUnique(TSet<FString>& Values, const FString& Value)
{
	if (Values.Contains(Value))
	{
		return false;
	}
	Values.Add(Value);
	return true;
}

bool ParseInputType(const FString& Value, ECodeInputType& OutType)
{
	if (Value == TEXT("Texture2D"))
	{
		OutType = ECodeInputType::Texture2D;
		return true;
	}
	if (Value == TEXT("Scalar"))
	{
		OutType = ECodeInputType::Scalar;
		return true;
	}
	if (Value == TEXT("Vector"))
	{
		OutType = ECodeInputType::Vector;
		return true;
	}
	return false;
}

bool ParseSamplerType(const FString& Value, EMaterialSamplerType& OutType)
{
	if (Value == TEXT("Color"))
	{
		OutType = SAMPLERTYPE_Color;
		return true;
	}
	if (Value == TEXT("LinearColor"))
	{
		OutType = SAMPLERTYPE_LinearColor;
		return true;
	}
	if (Value == TEXT("Masks"))
	{
		OutType = SAMPLERTYPE_Masks;
		return true;
	}
	if (Value == TEXT("Normal"))
	{
		OutType = SAMPLERTYPE_Normal;
		return true;
	}
	return false;
}

bool ParseCollectionRole(const FString& Value, ECollectionRole& OutRole)
{
	if (Value == TEXT("BaseColor"))
	{
		OutRole = ECollectionRole::BaseColor;
		return true;
	}
	if (Value == TEXT("MRAH"))
	{
		OutRole = ECollectionRole::MRAH;
		return true;
	}
	if (Value == TEXT("NormalLight"))
	{
		OutRole = ECollectionRole::NormalLight;
		return true;
	}
	return false;
}

bool ParseOutputType(const FString& Value, ECustomMaterialOutputType& OutType)
{
	if (Value == TEXT("float1"))
	{
		OutType = CMOT_Float1;
		return true;
	}
	if (Value == TEXT("float2"))
	{
		OutType = CMOT_Float2;
		return true;
	}
	if (Value == TEXT("float3"))
	{
		OutType = CMOT_Float3;
		return true;
	}
	if (Value == TEXT("float4"))
	{
		OutType = CMOT_Float4;
		return true;
	}
	return false;
}

bool IsKnownMaterialProperty(const FString& Property)
{
	return Property == TEXT("BaseColor")
		|| Property == TEXT("Normal")
		|| Property == TEXT("AmbientOcclusion")
		|| Property == TEXT("Roughness")
		|| Property == TEXT("Metallic")
		|| Property == TEXT("Specular")
		|| Property == TEXT("EmissiveColor")
		|| Property == TEXT("PixelDepthOffset")
		|| Property == TEXT("None");
}

bool IsOutputTypeCompatible(const FOutputSpec& Output)
{
	if (Output.MaterialProperty == TEXT("None"))
	{
		return true;
	}
	if (Output.MaterialProperty == TEXT("BaseColor")
		|| Output.MaterialProperty == TEXT("Normal")
		|| Output.MaterialProperty == TEXT("EmissiveColor"))
	{
		return Output.Type == CMOT_Float3;
	}
	return Output.Type == CMOT_Float1;
}

bool ValidateDefaultObjectPath(const FString& ObjectPath, const FString& Location, TArray<FString>& Errors)
{
	if (!FPackageName::IsValidObjectPath(ObjectPath))
	{
		AddError(Errors, Location, FString::Printf(TEXT("`%s` is not a valid Unreal object path."), *ObjectPath));
		return false;
	}
	return true;
}

bool ParsePacking(
	const TSharedPtr<FJsonObject>& Root,
	const FString& Location,
	TArray<FString>& Errors)
{
	TSharedPtr<FJsonObject> Packing;
	if (!GetRequiredObject(Root, TEXT("packing"), Location, Packing, Errors))
	{
		return false;
	}

	bool bValid = ValidateAllowedFields(Packing, {TEXT("baseColor"), TEXT("normal"), TEXT("mrah")}, Location + TEXT(".packing"), Errors);
	FString BaseColor;
	FString Normal;
	bValid &= GetRequiredString(Packing, TEXT("baseColor"), Location + TEXT(".packing"), BaseColor, Errors);
	bValid &= GetRequiredString(Packing, TEXT("normal"), Location + TEXT(".packing"), Normal, Errors);
	if (!BaseColor.IsEmpty() && BaseColor != TEXT("RGB"))
	{
		AddError(Errors, Location + TEXT(".packing.baseColor"), TEXT("must be exactly `RGB`."));
		bValid = false;
	}
	if (!Normal.IsEmpty() && Normal != TEXT("TangentSpaceRG_LightMaskB"))
	{
		AddError(
			Errors,
			Location + TEXT(".packing.normal"),
			TEXT("must be exactly `TangentSpaceRG_LightMaskB`."));
		bValid = false;
	}

	TSharedPtr<FJsonObject> Mrah;
	if (!GetRequiredObject(Packing, TEXT("mrah"), Location + TEXT(".packing"), Mrah, Errors))
	{
		return false;
	}
	bValid &= ValidateAllowedFields(Mrah, {TEXT("r"), TEXT("g"), TEXT("b"), TEXT("a")}, Location + TEXT(".packing.mrah"), Errors);

	const TPair<const TCHAR*, const TCHAR*> ExpectedChannels[] =
	{
		{TEXT("r"), TEXT("Metallic")},
		{TEXT("g"), TEXT("Roughness")},
		{TEXT("b"), TEXT("AmbientOcclusion")},
		{TEXT("a"), TEXT("Height")}
	};
	for (const TPair<const TCHAR*, const TCHAR*>& Expected : ExpectedChannels)
	{
		FString Channel;
		if (!GetRequiredString(Mrah, Expected.Key, Location + TEXT(".packing.mrah"), Channel, Errors))
		{
			bValid = false;
		}
		else if (Channel != Expected.Value)
		{
			AddError(
				Errors,
				Location + TEXT(".packing.mrah.") + Expected.Key,
				FString::Printf(TEXT("must be exactly `%s`."), Expected.Value));
			bValid = false;
		}
	}
	return bValid;
}

bool ParseInput(
	const TSharedPtr<FJsonObject>& Object,
	const FString& Location,
	FCodeInputSpec& OutSpec,
	TArray<FString>& Errors)
{
	bool bValid = ValidateAllowedFields(
		Object,
		{
			TEXT("name"),
			TEXT("type"),
			TEXT("customInput"),
			TEXT("default"),
			TEXT("defaultAsset"),
			TEXT("sampler"),
			TEXT("samplerType"),
			TEXT("group"),
			TEXT("sortPriority"),
			TEXT("min"),
			TEXT("max"),
			TEXT("perInstanceDataIndex")
		},
		Location,
		Errors);

	FString TypeString;
	bValid &= GetRequiredString(Object, TEXT("name"), Location, OutSpec.Name, Errors);
	bValid &= GetRequiredString(Object, TEXT("type"), Location, TypeString, Errors);
	bValid &= GetRequiredBool(Object, TEXT("customInput"), Location, OutSpec.bCustomInput, Errors);
	bValid &= GetRequiredString(Object, TEXT("group"), Location, OutSpec.Group, Errors);
	bValid &= GetRequiredInt32(Object, TEXT("sortPriority"), Location, OutSpec.SortPriority, Errors);
	bValid &= ValidateIdentifier(OutSpec.Name, Location + TEXT(".name"), Errors);

	if (!ParseInputType(TypeString, OutSpec.Type))
	{
		AddError(Errors, Location + TEXT(".type"), TEXT("must be exactly `Texture2D`, `Scalar`, or `Vector`."));
		return false;
	}

	const bool bHasDefault = Object->Values.Contains(TEXT("default"));
	const bool bHasDefaultAsset = Object->Values.Contains(TEXT("defaultAsset"));
	const bool bHasSampler = Object->Values.Contains(TEXT("sampler"));
	const bool bHasSamplerType = Object->Values.Contains(TEXT("samplerType"));
	const bool bHasMin = Object->Values.Contains(TEXT("min"));
	const bool bHasMax = Object->Values.Contains(TEXT("max"));
	bValid &= GetOptionalInt32(
		Object,
		TEXT("perInstanceDataIndex"),
		Location,
		OutSpec.bHasPerInstanceDataIndex,
		OutSpec.PerInstanceDataIndex,
		Errors);

	if (OutSpec.Type == ECodeInputType::Texture2D)
	{
		if (bHasDefault || bHasMin || bHasMax)
		{
			AddError(Errors, Location, TEXT("Texture2D inputs cannot declare `default`, `min`, or `max`."));
			bValid = false;
		}
		bValid &= GetRequiredString(Object, TEXT("defaultAsset"), Location, OutSpec.DefaultAsset, Errors);
		bValid &= GetRequiredString(Object, TEXT("sampler"), Location, OutSpec.Sampler, Errors);
		FString SamplerTypeString;
		bValid &= GetRequiredString(Object, TEXT("samplerType"), Location, SamplerTypeString, Errors);
		bValid &= ValidateDefaultObjectPath(OutSpec.DefaultAsset, Location + TEXT(".defaultAsset"), Errors);
		bValid &= ValidateIdentifier(OutSpec.Sampler, Location + TEXT(".sampler"), Errors);
		if (!SamplerTypeString.IsEmpty() && !ParseSamplerType(SamplerTypeString, OutSpec.SamplerType))
		{
			AddError(
				Errors,
				Location + TEXT(".samplerType"),
				TEXT("must be exactly `Color`, `LinearColor`, `Masks`, or `Normal`."));
			bValid = false;
		}
		if (!OutSpec.Name.IsEmpty() && !OutSpec.Sampler.IsEmpty() && OutSpec.Sampler != OutSpec.Name + TEXT("Sampler"))
		{
			AddError(
				Errors,
				Location + TEXT(".sampler"),
				FString::Printf(TEXT("must be `%sSampler` because Custom auto-generates that sampler argument."), *OutSpec.Name));
			bValid = false;
		}
		UTexture2D* DefaultTexture = nullptr;
		if (!OutSpec.DefaultAsset.IsEmpty())
		{
			DefaultTexture = LoadObject<UTexture2D>(nullptr, *OutSpec.DefaultAsset);
		}
		if (!OutSpec.DefaultAsset.IsEmpty() && !DefaultTexture)
		{
			AddError(
				Errors,
				Location + TEXT(".defaultAsset"),
				FString::Printf(TEXT("could not load Texture2D `%s`."), *OutSpec.DefaultAsset));
			bValid = false;
		}
	}
	else if (OutSpec.Type == ECodeInputType::Scalar)
	{
		if (bHasDefaultAsset || bHasSampler || bHasSamplerType)
		{
			AddError(Errors, Location, TEXT("Scalar inputs cannot declare `defaultAsset`, `sampler`, or `samplerType`."));
			bValid = false;
		}
		bValid &= GetRequiredNumber(Object, TEXT("default"), Location, OutSpec.ScalarDefault, Errors);
		bValid &= GetOptionalNumber(Object, TEXT("min"), Location, OutSpec.bHasMin, OutSpec.Min, Errors);
		bValid &= GetOptionalNumber(Object, TEXT("max"), Location, OutSpec.bHasMax, OutSpec.Max, Errors);
		if (OutSpec.bHasMin != OutSpec.bHasMax)
		{
			AddError(Errors, Location, TEXT("Scalar slider bounds must declare both `min` and `max`, or neither."));
			bValid = false;
		}
		if (OutSpec.bHasMin && OutSpec.Min >= OutSpec.Max)
		{
			AddError(Errors, Location, TEXT("Scalar `min` must be lower than `max`."));
			bValid = false;
		}
		if (OutSpec.bHasMin && (OutSpec.ScalarDefault < OutSpec.Min || OutSpec.ScalarDefault > OutSpec.Max))
		{
			AddError(Errors, Location, TEXT("Scalar `default` must be within [`min`, `max`]."));
			bValid = false;
		}
	}
	else
	{
		if (bHasDefaultAsset || bHasSampler || bHasSamplerType || bHasMin || bHasMax)
		{
			AddError(Errors, Location, TEXT("Vector inputs cannot declare `defaultAsset`, `sampler`, `samplerType`, `min`, or `max`."));
			bValid = false;
		}

		const TArray<TSharedPtr<FJsonValue>>* DefaultArray = nullptr;
		if (!GetRequiredArray(Object, TEXT("default"), Location, DefaultArray, Errors))
		{
			bValid = false;
		}
		else if (DefaultArray->Num() != 4)
		{
			AddError(Errors, Location + TEXT(".default"), TEXT("Vector defaults must contain exactly four numbers."));
			bValid = false;
		}
		else
		{
			double Components[4] = {};
			for (int32 ComponentIndex = 0; ComponentIndex < 4; ++ComponentIndex)
			{
				const TSharedPtr<FJsonValue>& Component = (*DefaultArray)[ComponentIndex];
				if (!Component.IsValid() || Component->Type != EJson::Number || !FMath::IsFinite(Component->AsNumber()))
				{
					AddError(
						Errors,
						Location + TEXT(".default"),
						FString::Printf(TEXT("component %d must be a finite number."), ComponentIndex));
					bValid = false;
				}
				else
				{
					Components[ComponentIndex] = Component->AsNumber();
				}
			}
			OutSpec.VectorDefault = FLinearColor(
				static_cast<float>(Components[0]),
				static_cast<float>(Components[1]),
				static_cast<float>(Components[2]),
				static_cast<float>(Components[3]));
		}
	}

	if (OutSpec.bHasPerInstanceDataIndex)
	{
		if (OutSpec.Type != ECodeInputType::Scalar)
		{
			AddError(Errors, Location + TEXT(".perInstanceDataIndex"), TEXT("is supported only for Scalar inputs."));
			bValid = false;
		}
		if (!OutSpec.bCustomInput)
		{
			AddError(Errors, Location + TEXT(".perInstanceDataIndex"), TEXT("requires customInput=true."));
			bValid = false;
		}
		if (OutSpec.PerInstanceDataIndex < 0 || OutSpec.PerInstanceDataIndex > 31)
		{
			AddError(Errors, Location + TEXT(".perInstanceDataIndex"), TEXT("must be in the range [0, 31]."));
			bValid = false;
		}
	}

	return bValid;
}

bool ParseOutput(
	const TSharedPtr<FJsonObject>& Object,
	const FString& Location,
	FOutputSpec& OutSpec,
	TArray<FString>& Errors)
{
	bool bValid = ValidateAllowedFields(
		Object,
		{TEXT("name"), TEXT("type"), TEXT("materialProperty")},
		Location,
		Errors);

	FString TypeString;
	bValid &= GetRequiredString(Object, TEXT("name"), Location, OutSpec.Name, Errors);
	bValid &= GetRequiredString(Object, TEXT("type"), Location, TypeString, Errors);
	bValid &= GetRequiredString(Object, TEXT("materialProperty"), Location, OutSpec.MaterialProperty, Errors);
	bValid &= ValidateIdentifier(OutSpec.Name, Location + TEXT(".name"), Errors);

	if (!ParseOutputType(TypeString, OutSpec.Type))
	{
		AddError(Errors, Location + TEXT(".type"), TEXT("must be exactly `float1`, `float2`, `float3`, or `float4`."));
		bValid = false;
	}
	if (!IsKnownMaterialProperty(OutSpec.MaterialProperty))
	{
		AddError(Errors, Location + TEXT(".materialProperty"), TEXT("uses an unsupported material property."));
		bValid = false;
	}
	if (!IsOutputTypeCompatible(OutSpec))
	{
		AddError(Errors, Location, TEXT("output type does not match its material property."));
		bValid = false;
	}
	return bValid;
}

bool ParseTextureBackend(
	const TSharedPtr<FJsonObject>& Root,
	const FString& Location,
	FMaterialCodeSpec& OutSpec,
	TArray<FString>& Errors)
{
	TSharedPtr<FJsonObject> Backend;
	if (!GetRequiredObject(Root, TEXT("textureBackend"), Location, Backend, Errors))
	{
		return false;
	}

	bool bValid = ValidateAllowedFields(
		Backend,
		{TEXT("staticSwitch"), TEXT("default"), TEXT("collections")},
		Location + TEXT(".textureBackend"),
		Errors);
	bValid &= GetRequiredString(
		Backend,
		TEXT("staticSwitch"),
		Location + TEXT(".textureBackend"),
		OutSpec.TextureCollectionSwitch,
		Errors);
	bValid &= GetRequiredBool(
		Backend,
		TEXT("default"),
		Location + TEXT(".textureBackend"),
		OutSpec.bTextureCollectionDefault,
		Errors);
	bValid &= ValidateIdentifier(
		OutSpec.TextureCollectionSwitch,
		Location + TEXT(".textureBackend.staticSwitch"),
		Errors);
	if (!OutSpec.TextureCollectionSwitch.IsEmpty() && OutSpec.TextureCollectionSwitch != TEXT("UseTextureCollection"))
	{
		AddError(
			Errors,
			Location + TEXT(".textureBackend.staticSwitch"),
			TEXT("must be exactly `UseTextureCollection`."));
		bValid = false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Collections = nullptr;
	if (!GetRequiredArray(
		Backend,
		TEXT("collections"),
		Location + TEXT(".textureBackend"),
		Collections,
		Errors))
	{
		return false;
	}
	if (Collections->IsEmpty())
	{
		AddError(Errors, Location + TEXT(".textureBackend.collections"), TEXT("must contain at least one collection."));
		return false;
	}

	TSet<FString> CollectionParameters;
	TSet<uint8> CollectionRoles;
	TSet<FString> BoundTextureParameters;
	for (int32 CollectionIndex = 0; CollectionIndex < Collections->Num(); ++CollectionIndex)
	{
		const FString CollectionLocation = FString::Printf(
			TEXT("%s.textureBackend.collections[%d]"),
			*Location,
			CollectionIndex);
		const TSharedPtr<FJsonValue>& CollectionValue = (*Collections)[CollectionIndex];
		if (!CollectionValue.IsValid() || CollectionValue->Type != EJson::Object)
		{
			AddError(Errors, CollectionLocation, TEXT("must be an object."));
			bValid = false;
			continue;
		}

		const TSharedPtr<FJsonObject> CollectionObject = CollectionValue->AsObject();
		bValid &= ValidateAllowedFields(
			CollectionObject,
			{TEXT("parameter"), TEXT("role"), TEXT("defaultAsset"), TEXT("sampler"), TEXT("bindings")},
			CollectionLocation,
			Errors);

		FCollectionSpec Collection;
		FString RoleString;
		bValid &= GetRequiredString(CollectionObject, TEXT("parameter"), CollectionLocation, Collection.Parameter, Errors);
		bValid &= GetRequiredString(CollectionObject, TEXT("role"), CollectionLocation, RoleString, Errors);
		bValid &= GetRequiredString(CollectionObject, TEXT("defaultAsset"), CollectionLocation, Collection.DefaultAsset, Errors);
		bValid &= GetRequiredString(CollectionObject, TEXT("sampler"), CollectionLocation, Collection.Sampler, Errors);
		bValid &= ValidateIdentifier(Collection.Parameter, CollectionLocation + TEXT(".parameter"), Errors);
		bValid &= ValidateIdentifier(Collection.Sampler, CollectionLocation + TEXT(".sampler"), Errors);
		bValid &= ValidateDefaultObjectPath(Collection.DefaultAsset, CollectionLocation + TEXT(".defaultAsset"), Errors);
		if (!RoleString.IsEmpty() && !ParseCollectionRole(RoleString, Collection.Role))
		{
			AddError(
				Errors,
				CollectionLocation + TEXT(".role"),
				TEXT("must be exactly `BaseColor`, `MRAH`, or `NormalLight`."));
			bValid = false;
		}
		else if (!RoleString.IsEmpty()
			&& CollectionRoles.Contains(static_cast<uint8>(Collection.Role)))
		{
			AddError(Errors, CollectionLocation + TEXT(".role"), TEXT("collection roles must be unique."));
			bValid = false;
		}
		else if (!RoleString.IsEmpty())
		{
			CollectionRoles.Add(static_cast<uint8>(Collection.Role));
		}

		if (!Collection.Parameter.IsEmpty() && !TryAddUnique(CollectionParameters, Collection.Parameter))
		{
			AddError(Errors, CollectionLocation + TEXT(".parameter"), TEXT("collection parameter names must be unique."));
			bValid = false;
		}
		if (!Collection.Parameter.IsEmpty()
			&& !Collection.Sampler.IsEmpty()
			&& Collection.Sampler != Collection.Parameter + TEXT("Sampler"))
		{
			AddError(
				Errors,
				CollectionLocation + TEXT(".sampler"),
				FString::Printf(TEXT("must be `%sSampler`."), *Collection.Parameter));
			bValid = false;
		}
		if (!Collection.DefaultAsset.IsEmpty()
			&& !LoadObject<UTextureCollection>(nullptr, *Collection.DefaultAsset))
		{
			AddError(
				Errors,
				CollectionLocation + TEXT(".defaultAsset"),
				FString::Printf(TEXT("could not load TextureCollection `%s`."), *Collection.DefaultAsset));
			bValid = false;
		}

		const TArray<TSharedPtr<FJsonValue>>* Bindings = nullptr;
		if (!GetRequiredArray(CollectionObject, TEXT("bindings"), CollectionLocation, Bindings, Errors))
		{
			bValid = false;
			continue;
		}
		if (Bindings->IsEmpty())
		{
			AddError(Errors, CollectionLocation + TEXT(".bindings"), TEXT("must contain at least one binding."));
			bValid = false;
		}

		for (int32 BindingIndex = 0; BindingIndex < Bindings->Num(); ++BindingIndex)
		{
			const FString BindingLocation = FString::Printf(
				TEXT("%s.bindings[%d]"),
				*CollectionLocation,
				BindingIndex);
			const TSharedPtr<FJsonValue>& BindingValue = (*Bindings)[BindingIndex];
			if (!BindingValue.IsValid() || BindingValue->Type != EJson::Object)
			{
				AddError(Errors, BindingLocation, TEXT("must be an object."));
				bValid = false;
				continue;
			}

			const TSharedPtr<FJsonObject> BindingObject = BindingValue->AsObject();
			bValid &= ValidateAllowedFields(
				BindingObject,
				{TEXT("textureParameter"), TEXT("indexParameter")},
				BindingLocation,
				Errors);

			FCollectionBindingSpec Binding;
			bValid &= GetRequiredString(
				BindingObject,
				TEXT("textureParameter"),
				BindingLocation,
				Binding.TextureParameter,
				Errors);
			bValid &= GetRequiredString(
				BindingObject,
				TEXT("indexParameter"),
				BindingLocation,
				Binding.IndexParameter,
				Errors);
			bValid &= ValidateIdentifier(
				Binding.TextureParameter,
				BindingLocation + TEXT(".textureParameter"),
				Errors);
			bValid &= ValidateIdentifier(
				Binding.IndexParameter,
				BindingLocation + TEXT(".indexParameter"),
				Errors);

			if (!Binding.TextureParameter.IsEmpty() && !TryAddUnique(BoundTextureParameters, Binding.TextureParameter))
			{
				AddError(
					Errors,
					BindingLocation + TEXT(".textureParameter"),
					TEXT("a Texture2D input can be bound to only one TextureCollection."));
				bValid = false;
			}
			Collection.Bindings.Add(MoveTemp(Binding));
		}

		OutSpec.Collections.Add(MoveTemp(Collection));
	}

	return bValid;
}

bool ParseQualityInputs(
	const TSharedPtr<FJsonObject>& Root,
	const FString& Location,
	FMaterialCodeSpec& OutSpec,
	TArray<FString>& Errors)
{
	const TArray<TSharedPtr<FJsonValue>>* QualityInputs = nullptr;
	if (!GetRequiredArray(Root, TEXT("qualityInputs"), Location, QualityInputs, Errors))
	{
		return false;
	}

	bool bValid = true;
	TSet<FString> Names;
	for (int32 QualityInputIndex = 0; QualityInputIndex < QualityInputs->Num(); ++QualityInputIndex)
	{
		const FString InputLocation = FString::Printf(
			TEXT("%s.qualityInputs[%d]"),
			*Location,
			QualityInputIndex);
		const TSharedPtr<FJsonValue>& Value = (*QualityInputs)[QualityInputIndex];
		if (!Value.IsValid() || Value->Type != EJson::Object)
		{
			AddError(Errors, InputLocation, TEXT("must be an object."));
			bValid = false;
			continue;
		}

		const TSharedPtr<FJsonObject> Object = Value->AsObject();
		bValid &= ValidateAllowedFields(
			Object,
			{TEXT("name"), TEXT("default"), TEXT("low"), TEXT("medium"), TEXT("high"), TEXT("epic")},
			InputLocation,
			Errors);

		FQualityScalarSpec Spec;
		bValid &= GetRequiredString(Object, TEXT("name"), InputLocation, Spec.Name, Errors);
		bValid &= ValidateIdentifier(Spec.Name, InputLocation + TEXT(".name"), Errors);
		bValid &= GetRequiredNumber(Object, TEXT("default"), InputLocation, Spec.DefaultValue, Errors);
		bValid &= GetRequiredNumber(
			Object,
			TEXT("low"),
			InputLocation,
			Spec.Values[EMaterialQualityLevel::Low],
			Errors);
		bValid &= GetRequiredNumber(
			Object,
			TEXT("medium"),
			InputLocation,
			Spec.Values[EMaterialQualityLevel::Medium],
			Errors);
		bValid &= GetRequiredNumber(
			Object,
			TEXT("high"),
			InputLocation,
			Spec.Values[EMaterialQualityLevel::High],
			Errors);
		bValid &= GetRequiredNumber(
			Object,
			TEXT("epic"),
			InputLocation,
			Spec.Values[EMaterialQualityLevel::Epic],
			Errors);

		if (!Spec.Name.IsEmpty() && !TryAddUnique(Names, Spec.Name))
		{
			AddError(Errors, InputLocation + TEXT(".name"), TEXT("quality input names must be unique."));
			bValid = false;
		}
		OutSpec.QualityInputs.Add(MoveTemp(Spec));
	}
	return bValid;
}

bool IsGroundSurfaceRvtType(ERuntimeVirtualTextureMaterialType MaterialType)
{
	return MaterialType == ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular
		|| MaterialType == ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_YCoCg
		|| MaterialType == ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Specular_Mask_YCoCg;
}

bool ParseGroundRvt(
	const TSharedPtr<FJsonObject>& Root,
	const FString& Location,
	FMaterialCodeSpec& OutSpec,
	TArray<FString>& Errors)
{
	TSharedPtr<FJsonObject> Object;
	if (!GetRequiredObject(Root, TEXT("groundRVT"), Location, Object, Errors))
	{
		return false;
	}

	const FString GroundLocation = Location + TEXT(".groundRVT");
	bool bValid = ValidateAllowedFields(
		Object,
		{
			TEXT("staticSwitch"),
			TEXT("default"),
			TEXT("surfaceParameter"),
			TEXT("surfaceDefaultAsset"),
			TEXT("heightParameter"),
			TEXT("heightDefaultAsset"),
			TEXT("quality")
		},
		GroundLocation,
		Errors);
	bValid &= GetRequiredString(Object, TEXT("staticSwitch"), GroundLocation, OutSpec.GroundRvt.StaticSwitch, Errors);
	bValid &= GetRequiredBool(Object, TEXT("default"), GroundLocation, OutSpec.GroundRvt.bStaticDefault, Errors);
	bValid &= GetRequiredString(Object, TEXT("surfaceParameter"), GroundLocation, OutSpec.GroundRvt.SurfaceParameter, Errors);
	bValid &= GetRequiredString(Object, TEXT("surfaceDefaultAsset"), GroundLocation, OutSpec.GroundRvt.SurfaceDefaultAsset, Errors);
	bValid &= GetRequiredString(Object, TEXT("heightParameter"), GroundLocation, OutSpec.GroundRvt.HeightParameter, Errors);
	bValid &= GetRequiredString(Object, TEXT("heightDefaultAsset"), GroundLocation, OutSpec.GroundRvt.HeightDefaultAsset, Errors);
	bValid &= ValidateIdentifier(OutSpec.GroundRvt.StaticSwitch, GroundLocation + TEXT(".staticSwitch"), Errors);
	bValid &= ValidateIdentifier(OutSpec.GroundRvt.SurfaceParameter, GroundLocation + TEXT(".surfaceParameter"), Errors);
	bValid &= ValidateIdentifier(OutSpec.GroundRvt.HeightParameter, GroundLocation + TEXT(".heightParameter"), Errors);
	bValid &= ValidateDefaultObjectPath(OutSpec.GroundRvt.SurfaceDefaultAsset, GroundLocation + TEXT(".surfaceDefaultAsset"), Errors);
	bValid &= ValidateDefaultObjectPath(OutSpec.GroundRvt.HeightDefaultAsset, GroundLocation + TEXT(".heightDefaultAsset"), Errors);

	URuntimeVirtualTexture* SurfaceRvt =
		LoadObject<URuntimeVirtualTexture>(nullptr, *OutSpec.GroundRvt.SurfaceDefaultAsset);
	if (!SurfaceRvt)
	{
		AddError(
			Errors,
			GroundLocation + TEXT(".surfaceDefaultAsset"),
			TEXT("could not load the ground surface Runtime Virtual Texture."));
		bValid = false;
	}
	else if (!IsGroundSurfaceRvtType(SurfaceRvt->GetMaterialType()))
	{
		AddError(
			Errors,
			GroundLocation + TEXT(".surfaceDefaultAsset"),
			TEXT("must store BaseColor, Normal, Roughness, and Specular."));
		bValid = false;
	}

	if (OutSpec.GroundRvt.HeightDefaultAsset != GroundHeightRvtObjectPath)
	{
		AddError(
			Errors,
			GroundLocation + TEXT(".heightDefaultAsset"),
			FString::Printf(TEXT("is locked to `%s`."), GroundHeightRvtObjectPath));
		bValid = false;
	}
	if (URuntimeVirtualTexture* HeightRvt =
		LoadObject<URuntimeVirtualTexture>(nullptr, *OutSpec.GroundRvt.HeightDefaultAsset))
	{
		if (HeightRvt->GetMaterialType() != ERuntimeVirtualTextureMaterialType::WorldHeight)
		{
			AddError(
				Errors,
				GroundLocation + TEXT(".heightDefaultAsset"),
				TEXT("must use the WorldHeight RVT material type."));
			bValid = false;
		}
	}

	TSharedPtr<FJsonObject> Quality;
	if (!GetRequiredObject(Object, TEXT("quality"), GroundLocation, Quality, Errors))
	{
		return false;
	}
	bValid &= ValidateAllowedFields(
		Quality,
		{TEXT("low"), TEXT("medium"), TEXT("high"), TEXT("epic")},
		GroundLocation + TEXT(".quality"),
		Errors);
	bValid &= GetRequiredBool(
		Quality,
		TEXT("low"),
		GroundLocation + TEXT(".quality"),
		OutSpec.GroundRvt.QualityEnabled[EMaterialQualityLevel::Low],
		Errors);
	bValid &= GetRequiredBool(
		Quality,
		TEXT("medium"),
		GroundLocation + TEXT(".quality"),
		OutSpec.GroundRvt.QualityEnabled[EMaterialQualityLevel::Medium],
		Errors);
	bValid &= GetRequiredBool(
		Quality,
		TEXT("high"),
		GroundLocation + TEXT(".quality"),
		OutSpec.GroundRvt.QualityEnabled[EMaterialQualityLevel::High],
		Errors);
	bValid &= GetRequiredBool(
		Quality,
		TEXT("epic"),
		GroundLocation + TEXT(".quality"),
		OutSpec.GroundRvt.QualityEnabled[EMaterialQualityLevel::Epic],
		Errors);
	return bValid;
}

const TCHAR* GetCollectionRoleName(ECollectionRole Role)
{
	switch (Role)
	{
	case ECollectionRole::BaseColor:
		return TEXT("BaseColor");
	case ECollectionRole::MRAH:
		return TEXT("MRAH");
	case ECollectionRole::NormalLight:
		return TEXT("NormalLight");
	default:
		return TEXT("Unknown");
	}
}

const TCHAR* GetCollectionTextureSuffix(ECollectionRole Role)
{
	switch (Role)
	{
	case ECollectionRole::BaseColor:
		return TEXT("_BaseColor");
	case ECollectionRole::MRAH:
		return TEXT("_MRAH");
	case ECollectionRole::NormalLight:
		return TEXT("_NormalLight");
	default:
		return TEXT("");
	}
}

EMaterialSamplerType GetCollectionSamplerType(ECollectionRole Role)
{
	switch (Role)
	{
	case ECollectionRole::BaseColor:
		return SAMPLERTYPE_Color;
	case ECollectionRole::MRAH:
		return SAMPLERTYPE_Masks;
	case ECollectionRole::NormalLight:
		return SAMPLERTYPE_LinearColor;
	default:
		return SAMPLERTYPE_Color;
	}
}

bool ValidateTextureForCollectionRole(
	UTexture2D* Texture,
	ECollectionRole Role,
	const FString& Location,
	TArray<FString>& Errors,
	FString* OutStem = nullptr,
	bool bRequireRoleSuffix = true)
{
	if (!Texture)
	{
		AddError(Errors, Location, TEXT("must reference a Texture2D."));
		return false;
	}

	bool bValid = true;
	const FString Suffix = GetCollectionTextureSuffix(Role);
	FString Stem = Texture->GetName();
	if (bRequireRoleSuffix
		&& (!Stem.RemoveFromEnd(Suffix, ESearchCase::CaseSensitive) || Stem.IsEmpty()))
	{
		AddError(
			Errors,
			Location,
			FString::Printf(
				TEXT("texture `%s` must end with `%s` for role %s."),
				*Texture->GetPathName(),
				*Suffix,
				GetCollectionRoleName(Role)));
		bValid = false;
	}
	else if (OutStem)
	{
		*OutStem = MoveTemp(Stem);
	}

	if (Texture->VirtualTextureStreaming)
	{
		AddError(Errors, Location, TEXT("TextureCollection members must have Virtual Texture Streaming disabled."));
		bValid = false;
	}

	switch (Role)
	{
	case ECollectionRole::BaseColor:
		if (!Texture->SRGB || Texture->CompressionSettings != TC_Default)
		{
			AddError(Errors, Location, TEXT("BaseColor textures require sRGB=true and TC_Default compression."));
			bValid = false;
		}
		break;
	case ECollectionRole::MRAH:
		if (Texture->SRGB || Texture->CompressionSettings != TC_Masks)
		{
			AddError(Errors, Location, TEXT("MRAH textures require sRGB=false and TC_Masks compression."));
			bValid = false;
		}
		break;
	case ECollectionRole::NormalLight:
		if (Texture->SRGB || Texture->CompressionSettings != TC_BC7)
		{
			AddError(
				Errors,
				Location,
				TEXT("NormalLight textures require sRGB=false and TC_BC7; BC5/Normal compression discards LightMask.B."));
			bValid = false;
		}
		break;
	default:
		checkNoEntry();
		bValid = false;
		break;
	}
	return bValid;
}

bool ValidateParallelTextureCollections(
	const FString& Location,
	const FMaterialCodeSpec& Spec,
	TArray<FString>& Errors)
{
	constexpr int32 RoleCount = 3;
	const FCollectionSpec* RoleSpecs[RoleCount] = {};
	UTextureCollection* RoleCollections[RoleCount] = {};
	TArray<FString> RoleStems[RoleCount];
	bool bValid = true;

	if (Spec.Collections.Num() != RoleCount)
	{
		AddError(
			Errors,
			Location + TEXT(".textureBackend.collections"),
			TEXT("must contain exactly BaseColor, MRAH, and NormalLight collections."));
		bValid = false;
	}

	for (const FCollectionSpec& CollectionSpec : Spec.Collections)
	{
		const int32 RoleIndex = static_cast<int32>(CollectionSpec.Role);
		if (RoleIndex < 0 || RoleIndex >= RoleCount || RoleSpecs[RoleIndex])
		{
			bValid = false;
			continue;
		}
		RoleSpecs[RoleIndex] = &CollectionSpec;
		UTextureCollection* Collection =
			LoadObject<UTextureCollection>(nullptr, *CollectionSpec.DefaultAsset);
		RoleCollections[RoleIndex] = Collection;
		const FString CollectionLocation = FString::Printf(
			TEXT("%s.textureBackend.%s"),
			*Location,
			GetCollectionRoleName(CollectionSpec.Role));
		if (!Collection)
		{
			AddError(Errors, CollectionLocation, TEXT("could not load TextureCollection."));
			bValid = false;
			continue;
		}
		if (Collection->Textures.IsEmpty())
		{
			AddError(Errors, CollectionLocation, TEXT("TextureCollection must not be empty."));
			bValid = false;
			continue;
		}

		TSet<FString> UniqueStems;
		for (int32 TextureIndex = 0; TextureIndex < Collection->Textures.Num(); ++TextureIndex)
		{
			UTexture2D* Texture = Cast<UTexture2D>(Collection->Textures[TextureIndex]);
			FString Stem;
			const FString TextureLocation = FString::Printf(
				TEXT("%s[%d]"),
				*CollectionLocation,
				TextureIndex);
			bValid &= ValidateTextureForCollectionRole(
				Texture,
				CollectionSpec.Role,
				TextureLocation,
				Errors,
				&Stem);
			RoleStems[RoleIndex].Add(Stem);
			if (!Stem.IsEmpty() && UniqueStems.Contains(Stem))
			{
				AddError(Errors, TextureLocation, FString::Printf(TEXT("duplicate material stem `%s`."), *Stem));
				bValid = false;
			}
			else if (!Stem.IsEmpty())
			{
				UniqueStems.Add(Stem);
			}
		}
	}

	for (int32 RoleIndex = 0; RoleIndex < RoleCount; ++RoleIndex)
	{
		if (!RoleSpecs[RoleIndex])
		{
			AddError(
				Errors,
				Location + TEXT(".textureBackend.collections"),
				FString::Printf(TEXT("missing required collection role `%s`."), GetCollectionRoleName(static_cast<ECollectionRole>(RoleIndex))));
			bValid = false;
		}
	}

	if (RoleCollections[0] && RoleCollections[1] && RoleCollections[2])
	{
		const int32 ExpectedCount = RoleCollections[0]->Textures.Num();
		for (int32 RoleIndex = 1; RoleIndex < RoleCount; ++RoleIndex)
		{
			if (RoleCollections[RoleIndex]->Textures.Num() != ExpectedCount)
			{
				AddError(
					Errors,
					Location + TEXT(".textureBackend.collections"),
					FString::Printf(
						TEXT("parallel TextureCollections must have identical counts; BaseColor=%d, %s=%d."),
						ExpectedCount,
						GetCollectionRoleName(static_cast<ECollectionRole>(RoleIndex)),
						RoleCollections[RoleIndex]->Textures.Num()));
				bValid = false;
			}
		}
		if (RoleStems[1].Num() == ExpectedCount && RoleStems[2].Num() == ExpectedCount)
		{
			for (int32 TextureIndex = 0; TextureIndex < ExpectedCount; ++TextureIndex)
			{
				if (RoleStems[0][TextureIndex].IsEmpty()
					|| RoleStems[0][TextureIndex] != RoleStems[1][TextureIndex]
					|| RoleStems[0][TextureIndex] != RoleStems[2][TextureIndex])
				{
					AddError(
						Errors,
						Location + TEXT(".textureBackend.collections"),
						FString::Printf(
							TEXT("index %d is not aligned: BaseColor=`%s`, MRAH=`%s`, NormalLight=`%s`."),
							TextureIndex,
							*RoleStems[0][TextureIndex],
							*RoleStems[1][TextureIndex],
							*RoleStems[2][TextureIndex]));
					bValid = false;
				}
			}
		}
	}
	return bValid;
}

bool ValidateSemanticContract(
	const FString& Location,
	FMaterialCodeSpec& Spec,
	TArray<FString>& Errors)
{
	bool bValid = true;
	bValid &= ValidateParallelTextureCollections(Location, Spec, Errors);
	TMap<FString, const FCodeInputSpec*> InputsByName;
	TSet<FString> HlslNames;
	TSet<int32> PerInstanceDataIndices;

	for (const FCodeInputSpec& Input : Spec.Inputs)
	{
		if (InputsByName.Contains(Input.Name))
		{
			AddError(Errors, Location + TEXT(".inputs"), FString::Printf(TEXT("duplicate input name `%s`."), *Input.Name));
			bValid = false;
		}
		else
		{
			InputsByName.Add(Input.Name, &Input);
		}

		if (Input.bCustomInput)
		{
			if (!TryAddUnique(HlslNames, Input.Name))
			{
				AddError(Errors, Location + TEXT(".inputs"), FString::Printf(TEXT("duplicate HLSL argument `%s`."), *Input.Name));
				bValid = false;
			}
			if (Input.Type == ECodeInputType::Texture2D && !TryAddUnique(HlslNames, Input.Sampler))
			{
				AddError(Errors, Location + TEXT(".inputs"), FString::Printf(TEXT("duplicate HLSL sampler `%s`."), *Input.Sampler));
				bValid = false;
			}
		}
		if (Input.bHasPerInstanceDataIndex
			&& PerInstanceDataIndices.Contains(Input.PerInstanceDataIndex))
		{
			AddError(
				Errors,
				Location + TEXT(".inputs"),
				FString::Printf(
					TEXT("PerInstanceCustomData index %d is assigned more than once."),
					Input.PerInstanceDataIndex));
			bValid = false;
		}
		else if (Input.bHasPerInstanceDataIndex)
		{
			PerInstanceDataIndices.Add(Input.PerInstanceDataIndex);
		}
	}

	TSet<FString> BoundTextureParameters;
	for (const FCollectionSpec& Collection : Spec.Collections)
	{
		if (!TryAddUnique(HlslNames, Collection.Parameter))
		{
			AddError(
				Errors,
				Location + TEXT(".textureBackend.collections"),
				FString::Printf(TEXT("collection parameter `%s` collides with another HLSL name."), *Collection.Parameter));
			bValid = false;
		}

		for (const FCollectionBindingSpec& Binding : Collection.Bindings)
		{
			const FCodeInputSpec* const* TextureInputPtr = InputsByName.Find(Binding.TextureParameter);
			const FCodeInputSpec* const* IndexInputPtr = InputsByName.Find(Binding.IndexParameter);
			if (!TextureInputPtr)
			{
				AddError(
					Errors,
					Location + TEXT(".textureBackend.collections"),
					FString::Printf(TEXT("binding references missing Texture2D input `%s`."), *Binding.TextureParameter));
				bValid = false;
			}
			else if ((*TextureInputPtr)->Type != ECodeInputType::Texture2D || !(*TextureInputPtr)->bCustomInput)
			{
				AddError(
					Errors,
					Location + TEXT(".textureBackend.collections"),
					FString::Printf(TEXT("`%s` must be a Texture2D with customInput=true."), *Binding.TextureParameter));
				bValid = false;
			}
			else
			{
				const FCodeInputSpec* TextureInput = *TextureInputPtr;
				BoundTextureParameters.Add(Binding.TextureParameter);
				if (TextureInput->SamplerType != GetCollectionSamplerType(Collection.Role))
				{
					AddError(
						Errors,
						Location + TEXT(".textureBackend.collections"),
						FString::Printf(
							TEXT("binding `%s` must use the samplerType required by role %s."),
							*Binding.TextureParameter,
							GetCollectionRoleName(Collection.Role)));
					bValid = false;
				}
				UTexture2D* FallbackTexture =
					LoadObject<UTexture2D>(nullptr, *TextureInput->DefaultAsset);
				bValid &= ValidateTextureForCollectionRole(
					FallbackTexture,
					Collection.Role,
					Location + TEXT(".inputs.") + Binding.TextureParameter,
					Errors,
					nullptr,
					false);
			}

			if (!IndexInputPtr)
			{
				AddError(
					Errors,
					Location + TEXT(".textureBackend.collections"),
					FString::Printf(TEXT("binding references missing Scalar input `%s`."), *Binding.IndexParameter));
				bValid = false;
			}
			else if ((*IndexInputPtr)->Type != ECodeInputType::Scalar || !(*IndexInputPtr)->bCustomInput)
			{
				AddError(
					Errors,
					Location + TEXT(".textureBackend.collections"),
					FString::Printf(TEXT("`%s` must be a Scalar with customInput=true."), *Binding.IndexParameter));
				bValid = false;
			}
		}
	}

	for (const FCodeInputSpec& Input : Spec.Inputs)
	{
		if (Input.Type == ECodeInputType::Texture2D && Input.bCustomInput && !BoundTextureParameters.Contains(Input.Name))
		{
			AddError(
				Errors,
				Location + TEXT(".inputs"),
				FString::Printf(TEXT("Texture2D input `%s` has no TextureCollection binding."), *Input.Name));
			bValid = false;
		}
	}

	for (const FQualityScalarSpec& QualityInput : Spec.QualityInputs)
	{
		if (!TryAddUnique(HlslNames, QualityInput.Name))
		{
			AddError(
				Errors,
				Location + TEXT(".qualityInputs"),
				FString::Printf(TEXT("quality input `%s` collides with another HLSL name."), *QualityInput.Name));
			bValid = false;
		}
	}

	const FString GroundRvtNames[] =
	{
		TEXT("GroundRVTBaseColor"),
		TEXT("GroundRVTRoughness"),
		TEXT("GroundRVTSpecular"),
		TEXT("GroundRVTNormalTS"),
		TEXT("GroundRVTWorldHeight"),
		TEXT("GroundRVTEnabled")
	};
	for (const FString& GroundRvtName : GroundRvtNames)
	{
		if (!TryAddUnique(HlslNames, GroundRvtName))
		{
			AddError(
				Errors,
				Location + TEXT(".groundRVT"),
				FString::Printf(TEXT("built-in ground RVT argument `%s` collides with another HLSL name."), *GroundRvtName));
			bValid = false;
		}
	}

	const FString BuiltInNames[] =
	{
		Spec.TextureCollectionSwitch,
		TEXT("UV0"),
		TEXT("VertexColor"),
		TEXT("ViewDirTS"),
		TEXT("PixelDepth"),
		TEXT("AbsoluteWorldPosition")
	};
	for (const FString& Name : BuiltInNames)
	{
		if (!TryAddUnique(HlslNames, Name))
		{
			AddError(Errors, Location, FString::Printf(TEXT("built-in argument `%s` collides with another HLSL name."), *Name));
			bValid = false;
		}
	}

	TSet<FString> OutputNames;
	TSet<FString> ConnectedProperties;
	const auto ValidateOutputName = [&](const FOutputSpec& Output, bool bMain)
	{
		if (!TryAddUnique(OutputNames, Output.Name))
		{
			AddError(Errors, Location + TEXT(".outputs"), FString::Printf(TEXT("duplicate output name `%s`."), *Output.Name));
			bValid = false;
		}
		if (HlslNames.Contains(Output.Name))
		{
			AddError(Errors, Location + TEXT(".outputs"), FString::Printf(TEXT("output `%s` collides with a Custom argument."), *Output.Name));
			bValid = false;
		}
		if (Output.MaterialProperty != TEXT("None") && !TryAddUnique(ConnectedProperties, Output.MaterialProperty))
		{
			AddError(
				Errors,
				Location + TEXT(".outputs"),
				FString::Printf(TEXT("material property `%s` is connected more than once."), *Output.MaterialProperty));
			bValid = false;
		}
		if (bMain && (Output.Name != TEXT("BaseColor") || Output.MaterialProperty != TEXT("BaseColor") || Output.Type != CMOT_Float3))
		{
			AddError(Errors, Location + TEXT(".outputs.main"), TEXT("main output must be BaseColor/float3/BaseColor."));
			bValid = false;
		}
	};

	ValidateOutputName(Spec.MainOutput, true);
	for (const FOutputSpec& Output : Spec.AdditionalOutputs)
	{
		ValidateOutputName(Output, false);
	}

	const FString RequiredProperties[] =
	{
		TEXT("BaseColor"),
		TEXT("Normal"),
		TEXT("AmbientOcclusion"),
		TEXT("Roughness"),
		TEXT("Metallic"),
		TEXT("Specular")
	};
	for (const FString& RequiredProperty : RequiredProperties)
	{
		if (!ConnectedProperties.Contains(RequiredProperty))
		{
			AddError(
				Errors,
				Location + TEXT(".outputs"),
				FString::Printf(TEXT("required material property `%s` is missing."), *RequiredProperty));
			bValid = false;
		}
	}
	if (Spec.MaterialKind == PropKind
		&& !ConnectedProperties.Contains(TEXT("EmissiveColor")))
	{
		AddError(Errors, Location + TEXT(".outputs"), TEXT("Prop must connect EmissiveColor."));
		bValid = false;
	}

	return bValid;
}

FString ResolveProjectShaderInclude(const FString& ShaderInclude)
{
	const FString RelativeShaderPath = ShaderInclude.RightChop(FCString::Strlen(TEXT("/Project/")));
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"), RelativeShaderPath));
}

bool ValidateShaderInclude(
	const FString& ShaderInclude,
	const FString& Location,
	TArray<FString>& Errors)
{
	if (!ShaderInclude.StartsWith(TEXT("/Project/"))
		|| !ShaderInclude.EndsWith(TEXT(".ush"))
		|| ShaderInclude.Contains(TEXT("..")))
	{
		AddError(Errors, Location, TEXT("must be a `/Project/.../*.ush` virtual include without parent traversal."));
		return false;
	}

	const FString PhysicalShaderPath = ResolveProjectShaderInclude(ShaderInclude);
	if (!FPaths::FileExists(PhysicalShaderPath))
	{
		AddError(
			Errors,
			Location,
			FString::Printf(TEXT("mapped shader include does not exist: `%s`."), *PhysicalShaderPath));
		return false;
	}
	return true;
}

bool BuildGenerationFingerprint(
	const FString& JsonText,
	FMaterialCodeSpec& Spec,
	const FString& Location,
	TArray<FString>& Errors)
{
	const FString ShaderPath = ResolveProjectShaderInclude(Spec.ShaderInclude);
	const FString CommonShaderPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"), CommonShaderRelativePath));

	FString ShaderText;
	FString CommonShaderText;
	bool bValid = true;
	if (!FFileHelper::LoadFileToString(ShaderText, *ShaderPath))
	{
		AddError(
			Errors,
			Location + TEXT(".shaderInclude"),
			FString::Printf(TEXT("could not read shader source `%s` for fingerprinting."), *ShaderPath));
		bValid = false;
	}
	if (!FFileHelper::LoadFileToString(CommonShaderText, *CommonShaderPath))
	{
		AddError(
			Errors,
			Location + TEXT(".shaderInclude"),
			FString::Printf(TEXT("could not read common shader source `%s` for fingerprinting."), *CommonShaderPath));
		bValid = false;
	}
	if (!bValid)
	{
		return false;
	}

	FString NormalizedJson = JsonText;
	FString NormalizedShader = ShaderText;
	FString NormalizedCommonShader = CommonShaderText;
	NormalizedJson.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	NormalizedShader.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	NormalizedCommonShader.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

	const FString FingerprintPayload = FString::Printf(
		TEXT("Generator=%llu\nSchema=%s\nKind=%s\nJSON:\n%s\nShader:\n%s\nCommon:\n%s"),
		static_cast<unsigned long long>(CodeMaterialGeneratorVersion),
		SchemaName,
		*Spec.MaterialKind,
		*NormalizedJson,
		*NormalizedShader,
		*NormalizedCommonShader);
	Spec.GenerationFingerprint = FGuid::NewDeterministicGuid(
		FingerprintPayload,
		CodeMaterialGeneratorVersion).ToString(EGuidFormats::Digits);
	return true;
}

bool LoadAndValidateSpec(
	const FString& JsonPath,
	const FString& ExpectedKind,
	const FString& ExpectedAssetPath,
	FMaterialCodeSpec& OutSpec,
	TArray<FString>& Errors)
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *JsonPath))
	{
		AddError(Errors, JsonPath, TEXT("could not read JSON file."));
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		AddError(Errors, JsonPath, TEXT("is not valid JSON."));
		return false;
	}

	bool bValid = ValidateAllowedFields(
		Root,
		{
			TEXT("schema"),
			TEXT("materialKind"),
			TEXT("assetPath"),
			TEXT("shaderInclude"),
			TEXT("entryPoint"),
			TEXT("packing"),
			TEXT("inputs"),
			TEXT("outputs"),
			TEXT("textureBackend"),
			TEXT("qualityInputs"),
			TEXT("groundRVT"),
			TEXT("features")
		},
		JsonPath,
		Errors);

	FString Schema;
	bValid &= GetRequiredString(Root, TEXT("schema"), JsonPath, Schema, Errors);
	bValid &= GetRequiredString(Root, TEXT("materialKind"), JsonPath, OutSpec.MaterialKind, Errors);
	bValid &= GetRequiredString(Root, TEXT("assetPath"), JsonPath, OutSpec.AssetPath, Errors);
	bValid &= GetRequiredString(Root, TEXT("shaderInclude"), JsonPath, OutSpec.ShaderInclude, Errors);
	bValid &= GetRequiredString(Root, TEXT("entryPoint"), JsonPath, OutSpec.EntryPoint, Errors);

	if (!Schema.IsEmpty() && Schema != SchemaName)
	{
		AddError(Errors, JsonPath + TEXT(".schema"), FString::Printf(TEXT("must be exactly `%s`."), SchemaName));
		bValid = false;
	}
	if (!OutSpec.MaterialKind.IsEmpty() && OutSpec.MaterialKind != ExpectedKind)
	{
		AddError(
			Errors,
			JsonPath + TEXT(".materialKind"),
			FString::Printf(TEXT("must be exactly `%s` for this file."), *ExpectedKind));
		bValid = false;
	}
	if (!OutSpec.AssetPath.IsEmpty() && OutSpec.AssetPath != ExpectedAssetPath)
	{
		AddError(
			Errors,
			JsonPath + TEXT(".assetPath"),
			FString::Printf(TEXT("is locked to `%s`."), *ExpectedAssetPath));
		bValid = false;
	}
	bValid &= ValidateIdentifier(OutSpec.EntryPoint, JsonPath + TEXT(".entryPoint"), Errors);
	bValid &= ValidateShaderInclude(OutSpec.ShaderInclude, JsonPath + TEXT(".shaderInclude"), Errors);
	bValid &= ParsePacking(Root, JsonPath, Errors);

	const TArray<TSharedPtr<FJsonValue>>* Inputs = nullptr;
	if (!GetRequiredArray(Root, TEXT("inputs"), JsonPath, Inputs, Errors))
	{
		bValid = false;
	}
	else if (Inputs->IsEmpty())
	{
		AddError(Errors, JsonPath + TEXT(".inputs"), TEXT("must contain at least one input."));
		bValid = false;
	}
	else
	{
		for (int32 InputIndex = 0; InputIndex < Inputs->Num(); ++InputIndex)
		{
			const FString InputLocation = FString::Printf(TEXT("%s.inputs[%d]"), *JsonPath, InputIndex);
			const TSharedPtr<FJsonValue>& InputValue = (*Inputs)[InputIndex];
			if (!InputValue.IsValid() || InputValue->Type != EJson::Object)
			{
				AddError(Errors, InputLocation, TEXT("must be an object."));
				bValid = false;
				continue;
			}

			FCodeInputSpec Input;
			bValid &= ParseInput(InputValue->AsObject(), InputLocation, Input, Errors);
			OutSpec.Inputs.Add(MoveTemp(Input));
		}
	}

	TSharedPtr<FJsonObject> Outputs;
	if (!GetRequiredObject(Root, TEXT("outputs"), JsonPath, Outputs, Errors))
	{
		bValid = false;
	}
	else
	{
		bValid &= ValidateAllowedFields(Outputs, {TEXT("main"), TEXT("additional")}, JsonPath + TEXT(".outputs"), Errors);

		TSharedPtr<FJsonObject> MainOutput;
		if (GetRequiredObject(Outputs, TEXT("main"), JsonPath + TEXT(".outputs"), MainOutput, Errors))
		{
			bValid &= ParseOutput(MainOutput, JsonPath + TEXT(".outputs.main"), OutSpec.MainOutput, Errors);
		}
		else
		{
			bValid = false;
		}

		const TArray<TSharedPtr<FJsonValue>>* AdditionalOutputs = nullptr;
		if (!GetRequiredArray(Outputs, TEXT("additional"), JsonPath + TEXT(".outputs"), AdditionalOutputs, Errors))
		{
			bValid = false;
		}
		else
		{
			for (int32 OutputIndex = 0; OutputIndex < AdditionalOutputs->Num(); ++OutputIndex)
			{
				const FString OutputLocation = FString::Printf(
					TEXT("%s.outputs.additional[%d]"),
					*JsonPath,
					OutputIndex);
				const TSharedPtr<FJsonValue>& OutputValue = (*AdditionalOutputs)[OutputIndex];
				if (!OutputValue.IsValid() || OutputValue->Type != EJson::Object)
				{
					AddError(Errors, OutputLocation, TEXT("must be an object."));
					bValid = false;
					continue;
				}

				FOutputSpec Output;
				bValid &= ParseOutput(OutputValue->AsObject(), OutputLocation, Output, Errors);
				OutSpec.AdditionalOutputs.Add(MoveTemp(Output));
			}
		}
	}

	bValid &= ParseTextureBackend(Root, JsonPath, OutSpec, Errors);
	bValid &= ParseQualityInputs(Root, JsonPath, OutSpec, Errors);
	bValid &= ParseGroundRvt(Root, JsonPath, OutSpec, Errors);

	TSharedPtr<FJsonObject> Features;
	if (!GetRequiredObject(Root, TEXT("features"), JsonPath, Features, Errors))
	{
		bValid = false;
	}

	bValid &= ValidateSemanticContract(JsonPath, OutSpec, Errors);
	if (bValid)
	{
		bValid &= BuildGenerationFingerprint(JsonText, OutSpec, JsonPath, Errors);
	}
	return bValid;
}

FString ToObjectPath(const FString& PackagePath)
{
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
}

template <typename ExpressionType>
ExpressionType* AddExpression(UMaterial* Material, int32 NodeX, int32 NodeY)
{
	if (!Material || !Material->GetEditorOnlyData())
	{
		return nullptr;
	}

	ExpressionType* Expression = NewObject<ExpressionType>(Material);
	Expression->MaterialExpressionEditorX = NodeX;
	Expression->MaterialExpressionEditorY = NodeY;
	Material->GetEditorOnlyData()->ExpressionCollection.AddExpression(Expression);
	return Expression;
}

bool PatchGroundRvtWriter(
	TArray<UPackage*>& DirtyPackages,
	FString& OutError)
{
	UMaterial* GroundMaterial =
		LoadObject<UMaterial>(nullptr, GroundMaterialObjectPath);
	if (!GroundMaterial || !GroundMaterial->GetEditorOnlyData())
	{
		OutError = FString::Printf(
			TEXT("could not load ground RVT writer material `%s`."),
			GroundMaterialObjectPath);
		return false;
	}

	TArray<UMaterialExpressionRuntimeVirtualTextureOutput*> RvtOutputs;
	for (UMaterialExpression* Expression : GroundMaterial->GetExpressions())
	{
		if (UMaterialExpressionRuntimeVirtualTextureOutput* RvtOutput =
			Cast<UMaterialExpressionRuntimeVirtualTextureOutput>(Expression))
		{
			RvtOutputs.Add(RvtOutput);
		}
	}
	if (RvtOutputs.Num() != 1)
	{
		OutError = FString::Printf(
			TEXT("ground RVT writer `%s` must contain exactly one RuntimeVirtualTextureOutput; found %d."),
			GroundMaterialObjectPath,
			RvtOutputs.Num());
		return false;
	}

	UMaterialExpressionRuntimeVirtualTextureOutput* RvtOutput = RvtOutputs[0];
	if (!RvtOutput->Specular.Expression)
	{
		OutError = FString::Printf(
			TEXT("ground RVT writer `%s` must connect its Specular pin to the MaterialLightMask payload."),
			GroundMaterialObjectPath);
		return false;
	}
	bool bChanged = false;

	UMaterialExpressionWorldPosition* ExistingWorldPosition =
		Cast<UMaterialExpressionWorldPosition>(RvtOutput->WorldHeight.Expression);
	const bool bWorldHeightAlreadyUsesWorldZ =
		ExistingWorldPosition
		&& ExistingWorldPosition->WorldPositionShaderOffset == WPT_ExcludeAllShaderOffsets
		&& RvtOutput->WorldHeight.Mask != 0
		&& RvtOutput->WorldHeight.MaskR == 0
		&& RvtOutput->WorldHeight.MaskG == 0
		&& RvtOutput->WorldHeight.MaskB != 0
		&& RvtOutput->WorldHeight.MaskA == 0;

	UMaterialExpressionTransform* ExistingNormalTransform =
		Cast<UMaterialExpressionTransform>(RvtOutput->Normal.Expression);
	const bool bNormalAlreadyWorldSpace =
		ExistingNormalTransform
		&& ExistingNormalTransform->TransformSourceType == TRANSFORMSOURCE_Tangent
		&& ExistingNormalTransform->TransformType == TRANSFORM_World;
	if (bWorldHeightAlreadyUsesWorldZ && bNormalAlreadyWorldSpace)
	{
		return true;
	}

	GroundMaterial->Modify();
	RvtOutput->Modify();
	if (!bWorldHeightAlreadyUsesWorldZ)
	{
		UMaterialExpressionWorldPosition* WorldPosition = ExistingWorldPosition;
		if (!WorldPosition)
		{
			WorldPosition =
				AddExpression<UMaterialExpressionWorldPosition>(
					GroundMaterial,
					RvtOutput->MaterialExpressionEditorX - 320,
					RvtOutput->MaterialExpressionEditorY + 220);
		}
		if (!WorldPosition)
		{
			OutError = TEXT("could not create the ground RVT WorldHeight writer.");
			return false;
		}
		WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
		RvtOutput->WorldHeight.Connect(0, WorldPosition);
		RvtOutput->WorldHeight.SetMask(1, 0, 0, 1, 0);
		bChanged = true;
	}

	if (!bNormalAlreadyWorldSpace)
	{
		UMaterialExpression* ExistingNormal = RvtOutput->Normal.Expression;
		const int32 ExistingNormalOutputIndex = RvtOutput->Normal.OutputIndex;
		if (!ExistingNormal)
		{
			OutError = FString::Printf(
				TEXT("ground RVT writer `%s` has no Normal input to transform into world space."),
				GroundMaterialObjectPath);
			return false;
		}

		UMaterialExpressionTransform* NormalToWorld =
			AddExpression<UMaterialExpressionTransform>(
				GroundMaterial,
				RvtOutput->MaterialExpressionEditorX - 320,
				RvtOutput->MaterialExpressionEditorY + 100);
		if (!NormalToWorld)
		{
			OutError = TEXT("could not create the ground RVT Tangent-to-World normal transform.");
			return false;
		}
		NormalToWorld->TransformSourceType = TRANSFORMSOURCE_Tangent;
		NormalToWorld->TransformType = TRANSFORM_World;
		NormalToWorld->Input.Connect(ExistingNormalOutputIndex, ExistingNormal);
		RvtOutput->Normal.Connect(0, NormalToWorld);
		bChanged = true;
	}

	check(bChanged);
	GroundMaterial->PostEditChange();
	GroundMaterial->ForceRecompileForRendering();
	FMaterialResource* MaterialResource =
		GroundMaterial->GetMaterialResource(GMaxRHIShaderPlatform);
	if (!MaterialResource)
	{
		OutError = FString::Printf(
			TEXT("ground RVT writer `%s` has no material resource for shader platform %d."),
			GroundMaterialObjectPath,
			static_cast<int32>(GMaxRHIShaderPlatform));
		return false;
	}
	MaterialResource->FinishCompilation();
	if (!MaterialResource->GetCompileErrors().IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("ground RVT writer `%s` failed shader compilation:\n  %s"),
			GroundMaterialObjectPath,
			*FString::Join(MaterialResource->GetCompileErrors(), TEXT("\n  ")));
		return false;
	}
	if (!MaterialResource->HasValidGameThreadShaderMap())
	{
		OutError = FString::Printf(
			TEXT("ground RVT writer `%s` produced no valid shader map for platform %d."),
			GroundMaterialObjectPath,
			static_cast<int32>(GMaxRHIShaderPlatform));
		return false;
	}

	UPackage* Package = GroundMaterial->GetOutermost();
	GroundMaterial->MarkPackageDirty();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	return true;
}

void Disconnect(FExpressionInput& Input)
{
	Input.Expression = nullptr;
	Input.OutputIndex = 0;
	Input.SetMask(0, 0, 0, 0, 0);
}

void ResetSurfaceMaterial(UMaterial* Material)
{
	check(Material);
	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	check(Data);

	Material->Modify();
	Data->ExpressionCollection.Empty();
	Disconnect(Data->BaseColor);
	Disconnect(Data->Metallic);
	Disconnect(Data->Specular);
	Disconnect(Data->Roughness);
	Disconnect(Data->Anisotropy);
	Disconnect(Data->Normal);
	Disconnect(Data->Tangent);
	Disconnect(Data->EmissiveColor);
	Disconnect(Data->Opacity);
	Disconnect(Data->OpacityMask);
	Disconnect(Data->WorldPositionOffset);
	Disconnect(Data->Displacement);
	Disconnect(Data->SubsurfaceColor);
	Disconnect(Data->ClearCoat);
	Disconnect(Data->ClearCoatRoughness);
	Disconnect(Data->AmbientOcclusion);
	Disconnect(Data->Refraction);
	for (FVector2MaterialInput& CustomizedUv : Data->CustomizedUVs)
	{
		Disconnect(CustomizedUv);
	}
	Disconnect(Data->MaterialAttributes);
	Data->MaterialAttributes.PropertyConnectedMask = 0;
	Disconnect(Data->PixelDepthOffset);
	Disconnect(Data->ShadingModelFromMaterialExpression);
	Disconnect(Data->SurfaceThickness);
	Disconnect(Data->FrontMaterial);
	Data->ParameterGroupData.Empty();

	Material->MaterialDomain = MD_Surface;
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_DefaultLit);
	Material->TwoSided = false;
	Material->bTangentSpaceNormal = true;
	Material->bUseMaterialAttributes = false;
}

void InitializeParameter(
	UMaterialExpressionParameter* Parameter,
	const FCodeInputSpec& Spec,
	const FString& AssetPath)
{
	Parameter->ParameterName = FName(*Spec.Name);
	const FString StableIdentity = FString::Printf(
		TEXT("%s|%s|%s"),
		*AssetPath,
		*Parameter->GetClass()->GetPathName(),
		*Spec.Name);
	Parameter->ExpressionGUID = FGuid::NewDeterministicGuid(StableIdentity);
	Parameter->Group = FName(*Spec.Group);
	Parameter->SortPriority = Spec.SortPriority;
}

template <typename ParameterExpressionType>
void InitializeNamedParameter(
	ParameterExpressionType* Parameter,
	const FString& ParameterName,
	const FString& AssetPath)
{
	Parameter->ParameterName = FName(*ParameterName);
	const FString StableIdentity = FString::Printf(
		TEXT("%s|%s|%s"),
		*AssetPath,
		*Parameter->GetClass()->GetPathName(),
		*ParameterName);
	Parameter->ExpressionGUID = FGuid::NewDeterministicGuid(StableIdentity);
}

bool ValidateTargetPackageWritable(const FMaterialCodeSpec& Spec, FString& OutError)
{
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(
		Spec.AssetPath,
		FPackageName::GetAssetPackageExtension());
	if (PackageFilename.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("could not resolve target package filename for `%s`."),
			*Spec.AssetPath);
		return false;
	}
	if (FPaths::FileExists(PackageFilename) && IFileManager::Get().IsReadOnly(*PackageFilename))
	{
		OutError = FString::Printf(
			TEXT("target `%s` is read-only. Open the exact asset in the intended P4 changelist before Apply."),
			*PackageFilename);
		return false;
	}
	return true;
}

bool ValidatePackagePathWritable(const FString& PackagePath, FString& OutError)
{
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(
		PackagePath,
		FPackageName::GetAssetPackageExtension());
	if (PackageFilename.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("could not resolve package filename for `%s`."),
			*PackagePath);
		return false;
	}
	if (FPaths::FileExists(PackageFilename) && IFileManager::Get().IsReadOnly(*PackageFilename))
	{
		OutError = FString::Printf(
			TEXT("target `%s` is read-only. Open the exact asset in the intended P4 changelist before Apply."),
			*PackageFilename);
		return false;
	}
	return true;
}

bool EnsureGroundHeightRvt(TArray<UPackage*>& DirtyPackages, FString& OutError)
{
	UObject* ExistingObject =
		StaticLoadObject(UObject::StaticClass(), nullptr, GroundHeightRvtObjectPath, nullptr, LOAD_NoWarn);
	if (ExistingObject)
	{
		URuntimeVirtualTexture* ExistingRvt = Cast<URuntimeVirtualTexture>(ExistingObject);
		if (!ExistingRvt)
		{
			OutError = FString::Printf(
				TEXT("support asset `%s` exists but is not a RuntimeVirtualTexture."),
				GroundHeightRvtObjectPath);
			return false;
		}
		if (ExistingRvt->GetMaterialType() != ERuntimeVirtualTextureMaterialType::WorldHeight)
		{
			OutError = FString::Printf(
				TEXT("support asset `%s` must use the WorldHeight material type."),
				GroundHeightRvtObjectPath);
			return false;
		}
		return true;
	}

	UPackage* Package = CreatePackage(GroundHeightRvtAssetPath);
	if (!Package)
	{
		OutError = FString::Printf(TEXT("could not create package `%s`."), GroundHeightRvtAssetPath);
		return false;
	}
	const FString AssetName = FPackageName::GetLongPackageAssetName(GroundHeightRvtAssetPath);
	URuntimeVirtualTexture* HeightRvt = NewObject<URuntimeVirtualTexture>(
		Package,
		*AssetName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!HeightRvt)
	{
		OutError = TEXT("could not create the ground WorldHeight RuntimeVirtualTexture.");
		return false;
	}

	FEnumProperty* MaterialTypeProperty =
		FindFProperty<FEnumProperty>(URuntimeVirtualTexture::StaticClass(), TEXT("MaterialType"));
	if (!MaterialTypeProperty)
	{
		OutError = TEXT("could not resolve URuntimeVirtualTexture.MaterialType.");
		return false;
	}
	void* MaterialTypeAddress = MaterialTypeProperty->ContainerPtrToValuePtr<void>(HeightRvt);
	MaterialTypeProperty->GetUnderlyingProperty()->SetIntPropertyValue(
		MaterialTypeAddress,
		static_cast<int64>(ERuntimeVirtualTextureMaterialType::WorldHeight));

	FAssetRegistryModule::AssetCreated(HeightRvt);
	HeightRvt->PostEditChange();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	return true;
}

void AppendCustomInput(
	UMaterialExpressionCustom* Custom,
	const FString& Name,
	UMaterialExpression* Expression,
	int32 OutputIndex = 0)
{
	FCustomInput& Input = Custom->Inputs.AddDefaulted_GetRef();
	Input.InputName = FName(*Name);
	Input.Input.Connect(OutputIndex, Expression);
}

UMaterialExpressionConstant* AddScalarConstant(
	UMaterial* Material,
	float Value,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionConstant* Constant =
		AddExpression<UMaterialExpressionConstant>(Material, NodeX, NodeY);
	if (Constant)
	{
		Constant->R = Value;
	}
	return Constant;
}

UMaterialExpressionQualitySwitch* AddQualityScalar(
	UMaterial* Material,
	const FQualityScalarSpec& Spec,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionQualitySwitch* QualitySwitch =
		AddExpression<UMaterialExpressionQualitySwitch>(Material, NodeX + 260, NodeY);
	UMaterialExpressionConstant* Default =
		AddScalarConstant(Material, static_cast<float>(Spec.DefaultValue), NodeX, NodeY - 80);
	if (!QualitySwitch || !Default)
	{
		return nullptr;
	}
	QualitySwitch->Default.Connect(0, Default);

	for (int32 QualityIndex = 0; QualityIndex < EMaterialQualityLevel::Num; ++QualityIndex)
	{
		UMaterialExpressionConstant* QualityValue =
			AddScalarConstant(
				Material,
				static_cast<float>(Spec.Values[QualityIndex]),
				NodeX,
				NodeY + QualityIndex * 75);
		if (!QualityValue)
		{
			return nullptr;
		}
		QualitySwitch->Inputs[QualityIndex].Connect(0, QualityValue);
	}
	return QualitySwitch;
}

UMaterialExpressionQualitySwitch* AddQualityGate(
	UMaterial* Material,
	UMaterialExpression* EnabledExpression,
	int32 EnabledOutputIndex,
	UMaterialExpression* DisabledExpression,
	const bool QualityEnabled[EMaterialQualityLevel::Num],
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionQualitySwitch* QualitySwitch =
		AddExpression<UMaterialExpressionQualitySwitch>(Material, NodeX, NodeY);
	if (!QualitySwitch || !EnabledExpression || !DisabledExpression)
	{
		return nullptr;
	}
	QualitySwitch->Default.Connect(0, DisabledExpression);
	for (int32 QualityIndex = 0; QualityIndex < EMaterialQualityLevel::Num; ++QualityIndex)
	{
		QualitySwitch->Inputs[QualityIndex].Connect(
			QualityEnabled[QualityIndex] ? EnabledOutputIndex : 0,
			QualityEnabled[QualityIndex] ? EnabledExpression : DisabledExpression);
	}
	return QualitySwitch;
}

UMaterialExpressionStaticSwitch* AddStaticGate(
	UMaterial* Material,
	UMaterialExpression* EnabledExpression,
	UMaterialExpression* DisabledExpression,
	UMaterialExpressionStaticBoolParameter* StaticBool,
	bool bDefaultValue,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionStaticSwitch* StaticSwitch =
		AddExpression<UMaterialExpressionStaticSwitch>(Material, NodeX, NodeY);
	if (!StaticSwitch || !EnabledExpression || !DisabledExpression || !StaticBool)
	{
		return nullptr;
	}
	StaticSwitch->DefaultValue = bDefaultValue;
	StaticSwitch->A.Connect(0, EnabledExpression);
	StaticSwitch->B.Connect(0, DisabledExpression);
	StaticSwitch->Value.Connect(0, StaticBool);
	return StaticSwitch;
}

FExpressionInput* ResolveMaterialPropertyInput(
	UMaterialEditorOnlyData* Data,
	const FString& MaterialProperty)
{
	if (MaterialProperty == TEXT("BaseColor"))
	{
		return &Data->BaseColor;
	}
	if (MaterialProperty == TEXT("Normal"))
	{
		return &Data->Normal;
	}
	if (MaterialProperty == TEXT("AmbientOcclusion"))
	{
		return &Data->AmbientOcclusion;
	}
	if (MaterialProperty == TEXT("Roughness"))
	{
		return &Data->Roughness;
	}
	if (MaterialProperty == TEXT("Metallic"))
	{
		return &Data->Metallic;
	}
	if (MaterialProperty == TEXT("Specular"))
	{
		return &Data->Specular;
	}
	if (MaterialProperty == TEXT("EmissiveColor"))
	{
		return &Data->EmissiveColor;
	}
	if (MaterialProperty == TEXT("PixelDepthOffset"))
	{
		return &Data->PixelDepthOffset;
	}
	return nullptr;
}

FString MakeGeneratedMaterialDescription(const FMaterialCodeSpec& Spec)
{
	return FString::Printf(
		TEXT("YogArt %s JSON/HLSL | Generator=%llu | Fingerprint=%s"),
		*Spec.MaterialKind,
		static_cast<unsigned long long>(CodeMaterialGeneratorVersion),
		*Spec.GenerationFingerprint);
}

bool IsGeneratedMaterialUpToDate(const UMaterial* Material, const FMaterialCodeSpec& Spec)
{
	if (!Material)
	{
		return false;
	}

	int32 CustomExpressionCount = 0;
	bool bFingerprintMatches = false;
	for (UMaterialExpression* Expression : Material->GetExpressions())
	{
		if (const UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression))
		{
			++CustomExpressionCount;
			bFingerprintMatches |= Custom->Description == MakeGeneratedMaterialDescription(Spec);
		}
	}
	return CustomExpressionCount == 1 && bFingerprintMatches;
}

bool ValidateGroundRvtActivePermutation(
	UMaterial* Material,
	const FMaterialCodeSpec& Spec,
	FString& OutError)
{
	IConsoleVariable* MaterialQualityCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.MaterialQualityLevel"));
	if (!MaterialQualityCVar)
	{
		OutError = TEXT("could not find r.MaterialQualityLevel for all-quality material validation.");
		return false;
	}

	const int32 OriginalQuality = MaterialQualityCVar->GetInt();
	bool bValid = true;
	for (int32 QualityIndex = 0; QualityIndex < EMaterialQualityLevel::Num && bValid; ++QualityIndex)
	{
		const EMaterialQualityLevel::Type QualityLevel =
			static_cast<EMaterialQualityLevel::Type>(QualityIndex);
		MaterialQualityCVar->ReplaceCurrentPriorityAndTag(QualityIndex);
		IConsoleManager::Get().CallAllConsoleVariableSinks();
		if (GetCachedScalabilityCVars().MaterialQualityLevel != QualityLevel)
		{
			OutError = FString::Printf(
				TEXT("could not activate %s material quality for validation."),
				*LexToString(QualityLevel));
			bValid = false;
			break;
		}

		for (int32 GroundRvtValue = 0; GroundRvtValue <= 1 && bValid; ++GroundRvtValue)
		{
			const bool bUseGroundRvt = GroundRvtValue != 0;
			TStrongObjectPtr<UMaterialInstanceConstant> ValidationInstance(
				NewObject<UMaterialInstanceConstant>(
					GetTransientPackage(),
					NAME_None,
					RF_Transient));
			if (!ValidationInstance.IsValid())
			{
				OutError = TEXT("could not create the transient ground-RVT permutation validator.");
				bValid = false;
				break;
			}

			ValidationInstance->SetParentEditorOnly(Material, false);
			ValidationInstance->SetStaticSwitchParameterValueEditorOnly(
				FMaterialParameterInfo(FName(*Spec.GroundRvt.StaticSwitch)),
				bUseGroundRvt);
			// UpdateStaticPermutation caches the resource for the temporarily active quality.
			ValidationInstance->UpdateStaticPermutation();
			FMaterialResource* MaterialResource =
				ValidationInstance->GetMaterialResource(
					GMaxRHIShaderPlatform,
					QualityLevel);
			if (!MaterialResource)
			{
				OutError = FString::Printf(
					TEXT("material `%s` has no transient UseGroundRVT=%s resource for %s quality."),
					*ToObjectPath(Spec.AssetPath),
					bUseGroundRvt ? TEXT("true") : TEXT("false"),
					*LexToString(QualityLevel));
				bValid = false;
				break;
			}

			MaterialResource->FinishCompilation();
			if (!MaterialResource->GetCompileErrors().IsEmpty())
			{
				OutError = FString::Printf(
					TEXT("material `%s` failed UseGroundRVT=%s/%s shader compilation:\n  %s"),
					*ToObjectPath(Spec.AssetPath),
					bUseGroundRvt ? TEXT("true") : TEXT("false"),
					*LexToString(QualityLevel),
					*FString::Join(
						MaterialResource->GetCompileErrors(),
						TEXT("\n  ")));
				bValid = false;
				break;
			}
			UE_LOG(
				LogYogArtCodeMaterialSetup,
				Display,
				TEXT("Validated `%s` with UseGroundRVT=%s at %s quality (transient permutation compiled with no errors)."),
				*ToObjectPath(Spec.AssetPath),
				bUseGroundRvt ? TEXT("true") : TEXT("false"),
				*LexToString(QualityLevel));
		}
	}

	MaterialQualityCVar->ReplaceCurrentPriorityAndTag(OriginalQuality);
	IConsoleManager::Get().CallAllConsoleVariableSinks();
	return bValid;
}

bool BuildMaterialFromSpec(
	const FMaterialCodeSpec& Spec,
	bool bForceRebuild,
	TArray<UPackage*>& DirtyPackages,
	bool& bOutRebuilt,
	FString& OutError)
{
	bOutRebuilt = false;
	const FString ObjectPath = ToObjectPath(Spec.AssetPath);
	UObject* ExistingObject = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn);
	UMaterial* Material = Cast<UMaterial>(ExistingObject);
	if (ExistingObject && !Material)
	{
		OutError = FString::Printf(TEXT("target `%s` exists but is not a UMaterial."), *ObjectPath);
		return false;
	}

	UPackage* Package = Material ? Material->GetOutermost() : CreatePackage(*Spec.AssetPath);
	if (!Package)
	{
		OutError = FString::Printf(TEXT("could not create or load package `%s`."), *Spec.AssetPath);
		return false;
	}

	if (!Material)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(Spec.AssetPath);
		Material = NewObject<UMaterial>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Material);
	}
	if (!Material || !Material->GetEditorOnlyData())
	{
		OutError = FString::Printf(TEXT("material editor-only data is unavailable for `%s`."), *ObjectPath);
		return false;
	}
	if (!bForceRebuild && IsGeneratedMaterialUpToDate(Material, Spec))
	{
		if (!ValidateGroundRvtActivePermutation(Material, Spec, OutError))
		{
			return false;
		}
		return true;
	}

	ResetSurfaceMaterial(Material);

	TMap<FString, UMaterialExpression*> InputExpressions;
	int32 InputNodeY = -800;
	for (const FCodeInputSpec& InputSpec : Spec.Inputs)
	{
		UMaterialExpression* Expression = nullptr;
		if (InputSpec.Type == ECodeInputType::Texture2D)
		{
			UTexture2D* DefaultTexture = LoadObject<UTexture2D>(nullptr, *InputSpec.DefaultAsset);
			if (!DefaultTexture)
			{
				OutError = FString::Printf(
					TEXT("Texture2D `%s` disappeared after validation."),
					*InputSpec.DefaultAsset);
				return false;
			}

			UMaterialExpressionTextureObjectParameter* TextureParameter =
				AddExpression<UMaterialExpressionTextureObjectParameter>(Material, -1400, InputNodeY);
			if (!TextureParameter)
			{
				OutError = TEXT("could not create TextureObject parameter.");
				return false;
			}
			InitializeNamedParameter(TextureParameter, InputSpec.Name, Spec.AssetPath);
			TextureParameter->Group = FName(*InputSpec.Group);
			TextureParameter->SortPriority = InputSpec.SortPriority;
			TextureParameter->Texture = DefaultTexture;
			TextureParameter->SamplerType = InputSpec.SamplerType;
			Expression = TextureParameter;
		}
		else if (InputSpec.Type == ECodeInputType::Scalar)
		{
			UMaterialExpressionScalarParameter* Scalar =
				AddExpression<UMaterialExpressionScalarParameter>(Material, -1400, InputNodeY);
			if (!Scalar)
			{
				OutError = TEXT("could not create Scalar parameter.");
				return false;
			}
			InitializeParameter(Scalar, InputSpec, Spec.AssetPath);
			Scalar->DefaultValue = static_cast<float>(InputSpec.ScalarDefault);
			if (InputSpec.bHasMin && InputSpec.bHasMax)
			{
				Scalar->SliderMin = static_cast<float>(InputSpec.Min);
				Scalar->SliderMax = static_cast<float>(InputSpec.Max);
			}
			if (InputSpec.bHasPerInstanceDataIndex)
			{
				UMaterialExpressionPerInstanceCustomData* PerInstanceData =
					AddExpression<UMaterialExpressionPerInstanceCustomData>(Material, -1080, InputNodeY);
				if (!PerInstanceData)
				{
					OutError = TEXT("could not create PerInstanceCustomData expression.");
					return false;
				}
				PerInstanceData->DataIndex = static_cast<uint32>(InputSpec.PerInstanceDataIndex);
				PerInstanceData->ConstDefaultValue = static_cast<float>(InputSpec.ScalarDefault);
				PerInstanceData->DefaultValue.Connect(0, Scalar);
				Expression = PerInstanceData;
			}
			else
			{
				Expression = Scalar;
			}
		}
		else
		{
			UMaterialExpressionVectorParameter* Vector =
				AddExpression<UMaterialExpressionVectorParameter>(Material, -1400, InputNodeY);
			if (!Vector)
			{
				OutError = TEXT("could not create Vector parameter.");
				return false;
			}
			InitializeParameter(Vector, InputSpec, Spec.AssetPath);
			Vector->DefaultValue = InputSpec.VectorDefault;
			Expression = Vector;
		}

		InputExpressions.Add(InputSpec.Name, Expression);
		InputNodeY += 145;
	}

	TArray<UMaterialExpressionTextureCollectionParameter*> CollectionExpressions;
	int32 CollectionNodeY = -600;
	for (int32 CollectionIndex = 0; CollectionIndex < Spec.Collections.Num(); ++CollectionIndex)
	{
		const FCollectionSpec& CollectionSpec = Spec.Collections[CollectionIndex];
		UTextureCollection* DefaultCollection =
			LoadObject<UTextureCollection>(nullptr, *CollectionSpec.DefaultAsset);
		if (!DefaultCollection)
		{
			OutError = FString::Printf(
				TEXT("TextureCollection `%s` disappeared after validation."),
				*CollectionSpec.DefaultAsset);
			return false;
		}

		UMaterialExpressionTextureCollectionParameter* Collection =
			AddExpression<UMaterialExpressionTextureCollectionParameter>(Material, -900, CollectionNodeY);
		if (!Collection)
		{
			OutError = TEXT("could not create TextureCollection parameter.");
			return false;
		}
		InitializeNamedParameter(Collection, CollectionSpec.Parameter, Spec.AssetPath);
		Collection->Group = TEXT("Texture Backend");
		Collection->SortPriority = CollectionIndex;
		Collection->TextureCollection = DefaultCollection;
		CollectionExpressions.Add(Collection);
		CollectionNodeY += 180;
	}

	UMaterialExpressionStaticBoolParameter* CollectionBool =
		AddExpression<UMaterialExpressionStaticBoolParameter>(Material, -900, 80);
	UMaterialExpressionConstant* ConstantOne =
		AddExpression<UMaterialExpressionConstant>(Material, -900, 240);
	UMaterialExpressionConstant* ConstantZero =
		AddExpression<UMaterialExpressionConstant>(Material, -900, 340);
	UMaterialExpressionStaticSwitch* CollectionSwitch =
		AddExpression<UMaterialExpressionStaticSwitch>(Material, -620, 180);
	if (!CollectionBool || !ConstantOne || !ConstantZero || !CollectionSwitch)
	{
		OutError = TEXT("could not create TextureCollection static-switch contract.");
		return false;
	}

	InitializeNamedParameter(CollectionBool, Spec.TextureCollectionSwitch, Spec.AssetPath);
	CollectionBool->Group = TEXT("Texture Backend");
	CollectionBool->SortPriority = 0;
	CollectionBool->DefaultValue = Spec.bTextureCollectionDefault;
	CollectionBool->DynamicBranch = 0;
	ConstantOne->R = 1.0f;
	ConstantZero->R = 0.0f;
	CollectionSwitch->DefaultValue = Spec.bTextureCollectionDefault;
	CollectionSwitch->A.Connect(0, ConstantOne);
	CollectionSwitch->B.Connect(0, ConstantZero);
	CollectionSwitch->Value.Connect(0, CollectionBool);

	TMap<FString, UMaterialExpressionQualitySwitch*> QualityExpressions;
	int32 QualityNodeY = -650;
	for (const FQualityScalarSpec& QualitySpec : Spec.QualityInputs)
	{
		UMaterialExpressionQualitySwitch* QualityExpression =
			AddQualityScalar(Material, QualitySpec, -360, QualityNodeY);
		if (!QualityExpression)
		{
			OutError = FString::Printf(
				TEXT("could not create quality scalar `%s`."),
				*QualitySpec.Name);
			return false;
		}
		QualityExpressions.Add(QualitySpec.Name, QualityExpression);
		QualityNodeY += 380;
	}

	URuntimeVirtualTexture* GroundSurfaceRvt =
		LoadObject<URuntimeVirtualTexture>(nullptr, *Spec.GroundRvt.SurfaceDefaultAsset);
	URuntimeVirtualTexture* GroundHeightRvt =
		LoadObject<URuntimeVirtualTexture>(nullptr, *Spec.GroundRvt.HeightDefaultAsset);
	if (!GroundSurfaceRvt || !GroundHeightRvt)
	{
		OutError = TEXT("ground RVT support assets disappeared after validation/creation.");
		return false;
	}

	UMaterialExpressionRuntimeVirtualTextureSampleParameter* GroundSurfaceSample =
		AddExpression<UMaterialExpressionRuntimeVirtualTextureSampleParameter>(Material, -1360, 1120);
	UMaterialExpressionRuntimeVirtualTextureSampleParameter* GroundHeightSample =
		AddExpression<UMaterialExpressionRuntimeVirtualTextureSampleParameter>(Material, -1360, 1360);
	UMaterialExpressionStaticBoolParameter* GroundRvtBool =
		AddExpression<UMaterialExpressionStaticBoolParameter>(Material, -1360, 1600);
	UMaterialExpressionConstant3Vector* GroundZeroVector =
		AddExpression<UMaterialExpressionConstant3Vector>(Material, -1120, 1790);
	UMaterialExpressionConstant3Vector* GroundDefaultNormal =
		AddExpression<UMaterialExpressionConstant3Vector>(Material, -1120, 1900);
	UMaterialExpressionConstant* GroundZeroScalar =
		AddScalarConstant(Material, 0.0f, -1120, 2010);
	UMaterialExpressionConstant* GroundOneScalar =
		AddScalarConstant(Material, 1.0f, -1120, 2120);
	if (!GroundSurfaceSample
		|| !GroundHeightSample
		|| !GroundRvtBool
		|| !GroundZeroVector
		|| !GroundDefaultNormal
		|| !GroundZeroScalar
		|| !GroundOneScalar)
	{
		OutError = TEXT("could not create ground RVT expressions.");
		return false;
	}

	InitializeNamedParameter(GroundSurfaceSample, Spec.GroundRvt.SurfaceParameter, Spec.AssetPath);
	GroundSurfaceSample->Group = TEXT("Ground RVT");
	GroundSurfaceSample->SortPriority = 0;
	GroundSurfaceSample->VirtualTexture = GroundSurfaceRvt;
	GroundSurfaceSample->InitVirtualTextureDependentSettings();

	InitializeNamedParameter(GroundHeightSample, Spec.GroundRvt.HeightParameter, Spec.AssetPath);
	GroundHeightSample->Group = TEXT("Ground RVT");
	GroundHeightSample->SortPriority = 1;
	GroundHeightSample->VirtualTexture = GroundHeightRvt;
	GroundHeightSample->InitVirtualTextureDependentSettings();

	InitializeNamedParameter(GroundRvtBool, Spec.GroundRvt.StaticSwitch, Spec.AssetPath);
	GroundRvtBool->Group = TEXT("Ground RVT");
	GroundRvtBool->SortPriority = 2;
	GroundRvtBool->DefaultValue = Spec.GroundRvt.bStaticDefault;
	GroundRvtBool->DynamicBranch = 0;
	GroundZeroVector->Constant = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GroundDefaultNormal->Constant = FLinearColor(0.0f, 0.0f, 1.0f, 0.0f);

	UMaterialExpressionQualitySwitch* GroundBaseColorQuality =
		AddQualityGate(
			Material,
			GroundSurfaceSample,
			0,
			GroundZeroVector,
			Spec.GroundRvt.QualityEnabled,
			-820,
			1120);
	UMaterialExpressionQualitySwitch* GroundRoughnessQuality =
		AddQualityGate(
			Material,
			GroundSurfaceSample,
			2,
			GroundZeroScalar,
			Spec.GroundRvt.QualityEnabled,
			-820,
			1260);
	UMaterialExpressionQualitySwitch* GroundSpecularQuality =
		AddQualityGate(
			Material,
			GroundSurfaceSample,
			1,
			GroundZeroScalar,
			Spec.GroundRvt.QualityEnabled,
			-820,
			1330);
	UMaterialExpressionQualitySwitch* GroundNormalQuality =
		AddQualityGate(
			Material,
			GroundSurfaceSample,
			3,
			GroundDefaultNormal,
			Spec.GroundRvt.QualityEnabled,
			-820,
			1400);
	UMaterialExpressionQualitySwitch* GroundHeightQuality =
		AddQualityGate(
			Material,
			GroundHeightSample,
			4,
			GroundZeroScalar,
			Spec.GroundRvt.QualityEnabled,
			-820,
			1540);
	UMaterialExpressionQualitySwitch* GroundEnabledQuality =
		AddQualityGate(
			Material,
			GroundOneScalar,
			0,
			GroundZeroScalar,
			Spec.GroundRvt.QualityEnabled,
			-820,
			1680);
	if (!GroundBaseColorQuality
		|| !GroundRoughnessQuality
		|| !GroundSpecularQuality
		|| !GroundNormalQuality
		|| !GroundHeightQuality
		|| !GroundEnabledQuality)
	{
		OutError = TEXT("could not create ground RVT quality gates.");
		return false;
	}

	UMaterialExpressionStaticSwitch* GroundBaseColorGate =
		AddStaticGate(
			Material,
			GroundBaseColorQuality,
			GroundZeroVector,
			GroundRvtBool,
			Spec.GroundRvt.bStaticDefault,
			-520,
			1120);
	UMaterialExpressionStaticSwitch* GroundRoughnessGate =
		AddStaticGate(
			Material,
			GroundRoughnessQuality,
			GroundZeroScalar,
			GroundRvtBool,
			Spec.GroundRvt.bStaticDefault,
			-520,
			1260);
	UMaterialExpressionStaticSwitch* GroundSpecularGate =
		AddStaticGate(
			Material,
			GroundSpecularQuality,
			GroundZeroScalar,
			GroundRvtBool,
			Spec.GroundRvt.bStaticDefault,
			-520,
			1330);
	UMaterialExpressionStaticSwitch* GroundNormalGate =
		AddStaticGate(
			Material,
			GroundNormalQuality,
			GroundDefaultNormal,
			GroundRvtBool,
			Spec.GroundRvt.bStaticDefault,
			-520,
			1400);
	UMaterialExpressionStaticSwitch* GroundHeightGate =
		AddStaticGate(
			Material,
			GroundHeightQuality,
			GroundZeroScalar,
			GroundRvtBool,
			Spec.GroundRvt.bStaticDefault,
			-520,
			1540);
	UMaterialExpressionStaticSwitch* GroundEnabledGate =
		AddStaticGate(
			Material,
			GroundEnabledQuality,
			GroundZeroScalar,
			GroundRvtBool,
			Spec.GroundRvt.bStaticDefault,
			-520,
			1680);
	if (!GroundBaseColorGate
		|| !GroundRoughnessGate
		|| !GroundSpecularGate
		|| !GroundNormalGate
		|| !GroundHeightGate
		|| !GroundEnabledGate)
	{
		OutError = TEXT("could not create ground RVT static gates.");
		return false;
	}

	UMaterialExpressionTransform* GroundNormalTs =
		AddExpression<UMaterialExpressionTransform>(Material, -220, 1400);
	if (!GroundNormalTs)
	{
		OutError = TEXT("could not create ground RVT normal transform.");
		return false;
	}
	GroundNormalTs->TransformSourceType = TRANSFORMSOURCE_World;
	GroundNormalTs->TransformType = TRANSFORM_Tangent;
	GroundNormalTs->Input.Connect(0, GroundNormalGate);

	UMaterialExpressionTextureCoordinate* Uv0 =
		AddExpression<UMaterialExpressionTextureCoordinate>(Material, -900, 560);
	UMaterialExpressionVertexColor* VertexColor =
		AddExpression<UMaterialExpressionVertexColor>(Material, -900, 680);
	UMaterialExpressionAppendVector* VertexColorRgba =
		AddExpression<UMaterialExpressionAppendVector>(Material, -620, 680);
	UMaterialExpressionCameraVectorWS* CameraVector =
		AddExpression<UMaterialExpressionCameraVectorWS>(Material, -900, 800);
	UMaterialExpressionTransform* ViewDirTs =
		AddExpression<UMaterialExpressionTransform>(Material, -620, 800);
	UMaterialExpressionPixelDepth* PixelDepth =
		AddExpression<UMaterialExpressionPixelDepth>(Material, -900, 940);
	UMaterialExpressionWorldPosition* AbsoluteWorldPosition =
		AddExpression<UMaterialExpressionWorldPosition>(Material, -900, 1040);
	if (!Uv0
		|| !VertexColor
		|| !VertexColorRgba
		|| !CameraVector
		|| !ViewDirTs
		|| !PixelDepth
		|| !AbsoluteWorldPosition)
	{
		OutError = TEXT("could not create required built-in material inputs.");
		return false;
	}
	Uv0->CoordinateIndex = 0;
	VertexColorRgba->A.Connect(0, VertexColor);
	VertexColorRgba->B.Connect(4, VertexColor);
	ViewDirTs->TransformSourceType = TRANSFORMSOURCE_World;
	ViewDirTs->TransformType = TRANSFORM_Tangent;
	ViewDirTs->Input.Connect(0, CameraVector);
	AbsoluteWorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;

	UMaterialExpressionCustom* Custom =
		AddExpression<UMaterialExpressionCustom>(Material, 100, 0);
	if (!Custom)
	{
		OutError = TEXT("could not create Custom expression.");
		return false;
	}

	Custom->Description = MakeGeneratedMaterialDescription(Spec);
	Custom->OutputType = Spec.MainOutput.Type;
	Custom->IncludeFilePaths.Add(Spec.ShaderInclude);
	Custom->ContainsClipInstruction = CMCI_No;

	TArray<FString> CallArguments;
	for (const FCodeInputSpec& InputSpec : Spec.Inputs)
	{
		if (!InputSpec.bCustomInput)
		{
			continue;
		}

		UMaterialExpression* const* Expression = InputExpressions.Find(InputSpec.Name);
		if (!Expression || !*Expression)
		{
			OutError = FString::Printf(TEXT("missing expression for input `%s`."), *InputSpec.Name);
			return false;
		}
		// VectorParameter output 0 is RGB; output 5 is the explicit RGBA pin.
		// The JSON contract stores four components, so never silently discard A.
		const int32 OutputIndex = InputSpec.Type == ECodeInputType::Vector ? 5 : 0;
		AppendCustomInput(Custom, InputSpec.Name, *Expression, OutputIndex);
		CallArguments.Add(InputSpec.Name);
		if (InputSpec.Type == ECodeInputType::Texture2D)
		{
			CallArguments.Add(InputSpec.Sampler);
		}
	}

	for (int32 CollectionIndex = 0; CollectionIndex < Spec.Collections.Num(); ++CollectionIndex)
	{
		const FString& CollectionName = Spec.Collections[CollectionIndex].Parameter;
		AppendCustomInput(Custom, CollectionName, CollectionExpressions[CollectionIndex]);
		CallArguments.Add(CollectionName);
	}

	AppendCustomInput(Custom, Spec.TextureCollectionSwitch, CollectionSwitch);
	CallArguments.Add(Spec.TextureCollectionSwitch);

	for (const FQualityScalarSpec& QualitySpec : Spec.QualityInputs)
	{
		UMaterialExpressionQualitySwitch* const* QualityExpression =
			QualityExpressions.Find(QualitySpec.Name);
		if (!QualityExpression || !*QualityExpression)
		{
			OutError = FString::Printf(TEXT("missing quality expression `%s`."), *QualitySpec.Name);
			return false;
		}
		AppendCustomInput(Custom, QualitySpec.Name, *QualityExpression);
		CallArguments.Add(QualitySpec.Name);
	}

	AppendCustomInput(Custom, TEXT("GroundRVTBaseColor"), GroundBaseColorGate);
	CallArguments.Add(TEXT("GroundRVTBaseColor"));
	AppendCustomInput(Custom, TEXT("GroundRVTRoughness"), GroundRoughnessGate);
	CallArguments.Add(TEXT("GroundRVTRoughness"));
	AppendCustomInput(Custom, TEXT("GroundRVTSpecular"), GroundSpecularGate);
	CallArguments.Add(TEXT("GroundRVTSpecular"));
	AppendCustomInput(Custom, TEXT("GroundRVTNormalTS"), GroundNormalTs);
	CallArguments.Add(TEXT("GroundRVTNormalTS"));
	AppendCustomInput(Custom, TEXT("GroundRVTWorldHeight"), GroundHeightGate);
	CallArguments.Add(TEXT("GroundRVTWorldHeight"));
	AppendCustomInput(Custom, TEXT("GroundRVTEnabled"), GroundEnabledGate);
	CallArguments.Add(TEXT("GroundRVTEnabled"));

	AppendCustomInput(Custom, TEXT("UV0"), Uv0);
	CallArguments.Add(TEXT("UV0"));
	AppendCustomInput(Custom, TEXT("VertexColor"), VertexColorRgba);
	CallArguments.Add(TEXT("VertexColor"));
	AppendCustomInput(Custom, TEXT("ViewDirTS"), ViewDirTs);
	CallArguments.Add(TEXT("ViewDirTS"));
	AppendCustomInput(Custom, TEXT("PixelDepth"), PixelDepth);
	CallArguments.Add(TEXT("PixelDepth"));
	AppendCustomInput(Custom, TEXT("AbsoluteWorldPosition"), AbsoluteWorldPosition);
	CallArguments.Add(TEXT("AbsoluteWorldPosition"));

	for (const FOutputSpec& Output : Spec.AdditionalOutputs)
	{
		FCustomOutput& CustomOutput = Custom->AdditionalOutputs.AddDefaulted_GetRef();
		CustomOutput.OutputName = FName(*Output.Name);
		CustomOutput.OutputType = Output.Type;
		CallArguments.Add(Output.Name);
	}

	Custom->Code = FString::Printf(
		TEXT("return %s(\n\t%s\n);"),
		*Spec.EntryPoint,
		*FString::Join(CallArguments, TEXT(",\n\t")));
	Custom->RebuildOutputs();

	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	FExpressionInput* MainInput = ResolveMaterialPropertyInput(Data, Spec.MainOutput.MaterialProperty);
	if (!MainInput)
	{
		OutError = FString::Printf(
			TEXT("could not resolve main material property `%s`."),
			*Spec.MainOutput.MaterialProperty);
		return false;
	}
	MainInput->Connect(0, Custom);

	for (int32 OutputIndex = 0; OutputIndex < Spec.AdditionalOutputs.Num(); ++OutputIndex)
	{
		const FOutputSpec& Output = Spec.AdditionalOutputs[OutputIndex];
		if (Output.MaterialProperty == TEXT("None"))
		{
			continue;
		}

		FExpressionInput* MaterialInput = ResolveMaterialPropertyInput(Data, Output.MaterialProperty);
		if (!MaterialInput)
		{
			OutError = FString::Printf(
				TEXT("could not resolve material property `%s`."),
				*Output.MaterialProperty);
			return false;
		}
		MaterialInput->Connect(OutputIndex + 1, Custom);
	}

	Material->SetUsageByFlag(MATUSAGE_InstancedStaticMeshes, true);
	Material->PostEditChange();
	Material->ForceRecompileForRendering();

	FMaterialResource* MaterialResource = Material->GetMaterialResource(GMaxRHIShaderPlatform);
	if (!MaterialResource)
	{
		OutError = FString::Printf(
			TEXT("material `%s` has no resource for shader platform %d."),
			*ObjectPath,
			static_cast<int32>(GMaxRHIShaderPlatform));
		return false;
	}

	MaterialResource->FinishCompilation();
	if (!MaterialResource->GetCompileErrors().IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("material `%s` failed shader compilation:\n  %s"),
			*ObjectPath,
			*FString::Join(MaterialResource->GetCompileErrors(), TEXT("\n  ")));
		return false;
	}
	if (!MaterialResource->HasValidGameThreadShaderMap())
	{
		OutError = FString::Printf(
			TEXT("material `%s` produced no valid shader map for platform %d."),
			*ObjectPath,
			static_cast<int32>(GMaxRHIShaderPlatform));
		return false;
	}
	if (!ValidateGroundRvtActivePermutation(Material, Spec, OutError))
	{
		return false;
	}

	Material->MarkPackageDirty();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	bOutRebuilt = true;
	return true;
}

bool ParseTarget(const FString& TargetValue, ECodeMaterialTarget& OutTarget)
{
	if (TargetValue.Equals(TEXT("Building"), ESearchCase::IgnoreCase))
	{
		OutTarget = ECodeMaterialTarget::Building;
		return true;
	}
	if (TargetValue.Equals(TEXT("Prop"), ESearchCase::IgnoreCase))
	{
		OutTarget = ECodeMaterialTarget::Prop;
		return true;
	}
	if (TargetValue.Equals(TEXT("All"), ESearchCase::IgnoreCase))
	{
		OutTarget = ECodeMaterialTarget::All;
		return true;
	}
	return false;
}

bool IncludesTarget(ECodeMaterialTarget Requested, ECodeMaterialTarget Candidate)
{
	return Requested == ECodeMaterialTarget::All || Requested == Candidate;
}
}

UYogArtCodeMaterialSetupCommandlet::UYogArtCodeMaterialSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UYogArtCodeMaterialSetupCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bNoP4 = FParse::Param(*Params, TEXT("nop4"));
	const bool bForceRebuild = FParse::Param(*Params, TEXT("Force"));
	FString TargetValue;
	const bool bHasTarget = FParse::Value(*Params, TEXT("Target="), TargetValue);
	if (bApply && !IsAllowCommandletRendering())
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("Apply requires -AllowCommandletRendering so every enabled shader permutation can be compiled before save. No assets were changed."));
		return 1;
	}

	ECodeMaterialTarget Target = ECodeMaterialTarget::All;
	if (bApply && !bHasTarget)
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("Apply requires an explicit -Target=Building|Prop|All. No assets were changed."));
		return 1;
	}
	if (bHasTarget && !ParseTarget(TargetValue, Target))
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("Invalid Target `%s`; expected Building, Prop, or All. No assets were changed."),
			*TargetValue);
		return 1;
	}

	const FString BuildingJsonPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), BuildingJsonRelativePath));
	const FString PropJsonPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), PropJsonRelativePath));

	FMaterialCodeSpec BuildingSpec;
	FMaterialCodeSpec PropSpec;
	TArray<FString> ValidationErrors;
	const bool bBuildingValid = LoadAndValidateSpec(
		BuildingJsonPath,
		BuildingKind,
		BuildingAssetPath,
		BuildingSpec,
		ValidationErrors);
	const bool bPropValid = LoadAndValidateSpec(
		PropJsonPath,
		PropKind,
		PropAssetPath,
		PropSpec,
		ValidationErrors);

	if (!bBuildingValid || !bPropValid || !ValidationErrors.IsEmpty())
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("JSON validation failed; both specifications must pass before any asset can be changed."));
		for (const FString& Error : ValidationErrors)
		{
			UE_LOG(LogYogArtCodeMaterialSetup, Error, TEXT("  %s"), *Error);
		}
		return 1;
	}

	UE_LOG(
		LogYogArtCodeMaterialSetup,
		Display,
		TEXT("Validated Building (%d inputs, %d collections) and Prop (%d inputs, %d collections)."),
		BuildingSpec.Inputs.Num(),
		BuildingSpec.Collections.Num(),
		PropSpec.Inputs.Num(),
		PropSpec.Collections.Num());
	UE_LOG(
		LogYogArtCodeMaterialSetup,
		Warning,
		TEXT("This contract requires Shader Model 6 with Bindless Resources set to at least Minimal. ")
		TEXT("UseTextureCollection selects the algorithm path; it is not a non-bindless compiler fallback."));

	if (!bApply)
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Display,
			TEXT("Dry-run complete. No packages were created, rebuilt, dirtied, or saved. ")
			TEXT("Use -Apply -Target=Building|Prop|All to write explicitly."));
		return 0;
	}
	if (!bNoP4)
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("Apply requires -nop4. Pre-open the selected assets in the intended P4 changelist; ")
			TEXT("this commandlet will never request checkout or create source-control state."));
		return 1;
	}

	TArray<FString> PreflightErrors;
	if (IncludesTarget(Target, ECodeMaterialTarget::Building))
	{
		FString Error;
		if (!ValidateTargetPackageWritable(BuildingSpec, Error))
		{
			PreflightErrors.Add(MoveTemp(Error));
		}
	}
	if (IncludesTarget(Target, ECodeMaterialTarget::Prop))
	{
		FString Error;
		if (!ValidateTargetPackageWritable(PropSpec, Error))
		{
			PreflightErrors.Add(MoveTemp(Error));
		}
	}
	{
		FString Error;
		if (!ValidatePackagePathWritable(GroundHeightRvtAssetPath, Error))
		{
			PreflightErrors.Add(MoveTemp(Error));
		}
	}
	{
		FString Error;
		if (!ValidatePackagePathWritable(GroundMaterialAssetPath, Error))
		{
			PreflightErrors.Add(MoveTemp(Error));
		}
	}
	if (!PreflightErrors.IsEmpty())
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("Apply preflight failed before any material was rebuilt."));
		for (const FString& Error : PreflightErrors)
		{
			UE_LOG(LogYogArtCodeMaterialSetup, Error, TEXT("  %s"), *Error);
		}
		return 1;
	}

	TArray<UPackage*> DirtyPackages;
	{
		FString Error;
		if (!EnsureGroundHeightRvt(DirtyPackages, Error))
		{
			UE_LOG(LogYogArtCodeMaterialSetup, Error, TEXT("Ground RVT support setup failed: %s"), *Error);
			return 1;
		}
	}
	{
		FString Error;
		if (!PatchGroundRvtWriter(DirtyPackages, Error))
		{
			UE_LOG(LogYogArtCodeMaterialSetup, Error, TEXT("Ground RVT writer setup failed: %s"), *Error);
			return 1;
		}
	}
	bool bBuiltAll = true;
	int32 SelectedCount = 0;
	int32 UpToDateCount = 0;
	if (IncludesTarget(Target, ECodeMaterialTarget::Building))
	{
		++SelectedCount;
		FString Error;
		bool bRebuilt = false;
		if (!BuildMaterialFromSpec(BuildingSpec, bForceRebuild, DirtyPackages, bRebuilt, Error))
		{
			UE_LOG(LogYogArtCodeMaterialSetup, Error, TEXT("Building rebuild failed: %s"), *Error);
			bBuiltAll = false;
		}
		else if (!bRebuilt)
		{
			++UpToDateCount;
		}
	}
	if (IncludesTarget(Target, ECodeMaterialTarget::Prop))
	{
		++SelectedCount;
		FString Error;
		bool bRebuilt = false;
		if (!BuildMaterialFromSpec(PropSpec, bForceRebuild, DirtyPackages, bRebuilt, Error))
		{
			UE_LOG(LogYogArtCodeMaterialSetup, Error, TEXT("Prop rebuild failed: %s"), *Error);
			bBuiltAll = false;
		}
		else if (!bRebuilt)
		{
			++UpToDateCount;
		}
	}

	if (!bBuiltAll)
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("At least one selected material failed to rebuild; no package save was requested."));
		return 1;
	}
	if (DirtyPackages.IsEmpty())
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Display,
			TEXT("All %d selected code-material package(s) already match their generation fingerprints; ")
			TEXT("no package was rebuilt or saved. Use -Force only when intentionally regenerating unchanged inputs."),
			SelectedCount);
		return 0;
	}
	if (!UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, true))
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Error,
			TEXT("Failed to save one or more material packages. Check Perforce/read-only state before retrying."));
		return 1;
	}

	UE_LOG(
		LogYogArtCodeMaterialSetup,
		Display,
		TEXT("Saved %d code-material/support package(s)."),
		DirtyPackages.Num());
	if (UpToDateCount > 0)
	{
		UE_LOG(
			LogYogArtCodeMaterialSetup,
			Display,
			TEXT("%d additional selected package(s) already matched their generation fingerprints."),
			UpToDateCount);
	}
	return 0;
}
