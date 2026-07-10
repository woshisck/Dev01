#include "Tools/SEnvBatchTaggerWidget.h"

#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Tools/EnvBatchSourceTagRules.h"
#include "Tools/DevKitArtToolUI.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SEnvBatchTaggerWidget"

namespace
{
UWorld* GetEditorWorld()
{
	return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

bool IsEnvBatchSourceTag(const FName& Tag)
{
	return IsEnvBatchSourceTagString(Tag.ToString());
}

bool HasEnvBatchSourceTag(const AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	for (const FName& Tag : Actor->Tags)
	{
		if (IsEnvBatchSourceTag(Tag))
		{
			return true;
		}
	}

	return false;
}

bool ActorHasTagString(const AActor* Actor, const FString& TagString)
{
	return Actor && Actor->Tags.Contains(FName(*TagString));
}

FString ActorKindToToken(EEnvBatchActorKind InActorKind)
{
	switch (InActorKind)
	{
	case EEnvBatchActorKind::Building:
		return TEXT("Building");
	case EEnvBatchActorKind::Ground:
		return TEXT("Ground");
	case EEnvBatchActorKind::Prop:
	default:
		return TEXT("Prop");
	}
}
}

void SEnvBatchTaggerWidget::Construct(const FArguments& InArgs)
{
	RefreshSourceTagList();

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
						LOCTEXT("Title", "环境合批标记"),
						LOCTEXT("Description", "为场景 Actor 写入统一的 EnvBatch.Source.* 标记。此工具不创建 GameplayTags、不合并模型，也不修改材质。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(this, &SEnvBatchTaggerWidget::GetSelectionSummaryText)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(this, &SEnvBatchTaggerWidget::GetAssetReadinessSummaryText)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(1, LOCTEXT("BatchNameSection", "定义关卡与共享组"), LOCTEXT("BatchNameSectionDesc", "关卡名和共享贴图集合组用于归类同一批场景资产。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LevelLabel", "关卡/区域名"))
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
					SAssignNew(LevelNameTextBox, SEditableTextBox)
					.Text(FText::FromString(GetCurrentLevelName()))
					.HintText(LOCTEXT("LevelHint", "当前关卡名，或美术自定义的区域名"))
				]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("UseCurrentLevel", "获取当前关卡名"))
						.OnClicked(this, &SEnvBatchTaggerWidget::UseCurrentLevelName)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VTCGroupLabel", "共享贴图集合组"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(VTCGroupTextBox, SEditableTextBox)
					.Text(FText::FromString(GetDefaultEnvBatchVTCGroup()))
					.HintText(LOCTEXT("VTCGroupHint", "TC-A、TC-B... 同一组可跨多个流水号共享 Texture Collection"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(2, LOCTEXT("ClassificationSection", "设置分类、处理方式与流水号"), LOCTEXT("ClassificationSectionDesc", "对象类型、处理方式和手动填写的 01/02 流水号会写入 Tag，作为后续流程的筛选依据。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("KindLabel", "对象类型"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 16.f, 0.f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SEnvBatchTaggerWidget::IsActorKindChecked, EEnvBatchActorKind::Prop)
						.OnCheckStateChanged(this, &SEnvBatchTaggerWidget::SetActorKind, EEnvBatchActorKind::Prop)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("KindProp", "物件 Prop"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 16.f, 0.f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SEnvBatchTaggerWidget::IsActorKindChecked, EEnvBatchActorKind::Building)
						.OnCheckStateChanged(this, &SEnvBatchTaggerWidget::SetActorKind, EEnvBatchActorKind::Building)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("KindBuilding", "建筑 Building"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.IsChecked(this, &SEnvBatchTaggerWidget::IsActorKindChecked, EEnvBatchActorKind::Ground)
						.OnCheckStateChanged(this, &SEnvBatchTaggerWidget::SetActorKind, EEnvBatchActorKind::Ground)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("KindGround", "地面 Ground"))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ModeLabel", "处理方式"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 16.f, 0.f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SEnvBatchTaggerWidget::IsProcessingModeChecked, EEnvBatchProcessingMode::Instance)
						.OnCheckStateChanged(this, &SEnvBatchTaggerWidget::SetProcessingMode, EEnvBatchProcessingMode::Instance)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ModeInstance", "实例化 Instance"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.IsChecked(this, &SEnvBatchTaggerWidget::IsProcessingModeChecked, EEnvBatchProcessingMode::Batched)
						.OnCheckStateChanged(this, &SEnvBatchTaggerWidget::SetProcessingMode, EEnvBatchProcessingMode::Batched)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ModeBatched", "合批 Batched"))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SerialLabel", "流水号"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(SerialNumberTextBox, SEditableTextBox)
					.Text(LOCTEXT("DefaultSerial", "01"))
					.HintText(LOCTEXT("SerialHint", "手动填写：01、02、03..."))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitArtToolUI::MakeSectionHeader(3, LOCTEXT("WriteSection", "预览、写入与管理"), LOCTEXT("WriteSectionDesc", "确认预览 Tag 后写入当前选择；下方列表用于选择已有批次、回填参数或移除标记。"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(this, &SEnvBatchTaggerWidget::GetTagPreviewText)
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
						.Text(LOCTEXT("ApplySource", "写入 Source Tag"))
						.ToolTipText(LOCTEXT("ApplySourceTooltip", "移除选中 Actor 上已有的 EnvBatch.Source.*，再写入预览中的新 tag。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplySourceTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("RemoveSource", "移除 Source Tag"))
						.ToolTipText(LOCTEXT("RemoveSourceTooltip", "只移除选中 Actor 上的 EnvBatch.Source.*，其他 Actor Tags 保留。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::RemoveSourceTags)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("RefreshList", "刷新场景列表"))
						.ToolTipText(LOCTEXT("RefreshListTooltip", "重新扫描当前已加载编辑器世界里的 EnvBatch.Source.* Actor Tags。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::RefreshSourceTags)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("SelectListedActors", "选择列表对象"))
						.ToolTipText(LOCTEXT("SelectListedActorsTooltip", "选择当前列表行对应的所有 Actor。列表行单击或双击也会执行同样选择。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::SelectSelectedSourceTagActors)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("UseListedTag", "套用列表 Tag"))
						.ToolTipText(LOCTEXT("UseListedTagTooltip", "把当前列表行的关卡名、类型、方式和流水号回填到上方输入区。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::UseSelectedSourceTagAsTemplate)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SceneSourceTagListTitle", "当前场景 Source Tag 列表"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(this, &SEnvBatchTaggerWidget::GetSourceTagListSummaryText)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SBorder)
					.Padding(6.f)
					.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
					[
						SNew(SBox)
						.MinDesiredHeight(180.f)
						[
							SAssignNew(SourceTagListView, SListView<FSourceTagItemPtr>)
							.ListItemsSource(&SourceTagItems)
							.SelectionMode(ESelectionMode::Single)
							.OnGenerateRow(this, &SEnvBatchTaggerWidget::GenerateSourceTagRow)
							.OnSelectionChanged(this, &SEnvBatchTaggerWidget::OnSourceTagSelectionChanged)
							.OnMouseButtonDoubleClick(this, &SEnvBatchTaggerWidget::OnSourceTagDoubleClicked)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("EmptySourceTagHint", "当前场景还没有扫描到 EnvBatch.Source.*。先选择 Actor 写入 Source Tag，或确认目标子关卡/Data Layer 已加载；写入或切换关卡后点击“刷新场景列表”。"))
					.Visibility(this, &SEnvBatchTaggerWidget::GetEmptySourceTagHintVisibility)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("InitialStatus", "就绪。选择 Actor，设置类型和处理方式，然后写入 Source Tag。"))
					.AutoWrapText(true)
				]
			]
		]
	];
}

TArray<AActor*> SEnvBatchTaggerWidget::GetSelectedActors() const
{
	TArray<AActor*> Actors;
	if (!GEditor)
	{
		return Actors;
	}

	USelection* Selection = GEditor->GetSelectedActors();
	if (!Selection)
	{
		return Actors;
	}

	for (FSelectionIterator It(*Selection); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			Actors.Add(Actor);
		}
	}

	return Actors;
}

TArray<AActor*> SEnvBatchTaggerWidget::GetEditorWorldActors() const
{
	TArray<AActor*> Actors;
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return Actors;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (AActor* Actor = *It)
		{
			Actors.Add(Actor);
		}
	}

	return Actors;
}

FString SEnvBatchTaggerWidget::GetCurrentLevelName() const
{
	const UWorld* World = GetEditorWorld();
	if (!World)
	{
		return TEXT("Level");
	}

	const FString PackageName = World->GetOutermost() ? World->GetOutermost()->GetName() : World->GetMapName();
	return SanitizeEnvBatchTagToken(FPackageName::GetShortName(PackageName), TEXT("Level"));
}

FString SEnvBatchTaggerWidget::GetLevelName() const
{
	const FString LevelName = LevelNameTextBox.IsValid() ? LevelNameTextBox->GetText().ToString() : GetCurrentLevelName();
	return SanitizeEnvBatchTagToken(LevelName, TEXT("Level"));
}

FString SEnvBatchTaggerWidget::GetVTCGroup() const
{
	const FString VTCGroup = VTCGroupTextBox.IsValid() ? VTCGroupTextBox->GetText().ToString() : GetDefaultEnvBatchVTCGroup();
	return SanitizeEnvBatchTagToken(VTCGroup, GetDefaultEnvBatchVTCGroup());
}

int32 SEnvBatchTaggerWidget::GetSerialNumber() const
{
	const FString SerialText = SerialNumberTextBox.IsValid() ? SerialNumberTextBox->GetText().ToString() : FString(TEXT("1"));
	return FMath::Max(1, FCString::Atoi(*SerialText));
}

FString SEnvBatchTaggerWidget::BuildSourceTag() const
{
	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = GetLevelName();
	Spec.ActorKind = ActorKindToToken(ActorKind);
	Spec.ProcessingMode = ProcessingMode == EEnvBatchProcessingMode::Instance ? TEXT("Instance") : TEXT("Batched");
	Spec.VTCGroup = GetVTCGroup();
	Spec.SerialNumber = GetSerialNumber();
	Spec.bHasExplicitVTCGroup = true;
	return BuildEnvBatchSourceTag(Spec);
}

FString SEnvBatchTaggerWidget::BuildSourceTagPrefix() const
{
	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = GetLevelName();
	Spec.ActorKind = ActorKindToToken(ActorKind);
	Spec.ProcessingMode = ProcessingMode == EEnvBatchProcessingMode::Instance ? TEXT("Instance") : TEXT("Batched");
	Spec.VTCGroup = GetVTCGroup();
	Spec.bHasExplicitVTCGroup = true;
	return BuildEnvBatchSourceTagPrefix(Spec);
}

FText SEnvBatchTaggerWidget::GetTagPreviewText() const
{
	return FText::Format(LOCTEXT("TagPreview", "预览：{0}"), FText::FromString(BuildSourceTag()));
}

FText SEnvBatchTaggerWidget::GetSourceTagListSummaryText() const
{
	if (SourceTagItems.IsEmpty())
	{
		return FText::Format(
			LOCTEXT("SourceTagListSummaryEmpty", "已扫描 {0} 个当前已加载 Actor，Source Tag 组为 0。"),
			FText::AsNumber(LastScannedActorCount));
	}

	return FText::Format(
		LOCTEXT("SourceTagListSummary", "已扫描 {0} 个当前已加载 Actor，找到 {1} 个 Source Tag 组。单击列表行会选中带该 Tag 的 Actor，并把该 Tag 回填到上方输入区。"),
		FText::AsNumber(LastScannedActorCount),
		FText::AsNumber(SourceTagItems.Num()));
}

EVisibility SEnvBatchTaggerWidget::GetEmptySourceTagHintVisibility() const
{
	return SourceTagItems.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SEnvBatchTaggerWidget::ApplySourceTag()
{
	const TArray<AActor*> Actors = GetSelectedActors();
	if (Actors.IsEmpty())
	{
		SetStatus(LOCTEXT("NoSelectionApply", "当前没有选中 Actor。"));
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ApplyEnvBatchSourceTagTransaction", "写入 EnvBatch Source Tag"));
	const FString SourceTag = BuildSourceTag();
	int32 ChangedCount = 0;
	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}

		Actor->Modify();
		Actor->Tags.RemoveAll([](const FName& ExistingTag)
		{
			return IsEnvBatchSourceTag(ExistingTag);
		});
		Actor->Tags.AddUnique(FName(*SourceTag));
		Actor->MarkPackageDirty();
		++ChangedCount;
	}

	RefreshSourceTagList();
	SetStatus(FText::Format(
		LOCTEXT("AppliedStatus", "已将 {0} 写入 {1} 个选中 Actor。完成后请保存对应关卡或子关卡。"),
		FText::FromString(SourceTag),
		FText::AsNumber(ChangedCount)));
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::RemoveSourceTags()
{
	const TArray<AActor*> Actors = GetSelectedActors();
	if (Actors.IsEmpty())
	{
		SetStatus(LOCTEXT("NoSelectionRemove", "当前没有选中 Actor。"));
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("RemoveEnvBatchSourceTagTransaction", "移除 EnvBatch Source Tag"));
	int32 ChangedCount = 0;
	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}

		Actor->Modify();
		const int32 OldTagCount = Actor->Tags.Num();
		Actor->Tags.RemoveAll([](const FName& ExistingTag)
		{
			return IsEnvBatchSourceTag(ExistingTag);
		});

		if (Actor->Tags.Num() != OldTagCount)
		{
			Actor->MarkPackageDirty();
			++ChangedCount;
		}
	}

	RefreshSourceTagList();
	SetStatus(FText::Format(
		LOCTEXT("RemovedStatus", "已从 {0} 个选中 Actor 移除 EnvBatch.Source.*。"),
		FText::AsNumber(ChangedCount)));
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::RefreshSourceTags()
{
	RefreshSourceTagList();
	if (SourceTagItems.IsEmpty())
	{
		SetStatus(FText::Format(
			LOCTEXT("RefreshNoSourceTags", "刷新完成：扫描 {0} 个 Actor，当前已加载编辑器世界里没有 EnvBatch.Source.*。如果刚写入过，请确认目标关卡、子关卡或 Data Layer 已加载。"),
			FText::AsNumber(LastScannedActorCount)));
	}
	else
	{
		SetStatus(FText::Format(
			LOCTEXT("RefreshSourceTagsStatus", "刷新完成：扫描 {0} 个 Actor，找到 {1} 个 Source Tag 组。点选列表行可以在关卡中选中对应 Actor。"),
			FText::AsNumber(LastScannedActorCount),
			FText::AsNumber(SourceTagItems.Num())));
	}
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::SelectSelectedSourceTagActors()
{
	FSourceTagItemPtr Item = SelectedSourceTagItem;
	if (!Item.IsValid() && SourceTagListView.IsValid())
	{
		const TArray<FSourceTagItemPtr> SelectedItems = SourceTagListView->GetSelectedItems();
		Item = SelectedItems.IsEmpty() ? nullptr : SelectedItems[0];
	}

	if (!Item.IsValid())
	{
		SetStatus(LOCTEXT("NoSourceTagRowSelected", "请先在 Source Tag 列表中选择一行。"));
		return FReply::Handled();
	}

	ApplySourceTagToControls(Item->Tag);
	SelectActorsWithSourceTag(Item->Tag);
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::UseSelectedSourceTagAsTemplate()
{
	FSourceTagItemPtr Item = SelectedSourceTagItem;
	if (!Item.IsValid() && SourceTagListView.IsValid())
	{
		const TArray<FSourceTagItemPtr> SelectedItems = SourceTagListView->GetSelectedItems();
		Item = SelectedItems.IsEmpty() ? nullptr : SelectedItems[0];
	}

	if (!Item.IsValid())
	{
		SetStatus(LOCTEXT("NoTemplateSourceTagRowSelected", "请先在 Source Tag 列表中选择一行。"));
		return FReply::Handled();
	}

	ApplySourceTagToControls(Item->Tag);
	SetStatus(FText::Format(
		LOCTEXT("SourceTagAppliedToControls", "已将 {0} 回填到上方输入区。"),
		FText::FromString(Item->Tag)));
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::UseCurrentLevelName()
{
	if (LevelNameTextBox.IsValid())
	{
		LevelNameTextBox->SetText(FText::FromString(GetCurrentLevelName()));
	}
	return FReply::Handled();
}

void SEnvBatchTaggerWidget::RefreshSourceTagList()
{
	TMap<FString, int32> TagCounts;
	LastScannedActorCount = 0;

	for (const AActor* Actor : GetEditorWorldActors())
	{
		if (!Actor)
		{
			continue;
		}

		++LastScannedActorCount;

		for (const FName& ActorTag : Actor->Tags)
		{
			if (IsEnvBatchSourceTag(ActorTag))
			{
				TagCounts.FindOrAdd(ActorTag.ToString())++;
			}
		}
	}

	TagCounts.KeySort([](const FString& A, const FString& B)
	{
		return A < B;
	});

	SourceTagItems.Reset();
	for (const TPair<FString, int32>& TagCount : TagCounts)
	{
		FSourceTagItemPtr Item = MakeShared<FEnvBatchSourceTagListItem>();
		Item->Tag = TagCount.Key;
		Item->ActorCount = TagCount.Value;
		SourceTagItems.Add(Item);
	}

	if (SourceTagListView.IsValid())
	{
		SourceTagListView->RequestListRefresh();
	}
}

void SEnvBatchTaggerWidget::SelectActorsWithSourceTag(const FString& SourceTag) const
{
	if (!GEditor)
	{
		return;
	}

	GEditor->SelectNone(false, true, false);

	int32 SelectedCount = 0;
	for (AActor* Actor : GetEditorWorldActors())
	{
		if (ActorHasTagString(Actor, SourceTag))
		{
			GEditor->SelectActor(Actor, true, false);
			++SelectedCount;
		}
	}

	GEditor->NoteSelectionChange();
	SetStatus(FText::Format(
		LOCTEXT("SelectedByTag", "已选中 {0} 个带有 {1} 的 Actor。"),
		FText::AsNumber(SelectedCount),
		FText::FromString(SourceTag)));
}

void SEnvBatchTaggerWidget::ApplySourceTagToControls(const FString& SourceTag)
{
	FEnvBatchSourceTagSpec Spec;
	if (!ParseEnvBatchSourceTag(SourceTag, Spec))
	{
		return;
	}

	if (LevelNameTextBox.IsValid())
	{
		LevelNameTextBox->SetText(FText::FromString(Spec.LevelName));
	}

	if (Spec.ActorKind == TEXT("Prop"))
	{
		ActorKind = EEnvBatchActorKind::Prop;
	}
	else if (Spec.ActorKind == TEXT("Building"))
	{
		ActorKind = EEnvBatchActorKind::Building;
	}
	else if (Spec.ActorKind == TEXT("Ground"))
	{
		ActorKind = EEnvBatchActorKind::Ground;
	}

	if (Spec.ProcessingMode == TEXT("Instance"))
	{
		ProcessingMode = EEnvBatchProcessingMode::Instance;
	}
	else if (Spec.ProcessingMode == TEXT("Batched"))
	{
		ProcessingMode = EEnvBatchProcessingMode::Batched;
	}

	if (VTCGroupTextBox.IsValid())
	{
		VTCGroupTextBox->SetText(FText::FromString(Spec.VTCGroup));
	}

	if (SerialNumberTextBox.IsValid())
	{
		SerialNumberTextBox->SetText(FText::FromString(FString::Printf(TEXT("%02d"), Spec.SerialNumber)));
	}
}

void SEnvBatchTaggerWidget::SetActorKind(ECheckBoxState NewState, EEnvBatchActorKind InActorKind)
{
	if (NewState == ECheckBoxState::Checked)
	{
		ActorKind = InActorKind;
	}
}

void SEnvBatchTaggerWidget::SetProcessingMode(ECheckBoxState NewState, EEnvBatchProcessingMode InProcessingMode)
{
	if (NewState == ECheckBoxState::Checked)
	{
		ProcessingMode = InProcessingMode;
	}
}

ECheckBoxState SEnvBatchTaggerWidget::IsActorKindChecked(EEnvBatchActorKind InActorKind) const
{
	return ActorKind == InActorKind ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SEnvBatchTaggerWidget::IsProcessingModeChecked(EEnvBatchProcessingMode InProcessingMode) const
{
	return ProcessingMode == InProcessingMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

TSharedRef<ITableRow> SEnvBatchTaggerWidget::GenerateSourceTagRow(FSourceTagItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(STableRow<FSourceTagItemPtr>, OwnerTable)
	.Padding(4.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SNew(STextBlock)
			.Text(Item.IsValid() ? FText::FromString(Item->Tag) : LOCTEXT("InvalidSourceTagRow", "<invalid>"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(12.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Item.IsValid()
				? FText::Format(LOCTEXT("SourceTagActorCount", "{0} 个 Actor"), FText::AsNumber(Item->ActorCount))
				: FText::GetEmpty())
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
	];
}

void SEnvBatchTaggerWidget::OnSourceTagSelectionChanged(FSourceTagItemPtr Item, ESelectInfo::Type SelectInfo)
{
	SelectedSourceTagItem = Item;
	if (Item.IsValid())
	{
		ApplySourceTagToControls(Item->Tag);
	}

	if (Item.IsValid() && SelectInfo != ESelectInfo::Direct)
	{
		SelectActorsWithSourceTag(Item->Tag);
	}
}

void SEnvBatchTaggerWidget::OnSourceTagDoubleClicked(FSourceTagItemPtr Item)
{
	if (Item.IsValid())
	{
		SelectedSourceTagItem = Item;
		ApplySourceTagToControls(Item->Tag);
		SelectActorsWithSourceTag(Item->Tag);
	}
}

void SEnvBatchTaggerWidget::SetStatus(const FText& InStatus) const
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(InStatus);
	}
}

FText SEnvBatchTaggerWidget::GetSelectionSummaryText() const
{
	return FText::Format(
		LOCTEXT("SelectionSummary", "当前选中 Actor 数量：{0}"),
		FText::AsNumber(GetSelectedActors().Num()));
}

FText SEnvBatchTaggerWidget::GetAssetReadinessSummaryText() const
{
	const TArray<AActor*> Actors = GetSelectedActors();
	if (Actors.IsEmpty())
	{
		return LOCTEXT("AssetReadinessNoSelection", "使用方式：输入或获取关卡名，选择 Prop/Building/Ground、Instance/Batched 和共享贴图集合组，再给选中的 Actor 写入 Source Tag。地面合批请使用 Ground.Batched，合批时会尝试保留关卡 Mesh Paint 顶点色。");
	}

	int32 SourceCount = 0;
	int32 UntaggedCount = 0;
	for (const AActor* Actor : Actors)
	{
		if (HasEnvBatchSourceTag(Actor))
		{
			++SourceCount;
		}
		else
		{
			++UntaggedCount;
		}
	}

	return FText::Format(
		LOCTEXT("AssetReadinessSummary", "选择诊断：已有 Source Tag {0} 个，未打 Source Tag {1} 个。写入时只替换每个 Actor 上已有的 EnvBatch.Source.*。"),
		FText::AsNumber(SourceCount),
		FText::AsNumber(UntaggedCount));
}

#undef LOCTEXT_NAMESPACE
