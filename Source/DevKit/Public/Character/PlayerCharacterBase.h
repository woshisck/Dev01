// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "Data/RuneDataAsset.h"
#include "GameFramework/ForceFeedbackEffect.h"

#include "PlayerCharacterBase.generated.h"

/**
 * 
 */
//class AAuraBase;
class ARewardPickup;
class AWeaponSpawner;
class AWeaponInstance;
class UYogSaveGame;
class UBackpackGridComponent;
class UBuffFlowComponent;
class USkillChargeComponent;
class UWeaponDefinition;
UENUM()
enum class EPlayerState : uint8
{
	OnMove			UMETA(DisplayName = "OnMove"),
	OnAction		UMETA(DisplayName = "Action")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerStateDelegate, EPlayerState, State);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemInteractEndDelegate, APlayerCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemInteractStartDelegate, APlayerCharacterBase*, Character, AActor*, actor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeatUpdateDelegate, const float, HeatPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMaxHeatUpdateDelegate, const float, MaxHeatValue);
// 热度阶段变化（0=无热度, 1=白光, 2=绿光, 3=橙黄, 4=过热红光）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeatPhaseDelegate, int32, Phase);

UCLASS()
class DEVKIT_API APlayerCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnRep_PlayerState() override;


	APlayerCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void SetOwnCamera(AYogCameraPawn* cameraActor);

	UFUNCTION(BlueprintCallable)
	AYogCameraPawn* GetOwnCamera();

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die() override;

	UFUNCTION(BlueprintCallable, Category = "Item")
	void ItemInteract(const AItemSpawner* item);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TObjectPtr<UBackpackGridComponent> BackpackGridComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuffFlow")
	TObjectPtr<UBuffFlowComponent> BuffFlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkillCharge")
	TObjectPtr<USkillChargeComponent> SkillChargeComponent;

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TObjectPtr<UPlayerAttributeSet> PlayerAttributeSet;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FItemInteractStartDelegate OnItemInterActionStart;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FItemInteractEndDelegate OnItemInterActionEnd;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FPlayerStateDelegate OnPlayerStateUpdate;

	UPROPERTY(BlueprintAssignable, Category = "State")
	FPlayerStateDelegate OnFPlayerStateDeleg;

	// 热度阶段广播（由热度系统调用，武器 Instance 订阅以更新发光颜色）
	UPROPERTY(BlueprintAssignable, Category = "Heat")
	FHeatPhaseDelegate OnHeatPhaseChanged;

	/** 热度升阶时的手柄震动效果（在角色蓝图 Details 中填入 ForceFeedbackEffect 资产） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Heat|Feedback")
	TObjectPtr<UForceFeedbackEffect> PhaseUpForceFeedback;

	UPROPERTY()
	TObjectPtr<AItemSpawner> OverlappingSpawner;

	// 当前在拾取范围内的 RewardPickup（按 E 键时触发拾取）
	UPROPERTY()
	TObjectPtr<ARewardPickup> PendingPickup;

	// 当前在拾取范围内的 WeaponSpawner（按 E 键时触发武器拾取）
	UPROPERTY()
	TObjectPtr<AWeaponSpawner> PendingWeaponSpawner;

	// 当前装备的武器 Actor（换武器时 Destroy）
	UPROPERTY()
	TObjectPtr<AWeaponInstance> EquippedWeaponInstance;

	// 提供当前武器的 Spawner（换武器时恢复其展示网格颜色）
	UPROPERTY()
	TObjectPtr<AWeaponSpawner> EquippedFromSpawner;

	UFUNCTION(BlueprintPure, Category = "Backpack")
	UBackpackGridComponent* GetBackpackGridComponent();

	// 将符文加入待放置列表（由 GameMode 的 SelectLoot 调用）
	UFUNCTION(BlueprintCallable, Category = "Backpack")
	void AddRuneToInventory(const FRuneInstance& Rune);

	// 待放置符文列表（整理阶段从此处拖放到格子）
	UPROPERTY(BlueprintReadOnly, Category = "Backpack")
	TArray<FRuneInstance> PendingRunes;

	// 切关后从 GameInstance.PendingRunState 恢复 HP / 金币 / 符文 / 热度阶段
	void RestoreRunStateFromGI();

	// BeginPlay 末尾重新 Link 武器动画层（GAS 授能可能覆盖切关时已 Link 的层）
	void RelinkWeaponAnimLayer();

	// 当前装备的武器定义（切关时写入 RunState，由 RestoreRunStateFromGI 重新装备）
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> EquippedWeaponDef;

	// ─── 最后输入方向（冲刺朝向使用）────────────────────────────────
	// 由 Controller.Move() 在每次非零输入时更新，世界空间单位向量
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FVector LastInputDirection = FVector::ForwardVector;

	friend UPlayerAttributeSet;

protected:

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AYogCameraPawn> CameraPawnActor;

private:

	/** BeginPlay 中注册 GAS Tag 事件，监听热度阶段变化并广播 OnHeatPhaseChanged */
	void SetupHeatPhaseTagListeners();

	/** Phase.1/2/3 tag 新增时广播对应阶段；移除时由 parent tag 回调处理 */
	void OnHeatPhaseTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** parent tag Buff.Status.Heat.Phase 计数归零时广播 Phase=0（关闭发光） */
	void OnHeatPhaseParentTagChanged(const FGameplayTag Tag, int32 NewCount);

};
