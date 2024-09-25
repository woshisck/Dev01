// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class IPropertyHandle;

/** Detail customization for GSCGameplayEffectTemplate, one to one matching FGameplayEffectDetails for GE. */
class FGSCGameplayEffectTemplateDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

private:
	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	

	TArray<TSharedPtr<FString>> PropertyOptions;

	TSharedPtr<IPropertyHandle> MyProperty;

	IDetailLayoutBuilder* MyDetailLayout;

	void OnDurationPolicyChange();
};

