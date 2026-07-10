#include "Tools/MapCreator/SDevKitMapCreatorWidget.h"

#include "ContentBrowserItemPath.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Tools/DevKitArtToolUI.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDevKitMapCreatorWidget"

void SDevKitMapCreatorWidget::Construct(const FArguments& InArgs)
{
	for (const FString& Layer : FDevKitMapCreatorService::GetDungeonLayers())
	{
		DungeonLayerOptions.Add(MakeShared<FString>(Layer));
	}
	for (const FString& LevelType : FDevKitMapCreatorService::GetLevelTypes())
	{
		LevelTypeOptions.Add(MakeShared<FString>(LevelType));
	}

	SelectedDungeonLayer = DungeonLayerOptions.Num() > 0 ? DungeonLayerOptions[0] : nullptr;
	SelectedLevelType = LevelTypeOptions.Num() > 0 ? LevelTypeOptions[0] : nullptr;
	StatusText = LOCTEXT("InitialStatus", "就绪。选择保存根目录，填写地图名称后缀，然后创建地图包。");

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
					DevKitArtToolUI::MakeHeader(
						LOCTEXT("Title", "地图创建器"),
						LOCTEXT("Description", "按项目规范创建命名文件夹、Persistent 关卡、Art/DataBake/Gameplay/Light/PLA 子关卡和 YogMapDefinition 数据资产。"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(1, LOCTEXT("FolderSection", "选择保存位置"), LOCTEXT("FolderSectionDesc", "目标目录必须位于 /Game；工具会在此目录下创建关卡专属文件夹。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RootFolderLabel", "保存根目录"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SAssignNew(RootFolderTextBox, SEditableTextBox)
						.Text(FText::FromString(FDevKitMapCreatorService::GetDefaultRootFolder()))
						.OnTextChanged(this, &SDevKitMapCreatorWidget::OnRootFolderChanged)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("UseContentBrowser", "使用内容浏览器目录"))
						.ToolTipText(LOCTEXT("UseContentBrowserTooltip", "把当前内容浏览器所在的 /Game 内部目录作为保存根目录。"))
						.OnClicked(this, &SDevKitMapCreatorWidget::UseContentBrowserFolder)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(2, LOCTEXT("NamingSection", "设置关卡命名"), LOCTEXT("NamingSectionDesc", "选择 L1/L2/L3 与关卡类型，并填写地图名称后缀。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 10.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(STextBlock).Text(LOCTEXT("DungeonLayerLabel", "地牢层级"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							BuildOptionCombo(
								DungeonLayerOptions,
								SelectedDungeonLayer,
								&SDevKitMapCreatorWidget::OnDungeonLayerChanged,
								&SDevKitMapCreatorWidget::GetSelectedDungeonLayerText,
								120.f)
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 10.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(STextBlock).Text(LOCTEXT("LevelTypeLabel", "关卡类型"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							BuildOptionCombo(
								LevelTypeOptions,
								SelectedLevelType,
								&SDevKitMapCreatorWidget::OnLevelTypeChanged,
								&SDevKitMapCreatorWidget::GetSelectedLevelTypeText,
								160.f)
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(STextBlock).Text(LOCTEXT("SuffixLabel", "地图名称后缀"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(SuffixTextBox, SEditableTextBox)
							.HintText(LOCTEXT("SuffixHint", "Corridor_S_Goth"))
							.OnTextChanged(this, &SDevKitMapCreatorWidget::OnSuffixChanged)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(3, LOCTEXT("CreateSection", "确认并创建"), LOCTEXT("CreateSectionDesc", "检查最终路径和模板映射；已有同名资产时工具会阻止覆盖。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 2.f, 0.f, 10.f)
				[
					SNew(SBorder)
					.Padding(10.f)
					.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 6.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PreviewTitle", "创建预览"))
							.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
						]
						+ SVerticalBox::Slot().AutoHeight()[MakePathRow(LOCTEXT("BaseNamePreview", "基础名称"), TAttribute<FText>(this, &SDevKitMapCreatorWidget::GetBaseNameText))]
						+ SVerticalBox::Slot().AutoHeight()[MakePathRow(LOCTEXT("TargetFolderPreview", "目标文件夹"), TAttribute<FText>(this, &SDevKitMapCreatorWidget::GetTargetFolderText))]
						+ SVerticalBox::Slot().AutoHeight()[MakePathRow(LOCTEXT("PersistentMapPreview", "Persistent 关卡"), TAttribute<FText>(this, &SDevKitMapCreatorWidget::GetPersistentMapText))]
						+ SVerticalBox::Slot().AutoHeight()[MakePathRow(LOCTEXT("MapDefinitionPreview", "地图数据"), TAttribute<FText>(this, &SDevKitMapCreatorWidget::GetMapDefinitionText))]
						+ SVerticalBox::Slot().AutoHeight()[MakePathRow(LOCTEXT("SublevelsPreview", "子关卡"), TAttribute<FText>(this, &SDevKitMapCreatorWidget::GetSublevelText))]
						+ SVerticalBox::Slot().AutoHeight()[MakePathRow(LOCTEXT("TemplatesPreview", "默认模板"), TAttribute<FText>(this, &SDevKitMapCreatorWidget::GetTemplateText))]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("CreateButton", "创建地图包"))
						.ToolTipText(LOCTEXT("CreateButtonTooltip", "创建所有地图资产，并保存到预览中的目标文件夹。"))
						.IsEnabled(this, &SDevKitMapCreatorWidget::CanCreate)
						.OnClicked(this, &SDevKitMapCreatorWidget::CreateMapPackage)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					DevKitArtToolUI::MakeStatus(
						TAttribute<FText>(this, &SDevKitMapCreatorWidget::GetStatusText),
						TAttribute<FSlateColor>(this, &SDevKitMapCreatorWidget::GetStatusColor))
				]
			]
		]
	];

	RefreshPreview();
}

TSharedRef<SWidget> SDevKitMapCreatorWidget::BuildOptionCombo(
	TArray<FStringOption>& Options,
	FStringOption& SelectedOption,
	void (SDevKitMapCreatorWidget::*SelectionHandler)(FStringOption, ESelectInfo::Type),
	FText (SDevKitMapCreatorWidget::*TextGetter)() const,
	float Width)
{
	return SNew(SBox)
		.WidthOverride(Width)
		[
			SNew(SComboBox<FStringOption>)
			.OptionsSource(&Options)
			.InitiallySelectedItem(SelectedOption)
			.OnGenerateWidget_Lambda([](FStringOption Item)
			{
				return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
			})
			.OnSelectionChanged(this, SelectionHandler)
			[
				SNew(STextBlock)
				.Text(this, TextGetter)
			]
		];
}

TSharedRef<SWidget> SDevKitMapCreatorWidget::MakePathRow(const FText& Label, TAttribute<FText> Value) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 8.f, 4.f)
		[
			SNew(SBox)
			.WidthOverride(120.f)
			[
				SNew(STextBlock)
				.Text(Label)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text(Value)
			.AutoWrapText(true)
		];
}

void SDevKitMapCreatorWidget::RefreshPreview()
{
	PreviewPaths = FDevKitMapCreatorService::BuildPaths(BuildRequest(), PreviewError);
	if (!PreviewPaths.IsSet() && !PreviewError.IsEmpty())
	{
		SetStatus(PreviewError, true);
	}
}

void SDevKitMapCreatorWidget::SetStatus(const FText& InStatus, bool bInIsError)
{
	StatusText = InStatus;
	bStatusIsError = bInIsError;
}

FDevKitMapCreatorRequest SDevKitMapCreatorWidget::BuildRequest() const
{
	return FDevKitMapCreatorRequest{
		RootFolderTextBox.IsValid() ? RootFolderTextBox->GetText().ToString() : FDevKitMapCreatorService::GetDefaultRootFolder(),
		SelectedDungeonLayer.IsValid() ? *SelectedDungeonLayer : FString(),
		SelectedLevelType.IsValid() ? *SelectedLevelType : FString(),
		SuffixTextBox.IsValid() ? SuffixTextBox->GetText().ToString() : FString()
	};
}

FText SDevKitMapCreatorWidget::GetSelectedDungeonLayerText() const
{
	return FText::FromString(SelectedDungeonLayer.IsValid() ? *SelectedDungeonLayer : FString());
}

FText SDevKitMapCreatorWidget::GetSelectedLevelTypeText() const
{
	return FText::FromString(SelectedLevelType.IsValid() ? *SelectedLevelType : FString());
}

FText SDevKitMapCreatorWidget::GetBaseNameText() const
{
	return PreviewPaths.IsSet() ? FText::FromString(PreviewPaths->BaseName) : LOCTEXT("InvalidPreview", "-");
}

FText SDevKitMapCreatorWidget::GetTargetFolderText() const
{
	return PreviewPaths.IsSet() ? FText::FromString(PreviewPaths->TargetFolder) : LOCTEXT("InvalidTargetFolder", "-");
}

FText SDevKitMapCreatorWidget::GetPersistentMapText() const
{
	return PreviewPaths.IsSet() ? FText::FromString(PreviewPaths->PersistentMap) : LOCTEXT("InvalidPersistentMap", "-");
}

FText SDevKitMapCreatorWidget::GetMapDefinitionText() const
{
	return PreviewPaths.IsSet() ? FText::FromString(PreviewPaths->MapDefinition) : LOCTEXT("InvalidMapDefinition", "-");
}

FText SDevKitMapCreatorWidget::GetSublevelText() const
{
	if (!PreviewPaths.IsSet())
	{
		return LOCTEXT("InvalidSublevels", "-");
	}
	return FText::FromString(FString::Join(PreviewPaths->Sublevels, TEXT("\n")));
}

FText SDevKitMapCreatorWidget::GetTemplateText() const
{
	TArray<FString> TemplateLines;
	TemplateLines.Add(FString::Printf(TEXT("Persistent -> %s"), *FDevKitMapCreatorService::GetPersistentTemplateMapPath()));
	const FString DungeonLayer = SelectedDungeonLayer.IsValid() ? *SelectedDungeonLayer : FString();
	for (const FString& Suffix : FDevKitMapCreatorService::GetSublevelSuffixes())
	{
		const FString TemplatePath = FDevKitMapCreatorService::GetSublevelTemplateMapPath(Suffix, DungeonLayer);
		if (!TemplatePath.IsEmpty())
		{
			TemplateLines.Add(FString::Printf(TEXT("%s -> %s"), *Suffix, *TemplatePath));
		}
	}

	return TemplateLines.IsEmpty()
		? LOCTEXT("NoTemplates", "无")
		: FText::FromString(FString::Join(TemplateLines, TEXT("\n")));
}

FText SDevKitMapCreatorWidget::GetStatusText() const
{
	return StatusText;
}

FSlateColor SDevKitMapCreatorWidget::GetStatusColor() const
{
	return bStatusIsError
		? FSlateColor(FLinearColor(0.9f, 0.2f, 0.15f))
		: FSlateColor::UseForeground();
}

bool SDevKitMapCreatorWidget::CanCreate() const
{
	return PreviewPaths.IsSet();
}

void SDevKitMapCreatorWidget::OnDungeonLayerChanged(FStringOption NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedDungeonLayer = NewSelection;
	bStatusIsError = false;
	RefreshPreview();
}

void SDevKitMapCreatorWidget::OnLevelTypeChanged(FStringOption NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedLevelType = NewSelection;
	bStatusIsError = false;
	RefreshPreview();
}

void SDevKitMapCreatorWidget::OnRootFolderChanged(const FText& NewText)
{
	bStatusIsError = false;
	RefreshPreview();
}

void SDevKitMapCreatorWidget::OnSuffixChanged(const FText& NewText)
{
	bStatusIsError = false;
	RefreshPreview();
}

FReply SDevKitMapCreatorWidget::UseContentBrowserFolder()
{
	IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
	const FContentBrowserItemPath CurrentPath = ContentBrowser.GetCurrentPath();
	if (CurrentPath.HasInternalPath())
	{
		RootFolderTextBox->SetText(FText::FromString(CurrentPath.GetInternalPathString()));
		SetStatus(LOCTEXT("UsedContentBrowserFolder", "已使用内容浏览器当前目录作为保存根目录。"), false);
	}
	else
	{
		SetStatus(LOCTEXT("NoContentBrowserFolder", "当前内容浏览器位置不是 /Game 内部目录。"), true);
	}
	RefreshPreview();
	return FReply::Handled();
}

FReply SDevKitMapCreatorWidget::CreateMapPackage()
{
	RefreshPreview();

	const FDevKitMapCreatorResult Result = FDevKitMapCreatorService::CreateLevelStack(BuildRequest());
	SetStatus(Result.Message, !Result.bSuccess);

	if (Result.bSuccess)
	{
		IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
		ContentBrowser.SyncBrowserToFolders({Result.Paths.TargetFolder}, false, true);
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
