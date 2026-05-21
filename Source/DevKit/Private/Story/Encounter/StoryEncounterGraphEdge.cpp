#include "Story/Encounter/StoryEncounterGraphEdge.h"

UStoryEncounterGraphEdge::UStoryEncounterGraphEdge()
{
#if WITH_EDITORONLY_DATA
	bShouldDrawTitle = true;
	EdgeColour = FLinearColor(0.86f, 0.67f, 0.22f, 1.f);
#endif
}

#if WITH_EDITOR
FText UStoryEncounterGraphEdge::GetNodeTitle() const
{
	return TransitionLabel.IsEmpty() ? FText::FromString(TEXT("继续")) : TransitionLabel;
}

void UStoryEncounterGraphEdge::SetNodeTitle(const FText& NewTitle)
{
	TransitionLabel = NewTitle;
}
#endif
