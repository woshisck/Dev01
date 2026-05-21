#include "Story/Encounter/StoryEncounterPointDataAsset.h"

FName UStoryEncounterPointDA::GetStableNodeId() const
{
	if (!NodeId.IsNone())
	{
		return NodeId;
	}

	return GetFName();
}

FStoryEncounterNode UStoryEncounterPointDA::ToEncounterNode() const
{
	FStoryEncounterNode Node;
	Node.NodeId = GetStableNodeId();
	Node.DisplayName = DisplayName;
	Node.Kind = Kind;
	Node.PlayerFacingEvent = PlayerFacingEvent;
	Node.FirePolicy = FirePolicy;
	Node.Condition = Condition;
	Node.Actions = Actions;
	Node.EditorPosition = EditorPosition;
	return Node;
}
