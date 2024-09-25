// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GSCUserWidget.h"
#include "GSCUWDebugComboWidget.generated.h"

class UGSCComboManagerComponent;
class AGSCCharacterBase;
class UTextBlock;

UCLASS()
class GASCOMPANION_API UGSCUWDebugComboWidget : public UGSCUserWidget
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UGSCComboManagerComponent> OwnerComboManagerComponent;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> ComboWindowOpenedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> ShouldTriggerComboText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> RequestTriggerComboText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> NextComboAbilityActivatedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> ComboIndexText;

	FLinearColor WhiteColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	FLinearColor GreenColor = FLinearColor(0.729412f, 0.854902f, 0.333333f, 1.f);
	FLinearColor RedColor = FLinearColor(1.f, 0.388235f, 0.278431f, 1.f);

	/**
	* Set the Owner Character for this UserWidget.
	*
	* Used to pull out information needed for display
	*
	*/
	virtual void SetOwnerActor(AActor* Actor) override;

protected:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
