#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogInstanceSubSystem.generated.h"

UCLASS()
class DEVKIT_API UYogInstanceSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UYogInstanceSubSystem* Get(const UObject* WorldContextObject);
};
