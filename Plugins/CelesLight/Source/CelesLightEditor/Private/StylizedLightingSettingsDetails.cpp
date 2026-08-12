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
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterDirectLighting), TEXT("Enable Character Direct Lighting"), TEXT("启用角色直接光"), TEXT("Master switch for the custom direct-light path of the stylized character shading model."), TEXT("风格化角色直接光渲染路径的总开关。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableHalfLambertPartition), TEXT("Enable Half-Lambert Partition"), TEXT("启用半兰伯特分区"), TEXT("Uses N dot L plus material Diffuse Bias to form the half-Lambert lighting coordinate."), TEXT("使用 NdotL 与材质 Diffuse Bias 形成半兰伯特明暗坐标。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableDarkColorFloor), TEXT("Enable Character Minimum Indirect Diffuse"), TEXT("启用角色最低间接漫反射"), TEXT("Keeps non-metal character diffuse lighting above a neutral PBR-aware minimum. Metallic surfaces remain reflection-driven."), TEXT("为非金属角色表面提供中性的 PBR 最低间接漫反射；金属表面仍由环境反射决定，不添加伪漫反射。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableSceneShadowPartition), TEXT("Enable Scene Shadow Partition"), TEXT("启用场景投影分区"), TEXT("Lets shadows cast by scene geometry move the character into the dark side of the two-tone partition."), TEXT("允许场景物体投到角色上的阴影推动角色进入二分光照的暗面。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableLocalMultiLights), TEXT("Enable Local / Multiple Lights"), TEXT("启用局部/多光源"), TEXT("Enables point, spot and rect lights in addition to directional lights."), TEXT("除定向光外，启用点光、聚光和矩形光对角色的累加。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableWashLights), TEXT("Enable Wash Lights"), TEXT("启用 Wash 灯光"), TEXT("Enables lights configured with the Wash character-light mode."), TEXT("启用 Character Light Mode 为 Wash 的灯光。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableDirectLightColor), TEXT("Enable Direct Light Color"), TEXT("启用直接光颜色"), TEXT("Enables scene-light RGB influence on character direct lighting."), TEXT("启用场景灯光 RGB 对角色直接光的影响。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableDirectLightIntensity), TEXT("Enable Direct Light Intensity"), TEXT("启用直接光强度"), TEXT("Enables physical scene-light intensity influence on the character."), TEXT("启用场景灯光物理强度对角色的影响。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableGGXSpecular), TEXT("Enable GGX Specular"), TEXT("启用 GGX 高光"), TEXT("Enables the native energy-conserving GGX highlight."), TEXT("启用原生能量守恒 GGX 高光。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableMixMapSpecularControl), TEXT("Enable MixMap.R Specular Control"), TEXT("启用 MixMap.R 高光控制"), TEXT("Enables signed suppression/advance around the neutral MixMap.R value 0.5."), TEXT("启用以 MixMap.R=0.5 为中性值的高光抑制/提前控制。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableSpecularLitSideMask), TEXT("Enable Lit-Side Specular Mask"), TEXT("高光仅保留在亮面"), TEXT("Restricts direct specular to the lit side of the two-tone partition."), TEXT("限制直接高光只出现在二分光照的亮面。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableLumenDiffuseIndirect), TEXT("Enable Lumen Diffuse Indirect"), TEXT("启用 Lumen 漫反射间接光"), TEXT("Enables diffuse global illumination on stylized characters."), TEXT("启用风格化角色的漫反射全局照明。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableLumenIndirectColor), TEXT("Enable Lumen Indirect Color"), TEXT("启用 Lumen 间接光颜色"), TEXT("Enables environment RGB in diffuse GI; disabled keeps luminance only."), TEXT("启用环境 RGB 对漫反射 GI 的影响；关闭后只保留亮度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableGINormal), TEXT("Enable GI Smooth Normal"), TEXT("启用 GI 平滑法线"), TEXT("Enables the smooth vertex-normal override for GI response and occlusion."), TEXT("启用平滑顶点法线参与 GI 响应与遮蔽。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableEnvironmentReflections), TEXT("Enable Environment Reflections"), TEXT("启用环境反射"), TEXT("Enables Lumen/SSR environment reflections on stylized characters."), TEXT("启用风格化角色的 Lumen/SSR 环境反射。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableIndirectOcclusion), TEXT("Enable Indirect Occlusion"), TEXT("启用间接光遮蔽"), TEXT("Enables SSAO and Lumen short-range occlusion on stylized characters."), TEXT("启用风格化角色的 SSAO 与 Lumen 短距离遮蔽。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterTone), TEXT("Enable Character Exposure / Contrast"), TEXT("启用角色曝光/对比度"), TEXT("Enables character-only exposure and contrast processing."), TEXT("启用只作用于角色的曝光和对比度处理。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableStylizedLumenLighting), TEXT("Enable Stylized Lumen Lighting"), TEXT("启用风格化 Lumen 光照"), TEXT("Enables banded stylization for scene direct lighting and Lumen indirect lighting."), TEXT("为场景直接光和 Lumen 间接光启用分层风格化处理。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, BandCount), TEXT("Band Count"), TEXT("光照层数"), TEXT("Number of discrete lighting bands. Higher values retain more continuous shading."), TEXT("光照离散分层的数量。数值越高越接近连续光照，数值越低色块感越强。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, BandSoftness), TEXT("Band Softness"), TEXT("分层柔和度"), TEXT("Width of transitions between bands. Zero gives hard steps; higher values soften boundaries."), TEXT("相邻光照层之间的过渡宽度。0 为硬切，数值越高边界越柔和。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, GlossInfluence), TEXT("Gloss Influence"), TEXT("光泽度影响"), TEXT("Controls how strongly material glossiness shifts the band response."), TEXT("控制材质光泽度对光照分层位置和响应的影响程度。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, DirectBlend), TEXT("Direct Light Blend"), TEXT("直接光混合"), TEXT("Blend from native direct lighting at zero to fully banded direct lighting at one."), TEXT("直接光的风格化混合量。0 使用原生直接光，1 使用完整分层结果。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, IndirectBlend), TEXT("Indirect Light Blend"), TEXT("间接光混合"), TEXT("Blend from native Lumen diffuse indirect lighting at zero to fully banded indirect lighting at one."), TEXT("Lumen 漫反射间接光的风格化混合量。0 使用原生结果，1 使用完整分层结果。") },
		{ GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, IndirectMaxLuminance), TEXT("Indirect Light Luminance Range"), TEXT("间接光亮度范围"), TEXT("Scene-referred GI luminance represented by the brightest band. Lower values compress strong emissive GI sooner."), TEXT("最高间接光分层所代表的场景亮度。数值越低，强自发光 GI 越早被压缩，越不容易形成过曝白斑。") },
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
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectDiffuseIntensity), TEXT("Direct Diffuse Intensity"), TEXT("直接漫反射强度"), TEXT("Multiplier for character direct diffuse lighting after the two-tone half-Lambert partition."), TEXT("角色直接漫反射经过二分半兰伯特处理后的强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectLightIntensityInfluence), TEXT("Direct Light Intensity Influence"), TEXT("直接光强影响"), TEXT("How strongly physical light intensity affects the character. Zero normalizes intensity while retaining light hue and attenuation; one uses native intensity."), TEXT("物理光源强度影响角色的程度。0 会归一化光强但保留光色与距离衰减，1 使用原生光强。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SpecularTint), TEXT("Specular Tint"), TEXT("高光颜色"), TEXT("Color tint applied to character specular highlights."), TEXT("角色高光使用的颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SpecularIntensity), TEXT("Specular Intensity"), TEXT("高光强度"), TEXT("Per-profile multiplier for character direct specular highlights."), TEXT("当前角色配置的直接光高光强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectLightColorInfluence), TEXT("Direct Light Color Influence"), TEXT("直接光色影响"), TEXT("How much direct-light RGB affects the character. Zero keeps luminance only; one uses full light color."), TEXT("直接灯光颜色影响角色的程度。0 只接受亮度，1 完整接受灯光颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectLightColorInfluence), TEXT("Indirect Light Color Influence"), TEXT("间接光色影响"), TEXT("How much Lumen diffuse-indirect RGB affects the character. Zero keeps luminance only; one uses full environment color."), TEXT("Lumen 漫反射间接光颜色影响角色的程度。0 只接受亮度，1 完整接受环境光颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ReflectionColorInfluence), TEXT("Reflection Color Influence"), TEXT("反射颜色影响"), TEXT("How much environment-reflection RGB affects the character. Zero keeps luminance only; one uses full reflection color."), TEXT("环境反射颜色影响角色的程度。0 只接受亮度，1 完整接受反射颜色。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, GINormalBlend), TEXT("GI Smooth Normal Blend"), TEXT("GI 平滑法线混合"), TEXT("Blends from the normal-mapped surface at zero to the encoded smooth vertex normal at one for GI occlusion and response."), TEXT("GI 遮蔽与响应使用的法线。0 使用法线贴图表面法线，1 使用材质输出自动编码的平滑顶点法线。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectLightingIntensity), TEXT("Indirect Lighting Intensity"), TEXT("间接光强度"), TEXT("Multiplier for Lumen diffuse indirect lighting on the character."), TEXT("角色接受 Lumen 漫反射间接光的强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ReflectionIntensity), TEXT("Reflection Intensity"), TEXT("反射强度"), TEXT("Multiplier for environment reflections on the character."), TEXT("角色环境反射结果的强度倍率。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectOcclusionStrength), TEXT("Indirect Occlusion Strength"), TEXT("间接光遮蔽强度"), TEXT("Strength of SSAO and Lumen short-range occlusion on the character. Zero removes local self-occlusion darkening but keeps projected scene shadows."), TEXT("角色接受 SSAO 与 Lumen 短距离遮蔽的强度。设为 0 可去除局部自遮蔽压暗，但不会关闭场景物体投到角色上的投影，也不会关闭角色投向环境的阴影。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterBaseFill), TEXT("Minimum Indirect Diffuse Multiplier"), TEXT("最低间接漫反射倍率"), TEXT("Neutral minimum indirect diffuse for non-metal surfaces. Metallic surfaces remain reflection-driven; this does not replace light direction or create a new partition."), TEXT("非金属表面的中性最低间接漫反射。金属表面仍由环境反射决定；该参数不会替代灯光方向，也不会创建新的明暗分层。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterBaseFillSoftness), TEXT("Minimum Brightness Softness"), TEXT("最低明度过渡柔度"), TEXT("Softens the transition where current character lighting approaches the minimum brightness."), TEXT("柔化角色当前光照接近最低明度时的过渡边界。") },
		{ GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterBaseFillOcclusionInfluence), TEXT("Minimum Brightness AO Influence"), TEXT("最低明度受环境遮蔽影响"), TEXT("How strongly SSAO and Lumen occlusion may darken the minimum. Zero preserves the full floor; one applies full occlusion."), TEXT("SSAO 与 Lumen 遮蔽压暗最低明度的程度。0 完整保留下限，1 接受完整遮蔽。") },
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
	IDetailCategoryBuilder& BasicCategory = DetailBuilder.EditCategory(TEXT("Character Features|01 Basic Half Lambert"), SelectText(bChinese, TEXT("01 Basic Two-Tone / Half-Lambert"), TEXT("01 基础二分 / 半兰伯特")), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Character Features|02 Scene Projection"), SelectText(bChinese, TEXT("02 Projection (Scene)"), TEXT("02 投影（场景）")), ECategoryPriority::Important);
	IDetailCategoryBuilder& MultiLightCategory = DetailBuilder.EditCategory(TEXT("Character Features|03 Multiple Lights"), SelectText(bChinese, TEXT("03 Multiple Lights"), TEXT("03 多光源处理")), ECategoryPriority::Important);
	IDetailCategoryBuilder& SpecularCategory = DetailBuilder.EditCategory(TEXT("Character Features|04 Specular"), SelectText(bChinese, TEXT("04 Specular"), TEXT("04 高光部分")), ECategoryPriority::Important);
	IDetailCategoryBuilder& GICategory = DetailBuilder.EditCategory(TEXT("Character Features|05 Global Illumination Lumen"), SelectText(bChinese, TEXT("05 Global Illumination / Lumen"), TEXT("05 全局照明 / Lumen")), ECategoryPriority::Important);
	IDetailCategoryBuilder& ColorCategory = DetailBuilder.EditCategory(TEXT("Character Features|06 Color Fidelity"), SelectText(bChinese, TEXT("06 Color Fidelity"), TEXT("06 颜色与明度")), ECategoryPriority::Important);
	DetailBuilder.EditCategory(TEXT("Scene Lighting"), SelectText(bChinese, TEXT("Scene Lighting"), TEXT("场景光照")), ECategoryPriority::Default);

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
		// Keep the profile array as a backward-compatible storage format, but
		// expose profile zero as one classified global tuning surface.
		if (NumProfiles > 0)
		{
			const TSharedRef<IPropertyHandle> GlobalTuning = ProfilesArray->GetElement(0);
			auto Add = [&GlobalTuning](IDetailCategoryBuilder& Category, const FName PropertyName)
			{
				if (const TSharedPtr<IPropertyHandle> Handle = GlobalTuning->GetChildHandle(PropertyName))
				{
					Category.AddProperty(Handle.ToSharedRef());
				}
			};

			Add(BasicCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectDiffuseIntensity));
			Add(BasicCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterBaseFill));
			Add(BasicCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterBaseFillSoftness));
			Add(BasicCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterBaseFillOcclusionInfluence));

			Add(MultiLightCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectLightIntensityInfluence));
			Add(MultiLightCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, DirectLightColorInfluence));
			Add(SpecularCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SpecularTint));
			Add(SpecularCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, SpecularIntensity));
			Add(GICategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectLightColorInfluence));
			Add(GICategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, GINormalBlend));
			Add(GICategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectLightingIntensity));
			Add(GICategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ReflectionColorInfluence));
			Add(GICategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, ReflectionIntensity));
			Add(GICategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, IndirectOcclusionStrength));
			Add(ColorCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterExposure));
			Add(ColorCategory, GET_MEMBER_NAME_CHECKED(FStylizedCharacterLightingProfile, CharacterContrast));
		}
	}
	DetailBuilder.HideProperty(ProfilesHandle);
}
