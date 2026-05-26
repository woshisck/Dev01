#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuffFlowLifecycleProxy.generated.h"

class UBuffFlowComponent;
class USceneComponent;

UCLASS(NotBlueprintable)
class DEVKIT_API ABuffFlowLifecycleProxy : public AActor
{
	GENERATED_BODY()

public:
	ABuffFlowLifecycleProxy();

	UBuffFlowComponent* GetBuffFlowComponent() const { return BuffFlowComponent; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Lifecycle")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Lifecycle")
	TObjectPtr<UBuffFlowComponent> BuffFlowComponent;
};
