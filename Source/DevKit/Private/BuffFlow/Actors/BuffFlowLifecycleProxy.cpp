#include "BuffFlow/Actors/BuffFlowLifecycleProxy.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Components/SceneComponent.h"

ABuffFlowLifecycleProxy::ABuffFlowLifecycleProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	BuffFlowComponent = CreateDefaultSubobject<UBuffFlowComponent>(TEXT("BuffFlowComponent"));
}
