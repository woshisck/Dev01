// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "Data/RuneDataAsset.h"

#include "PlayerCharacterBase.generated.h"

/**
 * 
 */
//class AAuraBase;
class UYogSaveGame;
class UBackpackGridComponent;
class UBuffFlowComponent;
class USkillChargeComponent;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoldChangedDelegate, int32, NewGold);

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

	UPROPERTY()
	TObjectPtr<AItemSpawner> OverlappingSpawner;

	UFUNCTION(BlueprintPure, Category = "Backpack")
	UBackpackGridComponent* GetBackpackGridComponent();

	// 将符文加入待放置列表（由 GameMode 的 SelectLoot 调用）
	UFUNCTION(BlueprintCallable, Category = "Backpack")
	void AddRuneToInventory(const FRuneInstance& Rune);

	// 待放置符文列表（整理阶段从此处拖放到格子）
	UPROPERTY(BlueprintReadOnly, Category = "Backpack")
	TArray<FRuneInstance> PendingRunes;

	// ─── 货币 ─────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "Currency")
	int32 Gold = 0;

	UPROPERTY(BlueprintAssignable, Category = "Currency")
	FGoldChangedDelegate OnGoldChanged;

	UFUNCTION(BlueprintCallable, Category = "Currency")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Currency")
	int32 GetGold() const { return Gold; }

	// 切关后从 GameInstance.PendingRunState 恢复 HP / 金币 / 符文 / 热度阶段
	void RestoreRunStateFromGI();

	// ─── 最后输入方向（冲刺朝向使用）────────────────────────────────
	// 由 Controller.Move() 在每次非零输入时更新，世界空间单位向量
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FVector LastInputDirection = FVector::ForwardVector;

	friend UPlayerAttributeSet;

protected:

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AYogCameraPawn> CameraPawnActor;



};
