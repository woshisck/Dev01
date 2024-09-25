// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GSCUserWidget.h"
#include "GameplayEffectTypes.h"

#include "GSCUWHud.generated.h"

struct FGameplayAbilitySpecHandle;
class UProgressBar;
class UTextBlock;


/**
 * Base user widget class to inherit from for UMG that needs to interact with an Ability System Component.
 *
 * Suitable to use for typical case of Player HUD. Initialization happens automatically on NativeConstruct by setting up
 * owner actor based on Owning Player controller (when Widget is created from Blueprint, the Owning Player pin)
 *
 * If you'd like to use a user widget with a Widget Component, use UGSCUserWidget instead and manually invoke InitializeWithAbilitySystem().
 *
 * The other main difference with UGSCUserWidget is that this class also defines widget optional binding for
 * Health / Stamina / Mana attributes from UGSCAttributeSet.
 */
UCLASS()
class GASCOMPANION_API UGSCUWHud : public UGSCUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End UUserWidget interface
	
public:

	/** Init widget with attributes from owner character **/
	UFUNCTION(BlueprintCallable, Category="GAS Companion|UI")
	virtual void InitFromCharacter();

	/** Updates bound health widgets to reflect the new max health change */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetMaxHealth(float MaxHealth);

	/** Updates bound health widgets to reflect the new health change */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetHealth(float Health);

	/** Updates bound stamina progress bar with the new percent */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetHealthPercentage(float HealthPercentage);

	/** Updates bound stamina widgets to reflect the new max stamina change */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetMaxStamina(float MaxStamina);

	/** Updates bound stamina widgets to reflect the new stamina change */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetStamina(float Stamina);

	/** Updates bound health progress bar with the new percent */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetStaminaPercentage(float StaminaPercentage);

	/** Updates bound mana widgets to reflect the new max mana change */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetMaxMana(float MaxMana);

	/** Updates bound mana widgets to reflect the new mana change */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetMana(float Mana);

	/** Updates bound mana progress bar with the new percent */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|UI")
	virtual void SetManaPercentage(float ManaPercentage);


protected:
	/** Set in native construct if called too early to check for ASC in tick, and kick off initialization logic when it is ready */
	bool bLazyAbilitySystemInitialization = false;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GAS Companion|UI")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> StaminaText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GAS Companion|UI")
	TObjectPtr<UProgressBar> StaminaProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GAS Companion|UI")
	TObjectPtr<UTextBlock> ManaText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GAS Companion|UI")
	TObjectPtr<UProgressBar> ManaProgressBar;

	/** Updates bound widget whenever one of the attribute we care about is changed */
	virtual void HandleAttributeChange(FGameplayAttribute Attribute, float NewValue, float OldValue) override;


private:
	/** Array of active GE handle bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FActiveGameplayEffectHandle> GameplayEffectAddedHandles;

	/** Array of tags bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FGameplayTag> GameplayTagBoundToDelegates;

	static FString GetAttributeFormatString(float BaseValue, float MaxValue);

	/**
	 * Checks owner for a valid ASC and kick in initialization logic if it finds one
	 *
	 * @return Whether an ASC was found
	 */
	bool TryInitAbilitySystem();
};
