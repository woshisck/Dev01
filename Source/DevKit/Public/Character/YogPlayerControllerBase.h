// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"

#include "YogPlayerControllerBase.generated.h"


class AYogCameraPawn;
class AYogCharacterBase;
class AYogPlayerCameraManager;
class UInputMappingContext;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerHasInput, const FVector, Velocity);

UCLASS()
class DEVKIT_API AYogPlayerControllerBase : public AModularPlayerController
{
	GENERATED_BODY()

public:
	AYogPlayerControllerBase();


	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyParams& Params) override;


	//UFUNCTION(BlueprintCallable)
	//AYogCharacterBase* GetPossCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setting")
	TSubclassOf<AYogCameraPawn> CameraPawnClass;


	void LightAtack(const FInputActionValue& Value);
	void HeavyAtack(const FInputActionValue& Value);
	void HeavyAttackReleased(const FInputActionValue& Value);
	void MusketReload(const FInputActionValue& Value);
	void UseCombatItem(const FInputActionValue& Value);
	void SwitchCombatItem(const FInputActionValue& Value);
	void SwitchCombatItemPrevious(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_LightAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_HeavyAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_Dash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_Interact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_OpenBackpack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_Reload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_UseCombatItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_SwitchCombatItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_SwitchCombatItemPrevious;

	/** 手柄右摇杆 — 驱动相机微偏移（Vector2D InputAction） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_CameraLook;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UBackpackScreenWidget> BackpackWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class ULootSelectionWidget> LootSelectionWidgetClass;

	/** 战斗 HUD Widget 类（Details 面板里填入，如 WBP_CombatHUD）
	 *  有 MenuWidget 激活时自动隐藏，全部关闭后恢复显示 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> CombatHUDClass;

	UFUNCTION(BlueprintCallable)
	void OnInteractTriggered(const AItemSpawner* item);

	/////////////////////////////////////////// INPUT ACTION ///////////////////////////////////////////

	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "Character|Movement")
	FPlayerHasInput OnPlayerHasInput;



	UFUNCTION(BlueprintCallable)
	void ToggleInput(bool bEnable);


	UFUNCTION(BlueprintCallable, Category = "ASC")
	UYogAbilitySystemComponent* GetYogAbilitySystemComponent() const;


	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SetEnableRotationRate(FRotator RotationRate, bool isEnable);

	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SpawnCameraPawn(AYogCharacterBase* PossessedCharacter);

	UFUNCTION(BlueprintCallable, Category = "ASC")
	AYogCharacterBase* GetControlledCharacter();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetPlayerState(EYogCharacterState newState);

	void ToggleBackpack(const FInputActionValue& Value);

	// 由 TutorialManager 调用：强制打开背包（未激活时才打开）
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenBackpack();

	void CameraLook(const FInputActionValue& Value);
	void CameraLookReleased(const FInputActionValue& Value);

	/**
	 * 当任何 UI（背包/三选一）打开时调用 true，关闭时调用 false
	 * 蓝图可通过 "Get Owning Player → Cast → SetBlockGameInput" 调用
	 */
	/**
	 * bBlock=true 时屏蔽游戏输入并切换输入模式
	 * bUIOnly=true  → UIOnly 模式（三选一：LMB 不被攻击消耗，按钮可点击）
	 * bUIOnly=false → GameAndUI 模式（背包：Tab 键仍可关闭）
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetBlockGameInput(bool bBlock, bool bUIOnly = false);

	// ── Push/Pull 菜单计数 ──────────────────────────────────────────
	/** 当前有几个菜单层 widget 处于激活状态；归零时显示 CombatHUDWidget */
	void OnMenuWidgetActivated();
	void OnMenuWidgetDeactivated();

private:
	/** UI 打开期间为 true，屏蔽移动/攻击/冲刺输入 */
	bool HandleMenuBackInput(const FKey& Key);

	bool bBlockGameInput = false;

	int32 ActiveMenuCount = 0;

	UPROPERTY()
	TObjectPtr<UUserWidget> CombatHUDWidget;

	UPROPERTY()
	TObjectPtr<class ULootSelectionWidget> LootSelectionWidget;

	uint32 MoveInputHandle = INDEX_NONE;
	uint32 LightAttackInputHandle = INDEX_NONE;
	uint32 HeavyAttackInputHandle = INDEX_NONE;
	uint32 ReloadInputHandle = INDEX_NONE;
	uint32 UseCombatItemInputHandle = INDEX_NONE;
	uint32 SwitchCombatItemInputHandle = INDEX_NONE;
	uint32 SwitchCombatItemPreviousInputHandle = INDEX_NONE;
	uint32 DashInputHandle = INDEX_NONE;
	uint32 InteractInputHandle = INDEX_NONE;
	uint32 OpenBackpackInputHandle = INDEX_NONE;
	uint32 CameraLookInputHandle = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<class UBackpackScreenWidget> BackpackWidget;
};
