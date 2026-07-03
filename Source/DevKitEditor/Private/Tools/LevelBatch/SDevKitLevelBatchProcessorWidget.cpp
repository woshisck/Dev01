#include "Tools/LevelBatch/SDevKitLevelBatchProcessorWidget.h"

#include "Editor.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "Misc/PackageName.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SDevKitLevelBatchProcessorWidget"

namespace
{
	FString PackageFolderToFilename(const FString& PackageFolder)
	{
		return FPackageName::LongPackageNameToFilename(PackageFolder);
	}
}

void SDevKitLevelBatchProcessorWidget::Construct(const FArguments& InArgs)
{
	RefreshLevelItems();

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
					.Text(LOCTEXT("Title", "关卡合批处理"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT(
						"Description",
						"扫描 /Game/Art/Map/Map_Data 下的主关卡文件夹。执行合批会把 MaterialBatchBuild 的生成资产输出到该关卡的 BatchedAsset，并把状态写入 EnvBatchStatus.json；清理合批只恢复源 Actor 显示，不删除已生成 Actor、_Batched 子关卡或 BatchedAsset 内容。"))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(LOCTEXT("RootFolderLabel", "主关卡根目录"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SAssignNew(RootFolderTextBox, SEditableTextBox)
					.Text(FText::FromString(FDevKitLevelBatchService::GetDefaultMapDataRoot()))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(LOCTEXT("TierLabel", "合批生成档位"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(TierTextBox, SEditableTextBox)
					.Text(LOCTEXT("DefaultTier", "Mid"))
					.HintText(LOCTEXT("TierHint", "Epic / High / Mid / Low"))
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
						.Text(LOCTEXT("Refresh", "刷新列表"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::RefreshLevels)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("BatchCurrent", "合批当前关卡"))
						.ToolTipText(LOCTEXT("BatchCurrentTooltip", "根据当前打开的关卡定位 Map_Data 文件夹，并启动生成资产命令。"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::BatchCurrentLevel)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("BatchSelected", "合批选中关卡"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::BatchSelectedLevels)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("BatchAll", "批量全部合批"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::BatchAllLevels)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("CleanSelected", "清理选中合批"))
						.ToolTipText(LOCTEXT("CleanSelectedTooltip", "只恢复当前已加载世界中该关卡 Source Actor 的显示状态，并把状态退回未合批；生成物保留。"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::CleanSelectedLevels)
					]
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
						.Text(LOCTEXT("MarkArtReviewed", "标记已审查"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::MarkSelectedArtReviewed)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("MarkPerformanceApproved", "标记已通过"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::MarkSelectedPerformanceApproved)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("OpenBatchedAsset", "打开 BatchedAsset"))
						.OnClicked(this, &SDevKitLevelBatchProcessorWidget::OpenSelectedBatchedAssetFolder)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitLevelBatchProcessorWidget::GetListSummaryText)
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
						.MinDesiredHeight(320.f)
						[
							SAssignNew(LevelListView, SListView<FLevelBatchItemPtr>)
							.ListItemsSource(&LevelItems)
							.SelectionMode(ESelectionMode::Multi)
							.OnGenerateRow(this, &SDevKitLevelBatchProcessorWidget::GenerateLevelRow)
							.HeaderRow
							(
								SNew(SHeaderRow)
								+ SHeaderRow::Column(TEXT("Level")).DefaultLabel(LOCTEXT("LevelColumn", "关卡文件夹")).FillWidth(0.35f)
								+ SHeaderRow::Column(TEXT("Status")).DefaultLabel(LOCTEXT("StatusColumn", "状态")).FillWidth(0.14f)
								+ SHeaderRow::Column(TEXT("Assets")).DefaultLabel(LOCTEXT("AssetsColumn", "生成物")).FillWidth(0.12f)
								+ SHeaderRow::Column(TEXT("Paths")).DefaultLabel(LOCTEXT("PathsColumn", "路径")).FillWidth(0.39f)
							)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("InitialStatus", "就绪。选择关卡后可以合批、清理或更新审查状态。"))
					.AutoWrapText(true)
				]
			]
		]
	];

	RefreshLevelItems();
}

FReply SDevKitLevelBatchProcessorWidget::RefreshLevels()
{
	RefreshLevelItems();
	SetStatus(LOCTEXT("RefreshDone", "已刷新关卡合批状态。"));
	return FReply::Handled();
}

FReply SDevKitLevelBatchProcessorWidget::BatchCurrentLevel()
{
	if (!GEditor)
	{
		SetStatus(LOCTEXT("NoEditor", "当前没有可用编辑器实例。"));
		return FReply::Handled();
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	const FString MapPackageName = World && World->GetOutermost() ? World->GetOutermost()->GetName() : FString();
	const TOptional<FDevKitLevelBatchStatus> Status = FDevKitLevelBatchService::FindLevelForMapPackage(MapPackageName, GetRootPackagePath());
	if (!Status.IsSet())
	{
		SetStatus(FText::Format(LOCTEXT("CurrentLevelNotFound", "当前关卡不在主关卡根目录中：{0}"), FText::FromString(MapPackageName)));
		return FReply::Handled();
	}

	BatchItems({MakeShared<FDevKitLevelBatchStatus>(Status.GetValue())});
	return FReply::Handled();
}

FReply SDevKitLevelBatchProcessorWidget::BatchSelectedLevels()
{
	BatchItems(GetSelectedItems());
	return FReply::Handled();
}

FReply SDevKitLevelBatchProcessorWidget::BatchAllLevels()
{
	BatchItems(LevelItems);
	return FReply::Handled();
}

FReply SDevKitLevelBatchProcessorWidget::CleanSelectedLevels()
{
	const TArray<FLevelBatchItemPtr> SelectedItems = GetSelectedItems();
	if (SelectedItems.IsEmpty())
	{
		SetStatus(LOCTEXT("NoCleanSelection", "请先选择要清理的关卡。"));
		return FReply::Handled();
	}

	int32 RestoredActorTotal = 0;
	TArray<FString> Messages;
	for (const FLevelBatchItemPtr& Item : SelectedItems)
	{
		if (!Item.IsValid())
		{
			continue;
		}

		int32 RestoredActorCount = 0;
		FString ActorMessage;
		FDevKitLevelBatchService::RestoreSourceActorsForLoadedWorld(Item->Paths.LevelName, RestoredActorCount, ActorMessage);
		RestoredActorTotal += RestoredActorCount;

		FString Error;
		FDevKitLevelBatchService::WriteReviewStatus(Item->Paths, EDevKitLevelBatchReviewStatus::NotBatched, Error);
		Messages.Add(ActorMessage);
	}

	RefreshLevelItems();
	SetStatus(FText::Format(
		LOCTEXT("CleanDone", "清理完成：恢复源 Actor {0} 个。已保留生成 Actor、_Batched 子关卡和 BatchedAsset 内容。"),
		FText::AsNumber(RestoredActorTotal)));
	return FReply::Handled();
}

FReply SDevKitLevelBatchProcessorWidget::MarkSelectedArtReviewed()
{
	MarkSelectedLevels(EDevKitLevelBatchReviewStatus::ArtReviewed);
	return FReply::Handled();
}

FReply SDevKitLevelBatchProcessorWidget::MarkSelectedPerformanceApproved()
{
	MarkSelectedLevels(EDevKitLevelBatchReviewStatus::PerformanceApproved);
	return FReply::Handled();
}

FReply SDevKitLevelBatchProcessorWidget::OpenSelectedBatchedAssetFolder()
{
	const TArray<FLevelBatchItemPtr> SelectedItems = GetSelectedItems();
	if (SelectedItems.IsEmpty() || !SelectedItems[0].IsValid())
	{
		SetStatus(LOCTEXT("NoOpenSelection", "请先选择一个关卡。"));
		return FReply::Handled();
	}

	const FString FolderPath = PackageFolderToFilename(SelectedItems[0]->Paths.BatchedAssetFolder);
	FPlatformProcess::ExploreFolder(*FolderPath);
	SetStatus(FText::Format(LOCTEXT("OpenedBatchedAsset", "已请求打开目录：{0}"), FText::FromString(FolderPath)));
	return FReply::Handled();
}

void SDevKitLevelBatchProcessorWidget::RefreshLevelItems()
{
	LevelItems.Reset();
	for (const FDevKitLevelBatchStatus& Status : FDevKitLevelBatchService::ScanLevelFolders(GetRootPackagePath()))
	{
		LevelItems.Add(MakeShared<FDevKitLevelBatchStatus>(Status));
	}

	if (LevelListView.IsValid())
	{
		LevelListView->RequestListRefresh();
	}
}

void SDevKitLevelBatchProcessorWidget::BatchItems(const TArray<FLevelBatchItemPtr>& Items)
{
	if (Items.IsEmpty())
	{
		SetStatus(LOCTEXT("NoBatchSelection", "没有可合批的关卡。"));
		return;
	}

	int32 StartedCount = 0;
	int32 HiddenActorTotal = 0;
	TArray<FString> Failures;
	for (const FLevelBatchItemPtr& Item : Items)
	{
		if (!Item.IsValid())
		{
			continue;
		}

		FString Message;
		if (!FDevKitLevelBatchService::LaunchPartialApplyCommand(Item->Paths, GetTierName(), Message))
		{
			Failures.Add(Message);
			continue;
		}

		++StartedCount;
		FString Error;
		FDevKitLevelBatchService::WriteReviewStatus(Item->Paths, EDevKitLevelBatchReviewStatus::PendingReview, Error);

		if (IsCurrentEditorMapLevel(*Item))
		{
			int32 HiddenActorCount = 0;
			FString ActorMessage;
			FDevKitLevelBatchService::HideSourceActorsForLoadedWorld(Item->Paths.LevelName, HiddenActorCount, ActorMessage);
			HiddenActorTotal += HiddenActorCount;
		}
	}

	RefreshLevelItems();
	SetStatus(FText::Format(
		LOCTEXT("BatchStarted", "已启动 {0} 个合批生成命令，当前世界隐藏源 Actor {1} 个。生成物输出到各关卡 BatchedAsset，状态为待审查。{2}"),
		FText::AsNumber(StartedCount),
		FText::AsNumber(HiddenActorTotal),
		FText::FromString(Failures.IsEmpty() ? FString() : FString::Join(Failures, TEXT(" | ")))));
}

void SDevKitLevelBatchProcessorWidget::MarkSelectedLevels(EDevKitLevelBatchReviewStatus Status)
{
	const TArray<FLevelBatchItemPtr> SelectedItems = GetSelectedItems();
	if (SelectedItems.IsEmpty())
	{
		SetStatus(LOCTEXT("NoMarkSelection", "请先选择要更新状态的关卡。"));
		return;
	}

	int32 UpdatedCount = 0;
	for (const FLevelBatchItemPtr& Item : SelectedItems)
	{
		if (!Item.IsValid())
		{
			continue;
		}

		FString Error;
		if (FDevKitLevelBatchService::WriteReviewStatus(Item->Paths, Status, Error))
		{
			++UpdatedCount;
		}
	}

	RefreshLevelItems();
	SetStatus(FText::Format(
		LOCTEXT("MarkedStatus", "已将 {0} 个关卡标记为 {1}。"),
		FText::AsNumber(UpdatedCount),
		FDevKitLevelBatchService::StatusToDisplayText(Status)));
}

FString SDevKitLevelBatchProcessorWidget::GetRootPackagePath() const
{
	FString RootPath = RootFolderTextBox.IsValid()
		? RootFolderTextBox->GetText().ToString()
		: FDevKitLevelBatchService::GetDefaultMapDataRoot();
	RootPath.TrimStartAndEndInline();
	return RootPath.IsEmpty() ? FDevKitLevelBatchService::GetDefaultMapDataRoot() : RootPath;
}

FString SDevKitLevelBatchProcessorWidget::GetTierName() const
{
	FString TierName = TierTextBox.IsValid() ? TierTextBox->GetText().ToString() : FString(TEXT("Mid"));
	TierName.TrimStartAndEndInline();
	return TierName.IsEmpty() ? TEXT("Mid") : TierName;
}

TArray<SDevKitLevelBatchProcessorWidget::FLevelBatchItemPtr> SDevKitLevelBatchProcessorWidget::GetSelectedItems() const
{
	return LevelListView.IsValid() ? LevelListView->GetSelectedItems() : TArray<FLevelBatchItemPtr>();
}

bool SDevKitLevelBatchProcessorWidget::IsCurrentEditorMapLevel(const FDevKitLevelBatchStatus& Status) const
{
	if (!GEditor)
	{
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	const FString MapPackageName = World && World->GetOutermost() ? World->GetOutermost()->GetName() : FString();
	return MapPackageName == Status.Paths.PersistentMapPackage ||
		MapPackageName == Status.Paths.BatchedMapPackage ||
		MapPackageName.StartsWith(Status.Paths.LevelAssetFolder / TEXT(""));
}

void SDevKitLevelBatchProcessorWidget::SetStatus(const FText& InStatus) const
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(InStatus);
	}
}

FText SDevKitLevelBatchProcessorWidget::GetListSummaryText() const
{
	return FText::Format(
		LOCTEXT("ListSummary", "已扫描 {0} 个主关卡文件夹。状态来自 BatchedAsset/EnvBatchStatus.json；没有状态文件但已有生成物或 _Batched 子关卡时显示为待审查。"),
		FText::AsNumber(LevelItems.Num()));
}

TSharedRef<ITableRow> SDevKitLevelBatchProcessorWidget::GenerateLevelRow(FLevelBatchItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FDevKitLevelBatchStatus* Status = Item.IsValid() ? Item.Get() : nullptr;
	const FString LevelName = Status ? Status->Paths.LevelName : FString(TEXT("(invalid)"));
	const FText ReviewText = Status ? FDevKitLevelBatchService::StatusToDisplayText(Status->ReviewStatus) : FText::GetEmpty();
	const FString PathText = Status
		? FString::Printf(TEXT("LevelAsset: %s\nBatchedAsset: %s"), *Status->Paths.LevelAssetFolder, *Status->Paths.BatchedAssetFolder)
		: FString();
	const FString AssetText = Status
		? FString::Printf(TEXT("%d assets%s%s"),
			Status->GeneratedAssetCount,
			Status->bHasBatchedMap ? TEXT(", _Batched") : TEXT(""),
			Status->bHasStatusFile ? TEXT(", status") : TEXT(""))
		: FString();

	return SNew(STableRow<FLevelBatchItemPtr>, OwnerTable)
		.Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.35f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(LevelName))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.14f)
			[
				SNew(STextBlock)
				.Text(ReviewText)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.12f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(AssetText))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.39f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(PathText))
				.AutoWrapText(true)
			]
		];
}

#undef LOCTEXT_NAMESPACE
