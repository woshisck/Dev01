#pragma once

#include "Commandlets/Commandlet.h"
#include "UE58ScenePerformanceAuditCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UUE58ScenePerformanceAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUE58ScenePerformanceAuditCommandlet();

	virtual int32 Main(const FString& Params) override;
};
