#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogInstanceSubSystem.generated.h"

class UEffectRegistry;

UCLASS()
class DEVKIT_API UYogInstanceSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Yog")
	UEffectRegistry* GetEffectRegistry() const { return EffectRegistry; }

	static UYogInstanceSubSystem* Get(const UObject* WorldContextObject);

private:
	UPROPERTY()
	TObjectPtr<UEffectRegistry> EffectRegistry;
};
