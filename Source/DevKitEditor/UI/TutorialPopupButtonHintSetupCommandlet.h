#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "TutorialPopupButtonHintSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UTutorialPopupButtonHintSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UTutorialPopupButtonHintSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
