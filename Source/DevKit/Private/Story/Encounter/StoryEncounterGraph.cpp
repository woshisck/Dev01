#include "Story/Encounter/StoryEncounterGraph.h"

#include "Story/Encounter/StoryEncounterGraphEdge.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"

UStoryEncounterGraph::UStoryEncounterGraph()
{
	Name = TEXT("StoryEncounterGraph");
	NodeType = UStoryEncounterGraphNode::StaticClass();
	EdgeType = UStoryEncounterGraphEdge::StaticClass();
	bEdgeEnabled = true;

#if WITH_EDITORONLY_DATA
	bCanRenameNode = true;
	bCanBeCyclical = false;
#endif
}

UStoryEncounterGraphNode* UStoryEncounterGraph::FindNodeByStoryNodeId(FName NodeId) const
{
	if (NodeId.IsNone())
	{
		return nullptr;
	}

	for (UGenericGraphNode* Node : AllNodes)
	{
		UStoryEncounterGraphNode* StoryNode = Cast<UStoryEncounterGraphNode>(Node);
		if (StoryNode && StoryNode->GetStoryNodeId() == NodeId)
		{
			return StoryNode;
		}
	}

	return nullptr;
}
