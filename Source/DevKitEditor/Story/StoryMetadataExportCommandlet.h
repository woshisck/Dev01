#pragma once

#include "Commandlets/Commandlet.h"
#include "StoryMetadataExportCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UStoryMetadataExportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UStoryMetadataExportCommandlet();

	virtual int32 Main(const FString& Params) override;
};
