#include "Tools/SMaterialTextureRulesWidget.h"
#include "Tools/DevKitArtToolUI.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "IContentBrowserSingleton.h"
#include "MaterialBatch/MaterialBatchMaterialAudit.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "System/YogPerformanceAuthoringData.h"
#include "System/YogPerformanceSettingsLibrary.h"
#include "UObject/Package.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"

#include <initializer_list>

#define LOCTEXT_NAMESPACE "SMaterialTextureRulesWidget"

namespace
{
const TCHAR* ArtRootPath = TEXT("/Game/Art");

const TCHAR* GetDefaultTextureNamingConventionPackagePath()
{
	return TEXT("/Game/Docs/Performance/MaterialTextureRules/DA_MaterialTextureNamingRules_Default");
}

FString BuildObjectPathFromPackagePath(const FString& PackagePath)
{
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
}

FString GetAssetObjectPath(const FAssetData& AssetData)
{
	return AssetData.GetSoftObjectPath().ToString();
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

bool IsPowerOfTwo(const int32 Value)
{
	return Value > 0 && FMath::IsPowerOfTwo(Value);
}

FString EscapeRichText(const FString& Value)
{
	FString Result = Value;
	Result.ReplaceInline(TEXT("&"), TEXT("&amp;"));
	Result.ReplaceInline(TEXT("<"), TEXT("&lt;"));
	Result.ReplaceInline(TEXT(">"), TEXT("&gt;"));
	return Result;
}

const TCHAR* StatusToText(EMaterialComplianceStatus Status)
{
	switch (Status)
	{
	case EMaterialComplianceStatus::Pass:
		return TEXT("通过");
	case EMaterialComplianceStatus::Warning:
		return TEXT("警告");
	case EMaterialComplianceStatus::Blocked:
		return TEXT("阻断");
	default:
		return TEXT("未知");
	}
}

const TCHAR* RichTagForStatus(EMaterialComplianceStatus Status)
{
	switch (Status)
	{
	case EMaterialComplianceStatus::Pass:
		return TEXT("ok");
	case EMaterialComplianceStatus::Warning:
		return TEXT("warn");
	case EMaterialComplianceStatus::Blocked:
		return TEXT("bad");
	default:
		return TEXT("text");
	}
}

void RaiseStatus(FMaterialComplianceResult& Result, EMaterialComplianceStatus Status)
{
	if (Status == EMaterialComplianceStatus::Blocked ||
		(Status == EMaterialComplianceStatus::Warning && Result.Status == EMaterialComplianceStatus::Pass))
	{
		Result.Status = Status;
	}
}

void AddResultMessage(FMaterialComplianceResult& Result, EMaterialComplianceStatus Status, const FString& Message)
{
	RaiseStatus(Result, Status);
	Result.Messages.Add(FString::Printf(TEXT("<%s>[%s]</> %s"), RichTagForStatus(Status), StatusToText(Status), *EscapeRichText(Message)));
}

void AddInfoMessage(FMaterialComplianceResult& Result, const FString& Message)
{
	Result.Messages.Add(FString::Printf(TEXT("<info>[说明]</> %s"), *EscapeRichText(Message)));
}

const FSlateStyleSet& GetComplianceRichTextStyle()
{
	static TSharedPtr<FSlateStyleSet> StyleSet;
	if (!StyleSet.IsValid())
	{
		StyleSet = MakeShared<FSlateStyleSet>(TEXT("MaterialComplianceRichTextStyle"));

		const FTextBlockStyle BaseStyle = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText"));

		FTextBlockStyle DefaultStyle(BaseStyle);
		DefaultStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.86f, 0.86f)));
		StyleSet->Set(TEXT("Default"), DefaultStyle);
		StyleSet->Set(TEXT("text"), DefaultStyle);

		FTextBlockStyle LabelStyle(DefaultStyle);
		LabelStyle.SetFont(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")));
		LabelStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.82f, 1.00f)));
		StyleSet->Set(TEXT("label"), LabelStyle);

		FTextBlockStyle OkStyle(DefaultStyle);
		OkStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.20f, 0.78f, 0.34f)));
		StyleSet->Set(TEXT("ok"), OkStyle);

		FTextBlockStyle WarnStyle(DefaultStyle);
		WarnStyle.SetColorAndOpacity(FSlateColor(FLinearColor(1.00f, 0.68f, 0.22f)));
		StyleSet->Set(TEXT("warn"), WarnStyle);

		FTextBlockStyle BadStyle(DefaultStyle);
		BadStyle.SetColorAndOpacity(FSlateColor(FLinearColor(1.00f, 0.28f, 0.20f)));
		StyleSet->Set(TEXT("bad"), BadStyle);

		FTextBlockStyle InfoStyle(DefaultStyle);
		InfoStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.72f, 1.00f)));
		StyleSet->Set(TEXT("info"), InfoStyle);
	}
	return *StyleSet;
}

TSharedRef<SWidget> MakeRichTextBlock(TAttribute<FText> Text)
{
	const FSlateStyleSet& RichTextStyle = GetComplianceRichTextStyle();
	return SNew(SRichTextBlock)
		.Text(Text)
		.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
		.DecoratorStyleSet(&RichTextStyle)
		.AutoWrapText(true);
}

FYogMaterialTextureNamingRule MakeTextureNamingRule(
	const FName ChannelName,
	const FName CanonicalParameterName,
	std::initializer_list<const TCHAR*> AcceptedSuffixes,
	bool bSRGB,
	int32 RecommendedMaxTextureSize,
	const FString& CompressionIntent,
	const FString& Packing,
	const FString& Notes)
{
	FYogMaterialTextureNamingRule Rule;
	Rule.ChannelName = ChannelName;
	Rule.CanonicalParameterName = CanonicalParameterName;
	Rule.bSRGB = bSRGB;
	Rule.RecommendedMaxTextureSize = RecommendedMaxTextureSize;
	Rule.bRequirePowerOfTwo = true;
	Rule.bPreferVirtualTextureForBatchedEnvironment = false;
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
			4096,
			TEXT("Color/sRGB"),
			TEXT("RGB=albedo"),
			TEXT("Prefer _BaseColor or _BC. _A is accepted for legacy Albedo but should not be confused with A/B/C texture-set suffixes.")),
		MakeTextureNamingRule(
			TEXT("Normal"),
			TEXT("T_Normal_A"),
			{ TEXT("_Normal"), TEXT("_N"), TEXT("_NM"), TEXT("_NRM") },
			false,
			4096,
			TEXT("Normalmap"),
			TEXT("RGB=tangent normal"),
			TEXT("Use the project's normal-map compression settings.")),
		MakeTextureNamingRule(
			TEXT("ORM"),
			TEXT("T_ORM_A"),
			{ TEXT("_ORM"), TEXT("_MRA"), TEXT("_RMA"), TEXT("_Packed"), TEXT("_MaskMap") },
			false,
			4096,
			TEXT("Masks"),
			TEXT("R=AO, G=Roughness, B=Metallic"),
			TEXT("Packed material response data used by source and baked paths.")),
		MakeTextureNamingRule(
			TEXT("Height"),
			TEXT("T_Height_A"),
			{ TEXT("_Height"), TEXT("_H"), TEXT("_Disp"), TEXT("_Displacement") },
			false,
			2048,
			TEXT("Linear grayscale"),
			TEXT("R=height"),
			TEXT("Source or bake input only; Low should not depend on runtime height blending.")),
		MakeTextureNamingRule(
			TEXT("Mask"),
			TEXT("T_Mask_A"),
			{ TEXT("_Mask"), TEXT("_MSK"), TEXT("_Masks"), TEXT("_BlendMask"), TEXT("_UniqueMask") },
			false,
			2048,
			TEXT("Masks"),
			TEXT("Project-specific mask channels"),
			TEXT("Use for blend masks, damage masks, wetness, or other surface controls.")),
		MakeTextureNamingRule(
			TEXT("LightInfo"),
			TEXT("T_LightInfo"),
			{ TEXT("_LightInfo"), TEXT("_LI"), TEXT("_FakeLight"), TEXT("_MatLight") },
			false,
			1024,
			TEXT("Linear special data"),
			TEXT("Project-specific material light data"),
			TEXT("Handled by material-light quality budgets; do not pack into BaseColor/Normal/ORM atlases.")),
		MakeTextureNamingRule(
			TEXT("Emissive"),
			TEXT("T_Emissive_A"),
			{ TEXT("_Emissive"), TEXT("_E"), TEXT("_Emission"), TEXT("_Glow") },
			true,
			2048,
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

bool ContainsParameterName(const TArray<FMaterialBatchMaterialTextureParameter>& Parameters, const TCHAR* ParameterName)
{
	for (const FMaterialBatchMaterialTextureParameter& Parameter : Parameters)
	{
		if (Parameter.ParameterName.Equals(ParameterName, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

bool ContainsScalarParameterName(const TArray<FMaterialBatchMaterialScalarParameter>& Parameters, const TCHAR* ParameterName)
{
	for (const FMaterialBatchMaterialScalarParameter& Parameter : Parameters)
	{
		if (Parameter.ParameterName.Equals(ParameterName, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

void EvaluateBoundTexture(
	const UTexture2D* Texture,
	const FString& ParameterName,
	const UYogMaterialTextureNamingConvention* Convention,
	FMaterialComplianceResult& Result)
{
	if (!Texture)
	{
		return;
	}

	const FYogMaterialTextureNamingRule* Rule = FindMatchingRule(Texture->GetName(), Convention);
	if (!Rule)
	{
		AddResultMessage(
			Result,
			EMaterialComplianceStatus::Warning,
			FString::Printf(TEXT("参数 %s 绑定的贴图 %s 无法按命名规则识别，后续 VT/UDIM 合批工具可能无法归类。"), *ParameterName, *Texture->GetName()));
		return;
	}

	if (Texture->SRGB != Rule->bSRGB)
	{
		AddResultMessage(
			Result,
			EMaterialComplianceStatus::Blocked,
			FString::Printf(TEXT("参数 %s 绑定的贴图 %s sRGB=%s，但通道 %s 需要 sRGB=%s。"),
				*ParameterName,
				*Texture->GetName(),
				Texture->SRGB ? TEXT("true") : TEXT("false"),
				*Rule->ChannelName.ToString(),
				Rule->bSRGB ? TEXT("true") : TEXT("false")));
	}
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
					DevKitArtToolUI::MakeHeader(LOCTEXT("Title", "材质合规检查"), GetUsageText())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(1, LOCTEXT("RulesSection", "检查规则"), LOCTEXT("RulesSectionDesc", "创建或打开项目统一的贴图命名与通道规则。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("WriteRules", "写入默认贴图规则"))
						.ToolTipText(LOCTEXT("WriteRulesTooltip", "创建或重置 /Game/Docs/Performance/MaterialTextureRules/DA_MaterialTextureNamingRules_Default。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::WriteDefaultTextureNamingConventionAsset)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("OpenRules", "打开贴图规则表"))
						.ToolTipText(LOCTEXT("OpenRulesTooltip", "打开当前默认贴图命名规则资产；如果不存在会先创建。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::OpenDefaultTextureNamingConventionAsset)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(2, LOCTEXT("CheckSection", "选择检查范围"), LOCTEXT("CheckSectionDesc", "检查 Content Browser 当前选择，或扫描完整的 /Game/Art。所有操作只读，不会修改材质和贴图。"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("CheckTextures", "检查选中贴图"))
						.ToolTipText(LOCTEXT("CheckTexturesTooltip", "检查 Content Browser 中选中的 Texture2D 后缀、sRGB、尺寸和 VT 建议。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::CheckSelectedTextureNaming)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("CheckSelectedMaterialCompliance", "检查选中材质/贴图"))
						.ToolTipText(LOCTEXT("CheckSelectedMaterialComplianceTooltip", "检查 Content Browser 选中的 Texture2D、Material、MaterialInstance 是否符合合批材质接口。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::CheckSelectedMaterialCompliance)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("ScanArtMaterialCompliance", "扫描 /Game/Art"))
						.ToolTipText(LOCTEXT("ScanArtMaterialComplianceTooltip", "扫描 /Game/Art 下 Texture2D、Material、MaterialInstance，仅做合规检查，不执行合批或写生成资产。"))
						.OnClicked(this, &SMaterialTextureRulesWidget::ScanArtMaterialCompliance)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(3, LOCTEXT("ResultSection", "检查结果"), LOCTEXT("ResultSectionDesc", "先处理阻塞项，再处理警告；信息项用于说明当前资产状态。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(SBorder)
					.Padding(10.f)
					.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
					[
						SAssignNew(StatusTextBlock, STextBlock)
						.Text(LOCTEXT("InitialStatus", "就绪。先确认规则，再选择贴图或材质执行检查。"))
						.AutoWrapText(true)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeRichTextBlock(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SMaterialTextureRulesWidget::GetResultsRichText)))
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
		LOCTEXT("WriteDone", "默认贴图规则表已写入：{0}。保存 {1}，编辑器 {2}，规则数 {3}。"),
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
		LOCTEXT("OpenDone", "贴图规则表：{0}，保存 {1}，编辑器 {2}。"),
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
		SetStatus(LOCTEXT("NoRules", "命名规则表为空。请先点击“写入默认贴图规则”。"));
		return FReply::Handled();
	}

	TArray<FMaterialComplianceResult> Results;
	const TArray<UTexture2D*> Textures = CollectSelectedTextures();
	if (Textures.IsEmpty())
	{
		SetStatus(LOCTEXT("NoTextures", "没有检测到 Content Browser 选中的 Texture2D。请在 Content Browser 选中贴图资产后再检查。"));
		return FReply::Handled();
	}

	for (const UTexture2D* Texture : Textures)
	{
		if (!Texture)
		{
			continue;
		}

		FMaterialComplianceResult Result;
		Result.AssetName = Texture->GetName();
		Result.PackagePath = Texture->GetOutermost() ? Texture->GetOutermost()->GetName() : Texture->GetPathName();
		Result.AssetType = TEXT("Texture2D");

		const FYogMaterialTextureNamingRule* Rule = FindMatchingRule(Texture->GetName(), Convention);
		if (!Rule)
		{
			AddResultMessage(Result, EMaterialComplianceStatus::Blocked, TEXT("贴图后缀未命中规则，自动 VT/UDIM 合批工具无法稳定识别通道。"));
		}
		else
		{
			AddInfoMessage(Result, FString::Printf(TEXT("通道=%s，材质参数=%s，压缩意图=%s，打包=%s。"),
				*Rule->ChannelName.ToString(),
				*Rule->CanonicalParameterName.ToString(),
				*Rule->CompressionIntent,
				*Rule->Packing));
			if (Texture->SRGB != Rule->bSRGB)
			{
				AddResultMessage(
					Result,
					EMaterialComplianceStatus::Blocked,
					FString::Printf(TEXT("sRGB=%s，但通道 %s 需要 sRGB=%s。"),
						Texture->SRGB ? TEXT("true") : TEXT("false"),
						*Rule->ChannelName.ToString(),
						Rule->bSRGB ? TEXT("true") : TEXT("false")));
			}

			const int32 MaxDimension = FMath::Max(Texture->GetSizeX(), Texture->GetSizeY());
			if (Rule->RecommendedMaxTextureSize > 0 && MaxDimension > Rule->RecommendedMaxTextureSize)
			{
				AddResultMessage(
					Result,
					EMaterialComplianceStatus::Warning,
					FString::Printf(TEXT("尺寸 %dx%d 超过 %s 推荐上限 %d。"),
						Texture->GetSizeX(),
						Texture->GetSizeY(),
						*Rule->ChannelName.ToString(),
						Rule->RecommendedMaxTextureSize));
			}

			if (Rule->bRequirePowerOfTwo && (!IsPowerOfTwo(Texture->GetSizeX()) || !IsPowerOfTwo(Texture->GetSizeY())))
			{
				AddResultMessage(Result, EMaterialComplianceStatus::Warning, TEXT("尺寸不是 2 的幂；后续打包、mip 和 VT tile 规划建议使用 2 的幂。"));
			}

			if (Texture->VirtualTextureStreaming)
			{
				AddResultMessage(Result, EMaterialComplianceStatus::Warning, TEXT("普通场景模型、建筑、道具贴图不应开启 VT；只有地面 RuntimeVirtualTexture 资产使用虚拟纹理系统。需要集合共享时请使用普通 Texture Collection。"));
			}
			else
			{
				AddInfoMessage(Result, TEXT("普通场景模型贴图保持 NoVT，符合当前材质策略。"));
			}

		}

		if (Result.Status == EMaterialComplianceStatus::Pass)
		{
			AddResultMessage(Result, EMaterialComplianceStatus::Pass, TEXT("贴图命名、sRGB 和基础尺寸检查通过。"));
		}
		Results.Add(MoveTemp(Result));
	}

	UpdateResults(MoveTemp(Results), FText::Format(LOCTEXT("TextureCheckDone", "贴图检查完成：选中 {0} 个 Texture2D。"), FText::AsNumber(Textures.Num())));
	return FReply::Handled();
}

FReply SMaterialTextureRulesWidget::CheckSelectedMaterialCompliance()
{
	UYogMaterialTextureNamingConvention* Convention = LoadOrCreateDefaultConvention(false);
	if (!Convention || Convention->Rules.IsEmpty())
	{
		SetStatus(LOCTEXT("NoRulesForCompliance", "命名规则表为空。请先点击“写入默认贴图规则”。"));
		return FReply::Handled();
	}

	const TArray<FAssetData> Assets = CollectSelectedComplianceAssets();
	if (Assets.IsEmpty())
	{
		SetStatus(LOCTEXT("NoComplianceAssets", "没有检测到选中的 Texture2D、Material 或 MaterialInstance。"));
		return FReply::Handled();
	}

	TArray<FMaterialComplianceResult> Results;
	for (const FAssetData& AssetData : Assets)
	{
		UObject* Asset = AssetData.GetAsset();
		if (Cast<UTexture2D>(Asset))
		{
			EvaluateTextureAsset(AssetData, Convention, Results);
		}
		else if (Cast<UMaterialInterface>(Asset))
		{
			EvaluateMaterialAsset(AssetData, Convention, Results);
		}
	}

	UpdateResults(MoveTemp(Results), FText::Format(LOCTEXT("SelectedComplianceDone", "选中资产合规检查完成：{0} 个资产。"), FText::AsNumber(Assets.Num())));
	return FReply::Handled();
}

FReply SMaterialTextureRulesWidget::ScanArtMaterialCompliance()
{
	UYogMaterialTextureNamingConvention* Convention = LoadOrCreateDefaultConvention(false);
	if (!Convention || Convention->Rules.IsEmpty())
	{
		SetStatus(LOCTEXT("NoRulesForScan", "命名规则表为空。请先点击“写入默认贴图规则”。"));
		return FReply::Handled();
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.ScanPathsSynchronous({ ArtRootPath }, true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(ArtRootPath));
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);
	AssetDataList.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.LexicalLess(Right.PackageName);
	});

	TArray<FMaterialComplianceResult> Results;
	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (Cast<UTexture2D>(Asset))
		{
			EvaluateTextureAsset(AssetData, Convention, Results);
		}
		else if (Cast<UMaterialInterface>(Asset))
		{
			EvaluateMaterialAsset(AssetData, Convention, Results);
		}
	}

	UpdateResults(MoveTemp(Results), FText::Format(LOCTEXT("ArtScanDone", "/Game/Art 材质合规扫描完成：{0} 个贴图/材质资产。"), FText::AsNumber(AssetDataList.Num())));
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
		"UsageNoVT",
		"使用方式：普通场景模型、建筑、道具贴图默认保持 NoVT，材质检查器只检查命名、sRGB、尺寸、POT、材质参数和误开启 VT 的情况。地面 RVT Writer 使用非 VT 源贴图写入 RVT；需要集合共享时请使用普通 Texture Collection，不再要求开启 VirtualTextureStreaming。");
}

#if 0
	return LOCTEXT(
		"Usage",
		"使用方式：美术继续提交单体 unique 贴图，不手工维护 atlas/UDIM。该窗口先检查贴图命名、sRGB、尺寸、NoVT，以及材质是否暴露 Source 或 legacy Baked 所需参数。正式合批、Texture Collection、bake、替换和写资产仍统一在后续工具链执行。基础材质接口采用普通 UE 材质节点和材质实例参数完成。");
}

#endif

FText SMaterialTextureRulesWidget::GetResultsRichText() const
{
	if (LastResults.IsEmpty())
	{
		return LOCTEXT("NoResults", "<info>暂无检查结果。</>");
	}

	int32 PassCount = 0;
	int32 WarningCount = 0;
	int32 BlockedCount = 0;
	TArray<FString> Lines;
	for (const FMaterialComplianceResult& Result : LastResults)
	{
		switch (Result.Status)
		{
		case EMaterialComplianceStatus::Pass:
			++PassCount;
			break;
		case EMaterialComplianceStatus::Warning:
			++WarningCount;
			break;
		case EMaterialComplianceStatus::Blocked:
			++BlockedCount;
			break;
		default:
			break;
		}
	}

	Lines.Add(FString::Printf(
		TEXT("<label>汇总</> 通过 <ok>%d</>，警告 <warn>%d</>，阻断 <bad>%d</>，总数 %d。"),
		PassCount,
		WarningCount,
		BlockedCount,
		LastResults.Num()));

	const int32 MaxVisibleResults = 80;
	for (int32 Index = 0; Index < LastResults.Num() && Index < MaxVisibleResults; ++Index)
	{
		const FMaterialComplianceResult& Result = LastResults[Index];
		Lines.Add(FString::Printf(
			TEXT("\n<%s>[%s]</> <label>%s</>  %s\n<info>%s</>"),
			RichTagForStatus(Result.Status),
			StatusToText(Result.Status),
			*EscapeRichText(Result.AssetName),
			*EscapeRichText(Result.AssetType),
			*EscapeRichText(Result.PackagePath)));

		for (const FString& Message : Result.Messages)
		{
			Lines.Add(Message);
		}
	}

	if (LastResults.Num() > MaxVisibleResults)
	{
		Lines.Add(FString::Printf(TEXT("\n<info>仅显示前 %d 条；请用选中资产检查定位具体问题。</>"), MaxVisibleResults));
	}

	return FText::FromString(FString::Join(Lines, TEXT("\n")));
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

TArray<FAssetData> SMaterialTextureRulesWidget::CollectSelectedComplianceAssets() const
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<FAssetData> Results;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		UObject* Asset = AssetData.GetAsset();
		if (Cast<UTexture2D>(Asset) || Cast<UMaterialInterface>(Asset))
		{
			Results.Add(AssetData);
		}
	}
	return Results;
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
		Convention->Schema = TEXT("DevKit.MaterialTextureNamingConvention.v2");
		Convention->RuleUsage = TEXT("Texture suffix, sRGB, size, and NoVT guidance for scene materials. Ordinary model, building, and prop textures stay NoVT. Ground RuntimeVirtualTexture assets are the only VT path; shared texture grouping uses ordinary TextureCollection.");
		FillDefaultTextureNamingRules(Convention);
		Convention->MarkPackageDirty();
		Package->MarkPackageDirty();
	}
	return Convention;
}

void SMaterialTextureRulesWidget::EvaluateTextureAsset(
	const FAssetData& AssetData,
	const UYogMaterialTextureNamingConvention* Convention,
	TArray<FMaterialComplianceResult>& OutResults) const
{
	UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset());
	if (!Texture)
	{
		return;
	}

	FMaterialComplianceResult Result;
	Result.AssetData = AssetData;
	Result.AssetName = AssetData.AssetName.ToString();
	Result.PackagePath = AssetData.PackageName.ToString();
	Result.AssetType = TEXT("Texture2D");

	const FYogMaterialTextureNamingRule* Rule = FindMatchingRule(Texture->GetName(), Convention);
	if (!Rule)
	{
		AddResultMessage(Result, EMaterialComplianceStatus::Blocked, TEXT("贴图后缀未命中规则，自动 VT/UDIM 合批工具无法稳定识别通道。"));
	}
	else
	{
		AddInfoMessage(Result, FString::Printf(TEXT("通道=%s，材质参数=%s。"), *Rule->ChannelName.ToString(), *Rule->CanonicalParameterName.ToString()));
		if (Texture->SRGB != Rule->bSRGB)
		{
			AddResultMessage(
				Result,
				EMaterialComplianceStatus::Blocked,
				FString::Printf(TEXT("sRGB=%s，但通道 %s 需要 sRGB=%s。"),
					Texture->SRGB ? TEXT("true") : TEXT("false"),
					*Rule->ChannelName.ToString(),
					Rule->bSRGB ? TEXT("true") : TEXT("false")));
		}

		const int32 MaxDimension = FMath::Max(Texture->GetSizeX(), Texture->GetSizeY());
		if (Rule->RecommendedMaxTextureSize > 0 && MaxDimension > Rule->RecommendedMaxTextureSize)
		{
			AddResultMessage(
				Result,
				EMaterialComplianceStatus::Warning,
				FString::Printf(TEXT("尺寸 %dx%d 超过 %s 推荐上限 %d。"),
					Texture->GetSizeX(),
					Texture->GetSizeY(),
					*Rule->ChannelName.ToString(),
					Rule->RecommendedMaxTextureSize));
		}

		if (Rule->bRequirePowerOfTwo && (!IsPowerOfTwo(Texture->GetSizeX()) || !IsPowerOfTwo(Texture->GetSizeY())))
		{
			AddResultMessage(Result, EMaterialComplianceStatus::Warning, TEXT("尺寸不是 2 的幂；后续打包、mip 和 VT tile 规划建议使用 2 的幂。"));
		}
	}

	if (Texture->VirtualTextureStreaming)
	{
		AddResultMessage(Result, EMaterialComplianceStatus::Warning, TEXT("普通场景模型、建筑、道具贴图不应开启 VT；只有地面 RuntimeVirtualTexture 资产使用虚拟纹理系统。需要集合共享时请使用普通 Texture Collection。"));
	}
	else if (Rule)
	{
		AddInfoMessage(Result, TEXT("普通场景模型贴图保持 NoVT，符合当前材质策略。"));
	}

	if (Result.Status == EMaterialComplianceStatus::Pass)
	{
		AddResultMessage(Result, EMaterialComplianceStatus::Pass, TEXT("贴图基础合规检查通过。"));
	}
	OutResults.Add(MoveTemp(Result));
}

void SMaterialTextureRulesWidget::EvaluateMaterialAsset(
	const FAssetData& AssetData,
	const UYogMaterialTextureNamingConvention* Convention,
	TArray<FMaterialComplianceResult>& OutResults) const
{
	FMaterialComplianceResult Result;
	Result.AssetData = AssetData;
	Result.AssetName = AssetData.AssetName.ToString();
	Result.PackagePath = AssetData.PackageName.ToString();
	Result.AssetType = AssetData.AssetClassPath.GetAssetName().ToString();

	const FString ObjectPath = GetAssetObjectPath(AssetData);
	const FMaterialBatchMaterialAuditResult Audit = FMaterialBatchMaterialAuditBuilder::AuditMaterial(ObjectPath);
	if (!Audit.bLoaded)
	{
		AddResultMessage(Result, EMaterialComplianceStatus::Blocked, FString::Printf(TEXT("无法加载材质：%s。"), *Audit.LoadFailureReason));
		OutResults.Add(MoveTemp(Result));
		return;
	}

	AddInfoMessage(Result, FString::Printf(TEXT("BlendMode=%s，ShadingModel=%s，Parent=%s。"),
		*Audit.BlendMode,
		*Audit.ShadingModel,
		Audit.ParentMaterialPath.IsEmpty() ? TEXT("(none)") : *Audit.ParentMaterialPath));

	if (!Audit.BlendMode.Equals(TEXT("BLEND_Opaque"), ESearchCase::IgnoreCase) &&
		!Audit.BlendMode.Equals(TEXT("Opaque"), ESearchCase::IgnoreCase))
	{
		AddResultMessage(Result, EMaterialComplianceStatus::Warning, TEXT("合批静态环境材质推荐 Opaque。Masked/Translucent 应拆独立批次或保持非合批。"));
	}

	const bool bHasSourceRequired =
		ContainsParameterName(Audit.TextureParameters, TEXT("T_BaseColor_A")) &&
		ContainsParameterName(Audit.TextureParameters, TEXT("T_Normal_A")) &&
		ContainsParameterName(Audit.TextureParameters, TEXT("T_ORM_A"));
	const bool bHasBakedRequired =
		ContainsParameterName(Audit.TextureParameters, TEXT("VT_Atlas")) &&
		ContainsParameterName(Audit.TextureParameters, TEXT("_PropTexture")) &&
		ContainsScalarParameterName(Audit.ScalarParameters, TEXT("BatchRowCount")) &&
		ContainsScalarParameterName(Audit.ScalarParameters, TEXT("PropertyColumnCount"));

	if (!bHasSourceRequired && !bHasBakedRequired)
	{
		AddResultMessage(Result, EMaterialComplianceStatus::Blocked, TEXT("未满足 Source 主材或 legacy Baked 材质契约：Source 至少需要 T_BaseColor_A、T_Normal_A、T_ORM_A；legacy Baked 至少需要 VT_Atlas、_PropTexture、BatchRowCount、PropertyColumnCount。新的 Texture Collection 合批材质合同尚未落地，不能用 VTAtlas 作为新默认方案。"));
	}
	else if (bHasSourceRequired)
	{
		AddInfoMessage(Result, TEXT("Source 主材基础贴图接口存在：T_BaseColor_A、T_Normal_A、T_ORM_A。"));
	}
	else if (bHasBakedRequired)
	{
		AddInfoMessage(Result, TEXT("legacy Baked/VT Atlas 基础接口存在：VT_Atlas、_PropTexture、BatchRowCount、PropertyColumnCount。注意：这是历史兼容合同，不是新的 Texture Collection 默认方案。"));
	}

	const TArray<const TCHAR*> TierScalarParameters = {
		TEXT("TierMaterialQuality"),
		TEXT("MaxRuntimeBlendLayers"),
		TEXT("DynamicOverlayQuality"),
		TEXT("MaterialLightQuality"),
		TEXT("MaterialLightMaxLightInfoCount"),
		TEXT("UseBakedResult")
	};

	int32 MissingTierScalarCount = 0;
	for (const TCHAR* ParameterName : TierScalarParameters)
	{
		if (!ContainsScalarParameterName(Audit.ScalarParameters, ParameterName))
		{
			++MissingTierScalarCount;
		}
	}
	if (MissingTierScalarCount > 0 && bHasSourceRequired)
	{
		AddResultMessage(
			Result,
			EMaterialComplianceStatus::Warning,
			FString::Printf(TEXT("缺少 %d 个性能分级 Scalar 参数。建议 Source 主材暴露 TierMaterialQuality、MaxRuntimeBlendLayers、DynamicOverlayQuality、MaterialLightQuality、MaterialLightMaxLightInfoCount、UseBakedResult。"), MissingTierScalarCount));
	}

	for (const FMaterialBatchMaterialTextureParameter& Parameter : Audit.TextureParameters)
	{
		if (!Parameter.bFoundTexture || Parameter.TexturePath.IsEmpty())
		{
			continue;
		}

		if (const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Parameter.TexturePath))
		{
			EvaluateBoundTexture(Texture, Parameter.ParameterName, Convention, Result);
		}
	}

	const FYogMaterialPerformanceTierInterface Epic = UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier::Epic);
	const FYogMaterialPerformanceTierInterface Mid = UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier::Mid);
	const FYogMaterialPerformanceTierInterface Low = UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier::Low);
	AddInfoMessage(Result, FString::Printf(TEXT("当前分级接口：Epic 最多 %d 套 unique 贴图/%d 层运行时混合；Mid 最多 %d 套/%d 层；Low 倾向 baked=%s，最多 %d 套/%d 层。"),
		Epic.MaxUniqueTextureSets,
		Epic.MaxRuntimeBlendLayers,
		Mid.MaxUniqueTextureSets,
		Mid.MaxRuntimeBlendLayers,
		Low.bPreferBakedMaterial ? TEXT("true") : TEXT("false"),
		Low.MaxUniqueTextureSets,
		Low.MaxRuntimeBlendLayers));

	if (Result.Status == EMaterialComplianceStatus::Pass)
	{
		AddResultMessage(Result, EMaterialComplianceStatus::Pass, TEXT("材质接口基础检查通过。"));
	}
	OutResults.Add(MoveTemp(Result));
}

void SMaterialTextureRulesWidget::UpdateResults(TArray<FMaterialComplianceResult>&& InResults, const FText& Summary)
{
	LastResults = MoveTemp(InResults);

	int32 PassCount = 0;
	int32 WarningCount = 0;
	int32 BlockedCount = 0;
	for (const FMaterialComplianceResult& Result : LastResults)
	{
		switch (Result.Status)
		{
		case EMaterialComplianceStatus::Pass:
			++PassCount;
			break;
		case EMaterialComplianceStatus::Warning:
			++WarningCount;
			break;
		case EMaterialComplianceStatus::Blocked:
			++BlockedCount;
			break;
		default:
			break;
		}
	}

	SetStatus(FText::Format(
		LOCTEXT("ResultSummary", "{0} 通过 {1}，警告 {2}，阻断 {3}。"),
		Summary,
		FText::AsNumber(PassCount),
		FText::AsNumber(WarningCount),
		FText::AsNumber(BlockedCount)));
}

#undef LOCTEXT_NAMESPACE
