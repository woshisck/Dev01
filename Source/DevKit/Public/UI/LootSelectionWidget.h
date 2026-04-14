// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/LevelFlowTypes.h"
#include "LootSelectionWidget.generated.h"

/**
 * 战利品三选一 Widget 基类
 *
 * 使用方法：
 * 1. 在 Content Browser 新建 Widget Blueprint，父类选 LootSelectionWidget
 * 2. 实现 OnLootOptionsReady 事件：根据 LootOptions 数组填充 3 张符文卡片
 * 3. 实现 OnPhaseChanged 事件：Arrangement 时显示面板，其他阶段隐藏
 * 4. 3 张卡片的按钮点击分别调用 SelectRuneLoot(0/1/2)
 * 5. "确认" 按钮调用 ConfirmAndTransition
 * 6. 在 PlayerController 或 HUD 的 BeginPlay 里创建本 Widget 并 AddToViewport
 */
UCLASS()
class DEVKIT_API ULootSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 当前的三个战利品选项（UI 可直接读取符文名称和图标）
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TArray<FLootOption> CurrentLootOptions;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ---- Blueprint 实现这两个事件 ----

	// 收到战利品列表时触发，在 BP 里填充 3 张符文卡片的数据和图标
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnLootOptionsReady(const TArray<FLootOption>& LootOptions);

	// 关卡阶段变化时触发（Combat / Arrangement / Transitioning）
	// 在 BP 里控制整个面板的 Show/Hide
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnLevelPhaseChanged(ELevelPhase NewPhase);

public:
	// ---- 按钮绑定的函数 ----

	// 三张符文卡片的点击事件，Index = 0/1/2
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SelectRuneLoot(int32 Index);

	// A/D键盘导航：Delta=-1(左) +1(右)，蓝图实现高亮逻辑
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnNavigateSelection(int32 Delta);

	// "确认整理" 按钮：锁背包并加载下一关
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void ConfirmAndTransition();

private:
	UFUNCTION()
	void HandleLootGenerated(const TArray<FLootOption>& LootOptions);

	UFUNCTION()
	void HandlePhaseChanged(ELevelPhase NewPhase);
};
