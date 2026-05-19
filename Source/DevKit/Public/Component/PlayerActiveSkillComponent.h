#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/ActiveSkillDataAsset.h"
#include "PlayerActiveSkillComponent.generated.h"

class APlayerCharacterBase;
class UTexture2D;
class UYogAbilitySystemComponent;

USTRUCT(BlueprintType)
struct DEVKIT_API FActiveSkillSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Active Skill")
	FName SkillId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Active Skill")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Active Skill")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Active Skill")
	float CooldownRemaining = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Active Skill")
	float CooldownDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Active Skill")
	bool bSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Active Skill")
	bool bLocked = false;
};

USTRUCT()
struct DEVKIT_API FActiveSkillRuntimeSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UActiveSkillDataAsset> SkillAsset = nullptr;

	UPROPERTY()
	FActiveSkillConfig Config;

	UPROPERTY()
	float CooldownRemaining = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActiveSkillSlotsChangedDelegate, const TArray<FActiveSkillSlotView>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActiveSkillUseFailedDelegate, int32, SlotIndex, FText, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActiveSkillUsedDelegate, int32, SlotIndex, FActiveSkillSlotView, Slot);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UPlayerActiveSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerActiveSkillComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Active Skill|Loadout", meta = (ClampMin = "1"))
	int32 UnlockedSlotCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Active Skill|Loadout", meta = (ClampMin = "1"))
	int32 MaxSlotCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Active Skill|Loadout")
	TArray<TObjectPtr<UActiveSkillDataAsset>> DefaultSkillAssets;

	UPROPERTY(BlueprintAssignable, Category = "Active Skill")
	FActiveSkillSlotsChangedDelegate OnSkillSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Active Skill")
	FActiveSkillUsedDelegate OnSkillUsed;

	UPROPERTY(BlueprintAssignable, Category = "Active Skill")
	FActiveSkillUseFailedDelegate OnSkillUseFailed;

	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	bool UseActiveSkill();

	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	void SelectNextSkill();

	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "Active Skill")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Active Skill")
	TArray<FActiveSkillSlotView> GetSlotViews() const;

	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	void SetSkillLoadout(const TArray<UActiveSkillDataAsset*>& InSkills);

	UFUNCTION(BlueprintPure, Category = "Active Skill")
	TArray<UActiveSkillDataAsset*> GetSkillLoadout() const;

	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	void SetUnlockedSlotCount(int32 NewUnlockedSlotCount);

private:
	UPROPERTY(Transient)
	TArray<FActiveSkillRuntimeSlot> Slots;

	int32 ActiveSlotIndex = 0;

	void InitializeDefaultSlots();
	void RebuildSlotsFromAssets(const TArray<UActiveSkillDataAsset*>& InSkills);
	void BroadcastSlotsChanged() const;
	void GrantSkillAbilities();
	APlayerCharacterBase* GetPlayerOwner() const;
	UYogAbilitySystemComponent* GetOwnerYogASC() const;
};
