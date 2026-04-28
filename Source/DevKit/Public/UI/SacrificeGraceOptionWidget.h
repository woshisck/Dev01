#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "SacrificeGraceOptionWidget.generated.h"

class UTextBlock;
class UButton;
class USacrificeGraceDA;
class APlayerCharacterBase;
class ASacrificeGracePickup;

/**
 * USacrificeGraceOptionWidget — 献祭恩赐确认弹窗
 *
 * 玩家按 E 键触发 SacrificeGracePickup 后弹出。
 * 展示 DA 中的名称与效果描述，提供 Yes / No 两个选择：
 *   Yes → AcquireSacrificeGrace → 销毁拾取物 → 关闭
 *   No  → 复位拾取物状态 → 关闭（玩家可再次靠近触发）
 *
 * WBP 制作步骤：
 *   1. 新建 Widget Blueprint，父类选 SacrificeGraceOptionWidget
 *   2. Designer 中放以下控件（命名必须完全一致）：
 *        TextBlock  TitleText        ← 显示 DA.DisplayName
 *        TextBlock  DescriptionText  ← 显示 DA.Description
 *        Button     BtnYes           ← 确认接受
 *        Button     BtnNo            ← 放弃
 *   3. 在 Class Defaults 无需额外配置，C++ 已绑定所有按钮逻辑
 */
UCLASS(Abstract, Blueprintable)
class DEVKIT_API USacrificeGraceOptionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** 由 HUD 在 ActivateWidget 之前调用，注入 DA / 玩家 / 来源拾取物 */
	void Setup(USacrificeGraceDA* InDA, APlayerCharacterBase* InPlayer, ASacrificeGracePickup* InPickup);

	UFUNCTION(BlueprintCallable, Category = "SacrificeGrace")
	void CancelChoice();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** BP 事件：Setup 完成后调用，供 BP 侧执行自定义动画或额外数据绑定 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SacrificeGrace")
	void OnSetup(USacrificeGraceDA* DA);

	// ── BindWidget 控件（名称必须与 WBP 中完全一致）──────────────────

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnYes;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnNo;

private:
	UPROPERTY()
	TObjectPtr<USacrificeGraceDA> SacrificeDA;

	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;
	TWeakObjectPtr<ASacrificeGracePickup> SourcePickup;

	UFUNCTION()
	void OnYesClicked();

	UFUNCTION()
	void OnNoClicked();
};
