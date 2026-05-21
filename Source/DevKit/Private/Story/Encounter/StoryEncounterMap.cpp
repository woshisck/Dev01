#include "Story/Encounter/StoryEncounterMap.h"

const FStoryEncounterNode* UStoryEncounterMap::FindNode(FName NodeId) const
{
	return Nodes.FindByPredicate([NodeId](const FStoryEncounterNode& Node)
	{
		return Node.NodeId == NodeId;
	});
}

FStoryEncounterNode* UStoryEncounterMap::FindNodeMutable(FName NodeId)
{
	return Nodes.FindByPredicate([NodeId](const FStoryEncounterNode& Node)
	{
		return Node.NodeId == NodeId;
	});
}
