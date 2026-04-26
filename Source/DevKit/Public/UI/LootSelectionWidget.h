// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameModes/LevelFlowTypes.h"
#include "LootSelectionWidget.generated.h"

class UHorizontalBox;
class UButton;
class URuneInfoCardWidget;
class ARewardPickup;

/**
 * 焦点段：DPad Up/Down 在两段间切换
 *  Cards    — 在卡片间 Left/Right 切换
 *  Buttons  — 在底部按钮（跳过 / 预览背包）间切换
 */
UENUM(BlueprintType)
enum class ELootFocusSection : uint8
{
	Cards,
	Buttons,
};

/**
 * 战利品选择 Widget — 动态卡牌（N=1~5）+ 跳过 + 背包预览 + 鼠标点击
 *
 * WBP 制作步骤：
 *   1. 新建 Widget Blueprint，父类选 LootSelectionWidget
 *   2. Designer 放（命名必须一致）：
 *        HorizontalBox  CardContainer        ← 卡片容器，运行时动态填充
 *        Button         BtnSkip              ← 跳过按钮（OnClicked 绑 SkipSelection）
 *        Button         BtnBackpackPreview   ← 预览背包按钮（OnClicked 绑 OpenBackpackPreview）
 *   3. Class Defaults 配 RuneCardClass = WBP_RuneInfoCard
 *
 * 触发方式：由 RewardPickup 调 HUD->QueueLootSelection(Options, this)，HUD 内部决定立即弹/排队
 */
UCLASS()
class DEVKIT_API ULootSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** 当前的战利品选项（BP 可读） */
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TArray<FLootOption> CurrentLootOptions;

	/** 卡片数量上限（生成逻辑应保证不超过此值，UI 也强制 clamp） */
	static constexpr int32 MaxCards = 5;

	/** 由 HUD 调用：显示战利品 UI（带 SourcePickup 引用） */
	void ShowLootUI(const TArray<FLootOption>& Options, ARewardPickup* InSourcePickup);

	// ---- 公开操作（Button OnClicked 绑定 / 鼠标卡片点击）----

	/** 选中第 Index 张符文：销毁 SourcePickup → 打开背包（整理）→ 通知 HUD */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SelectRuneLoot(int32 Index);

	/** 跳过选择：复位 SourcePickup → 通知 HUD（pickup 保留在关卡里，玩家可再按 E 重开） */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SkipSelection();

	/** 打开背包预览（只读模式），关闭后自动恢复 LootSelection */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void OpenBackpackPreview();

	/**
	 * 重掷某张卡（暂未实现，留作后续 UI 动画扩展用）
	 * 后续可能支持选一张卡进行重新抽取（消耗资源、展示动画等）
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void RerollCard(int32 Index);

	// ---- 已废弃的旧接口（保留以兼容关卡引用） ----

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void ConfirmAndTransition();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	// ── CommonUI ────────────────────────────────────────────────
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	// ---- BP 可重写事件（高亮/动画等纯视觉表现） ----

	/** 选项就绪时调用：BP 可在此做整体淡入或自定义初始化（C++ 已处理 RebuildCards）*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnLootOptionsReady(const TArray<FLootOption>& LootOptions);

	/** 卡片焦点变化：BP 可在此做高亮/缩放（C++ 已切换 GenericEffectList 显隐） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnCardFocused(int32 FocusedIndex);

	/** A/D 导航：BP 可加音效（Delta=-1/+1）*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnNavigateSelection(int32 Delta);

	/** 段切换：BP 可调整按钮区/卡片区的高亮 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnSectionFocused(ELootFocusSection NewSection, int32 IndexInSection);

	/** 关卡阶段变化：BP 控制整体显隐 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnLevelPhaseChanged(ELevelPhase NewPhase);

	// ── BindWidget ──────────────────────────────────────────────

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> CardContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnSkip;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnBackpackPreview;

	// ── 配置 ────────────────────────────────────────────────────

	/** 单张卡 WBP 类（Class Defaults 配 WBP_RuneInfoCard） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TSubclassOf<URuneInfoCardWidget> RuneCardClass;

private:
	void RebuildCards(const TArray<FLootOption>& Options);
	void FocusCard(int32 Idx);
	void FocusButton(int32 Idx);
	void SetSection(ELootFocusSection NewSection);
	void FinishAndNotifyHUD();
	void ReactivateAfterPreview();
	int32 ClampCardIndex(int32 Idx) const;

	// 5 个静态卡片点击回调（dynamic delegate 不支持 lambda/带参绑定）
	UFUNCTION() void OnCardClicked0();
	UFUNCTION() void OnCardClicked1();
	UFUNCTION() void OnCardClicked2();
	UFUNCTION() void OnCardClicked3();
	UFUNCTION() void OnCardClicked4();

	UFUNCTION() void OnBtnSkipClicked();
	UFUNCTION() void OnBtnBackpackPreviewClicked();

	UFUNCTION() void HandlePhaseChanged(ELevelPhase NewPhase);

	// 状态
	TWeakObjectPtr<ARewardPickup> SourcePickup;

	UPROPERTY()
	TArray<TObjectPtr<URuneInfoCardWidget>> SpawnedCards;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> SpawnedCardButtons;

	/** 卡片数组索引 → CurrentLootOptions 索引（无效项被跳过时映射不连续） */
	TArray<int32> CardToOptionIndex;

	ELootFocusSection CurrentSection = ELootFocusSection::Cards;
	int32 CurrentCardIndex = 0;
	int32 CurrentButtonIndex = 0; // 0 = Skip, 1 = BackpackPreview
	bool bStickNavHeld = false;
	bool bIsBackpackPreviewing = false;

	static constexpr int32 NumBottomButtons = 2;
};
