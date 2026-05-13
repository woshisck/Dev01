#include "UI/YogUIManagerSubsystem.h"

#include "DevAssetManager.h"
#include "Blueprint/UserWidget.h"

void UYogUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedRegistry = UDevAssetManager::Get().GetUIRegistry();
}

UYogUIRegistry* UYogUIManagerSubsystem::GetRegistry() const
{
	return CachedRegistry;
}

TSubclassOf<UUserWidget> UYogUIManagerSubsystem::GetWidgetClass(EYogUIScreenId ScreenId) const
{
	if (const TSubclassOf<UUserWidget>* CachedClass = LoadedWidgetClasses.Find(ScreenId))
	{
		return *CachedClass;
	}

	if (!CachedRegistry)
	{
		return nullptr;
	}

	FYogUIRegistryEntry Entry;
	if (!CachedRegistry->FindEntry(ScreenId, Entry) || Entry.WidgetClass.IsNull())
	{
		return nullptr;
	}

	TSubclassOf<UUserWidget> LoadedClass = UDevAssetManager::GetSubclass<UUserWidget>(Entry.WidgetClass);
	if (LoadedClass)
	{
		LoadedWidgetClasses.Add(ScreenId, LoadedClass);
	}

	return LoadedClass;
}

int32 UYogUIManagerSubsystem::GetZOrder(EYogUIScreenId ScreenId, int32 FallbackZOrder) const
{
	return CachedRegistry ? CachedRegistry->GetZOrder(ScreenId, FallbackZOrder) : FallbackZOrder;
}
