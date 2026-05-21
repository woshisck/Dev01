#include "Story/Encounter/StoryProductionBoard.h"

const FStoryProductionRow* UStoryProductionBoardDA::FindRow(FName RequirementId) const
{
	return Rows.FindByPredicate([RequirementId](const FStoryProductionRow& Row)
	{
		return Row.RequirementId == RequirementId;
	});
}
