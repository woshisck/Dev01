#pragma once

#include "Commandlets/Commandlet.h"
#include "DevKitLookDevSetupCommandlet.generated.h"

/** Creates missing LookDev maps and can explicitly prune retired lighting references from LookDevMain. */
UCLASS()
class YOGLOOKDEVEDITOR_API UDevKitLookDevSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UDevKitLookDevSetupCommandlet();
	virtual int32 Main(const FString& Params) override;
};
