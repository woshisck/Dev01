#include "Tools/SEnvBatchTaggerWidget.h"

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "GameFramework/Actor.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SEnvBatchTaggerWidget"

namespace
{
bool IsEnvBatchTag(const FName& Tag)
{
	return Tag.ToString().StartsWith(TEXT("EnvBatch."));
}

bool HasEnvBatchPrefix(const AActor* Actor, const TCHAR* Prefix)
{
	if (!Actor)
	{
		return false;
	}

	for (const FName& Tag : Actor->Tags)
	{
		if (Tag.ToString().StartsWith(Prefix))
		{
			return true;
		}
	}
	return false;
}

bool HasEnvBatchExactTag(const AActor* Actor, const TCHAR* TagName)
{
	return Actor && Actor->Tags.Contains(FName(TagName));
}

bool HasStaticMeshAsset(const AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	Actor->GetComponents(StaticMeshComponents);
	for (const UStaticMeshComponent* Component : StaticMeshComponents)
	{
		if (Component && Component->GetStaticMesh())
		{
			return true;
		}
	}
	return false;
}
}

void SEnvBatchTaggerWidget::Construct(const FArguments& InArgs)
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
					.Text(LOCTEXT("Title", "环境合批标记"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT(
						"Description",
						"这个窗口只负责给关卡里的静态环境 Actor 写入 EnvBatch Actor Tag。后续关卡模型材质合批工具会按这些 tag 扫描、合并、生成代理资产。选中普通 StaticMeshActor、带 StaticMeshComponent 的蓝图，或已经处理好的 Level Instance 边界对象后再打标。"))
					.AutoWrapText(true)
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
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(GroupNameTextBox, SEditableTextBox)
					.Text(LOCTEXT("DefaultGroupName", "Corridor_01b"))
					.HintText(LOCTEXT("GroupHint", "合批组名，例如 Corridor_01b、Prison_S_01、Building_Stone"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplySource", "标记 Source.<Group>"))
						.ToolTipText(LOCTEXT("ApplySourceTooltip", "把选中的静态环境对象标记为合批输入。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplySourceTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyProxyMid", "标记 Proxy.<Group>.Mid"))
						.ToolTipText(LOCTEXT("ApplyProxyMidTooltip", "把选中的代理或显式合批结果标记为 Mid 及以下可用。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyProxyMidTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyProxyLow", "标记 Proxy.<Group>.Low"))
						.ToolTipText(LOCTEXT("ApplyProxyLowTooltip", "把选中的代理或显式合批结果标记为 Low 可用。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyProxyLowTag)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyBakedGroundMid", "标记 Baked.Ground.Mid"))
						.ToolTipText(LOCTEXT("ApplyBakedGroundMidTooltip", "标记地面静态烘焙结果，供 Mid 档及以下使用。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyBakedGroundMidTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyBakedGroundLow", "标记 Baked.Ground.Low"))
						.ToolTipText(LOCTEXT("ApplyBakedGroundLowTooltip", "标记地面静态烘焙结果，供 Low 档使用。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyBakedGroundLowTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyBakedWallMid", "标记 Baked.Wall.Mid"))
						.ToolTipText(LOCTEXT("ApplyBakedWallMidTooltip", "标记墙面静态烘焙结果，供 Mid 档及以下使用。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyBakedWallMidTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyBakedWallLow", "标记 Baked.Wall.Low"))
						.ToolTipText(LOCTEXT("ApplyBakedWallLowTooltip", "标记墙面静态烘焙结果，供 Low 档使用。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyBakedWallLowTag)
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
						.Text(LOCTEXT("ApplyExclude", "排除合批"))
						.ToolTipText(LOCTEXT("ApplyExcludeTooltip", "明确排除选中对象，不让它进入 MaterialBatch 扫描和生成。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyExcludeTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("RemoveTags", "清除 EnvBatch 标记"))
						.ToolTipText(LOCTEXT("RemoveTagsTooltip", "清除选中 Actor 上所有 EnvBatch.* Actor Tag。"))
						.OnClicked(this, &SEnvBatchTaggerWidget::RemoveEnvBatchTags)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("Refresh", "刷新选择"))
						.OnClicked(this, &SEnvBatchTaggerWidget::RefreshSelection)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("InitialStatus", "就绪。先在关卡中选择对象，再点击上方按钮写入 Actor Tag。"))
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

FString SEnvBatchTaggerWidget::GetGroupName() const
{
	FString GroupName = GroupNameTextBox.IsValid() ? GroupNameTextBox->GetText().ToString() : FString();
	GroupName.TrimStartAndEndInline();
	GroupName.ReplaceInline(TEXT(" "), TEXT("_"));
	return GroupName.IsEmpty() ? FString(TEXT("Default")) : GroupName;
}

FReply SEnvBatchTaggerWidget::ApplySourceTag()
{
	return ApplyExclusiveTag(FString::Printf(TEXT("EnvBatch.Source.%s"), *GetGroupName()));
}

FReply SEnvBatchTaggerWidget::ApplyProxyMidTag()
{
	return ApplyExclusiveTag(FString::Printf(TEXT("EnvBatch.Proxy.%s.Mid"), *GetGroupName()));
}

FReply SEnvBatchTaggerWidget::ApplyProxyLowTag()
{
	return ApplyExclusiveTag(FString::Printf(TEXT("EnvBatch.Proxy.%s.Low"), *GetGroupName()));
}

FReply SEnvBatchTaggerWidget::ApplyBakedGroundMidTag()
{
	return ApplyExclusiveTag(TEXT("EnvBatch.Baked.Ground.Mid"));
}

FReply SEnvBatchTaggerWidget::ApplyBakedGroundLowTag()
{
	return ApplyExclusiveTag(TEXT("EnvBatch.Baked.Ground.Low"));
}

FReply SEnvBatchTaggerWidget::ApplyBakedWallMidTag()
{
	return ApplyExclusiveTag(TEXT("EnvBatch.Baked.Wall.Mid"));
}

FReply SEnvBatchTaggerWidget::ApplyBakedWallLowTag()
{
	return ApplyExclusiveTag(TEXT("EnvBatch.Baked.Wall.Low"));
}

FReply SEnvBatchTaggerWidget::ApplyExcludeTag()
{
	return ApplyExclusiveTag(TEXT("EnvBatch.Exclude"));
}

FReply SEnvBatchTaggerWidget::RemoveEnvBatchTags()
{
	int32 ChangedCount = 0;
	for (AActor* Actor : GetSelectedActors())
	{
		if (!Actor)
		{
			continue;
		}

		const int32 OldTagCount = Actor->Tags.Num();
		Actor->Modify();
		Actor->Tags.RemoveAll([](const FName& ExistingTag)
		{
			return IsEnvBatchTag(ExistingTag);
		});

		if (Actor->Tags.Num() != OldTagCount)
		{
			Actor->MarkPackageDirty();
			++ChangedCount;
		}
	}

	SetStatus(FText::Format(LOCTEXT("RemovedStatus", "已从 {0} 个 Actor 清除 EnvBatch 标记。"), FText::AsNumber(ChangedCount)));
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::RefreshSelection()
{
	SetStatus(GetAssetReadinessSummaryText());
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::ApplyExclusiveTag(const FString& EnvBatchTag)
{
	int32 ChangedCount = 0;
	for (AActor* Actor : GetSelectedActors())
	{
		if (!Actor)
		{
			continue;
		}

		Actor->Modify();
		Actor->Tags.RemoveAll([](const FName& ExistingTag)
		{
			return IsEnvBatchTag(ExistingTag);
		});
		Actor->Tags.AddUnique(FName(*EnvBatchTag));
		Actor->MarkPackageDirty();
		++ChangedCount;
	}

	SetStatus(FText::Format(
		LOCTEXT("AppliedStatus", "已将 {0} 写入 {1} 个 Actor。请按 UE 正常流程保存对应关卡或子关卡。"),
		FText::FromString(EnvBatchTag),
		FText::AsNumber(ChangedCount)));
	return FReply::Handled();
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
		LOCTEXT("SelectionSummary", "当前选择 Actor 数量：{0}"),
		FText::AsNumber(GetSelectedActors().Num()));
}

FText SEnvBatchTaggerWidget::GetAssetReadinessSummaryText() const
{
	const TArray<AActor*> Actors = GetSelectedActors();
	if (Actors.IsEmpty())
	{
		return LOCTEXT("AssetReadinessNoSelection", "使用方式：在 Level Viewport 或 World Outliner 选择要合并的一组静态环境对象，填写 Group，然后写入 Source/Proxy/Baked/Exclude。合批工具后续只根据这些 Actor Tag 扫描，不在这里配置模型 LOD 或贴图命名。");
	}

	int32 SourceCount = 0;
	int32 SourceWithMeshCount = 0;
	int32 ProxyCount = 0;
	int32 BakedCount = 0;
	int32 ExcludedCount = 0;
	int32 ConflictCount = 0;
	int32 UntaggedCount = 0;

	for (const AActor* Actor : Actors)
	{
		const bool bHasSource = HasEnvBatchPrefix(Actor, TEXT("EnvBatch.Source."));
		const bool bHasProxy = HasEnvBatchPrefix(Actor, TEXT("EnvBatch.Proxy."));
		const bool bHasBaked = HasEnvBatchPrefix(Actor, TEXT("EnvBatch.Baked."));
		const bool bHasExclude = HasEnvBatchExactTag(Actor, TEXT("EnvBatch.Exclude"));
		const int32 ActiveTagCount = (bHasSource ? 1 : 0) + (bHasProxy ? 1 : 0) + (bHasBaked ? 1 : 0) + (bHasExclude ? 1 : 0);

		if (ActiveTagCount > 1)
		{
			++ConflictCount;
		}
		if (bHasSource)
		{
			++SourceCount;
			SourceWithMeshCount += HasStaticMeshAsset(Actor) ? 1 : 0;
		}
		else if (bHasProxy)
		{
			++ProxyCount;
		}
		else if (bHasBaked)
		{
			++BakedCount;
		}
		else if (bHasExclude)
		{
			++ExcludedCount;
		}
		else
		{
			++UntaggedCount;
		}
	}

	return FText::Format(
		LOCTEXT(
			"AssetReadinessSummary",
			"选择诊断：Source {0}（有 StaticMesh {1}），Proxy {2}，Baked {3}，Exclude {4}，冲突 {5}，未标记 {6}。合批组可以是不连续对象；带 StaticMeshComponent 的蓝图可作为一个输入边界，处理好的 Level Instance 可按边界打 tag。"),
		FText::AsNumber(SourceCount),
		FText::AsNumber(SourceWithMeshCount),
		FText::AsNumber(ProxyCount),
		FText::AsNumber(BakedCount),
		FText::AsNumber(ExcludedCount),
		FText::AsNumber(ConflictCount),
		FText::AsNumber(UntaggedCount));
}

#undef LOCTEXT_NAMESPACE
