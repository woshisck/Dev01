#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "Story/StoryNextRoomPlanTypes.h"
#include "SNode_SetNextRoomPlan.generated.h"

class UYogGameInstanceBase;

/** Sets or clears the pending Story-authored plan used the next time portals open. */
UCLASS(meta = (DisplayName = "Set Next Room Plan"))
class DEVKIT_API USNode_SetNextRoomPlan : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Story Next Room")
	bool bClearPlan = false;

	UPROPERTY(EditAnywhere, Category = "Story Next Room", meta = (EditCondition = "!bClearPlan"))
	FStoryNextRoomPlan Plan;

	bool ApplyNextRoomPlan(UYogGameInstanceBase* GameInstance) const;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
