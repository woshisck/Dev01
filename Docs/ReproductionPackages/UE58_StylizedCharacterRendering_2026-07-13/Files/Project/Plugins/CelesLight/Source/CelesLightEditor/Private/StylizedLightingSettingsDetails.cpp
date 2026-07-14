#include "StylizedLightingSettingsDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "StylizedLightingSettings.h"

namespace
{
	struct FLocalizedPropertyText
	{
		FName PropertyName;
		const TCHAR* EnglishName;
		const TCHAR* ChineseName;
		const TCHAR* EnglishDescription;
		const TCHAR* ChineseDescription;
	};

	FText SelectText(bool bChinese, const TCHAR* English, const TCHAR* Chinese)
	{
		return FText::FromString(bChinese ? Chinese : English);
	}

	bool ShouldUseChinese(const UStylizedLightingSettings* Settings)
	{
		if (Settings)
		{
			if (Settings->EditorLanguage == EStylizedLightingEditorLanguage::SimplifiedChinese)
			{
				return true;
			}
			if (Settings->EditorLanguage == EStylizedLightingEditorLanguage::English)
			{
				return false;
			}
		}

		return FInternationalization::Get().GetCurrentCulture()->GetName().StartsWith(TEXT("zh"));
	}

	void ApplyText(const TSharedPtr<IPropertyHandle>& Handle, const FLocalizedPropertyText& Text, bool bChinese)
	{
		if (!Handle.IsValid() || !Handle->IsValidHandle())
		{
			return;
		}

		Handle->SetPropertyDisplayName(SelectText(bChinese, Text.EnglishName, Text.ChineseName));
		Handle->SetToolTipText(SelectText(bChinese, Text.EnglishDescription, Text.ChineseDescription));
	}

	void SetReflectionModeNames(bool bChinese)
	{
		if (UEnum* Enum = StaticEnum<EStylizedReflectionKuwaharaMode>())
		{
			Enum->SetMetaData(TEXT("DisplayName"), bChinese ? TEXT("自动（史诗与影视级开启）") : TEXT("Auto (Epic and Cinematic)"), Enum->GetIndexByValue((int64)EStylizedReflectionKuwaharaMode::Auto));
			Enum->SetMetaData(TEXT("DisplayName"), bChinese ? TEXT("关闭") : TEXT("Disabled"), Enum->GetIndexByValue((int64)EStylizedReflectionKuwaharaMode::Disabled));
			Enum->SetMetaData(TEXT("DisplayName"), bChinese ? TEXT("开启") : TEXT("Enabled"), Enum->GetIndexByValue((int64)EStylizedReflectionKuwaharaMode::Enabled));
		}
	}

	const FLocalizedPropertyText SettingsProperties[] =
	{
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, EditorLanguage), TEXT("Interface Language"), TEXT("界面语言"), TEXT("Controls only the language of this settings panel. Auto follows the Unreal Editor culture."), TEXT("只控制当前设置界面的显示语言，不影响渲染。自动模式跟随虚幻编辑器语言。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterRampAtlas), TEXT("Character Ramp Atlas"), TEXT("角色 Ramp 图集"), TEXT("Shared Curve Linear Color Atlas containing every attenuation ramp selected by the character lighting profiles."), TEXT("保存所有角色明暗 Ramp 的 Curve Linear Color Atlas。各光照配置选择的曲线必须包含在此图集中。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableStylizedLumenLighting), TEXT("Enable Stylized Lumen Lighting"), TEXT("启用风格化 Lumen 光照"), TEXT("Enables banded stylization for scene direct lighting and Lumen indirect lighting."), TEXT("为场景直接光和 Lumen 间接光启用分层风格化处理。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, BandCount), TEXT("Band Count"), TEXT("光照层数"), TEXT("Number of discrete lighting bands. Higher values retain more continuous shading."), TEXT("光照离散分层的数量。数值越高越接近连续光照，数值越低色块感越强。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, BandSoftness), TEXT("Band Softness"), TEXT("分层柔和度"), TEXT("Width of transitions between bands. Zero gives hard steps; higher values soften boundaries."), TEXT("相邻光照层之间的过渡宽度。0 为硬切，数值越高边界越柔和。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, GlossInfluence), TEXT("Gloss Influence"), TEXT("光泽度影响"), TEXT("Controls how strongly material glossiness shifts the band response."), TEXT("控制材质光泽度对光照分层位置和响应的影响程度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, DirectBlend), TEXT("Direct Light Blend"), TEXT("直接光混合"), TEXT("Blend from native direct lighting at zero to fully banded direct lighting at one."), TEXT("直接光的风格化混合量。0 使用原生直接光，1 使用完整分层结果。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, IndirectBlend), TEXT("Indirect Light Blend"), TEXT("间接光混合"), TEXT("Blend from native Lumen diffuse indirect lighting at zero to fully banded indirect lighting at one."), TEXT("Lumen 漫反射间接光的风格化混合量。0 使用原生结果，1 使用完整分层结果。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularIntensity), TEXT("Scene Specular Intensity"), TEXT("场景高光强度"), TEXT("Global multiplier for stylized scene specular highlights. Character profile specular has a separate control."), TEXT("风格化场景高光的全局倍率。角色高光由角色光照配置单独控制。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularOffset), TEXT("Scene Specular Offset"), TEXT("场景高光偏移"), TEXT("Offset added to N dot H before scene specular evaluation. Positive values broaden highlights; negative values narrow them."), TEXT("场景高光计算前加到 NdotH 的偏移。正值扩大高光，负值收窄高光。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, ReflectionKuwaharaMode), TEXT("Reflection Kuwahara Mode"), TEXT("反射 Kuwahara 模式"), TEXT("Auto enables the reflection-only Kuwahara filter at Epic and Cinematic quality; Enabled or Disabled overrides quality tiers."), TEXT("自动模式仅在史诗和影视级质量启用反射 Kuwahara 滤镜；开启或关闭可覆盖性能分级。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, ReflectionKuwaharaStrength), TEXT("Reflection Kuwahara Strength"), TEXT("反射色块化强度"), TEXT("Blend strength of the reflection-only Kuwahara filter."), TEXT("仅作用于角色环境反射的 Kuwahara 色块化混合强度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterHalfViewSelfShadow), TEXT("Enable Half-View Self Shadow"), TEXT("启用半程视角自阴影"), TEXT("Optional screen-space stylized character self shadow. Keep disabled when native character self projection has been removed."), TEXT("可选的屏幕空间角色风格化自阴影。当前已移除原生角色自投影时建议保持关闭。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterHalfViewShadowBlend), TEXT("Half-View Direction Blend"), TEXT("半程视角方向混合"), TEXT("Blends the shadow direction from the light direction toward the camera half-view direction."), TEXT("将自阴影方向从灯光方向混合到相机半程视角方向。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterSelfShadowStrength), TEXT("Self Shadow Strength"), TEXT("自阴影强度"), TEXT("Opacity of the optional screen-space character self shadow."), TEXT("可选屏幕空间角色自阴影的压暗强度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterSelfShadowMaxTraceDistance), TEXT("Self Shadow Max Trace Distance"), TEXT("自阴影最大追踪距离"), TEXT("Maximum world-space distance used by the optional character self-shadow trace."), TEXT("可选角色自阴影追踪允许使用的最大世界空间距离。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterDirectLightColorInfluence), TEXT("Direct Light Color Influence Master"), TEXT("直接光色影响总倍率"), TEXT("Global master multiplied by the selected profile's direct-light color influence."), TEXT("与当前角色光照配置中的直接光色影响相乘，用于全局控制角色接受直接光颜色的程度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterIndirectLightColorInfluence), TEXT("Indirect Light Color Influence Master"), TEXT("间接光色影响总倍率"), TEXT("Global master multiplied by the selected profile's indirect-light color influence."), TEXT("与当前角色光照配置中的间接光色影响相乘，用于全局控制角色接受环境光颜色的程度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterReflectionColorInfluence), TEXT("Reflection Color Influence Master"), TEXT("反射颜色影响总倍率"), TEXT("Global master multiplied by the selected profile's reflection color influence."), TEXT("与当前角色光照配置中的反射颜色影响相乘，用于全局控制角色接受环境反射颜色的程度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterLightingProfiles), TEXT("Character Lighting Profiles"), TEXT("角色光照配置"), TEXT("Up to eight reusable looks. A material selects profile 0 through 7 with its Lighting Profile input, and a camera volume may override it."), TEXT("最多 8 套可复用角色光照外观。材质通过 Lighting Profile 输入选择 0 到 7，后处理区域也可以覆盖选择。") },
	};

	const FLocalizedPropertyText ProfileProperties[] =
	{
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ProfileName), TEXT("Profile Name"), TEXT("配置名称"), TEXT("Artist-facing name for this reusable character lighting profile."), TEXT("用于识别这套可复用角色光照外观的名称。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ShadowFadeTint), TEXT("Shadow Fade Tint"), TEXT("阴影过渡色"), TEXT("Tint for the transition between the deepest shadow and shadow region."), TEXT("最深阴影与阴影区之间的过渡颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ShadowTint), TEXT("Shadow Tint"), TEXT("阴影色"), TEXT("Tint for the deepest shadow region."), TEXT("七区 Ramp 中最深阴影区域的颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ShallowFadeTint), TEXT("Shallow Fade Tint"), TEXT("浅阴影过渡色"), TEXT("Tint for the transition between shadow and shallow-shadow regions."), TEXT("阴影区与浅阴影区之间的过渡颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ShallowTint), TEXT("Shallow Tint"), TEXT("浅阴影色"), TEXT("Tint for the shallow-shadow region."), TEXT("七区 Ramp 中浅阴影区域的颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SSSTint), TEXT("SSS Tint"), TEXT("次表面过渡色"), TEXT("Tint for the soft subsurface-style transition region."), TEXT("模拟次表面散射的柔和过渡区域颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, FrontTint), TEXT("Front Tint"), TEXT("正面光色"), TEXT("Tint for the front-lit region."), TEXT("朝向灯光的正面受光区域颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ForwardTint), TEXT("Forward Tint"), TEXT("最亮区颜色"), TEXT("Tint for the strongest forward-facing light region; also used by rim-only lights."), TEXT("最强正向受光区域的颜色，同时作为仅边缘光模式的边缘光颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, BaseAttenuationRamp), TEXT("Base Attenuation Ramp"), TEXT("基础明暗 Ramp"), TEXT("Curve row used by the original seven-region attenuation. The curve must be included in Character Ramp Atlas."), TEXT("复刻原材质七区 Attenuation 使用的曲线。该曲线必须包含在角色 Ramp 图集中。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ShadowFadePower), TEXT("Shadow Fade Power"), TEXT("阴影过渡强度"), TEXT("Controls the original seven-region ramp's shadow transition shaping. Zero matches the original master material default."), TEXT("控制原七区 Ramp 的阴影过渡塑形。0 与原角色主材质默认值一致。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectDiffuseIntensity), TEXT("Direct Diffuse Intensity"), TEXT("直接漫反射强度"), TEXT("Multiplier for character direct diffuse lighting after the attenuation ramp."), TEXT("角色直接漫反射经过 Attenuation Ramp 后的强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SpecularTint), TEXT("Specular Tint"), TEXT("高光颜色"), TEXT("Color tint applied to character specular highlights."), TEXT("角色高光使用的颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SpecularIntensity), TEXT("Specular Intensity"), TEXT("高光强度"), TEXT("Per-profile multiplier for character direct specular highlights."), TEXT("当前角色配置的直接光高光强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectLightColorInfluence), TEXT("Direct Light Color Influence"), TEXT("直接光色影响"), TEXT("How much direct-light RGB affects the character. Zero keeps luminance only; one uses full light color."), TEXT("直接灯光颜色影响角色的程度。0 只接受亮度，1 完整接受灯光颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectLightColorInfluence), TEXT("Indirect Light Color Influence"), TEXT("间接光色影响"), TEXT("How much Lumen diffuse-indirect RGB affects the character. Zero keeps luminance only; one uses full environment color."), TEXT("Lumen 漫反射间接光颜色影响角色的程度。0 只接受亮度，1 完整接受环境光颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ReflectionColorInfluence), TEXT("Reflection Color Influence"), TEXT("反射颜色影响"), TEXT("How much environment-reflection RGB affects the character. Zero keeps luminance only; one uses full reflection color."), TEXT("环境反射颜色影响角色的程度。0 只接受亮度，1 完整接受反射颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, GINormalBlend), TEXT("GI Smooth Normal Blend"), TEXT("GI 平滑法线混合"), TEXT("Blends from the normal-mapped surface at zero to the encoded smooth vertex normal at one for GI occlusion and response."), TEXT("GI 遮蔽与响应使用的法线。0 使用法线贴图表面法线，1 使用材质输出自动编码的平滑顶点法线。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectLightingIntensity), TEXT("Indirect Lighting Intensity"), TEXT("间接光强度"), TEXT("Multiplier for Lumen diffuse indirect lighting on the character."), TEXT("角色接受 Lumen 漫反射间接光的强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ReflectionIntensity), TEXT("Reflection Intensity"), TEXT("反射强度"), TEXT("Multiplier for environment reflections on the character."), TEXT("角色环境反射结果的强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectOcclusionStrength), TEXT("Indirect Occlusion Strength"), TEXT("间接光遮蔽强度"), TEXT("Strength of SSAO and Lumen short-range occlusion on the character. Zero removes local self-occlusion darkening but keeps projected scene shadows."), TEXT("角色接受 SSAO 与 Lumen 短距离遮蔽的强度。设为 0 可去除局部自遮蔽压暗，但不会关闭场景物体投到角色上的投影，也不会关闭角色投向环境的阴影。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterBaseFill), TEXT("Character Base Fill"), TEXT("角色底色补光"), TEXT("Minimum BaseColor contribution when scene lighting is weak. This does not emit light, create bloom, or illuminate the environment."), TEXT("场景光照不足时保留的最低 BaseColor 亮度。它不是自发光，不产生 Bloom，也不会照亮周围环境。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterExposure), TEXT("Character Exposure"), TEXT("角色曝光"), TEXT("Exposure in stops applied only to this character lighting profile. Plus one doubles lighting; minus one halves it."), TEXT("只作用于当前角色光照配置的曝光档位。+1 使光照翻倍，-1 使光照减半。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterContrast), TEXT("Character Contrast"), TEXT("角色光照对比度"), TEXT("Luminance contrast applied only to stylized character lighting. One is neutral; lower values lift dark lighting."), TEXT("只作用于风格化角色光照的亮度对比度。1 为不修改，小于 1 会抬高暗部光照。") },
	};
}

TSharedRef<IDetailCustomization> FStylizedLightingSettingsDetails::MakeInstance()
{
	return MakeShared<FStylizedLightingSettingsDetails>();
}

void FStylizedLightingSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	const UStylizedLightingSettings* Settings = Objects.Num() > 0 ? Cast<UStylizedLightingSettings>(Objects[0].Get()) : nullptr;
	const bool bChinese = ShouldUseChinese(Settings);

	SetReflectionModeNames(bChinese);

	DetailBuilder.EditCategory(TEXT("Interface"), SelectText(bChinese, TEXT("Interface"), TEXT("界面")), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Character Lighting"), SelectText(bChinese, TEXT("Character Lighting"), TEXT("角色光照")), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Character Lighting|Global Multipliers"), SelectText(bChinese, TEXT("Global Multipliers"), TEXT("全局倍率")), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Character Lighting Profiles"), SelectText(bChinese, TEXT("Character Lighting Profiles"), TEXT("角色光照配置")), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Scene Lighting"), SelectText(bChinese, TEXT("Scene Lighting"), TEXT("场景光照")), ECategoryPriority::Default);
	DetailBuilder.EditCategory(TEXT("Character Reflection"), SelectText(bChinese, TEXT("Character Reflection"), TEXT("角色反射")), ECategoryPriority::Default);
	DetailBuilder.EditCategory(TEXT("Character Self Shadow"), SelectText(bChinese, TEXT("Character Self Shadow"), TEXT("角色自阴影")), ECategoryPriority::Default);

	for (const FLocalizedPropertyText& PropertyText : SettingsProperties)
	{
		ApplyText(DetailBuilder.GetProperty(PropertyText.PropertyName, UStylizedLightingSettings::StaticClass()), PropertyText, bChinese);
	}

	const TSharedRef<IPropertyHandle> LanguageHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, EditorLanguage),
		UStylizedLightingSettings::StaticClass());
	const TWeakPtr<IPropertyUtilities> PropertyUtilities = DetailBuilder.GetPropertyUtilities();
	LanguageHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([PropertyUtilities]()
	{
		if (const TSharedPtr<IPropertyUtilities> Utilities = PropertyUtilities.Pin())
		{
			Utilities->ForceRefresh();
		}
	}));

	const TSharedRef<IPropertyHandle> ProfilesHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, CharacterLightingProfiles),
		UStylizedLightingSettings::StaticClass());
	if (const TSharedPtr<IPropertyHandleArray> ProfilesArray = ProfilesHandle->AsArray())
	{
		uint32 NumProfiles = 0;
		ProfilesArray->GetNumElements(NumProfiles);
		for (uint32 ProfileIndex = 0; ProfileIndex < NumProfiles; ++ProfileIndex)
		{
			const TSharedRef<IPropertyHandle> ProfileHandle = ProfilesArray->GetElement(ProfileIndex);
			for (const FLocalizedPropertyText& PropertyText : ProfileProperties)
			{
				ApplyText(ProfileHandle->GetChildHandle(PropertyText.PropertyName), PropertyText, bChinese);
			}
		}
	}
}
