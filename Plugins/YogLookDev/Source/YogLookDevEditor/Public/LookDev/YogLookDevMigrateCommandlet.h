#pragma once

#include "Commandlets/Commandlet.h"
#include "YogLookDevMigrateCommandlet.generated.h"

/** Explicit, backed-up migration of the current LookDev folder. Dry-run is the default. */
UCLASS()
class YOGLOOKDEVEDITOR_API UYogLookDevMigrateCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UYogLookDevMigrateCommandlet();
    virtual int32 Main(const FString& Params) override;
};
