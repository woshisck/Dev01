#include "System/YogInstanceSubSystem.h"
#include "Engine/GameInstance.h"

void UYogInstanceSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UYogInstanceSubSystem::Deinitialize()
{
	Super::Deinitialize();
}

UYogInstanceSubSystem* UYogInstanceSubSystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
		return nullptr;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
		return nullptr;

	return GI->GetSubsystem<UYogInstanceSubSystem>();
}
