#include "StylizedLightingSettingsDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "StylizedLightingSettings.h"
#include "UObject/UnrealType.h"

TSharedRef<IDetailCustomization> FStylizedLightingSettingsDetails::MakeInstance()
{
	return MakeShared<FStylizedLightingSettingsDetails>();
}

void FStylizedLightingSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Expose only parameters with a current shader consumer. Keep retired fields
	// serialized, but never offer them as editable controls.
	const TSet<FName> ActiveProperties = {
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableStylizedLumenLighting),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, BandSoftness),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, IndirectBlend),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, DirectBlend),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularIntensity),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, SpecularOffset),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableSceneShadowPartition),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableLumenDiffuseIndirect),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, NonMetalReflectionAlpha),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, MetalReflectionAlpha),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableIndirectOcclusion),
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterIndependentSelfShadow)
	};
	for (TFieldIterator<FProperty> It(UStylizedLightingSettings::StaticClass()); It; ++It)
	{
		if (!ActiveProperties.Contains(It->GetFName()))
		{
			DetailBuilder.HideProperty(It->GetFName(), UStylizedLightingSettings::StaticClass());
		}
	}
	// The accepted look uses native VSM; retain the checked preference without
	// presenting this inactive independent-map option as an effective edit.
	if (IDetailPropertyRow* Row = DetailBuilder.EditDefaultProperty(DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, bEnableCharacterIndependentSelfShadow),
		UStylizedLightingSettings::StaticClass())))
	{
		Row->IsEnabled(false);
	}
	DetailBuilder.EditCategory(TEXT("Scene Lighting"), FText::FromString(TEXT("二分光影 / 原生材质场景")), ECategoryPriority::Important)
		.InitiallyCollapsed(false);
	DetailBuilder.EditCategory(TEXT("Character Features|02 Scene Projection"), FText::FromString(TEXT("角色原生投影")))
		.InitiallyCollapsed(false);
	DetailBuilder.EditCategory(TEXT("Character Features|05 Global Illumination Lumen"), FText::FromString(TEXT("角色 GI / 反射与可选细碎遮蔽")))
		.InitiallyCollapsed(false);
}
