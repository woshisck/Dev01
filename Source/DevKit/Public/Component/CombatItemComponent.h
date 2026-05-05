#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "Data/CombatItemDataAsset.h"
#include "GameplayEffectTypes.h"
#include "CombatItemComponent.generated.h"

class ACombatItemAreaActor;
class APlayerCharacterBase;
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
	ECombatItemEffectType EffectType = ECombatItemEffectType::OilBottle;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	int32 Charges = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	int32 MaxCharges = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	float CooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	float CooldownDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Item")
	bool bSelected = false;
};

USTRUCT()
struct DEVKIT_API FRuntimeSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FCombatItemConfig Config;

	UPROPERTY()
	int32 Charges = 0;

	UPROPERTY()
	float CooldownRemaining = 0.0f;
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Item|Loadout")
	TArray<TObjectPtr<UCombatItemDataAsset>> DefaultItemAssets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Item|Loadout")
	bool bUseBuiltInDefaultItemsWhenEmpty = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Item|Spawn")
	TSubclassOf<ACombatItemAreaActor> AreaActorClass;

	UPROPERTY(BlueprintAssignable, Category = "Combat Item")
	FCombatItemSlotsChangedDelegate OnItemSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat Item")
	FCombatItemUsedDelegate OnItemUsed;

	UPROPERTY(BlueprintAssignable, Category = "Combat Item")
	FCombatItemUseFailedDelegate OnItemUseFailed;

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

	UFUNCTION(BlueprintCallable, Category = "Combat Item|Oil")
	bool ApplyOilBladeHitToTarget(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Combat Item|Smoke")
	void SetPlayerInsideSmoke(bool bInside, float DodgeBonus);

	UFUNCTION(BlueprintPure, Category = "Combat Item|Oil")
	static int32 GetStickyOilStackCount(const AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Combat Item|Oil")
	static float GetStickyOilTurnSpeedMultiplier(const AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Combat Item|Oil")
	static float GetStickyOilMoveSpeedMultiplier(const AActor* TargetActor);

	static void TryApplyOilFireBonus(UYogAbilitySystemComponent* SourceASC, UYogAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& Spec);
	static bool IsNoHitReactItemDamage(const FGameplayEffectSpec& Spec);
	static void ApplyItemPureDamage(AActor* SourceActor, AActor* TargetActor, float Damage, FName DamageType, bool bSuppressHitReact = true);

#if WITH_DEV_AUTOMATION_TESTS
	void SetSlotsForTest(const TArray<FCombatItemConfig>& InConfigs);
	void AdvanceCooldownsForTest(float DeltaTime);
#endif

private:
	struct FStickyOilRuntimeState
	{
		TWeakObjectPtr<AActor> TargetActor;
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		int32 Stacks = 0;
		float MoveSpeedDelta = 0.0f;
		float AttackSpeedDelta = 0.0f;
		FTimerHandle ExpireTimer;
	};

	UPROPERTY(Transient)
	TArray<FRuntimeSlot> Slots;

	int32 ActiveSlotIndex = 0;
	FTimerHandle OilBladeTimer;
	bool bSmokeDodgeApplied = false;
	int32 ActiveSmokeAreaCount = 0;
	float SmokeDodgeAppliedDelta = 0.0f;

	TMap<TObjectKey<AActor>, FStickyOilRuntimeState> StickyOilStates;

	void InitializeDefaultSlots();
	void BroadcastSlotsChanged() const;
	bool ExecuteItemEffect(const FRuntimeSlot& Slot);
	bool ExecuteOilBottle(const FCombatItemConfig& Config);
	bool ExecuteThunderStone(const FCombatItemConfig& Config);
	bool ExecuteSmokeBomb(const FCombatItemConfig& Config);
	void EndOilBlade();
	void ExpireStickyOil(AActor* TargetActor);
	void ClearAllStickyOil();
	void ApplyStickyOilAttributeDeltas(FStickyOilRuntimeState& State, const FCombatItemOilConfig& OilConfig);
	void RemoveStickyOilAttributeDeltas(FStickyOilRuntimeState& State);
	APlayerCharacterBase* GetPlayerOwner() const;
	UYogAbilitySystemComponent* GetOwnerYogASC() const;
};
