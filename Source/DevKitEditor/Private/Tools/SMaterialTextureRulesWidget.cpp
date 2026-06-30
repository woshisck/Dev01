#include "Tools/SMaterialTextureRulesWidget.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "System/YogPerformanceAuthoringData.h"
#include "UObject/Package.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include <initializer_list>

#define LOCTEXT_NAMESPACE "SMaterialTextureRulesWidget"

namespace
{
const TCHAR* GetDefaultTextureNamingConventionPackagePath()
{
	return TEXT("/Game/Generated/Performance/DA_MaterialTextureNamingRules_Default");
}

FString BuildObjectPathFromPackagePath(const FString& PackagePath)
{
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
}

bool SavePackageForObject(UObject* Object)
{
	if (!Object)
	{
		return false;
	}

	TArray<UPackage*> Packages;
	Packages.Add(Object->GetOutermost());
	return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
}

bool OpenAssetEditorForObject(UObject* Asset)
{
	if (!GEditor || !Asset)
	{
		return false;
	}

	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		return AssetEditorSubsystem->OpenEditorForAsset(Asset);
	}
	return false;
}

FYogMaterialTextureNamingRule MakeTextureNamingRule(
	const FName ChannelName,
	const FName CanonicalParameterName,
	std::initializer_list<const TCHAR*> AcceptedSuffixes,
	bool bSRGB,
	const FString& CompressionIntent,
	const FString& Packing,
	const FString& Notes)
{
	FYogMaterialTextureNamingRule Rule;
	Rule.ChannelName = ChannelName;
	Rule.CanonicalParameterName = CanonicalParameterName;
	Rule.bSRGB = bSRGB;
	Rule.CompressionIntent = CompressionIntent;
	Rule.Packing = Packing;
	Rule.Notes = Notes;
	for (const TCHAR* Suffix : AcceptedSuffixes)
	{
		Rule.AcceptedSuffixes.Add(Suffix);
	}
	return Rule;
}

void FillDefaultTextureNamingRules(UYogMaterialTextureNamingConvention* Convention)
{
	if (!Convention)
	{
		return;
	}

	Convention->Rules = {
		MakeTextureNamingRule(
			TEXT("BaseColor"),
			TEXT("T_BaseColor_A"),
			{ TEXT("_BaseColor"), TEXT("_BC"), TEXT("_D"), TEXT("_Diffuse"), TEXT("_Albedo"), TEXT("_A") },
			true,
			TEXT("Color/sRGB"),
			TEXT("RGB=albedo"),
			TEXT("Prefer _BaseColor or _BC. _A is accepted for legacy Albedo but should not be confused with A/B/C texture-set suffixes.")),
		MakeTextureNamingRule(
			TEXT("Normal"),
			TEXT("T_Normal_A"),
			{ TEXT("_Normal"), TEXT("_N"), TEXT("_NM"), TEXT("_NRM") },
			false,
			TEXT("Normalmap"),
			TEXT("RGB=tangent normal"),
			TEXT("Use the project's normal-map compression settings.")),
		MakeTextureNamingRule(
			TEXT("ORM"),
			TEXT("T_ORM_A"),
			{ TEXT("_ORM"), TEXT("_MRA"), TEXT("_RMA"), TEXT("_Packed"), TEXT("_MaskMap") },
			false,
			TEXT("Masks"),
			TEXT("R=AO, G=Roughness, B=Metallic"),
			TEXT("Packed material response data used by source and baked paths.")),
		MakeTextureNamingRule(
			TEXT("Height"),
			TEXT("T_Height_A"),
			{ TEXT("_Height"), TEXT("_H"), TEXT("_Disp"), TEXT("_Displacement") },
			false,
			TEXT("Linear grayscale"),
			TEXT("R=height"),
			TEXT("Source or bake input only; Low should not depend on runtime height blending.")),
		MakeTextureNamingRule(
			TEXT("Mask"),
			TEXT("T_Mask_A"),
			{ TEXT("_Mask"), TEXT("_MSK"), TEXT("_Masks"), TEXT("_BlendMask"), TEXT("_UniqueMask") },
			false,
			TEXT("Masks"),
			TEXT("Project-specific mask channels"),
			TEXT("Use for blend masks, damage masks, wetness, or other surface controls.")),
		MakeTextureNamingRule(
			TEXT("LightInfo"),
			TEXT("T_LightInfo"),
			{ TEXT("_LightInfo"), TEXT("_LI"), TEXT("_FakeLight"), TEXT("_MatLight") },
			false,
			TEXT("Linear special data"),
			TEXT("Project-specific material light data"),
			TEXT("Handled by material-light quality budgets; do not pack into BaseColor/Normal/ORM atlases.")),
		MakeTextureNamingRule(
			TEXT("Emissive"),
			TEXT("T_Emissive_A"),
			{ TEXT("_Emissive"), TEXT("_E"), TEXT("_Emission"), TEXT("_Glow") },
			true,
			TEXT("Color/sRGB"),
			TEXT("RGB=emissive color"),
			TEXT("Optional channel; not required for every batch material."))
	};
}

const FYogMaterialTextureNamingRule* FindMatchingRule(const FString& TextureName, const UYogMaterialTextureNamingConvention* Convention)
{
	if (!Convention)
	{
		return nullptr;
	}

	for (const FYogMaterialTextureNamingRule& Rule : Convention->Rules)
	{
		for (const FString& Suffix : Rule.AcceptedSuffixes)
		{
			if (!Suffix.IsEmpty() && TextureName.EndsWith(Suffix, ESearchCase::IgnoreCase))
			{
				return &Rule;
			}
		}
	}
	return nullptr;
}
}

void SMaterialTextureRulesWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(16.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "贴图命名规则与检查"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(this, &SMaterialTextureRulesWidget::GetUsageText)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("WriteRules", "写入默认命名规则表"))
						.ToolTipText(LOCTEXT("WriteRulesTooltip", "创建或重置 /Game/Generated/Performance/DA_MaterialTextureNamingRules_Default。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::WriteDefaultTextureNamingConventionAsset)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("OpenRules", "打开命名规则表"))
						.ToolTipText(LOCTEXT("OpenRulesTooltip", "打开当前默认贴图命名规则资产；如果不存在会先创建。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::OpenDefaultTextureNamingConventionAsset)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("CheckTextures", "检查选中贴图命名"))
						.ToolTipText(LOCTEXT("CheckTexturesTooltip", "检查 Content Browser 中选中的 Texture2D 是否命中规则后缀，并核对 sRGB 预期。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::CheckSelectedTextureNaming)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("InitialStatus", "就绪。先在 Content Browser 选择 Texture2D，再点击检查。"))
					.AutoWrapText(true)
				]
			]
		]
	];
}

FReply SMaterialTextureRulesWidget::WriteDefaultTextureNamingConventionAsset()
{
	UYogMaterialTextureNamingConvention* Convention = LoadOrCreateDefaultConvention(true);
	const bool bSaved = SavePackageForObject(Convention);
	const bool bOpened = OpenAssetEditorForObject(Convention);
	SetStatus(FText::Format(
		LOCTEXT("WriteDone", "默认贴图命名规则表已写入：{0}。保存 {1}，编辑器 {2}，规则数 {3}。"),
		FText::FromString(BuildObjectPathFromPackagePath(GetDefaultTextureNamingConventionPackagePath())),
		bSaved ? LOCTEXT("Saved", "成功") : LOCTEXT("SaveFailed", "失败"),
		bOpened ? LOCTEXT("Opened", "已打开") : LOCTEXT("OpenFailed", "未打开"),
		FText::AsNumber(Convention ? Convention->Rules.Num() : 0)));
	return FReply::Handled();
}

FReply SMaterialTextureRulesWidget::OpenDefaultTextureNamingConventionAsset()
{
	UYogMaterialTextureNamingConvention* Convention = LoadOrCreateDefaultConvention(false);
	const bool bSaved = SavePackageForObject(Convention);
	const bool bOpened = OpenAssetEditorForObject(Convention);
	SetStatus(FText::Format(
		LOCTEXT("OpenDone", "贴图命名规则表：{0}，保存 {1}，编辑器 {2}。"),
		FText::FromString(BuildObjectPathFromPackagePath(GetDefaultTextureNamingConventionPackagePath())),
		bSaved ? LOCTEXT("Saved", "成功") : LOCTEXT("SaveFailed", "失败"),
		bOpened ? LOCTEXT("Opened", "已打开") : LOCTEXT("OpenFailed", "未打开")));
	return FReply::Handled();
}

FReply SMaterialTextureRulesWidget::CheckSelectedTextureNaming()
{
	UYogMaterialTextureNamingConvention* Convention = LoadOrCreateDefaultConvention(false);
	if (!Convention || Convention->Rules.IsEmpty())
	{
		SetStatus(LOCTEXT("NoRules", "命名规则表为空。请先点击“写入默认命名规则表”。"));
		return FReply::Handled();
	}

	const TArray<UTexture2D*> Textures = CollectSelectedTextures();
	if (Textures.IsEmpty())
	{
		SetStatus(LOCTEXT("NoTextures", "没有检测到 Content Browser 选中的 Texture2D。请在 Content Browser 选中贴图资产后再检查。"));
		return FReply::Handled();
	}

	int32 MatchedCount = 0;
	int32 SRGBMismatchCount = 0;
	TArray<FString> UnmatchedNames;
	TArray<FString> SRGBMismatchNames;
	for (const UTexture2D* Texture : Textures)
	{
		if (!Texture)
		{
			continue;
		}

		const FString TextureName = Texture->GetName();
		const FYogMaterialTextureNamingRule* Rule = FindMatchingRule(TextureName, Convention);
		if (!Rule)
		{
			UnmatchedNames.Add(TextureName);
			continue;
		}

		++MatchedCount;
		if (Texture->SRGB != Rule->bSRGB)
		{
			++SRGBMismatchCount;
			SRGBMismatchNames.Add(FString::Printf(TEXT("%s(%s)"), *TextureName, *Rule->ChannelName.ToString()));
		}
	}

	auto JoinPreview = [](const TArray<FString>& Names)
	{
		TArray<FString> Preview;
		for (int32 Index = 0; Index < Names.Num() && Index < 8; ++Index)
		{
			Preview.Add(Names[Index]);
		}
		return Preview.IsEmpty() ? FString(TEXT("无")) : FString::Join(Preview, TEXT(", "));
	};

	SetStatus(FText::Format(
		LOCTEXT("CheckDone", "贴图命名检查完成：选中 {0}，命中规则 {1}，未命名匹配 {2}，sRGB 不符合规则 {3}。未匹配示例：{4}。sRGB 示例：{5}。"),
		FText::AsNumber(Textures.Num()),
		FText::AsNumber(MatchedCount),
		FText::AsNumber(UnmatchedNames.Num()),
		FText::AsNumber(SRGBMismatchCount),
		FText::FromString(JoinPreview(UnmatchedNames)),
		FText::FromString(JoinPreview(SRGBMismatchNames))));
	return FReply::Handled();
}

void SMaterialTextureRulesWidget::SetStatus(const FText& InStatus) const
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(InStatus);
	}
}

FText SMaterialTextureRulesWidget::GetUsageText() const
{
	return LOCTEXT(
		"Usage",
		"使用方式：美术继续提交单体 unique 贴图，不手工维护 atlas/UDIM。TA 先用“写入默认命名规则表”生成规则资产，之后美术在 Content Browser 选中 Texture2D 并点击检查。后续自动合批工具会根据这些后缀识别 BaseColor、Normal、ORM、Height、Mask、LightInfo、Emissive，并在打包/合批阶段生成 VT Atlas 或 baked 贴图。");
}

TArray<UTexture2D*> SMaterialTextureRulesWidget::CollectSelectedTextures() const
{
	TArray<UTexture2D*> Textures;
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset()))
		{
			Textures.AddUnique(Texture);
		}
	}
	return Textures;
}

UYogMaterialTextureNamingConvention* SMaterialTextureRulesWidget::LoadOrCreateDefaultConvention(bool bFillDefaultRules) const
{
	const FString PackagePath = GetDefaultTextureNamingConventionPackagePath();
	const FString ObjectPath = BuildObjectPathFromPackagePath(PackagePath);
	UYogMaterialTextureNamingConvention* Convention = Cast<UYogMaterialTextureNamingConvention>(
		StaticLoadObject(UYogMaterialTextureNamingConvention::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
	UPackage* Package = Convention ? Convention->GetOutermost() : CreatePackage(*PackagePath);
	if (!Package)
	{
		return nullptr;
	}

	if (!Convention)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		Convention = NewObject<UYogMaterialTextureNamingConvention>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Convention);
		bFillDefaultRules = true;
	}

	if (Convention && bFillDefaultRules)
	{
		Convention->Modify();
		FillDefaultTextureNamingRules(Convention);
		Convention->MarkPackageDirty();
		Package->MarkPackageDirty();
	}
	return Convention;
}

#undef LOCTEXT_NAMESPACE
