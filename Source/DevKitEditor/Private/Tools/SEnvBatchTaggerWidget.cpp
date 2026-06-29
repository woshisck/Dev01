#include "Tools/SEnvBatchTaggerWidget.h"

#include "Editor.h"
#include "Engine/Selection.h"
#include "GameFramework/Actor.h"
#include "Styling/CoreStyle.h"
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
					.Text(LOCTEXT("Title", "EnvBatch Tagger"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Description", "Select actors in the level, enter a group name, then assign an EnvBatch actor tag. Tags are exclusive inside the EnvBatch namespace."))
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
					SAssignNew(GroupNameTextBox, SEditableTextBox)
					.Text(LOCTEXT("DefaultGroupName", "Corridor_01b"))
					.HintText(LOCTEXT("GroupHint", "Group name, for example Corridor_01b or Building_Stone"))
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
						.Text(LOCTEXT("ApplySource", "Source.<Group>"))
						.ToolTipText(LOCTEXT("ApplySourceTooltip", "Mark selected static source actors as MaterialBatch inputs."))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplySourceTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyProxyMedium", "Proxy.<Group>.Medium"))
						.ToolTipText(LOCTEXT("ApplyProxyMediumTooltip", "Mark selected generated or authored proxy actors for Medium and lower tiers."))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyProxyMediumTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyProxyLow", "Proxy.<Group>.Low"))
						.ToolTipText(LOCTEXT("ApplyProxyLowTooltip", "Mark selected generated or authored proxy actors for Low tiers."))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyProxyLowTag)
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
						.Text(LOCTEXT("ApplyBakedGroundLow", "Baked.Ground.Low"))
						.ToolTipText(LOCTEXT("ApplyBakedGroundLowTooltip", "Mark a baked low-tier ground replacement."))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyBakedGroundLowTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ApplyExclude", "Exclude"))
						.ToolTipText(LOCTEXT("ApplyExcludeTooltip", "Explicitly exclude selected actors from MaterialBatch generation."))
						.OnClicked(this, &SEnvBatchTaggerWidget::ApplyExcludeTag)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("RemoveTags", "Remove EnvBatch Tags"))
						.ToolTipText(LOCTEXT("RemoveTagsTooltip", "Remove all EnvBatch.* tags from selected actors."))
						.OnClicked(this, &SEnvBatchTaggerWidget::RemoveEnvBatchTags)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("Refresh", "Refresh Selection"))
						.OnClicked(this, &SEnvBatchTaggerWidget::RefreshSelection)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("InitialStatus", "Ready."))
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

FReply SEnvBatchTaggerWidget::ApplyProxyMediumTag()
{
	return ApplyExclusiveTag(FString::Printf(TEXT("EnvBatch.Proxy.%s.Medium"), *GetGroupName()));
}

FReply SEnvBatchTaggerWidget::ApplyProxyLowTag()
{
	return ApplyExclusiveTag(FString::Printf(TEXT("EnvBatch.Proxy.%s.Low"), *GetGroupName()));
}

FReply SEnvBatchTaggerWidget::ApplyBakedGroundLowTag()
{
	return ApplyExclusiveTag(TEXT("EnvBatch.Baked.Ground.Low"));
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

	SetStatus(FText::Format(LOCTEXT("RemovedStatus", "Removed EnvBatch tags from {0} actor(s)."), FText::AsNumber(ChangedCount)));
	return FReply::Handled();
}

FReply SEnvBatchTaggerWidget::RefreshSelection()
{
	SetStatus(GetSelectionSummaryText());
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
		LOCTEXT("AppliedStatus", "Applied {0} to {1} actor(s)."),
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
		LOCTEXT("SelectionSummary", "Selected actors: {0}"),
		FText::AsNumber(GetSelectedActors().Num()));
}

#undef LOCTEXT_NAMESPACE
