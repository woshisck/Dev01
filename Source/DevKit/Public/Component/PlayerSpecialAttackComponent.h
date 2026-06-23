#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SpecialAttackDataAsset.h"
#include "PlayerSpecialAttackComponent.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct DEVKIT_API FSpecialAttackSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Deprecated Special Attack")
	FName SpecialAttackId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Deprecated Special Attack")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Deprecated Special Attack")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Deprecated Special Attack")
	float CooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Deprecated Special Attack")
	float CooldownDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Deprecated Special Attack")
	bool bEquipped = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecialAttackChangedDelegate, FSpecialAttackSlotView, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecialAttackUseFailedDelegate, FText, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecialAttackUsedDelegate, FSpecialAttackSlotView, Slot);

// Deprecated compatibility shell. Player-selected active skills replaced equipped special attacks.
UCLASS(ClassGroup=(Deprecated), meta=(DisplayName = "Deprecated Special Attack Component"))
class DEVKIT_API UPlayerSpecialAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerSpecialAttackComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deprecated Special Attack", meta = (DeprecatedProperty, DeprecationMessage = "Use UPlayerActiveSkillComponent DefaultSkillAssets."))
	TObjectPtr<USpecialAttackDataAsset> DefaultSpecialAttack;

	UPROPERTY(BlueprintAssignable, Category = "Deprecated Special Attack")
	FSpecialAttackChangedDelegate OnSpecialAttackChanged;

	UPROPERTY(BlueprintAssignable, Category = "Deprecated Special Attack")
	FSpecialAttackUsedDelegate OnSpecialAttackUsed;

	UPROPERTY(BlueprintAssignable, Category = "Deprecated Special Attack")
	FSpecialAttackUseFailedDelegate OnSpecialAttackUseFailed;

	UFUNCTION(BlueprintCallable, Category = "Deprecated Special Attack")
	void SetSpecialAttack(USpecialAttackDataAsset* InSpecialAttack);

	UFUNCTION(BlueprintPure, Category = "Deprecated Special Attack")
	USpecialAttackDataAsset* GetSpecialAttack() const { return nullptr; }

	UFUNCTION(BlueprintPure, Category = "Deprecated Special Attack")
	FSpecialAttackConfig GetSpecialAttackConfig() const { return FSpecialAttackConfig(); }

	UFUNCTION(BlueprintPure, Category = "Deprecated Special Attack")
	FSpecialAttackSlotView GetSlotView() const { return FSpecialAttackSlotView(); }

	UFUNCTION(BlueprintPure, Category = "Deprecated Special Attack")
	bool HasSpecialAttack() const { return false; }

	UFUNCTION(BlueprintCallable, Category = "Deprecated Special Attack")
	bool UseSpecialAttack();

	UFUNCTION(BlueprintCallable, Category = "Deprecated Special Attack")
	void ClearCooldown() {}

private:
	void BroadcastSpecialAttackChanged() const;
};
