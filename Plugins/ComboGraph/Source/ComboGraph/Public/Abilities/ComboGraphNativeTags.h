// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
struct COMBOGRAPH_API FComboGraphNativeTags : public FGameplayTagNativeAdder
{
	FGameplayTag Input;
	FGameplayTag StateChange;

	FGameplayTag ComboBegin;
	FGameplayTag ComboEnd;

	FORCEINLINE static const FComboGraphNativeTags& Get() { return NativeTags; }

	virtual void AddTags() override
	{
		UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

		Input = Manager.AddNativeGameplayTag(TEXT("Event.ComboGraph.Input"));
		StateChange = Manager.AddNativeGameplayTag(TEXT("Event.ComboGraph.StateChange"));
		ComboBegin = Manager.AddNativeGameplayTag(TEXT("Event.ComboGraph.Animation.ComboBegin"));
		ComboEnd = Manager.AddNativeGameplayTag(TEXT("Event.ComboGraph.Animation.ComboEnd"));
	}

private:

	static FComboGraphNativeTags NativeTags;
};
