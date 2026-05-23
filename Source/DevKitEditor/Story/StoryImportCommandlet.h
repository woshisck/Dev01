#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "StoryImportCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UStoryImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UStoryImportCommandlet();

	virtual int32 Main(const FString& Params) override;
};
