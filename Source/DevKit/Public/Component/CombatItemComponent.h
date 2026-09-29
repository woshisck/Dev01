#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "Data/CombatItemDataAsset.h"
#include "GameplayEffectTypes.h"
#include "CombatItemComponent.generated.h"

class UTexture2D;
class UYogAbilitySystemComponent;

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatItemSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	bool bSelected = false;
};

USTRUCT()
struct DEVKIT_API FRuntimeSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FCombatItemConfig Config;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatItemSlotsChangedDelegate, const TArray<FCombatItemSlotView>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatItemUseFailedDelegate, int32, SlotIndex, FText, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatItemUsedDelegate, int32, SlotIndex, FCombatItemSlotView, Slot);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UCombatItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatItemComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Item|Loadout")
	TArray<TObjectPtr<UCombatItemDataAsset>> DefaultItemAssets;

	UPROPERTY(BlueprintAssignable, Category = "Combat Item")
	FCombatItemSlotsChangedDelegate OnItemSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat Item")
	FCombatItemUsedDelegate OnItemUsed;

	UPROPERTY(BlueprintAssignable, Category = "Combat Item")
	FCombatItemUseFailedDelegate OnItemUseFailed;

	/** Fires the active item's ActivationTag as a gameplay event, then consumes the slot. */
	UFUNCTION(BlueprintCallable, Category = "Combat Item")
	bool UseActiveItem();

	UFUNCTION(BlueprintCallable, Category = "Combat Item")
	void SelectNextItem();

	UFUNCTION(BlueprintCallable, Category = "Combat Item")
	void SelectPreviousItem();

	UFUNCTION(BlueprintCallable, Category = "Combat Item")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "Combat Item")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Combat Item")
	TArray<FCombatItemSlotView> GetSlotViews() const;

	static bool IsNoHitReactItemDamage(const FGameplayEffectSpec& Spec);
	static void ApplyItemPureDamage(AActor* SourceActor, AActor* TargetActor, float Damage, FName DamageType, bool bSuppressHitReact = true);

#if WITH_DEV_AUTOMATION_TESTS
	void SetSlotsForTest(const TArray<FCombatItemConfig>& InConfigs);
#endif

private:
	UPROPERTY(Transient)
	TArray<FRuntimeSlot> Slots;

	int32 ActiveSlotIndex = 0;

	void InitializeDefaultSlots();
	void BroadcastSlotsChanged() const;
	UYogAbilitySystemComponent* GetOwnerYogASC() const;
};
