// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "Data/RuneDataAsset.h"
#include "GameplayEffectTypes.h"

#include "PlayerCharacterBase.generated.h"

/**
 * 
 */
//class AAuraBase;
class UYogSaveGame;
class UBackpackGridComponent;
class UBuffFlowComponent;
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

	// ─── 风行者冲刺充能 ───────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	int32 DashChargeCount = 0;

	int32 MaxDashChargeCount = 2;
	float DashChargeRegenInterval = 3.0f;
	FTimerHandle DashChargeRegenTimer;

	/** 激活风行者符文时调用：充满2格充能并启动回复计时器 */
	UFUNCTION(BlueprintCallable, Category = "Dash")
	void InitDashChargeSystem();

	/** 卸下风行者符文时调用：清空充能并停止计时器 */
	UFUNCTION(BlueprintCallable, Category = "Dash")
	void ShutdownDashChargeSystem();

	friend UPlayerAttributeSet;


protected:

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AYogCameraPawn> CameraPawnActor;



};
