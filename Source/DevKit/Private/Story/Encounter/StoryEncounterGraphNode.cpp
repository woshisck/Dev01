#include "Story/Encounter/StoryEncounterGraphNode.h"

#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"

UStoryEncounterGraphNode::UStoryEncounterGraphNode()
{
	FallbackNodeId = TEXT("Point");

#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UStoryEncounterGraph::StaticClass();
	BackgroundColor = FLinearColor(0.12f, 0.32f, 0.42f);
	ContextMenuName = FText::FromString(TEXT("剧情点"));
	ParentLimitType = ENodeLimit::Unlimited;
	ChildrenLimitType = ENodeLimit::Unlimited;
#endif
}

UStoryEncounterPointDA* UStoryEncounterGraphNode::GetPoint() const
{
	return Point;
}

FName UStoryEncounterGraphNode::GetEncounterId() const
{
	return Point ? Point->EncounterId : FallbackEncounterId;
}

FName UStoryEncounterGraphNode::GetStoryNodeId() const
{
	return Point ? Point->GetStableNodeId() : FallbackNodeId;
}

FStoryEncounterNode UStoryEncounterGraphNode::ToEncounterNode() const
{
	if (Point)
	{
		return Point->ToEncounterNode();
	}

	FStoryEncounterNode Node;
	Node.NodeId = FallbackNodeId;
	Node.DisplayName = FallbackTitle;
	Node.Kind = EStoryEncounterNodeKind::System;
	return Node;
}

#if WITH_EDITOR
FText UStoryEncounterGraphNode::GetNodeTitle() const
{
	if (Point && !Point->DisplayName.IsEmpty())
	{
		return Point->DisplayName;
	}
	if (!FallbackTitle.IsEmpty())
	{
		return FallbackTitle;
	}
	if (!GetStoryNodeId().IsNone())
	{
		return FText::FromName(GetStoryNodeId());
	}
	return FText::FromString(TEXT("未绑定剧情点"));
}

void UStoryEncounterGraphNode::SetNodeTitle(const FText& NewTitle)
{
	FallbackTitle = NewTitle;
}

bool UStoryEncounterGraphNode::CanCreateConnectionTo(UGenericGraphNode* Other, int32 NumberOfChildrenNodes,
	FText& ErrorMessage)
{
	if (Other == this)
	{
		ErrorMessage = FText::FromString(TEXT("不能连接到自身"));
		return false;
	}

	return Super::CanCreateConnectionTo(Other, NumberOfChildrenNodes, ErrorMessage);
}
#endif
