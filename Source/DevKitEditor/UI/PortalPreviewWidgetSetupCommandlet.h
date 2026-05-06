#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PortalPreviewWidgetSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UPortalPreviewWidgetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UPortalPreviewWidgetSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
