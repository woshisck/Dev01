#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SpecialAttackDataAsset.h"
#include "PlayerSpecialAttackComponent.generated.h"

class APlayerCharacterBase;
class UTexture2D;
class UYogAbilitySystemComponent;

USTRUCT(BlueprintType)
struct DEVKIT_API FSpecialAttackSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	FName SpecialAttackId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	float CooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	float CooldownDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	bool bEquipped = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecialAttackChangedDelegate, FSpecialAttackSlotView, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecialAttackUseFailedDelegate, FText, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpecialAttackUsedDelegate, FSpecialAttackSlotView, Slot);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UPlayerSpecialAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerSpecialAttackComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Special Attack")
	TObjectPtr<USpecialAttackDataAsset> DefaultSpecialAttack;

	UPROPERTY(BlueprintAssignable, Category = "Special Attack")
	FSpecialAttackChangedDelegate OnSpecialAttackChanged;

	UPROPERTY(BlueprintAssignable, Category = "Special Attack")
	FSpecialAttackUsedDelegate OnSpecialAttackUsed;

	UPROPERTY(BlueprintAssignable, Category = "Special Attack")
	FSpecialAttackUseFailedDelegate OnSpecialAttackUseFailed;

	UFUNCTION(BlueprintCallable, Category = "Special Attack")
	void SetSpecialAttack(USpecialAttackDataAsset* InSpecialAttack);

	UFUNCTION(BlueprintPure, Category = "Special Attack")
	USpecialAttackDataAsset* GetSpecialAttack() const { return SpecialAttackAsset; }

	UFUNCTION(BlueprintPure, Category = "Special Attack")
	FSpecialAttackConfig GetSpecialAttackConfig() const { return RuntimeConfig; }

	UFUNCTION(BlueprintPure, Category = "Special Attack")
	FSpecialAttackSlotView GetSlotView() const;

	UFUNCTION(BlueprintPure, Category = "Special Attack")
	bool HasSpecialAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Special Attack")
	bool UseSpecialAttack();

	UFUNCTION(BlueprintCallable, Category = "Special Attack")
	void ClearCooldown();

private:
	UPROPERTY(Transient)
	TObjectPtr<USpecialAttackDataAsset> SpecialAttackAsset = nullptr;

	UPROPERTY(Transient)
	FSpecialAttackConfig RuntimeConfig;

	UPROPERTY(Transient)
	float CooldownRemaining = 0.0f;

	void GrantSpecialAttackAbility();
	void BroadcastSpecialAttackChanged() const;
	APlayerCharacterBase* GetPlayerOwner() const;
	UYogAbilitySystemComponent* GetOwnerYogASC() const;
};
