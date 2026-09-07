#include "Tools/SMaterialTextureWorkbenchWidget.h"

#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Tools/SMaterialTextureRulesWidget.h"
#include "Tools/SRVTMaterialManagerWidget.h"
#include "Tools/STextureVTAuditWidget.h"
#include "Tools/SVirtualTextureCollectionManagerWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitMaterialTextureWorkbench"

namespace
{
	const FSlateStyleSet& GetMaterialWorkbenchRichTextStyle()
	{
		static FSlateStyleSet Style(TEXT("DevKitMaterialWorkbenchRichText"));
		static bool bInitialized = false;
		if (!bInitialized)
		{
			const FTextBlockStyle Base = FAppStyle::GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText"));
			Style.Set(TEXT("Default"), Base);
			Style.Set(TEXT("title"), FTextBlockStyle(Base).SetFont(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall"))).SetColorAndOpacity(FLinearColor(0.82f, 0.90f, 1.0f)));
			Style.Set(TEXT("label"), FTextBlockStyle(Base).SetFont(FAppStyle::GetFontStyle(TEXT("NormalFontBold"))).SetColorAndOpacity(FLinearColor(0.55f, 0.82f, 1.0f)));
			Style.Set(TEXT("good"), FTextBlockStyle(Base).SetColorAndOpacity(FLinearColor(0.35f, 0.86f, 0.48f)));
			Style.Set(TEXT("warn"), FTextBlockStyle(Base).SetColorAndOpacity(FLinearColor(1.0f, 0.68f, 0.24f)));
			Style.Set(TEXT("path"), FTextBlockStyle(Base).SetColorAndOpacity(FLinearColor(0.65f, 0.72f, 0.82f)));
			bInitialized = true;
		}
		return Style;
	}
}

void SMaterialTextureWorkbenchWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.Padding(FMargin(12.0f, 8.0f))
			.BorderBackgroundColor(FLinearColor(0.025f, 0.042f, 0.058f, 1.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 1.0f, 10.0f, 1.0f)
				[
					SNew(SBox).WidthOverride(4.0f)
					[
						SNew(SBorder).BorderBackgroundColor(FLinearColor(0.05f, 0.62f, 0.82f, 1.0f))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Title", "材质与贴图工作台"))
							.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SBorder)
							.Padding(FMargin(7.0f, 2.0f))
							.BorderBackgroundColor(FLinearColor(0.03f, 0.28f, 0.42f, 1.0f))
							[
								SNew(STextBlock)
								.Text(this, &SMaterialTextureWorkbenchWidget::GetPageTitle)
								.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SMaterialTextureWorkbenchWidget::GetPageSummary)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.AutoWrapText(true)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SComboButton)
					.HasDownArrow(false)
					.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
					.ContentPadding(FMargin(8.0f, 5.0f))
					.ToolTipText(LOCTEXT("HelpToolTip", "打开当前工具的完整使用说明；说明以浮层显示，不占用工作区。"))
					.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
					{
						return BuildHelpPanel();
					})
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush(TEXT("Icons.Info")))
							.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(LOCTEXT("HelpButton", "使用说明"))
						]
					]
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SSplitter).PhysicalSplitterHandleSize(3.0f)
			+ SSplitter::Slot().Value(0.14f).MinSize(185.0f)[BuildNavigation()]
			+ SSplitter::Slot().Value(0.86f).MinSize(500.0f)
			[
				SAssignNew(ToolSwitcher, SWidgetSwitcher)
				.WidgetIndex(0)
				+ SWidgetSwitcher::Slot()[SNew(SRVTMaterialManagerWidget)]
				+ SWidgetSwitcher::Slot()[SNew(SMaterialTextureRulesWidget)]
				+ SWidgetSwitcher::Slot()[SNew(STextureVTAuditWidget)]
				+ SWidgetSwitcher::Slot()[SNew(SVirtualTextureCollectionManagerWidget)]
			]
		]
	];
}

TSharedRef<SWidget> SMaterialTextureWorkbenchWidget::BuildNavigation()
{
	return SNew(SBorder).Padding(10.0f).BorderBackgroundColor(FLinearColor(0.028f, 0.028f, 0.028f, 1.0f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 3.0f, 2.0f, 10.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("NavigationTitle", "美术材质工具")).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
		]
		+ SVerticalBox::Slot().AutoHeight()[BuildNavigationButton(EPage::RVTLibrary, LOCTEXT("RVTPage", "RVT 层材质库"), LOCTEXT("RVTPageSummary", "MLI、变体、贴图导入与动态层预设"))]
		+ SVerticalBox::Slot().AutoHeight()[BuildNavigationButton(EPage::MaterialCompliance, LOCTEXT("CompliancePage", "材质合规检查"), LOCTEXT("CompliancePageSummary", "母材、参数和性能规则"))]
		+ SVerticalBox::Slot().AutoHeight()[BuildNavigationButton(EPage::TextureAudit, LOCTEXT("AuditPage", "贴图 NoVT 审计"), LOCTEXT("AuditPageSummary", "普通贴图与 RVT 使用边界"))]
		+ SVerticalBox::Slot().AutoHeight()[BuildNavigationButton(EPage::TextureCollection, LOCTEXT("CollectionPage", "Texture Collection"), LOCTEXT("CollectionPageSummary", "Bindless 集合、索引和格式"))]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 12.0f, 2.0f, 2.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("LibraryRoot", "材质库根目录\n/Game/Art/Texture/CommonTex")).ColorAndOpacity(FSlateColor::UseSubduedForeground()).AutoWrapText(true)
		]
	];
}

TSharedRef<SWidget> SMaterialTextureWorkbenchWidget::BuildNavigationButton(const EPage Page, const FText& Title, const FText& Summary)
{
	return SNew(SButton).ButtonColorAndOpacity_Lambda([this, Page]() { return GetNavigationColor(Page); }).OnClicked(this, &SMaterialTextureWorkbenchWidget::SelectPage, Page)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(Summary).ColorAndOpacity(FSlateColor::UseSubduedForeground()).AutoWrapText(true)]
	];
}

TSharedRef<SWidget> SMaterialTextureWorkbenchWidget::BuildHelpPanel()
{
	const FSlateStyleSet& Style = GetMaterialWorkbenchRichTextStyle();
	return SNew(SBox)
		.WidthOverride(520.0f)
		.MaxDesiredHeight(500.0f)
	[
		SNew(SBorder)
		.Padding(14.0f)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.BorderBackgroundColor(FLinearColor(0.018f, 0.022f, 0.028f, 1.0f))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(this, &SMaterialTextureWorkbenchWidget::GetPageHelpRichText)
				.TextStyle(&Style.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
				.DecoratorStyleSet(&Style)
				.AutoWrapText(true)
			]
		]
	];
}

FReply SMaterialTextureWorkbenchWidget::SelectPage(const EPage Page)
{
	ActivePage = Page;
	if (ToolSwitcher.IsValid())
	{
		ToolSwitcher->SetActiveWidgetIndex(static_cast<int32>(Page));
	}
	return FReply::Handled();
}

FText SMaterialTextureWorkbenchWidget::GetPageTitle() const
{
	switch (ActivePage)
	{
	case EPage::RVTLibrary: return LOCTEXT("RVTTitle", "RVT 层材质库");
	case EPage::MaterialCompliance: return LOCTEXT("ComplianceTitle", "材质合规检查");
	case EPage::TextureAudit: return LOCTEXT("AuditTitle", "贴图 NoVT 审计");
	default: return LOCTEXT("CollectionTitle", "Texture Collection");
	}
}

FText SMaterialTextureWorkbenchWidget::GetPageSummary() const
{
	switch (ActivePage)
	{
	case EPage::RVTLibrary:
		return LOCTEXT("RVTSummary", "按分类浏览和拖拽 MLI；双击打开，Ctrl+B 定位。蓝色为基础材质，浅黄色表示已有变体。");
	case EPage::MaterialCompliance:
		return LOCTEXT("ComplianceSummary", "检查母材、参数、纹理类型和性能分级，并逐项处理阻断与警告。");
	case EPage::TextureAudit:
		return LOCTEXT("AuditSummary", "审计普通 Texture2D 与 RVT 的使用边界，以及 sRGB、压缩、分辨率和显存风险。");
	default:
		return LOCTEXT("CollectionSummary", "维护 Bindless 纹理集合的索引、顺序与格式合同，避免三套 Collection 错位。");
	}
}

FText SMaterialTextureWorkbenchWidget::GetPageHelpRichText() const
{
	switch (ActivePage)
	{
	case EPage::RVTLibrary:
		return LOCTEXT("RVTHelp", "<title>RVT 层材质库</>\n\n<label>适用对象</>\n静态网格地面与需要写入/采样地面 RVT 的层材质。\n\n<label>一组材质</>\n<path>BaseColor + MRAH + NormalLight</> 对应一个可拖拽的 MLI。\n\n<label>变体规则</>\n<good>蓝框</>：没有变体。\n<warn>浅黄框</>：存在 Damage、脏污或颜色变体。\n\n<label>层预设</>\nBackground 必填；普通层可以自由新增、删除和上下排序。每层默认使用 <path>MLBI_BasicMatMask_Base</>。\n\n<warn>注意</>\n创建并赋予只会标脏关卡，不会自动保存地图。");
	case EPage::MaterialCompliance:
		return LOCTEXT("ComplianceHelp", "<title>材质合规检查</>\n\n<label>用途</>\n检查项目材质是否遵守母材、参数、纹理类型和性能分级约定。\n\n<good>通过</> 表示当前规则没有阻断项。\n<warn>警告</> 需要美术判断是否属于允许的特例。\n\n<label>建议顺序</>\n先选择资产范围，再运行检查，最后逐项处理右侧说明；不要用批量修复覆盖人工制作的特殊材质。");
	case EPage::TextureAudit:
		return LOCTEXT("AuditHelp", "<title>贴图 NoVT 审计</>\n\n<label>项目边界</>\n普通建筑、物件和角色贴图保持普通 Texture2D；只有地面 RVT 数据使用虚拟纹理。\n\n<good>NoVT</>：普通贴图的预期状态。\n<warn>Virtual Texture</>：只有明确属于 RVT 流程时才允许。\n\n<label>检查重点</>\nsRGB、Compression、分辨率、估算显存和误开启 VT 的资产。");
	default:
		return LOCTEXT("CollectionHelp", "<title>Texture Collection</>\n\n<label>用途</>\n维护建筑和物件材质的 Bindless 纹理集合。三套 Collection 必须使用相同索引指向同一材质组。\n\n<label>索引合同</>\n<path>BaseColor[i] + MRAH[i] + NormalLight[i]</> 必须成组。\n\n<warn>高风险操作</>\n插入、删除或重排成员会改变后续索引。修改后必须重新验证数量、顺序、压缩格式和所有 PerInstance Custom Data。");
	}
}

FSlateColor SMaterialTextureWorkbenchWidget::GetNavigationColor(const EPage Page) const
{
	return Page == ActivePage ? FSlateColor(FLinearColor(0.03f, 0.28f, 0.42f, 1.0f)) : FSlateColor(FLinearColor(0.10f, 0.10f, 0.10f, 1.0f));
}

#undef LOCTEXT_NAMESPACE
