// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GSCUserWidget.h"
#include "GSCUWDebugAbilityQueue.generated.h"

class UGameplayAbility;
class UGSCAbilityQueueComponent;
class UTextBlock;
class UVerticalBox;
class UCanvasPanel;
class UAnimSequenceBase;

/**
 *
 */
UCLASS()
class GASCOMPANION_API UGSCUWDebugAbilityQueue : public UGSCUserWidget
{
	GENERATED_BODY()

public:

	/**
	 * Set the Owner Character for this UserWidget.
	 *
	 * Used to pull out information needed for display
	 *
	 */
	virtual void SetOwnerActor(AActor* Actor) override;


	/**
	 * Update the Allowed Abilities Panel with the provided AllowedAbilities array
	 *
	 * Alternatively, if the Owner Character allows all abilities to be queued (bAllowAllAbilitiesForAbilityQueue),
	 * it will instead add a single "All" entry.
	 */
	virtual void UpdateAllowedAbilities(TArray<TSubclassOf<UGameplayAbility>> AllowedAbilities);

	/**
	 * Adds a new Anim montage into the AbilityQueueFromMontage Panel.
	 *
	 * The entry can be eventually cleared and remove from the screen after a set amount of time (ClearFromMontageDelay)
	 *
	 * @param Anim The Animation from which the ability queue has been opened
	 * @param bStartClearTimer Start the clear timer immediately if true (defaults: true)
	 */
	virtual void AddAbilityQueueFromMontageRow(UAnimSequenceBase* Anim, bool bStartClearTimer = true);

	/**
	 * Start the clear timer for Montage rows
	 */
	virtual void StartClearFromMontageRowTimer();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	void OnAddAbilityQueueFromMontageRow(UAnimSequenceBase* Anim);
	void OnUpdateAllowedAbilities(TArray<TSubclassOf<UGameplayAbility>> Abilities);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void ClearFromMontageRow();

	TWeakObjectPtr<AActor> OwnerActor;
	TWeakObjectPtr<UGSCAbilityQueueComponent> OwnerAbilityQueueComponent;

	TArray<FTimerHandle> ClearFromMontageTimerHandles;

	FLinearColor WhiteColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	FLinearColor GreenColor = FLinearColor(0.729412f, 0.854902f, 0.333333f, 1.f);
	FLinearColor RedColor = FLinearColor(1.f, 0.388235f, 0.278431f, 1.f);

	/**
	 * The amount of time (in seconds) that "From Montage" rows stay on screen
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS Companion|UserWidget")
	float ClearFromMontageDelay = 8.f;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> AbilityQueueEnabledText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> AbilityQueueOpenedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> CurrentQueuedAbilityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> AllowAllAbilitiesText;

	/**
	 * Convenience TextBlock that serves as a "template" for allowed abilities row,
	 * so that we can customize its styling in Blueprints (font size, color, etc.)
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> AllowedAbilityTemplateText;

	/**
	 * Convenience TextBlock that serves as a "template" for ability queue from montages row,
	 * so that we can customize its styling in Blueprints (font size, color, etc.)
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> AbilityQueueFromMontageTemplateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UVerticalBox> AllowedAbilitiesBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UVerticalBox> AbilityQueueFromMontagesBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GAS Companion|UI")
	TObjectPtr<UCanvasPanel> AbilityQueueFromMontagesPanel;
};
