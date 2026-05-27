#pragma once

#include "CoreMinimal.h"
#include "Story/FirstRunTutorialDirectorSubsystem.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_SetFirstRunTutorialStage.generated.h"

UCLASS(meta = (DisplayName = "Set First Run Tutorial Stage"))
class DEVKIT_API USNode_SetFirstRunTutorialStage : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "First Run Tutorial")
	EFirstRunTutorialStage Stage = EFirstRunTutorialStage::None;

	bool ApplyStage(UGameInstance* GameInstance) const;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
