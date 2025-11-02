
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <AttributeSet.h>
#include "GameplayEffectTypes.h"
#include "AttributeStatComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangeDelegate, float, Health);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UAttributeStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UAttributeStatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;



public:


	//////////////////////////////////////////////////////////////////////

	UFUNCTION(BlueprintCallable)
	float GetAttribute(FGameplayAttribute attribute) const;

	UFUNCTION(BlueprintCallable)
	void AddAttribute(FGameplayAttribute attribute, float value_add) const;

	UFUNCTION(BlueprintCallable)
	void MultiplyAttribute(FGameplayAttribute attribute, float value_multiply) const;

	UFUNCTION(BlueprintCallable)
	void DivideAttribute(FGameplayAttribute attribute, float value_divide) const;

	UFUNCTION(BlueprintCallable)
	void OverrideAttribute(FGameplayAttribute attribute, float value_override) const;


	//////////////////////////////////////////////////////////////////////
	UFUNCTION(BlueprintPure)
	float GetStat_Attack() const;

	UFUNCTION(BlueprintPure)
	float GetStat_AttackPower() const;

	UFUNCTION(BlueprintPure)
	float GetStat_Health() const;

	UFUNCTION(BlueprintPure)
	float GetStat_MaxHealth() const;

	UFUNCTION(BlueprintPure)
	float GetStat_AttackSpeed() const;

	UFUNCTION(BlueprintPure)
	float GetStat_AttackRange() const;

	UFUNCTION(BlueprintPure)
	float GetStat_Sanity() const;

	UFUNCTION(BlueprintPure)
	float GetStat_MoveSpeed() const;

	UFUNCTION(BlueprintPure)
	float GetStat_Dodge() const;

	UFUNCTION(BlueprintPure)
	float GetStat_Resilience() const;

	UFUNCTION(BlueprintPure)
	float GetStat_Resist() const;

	UFUNCTION(BlueprintPure)
	float GetStat_DmgTaken() const;

	UFUNCTION(BlueprintPure)
	float GetStat_Crit_Rate() const;

	UFUNCTION(BlueprintPure)
	float GetStat_Crit_Damage() const;

public:
	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Stat Change")
	FOnHealthChangeDelegate OnHealthChange;

private:
	void HandleHealthChange(const FOnAttributeChangeData& data);
};
