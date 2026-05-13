#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/YogUIRegistry.h"
#include "YogUIManagerSubsystem.generated.h"

class UUserWidget;
class UYogUIRegistry;

UCLASS()
class DEVKIT_API UYogUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "UI")
	UYogUIRegistry* GetRegistry() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	TSubclassOf<UUserWidget> GetWidgetClass(EYogUIScreenId ScreenId) const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	int32 GetZOrder(EYogUIScreenId ScreenId, int32 FallbackZOrder = 0) const;

	template<typename WidgetT>
	TSubclassOf<WidgetT> GetTypedWidgetClass(EYogUIScreenId ScreenId) const
	{
		TSubclassOf<UUserWidget> WidgetClass = GetWidgetClass(ScreenId);
		UClass* ResolvedClass = WidgetClass.Get();
		if (ResolvedClass && ResolvedClass->IsChildOf(WidgetT::StaticClass()))
		{
			return TSubclassOf<WidgetT>(ResolvedClass);
		}

		return TSubclassOf<WidgetT>();
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<UYogUIRegistry> CachedRegistry;

	mutable TMap<EYogUIScreenId, TSubclassOf<UUserWidget>> LoadedWidgetClasses;
};
