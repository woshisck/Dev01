#include "System/YogInstanceSubSystem.h"
#include "System/YogSettings.h"
#include "Data/EffectRegistry.h"
#include "Engine/GameInstance.h"

void UYogInstanceSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UYogSettings* Settings = UYogSettings::Get();
	if (!Settings->DefaultEffectRegistry.IsNull())
	{
		EffectRegistry = Settings->DefaultEffectRegistry.LoadSynchronous();
	}
}

void UYogInstanceSubSystem::Deinitialize()
{
	EffectRegistry = nullptr;
	Super::Deinitialize();
}

UYogInstanceSubSystem* UYogInstanceSubSystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	return GI->GetSubsystem<UYogInstanceSubSystem>();
}
