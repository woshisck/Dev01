#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InfoPopupWidget.generated.h"

class UCommonRichTextBlock;
class UImage;
class UBackgroundBlur;
class UButton;
class UWidgetAnimation;
class ULevelInfoPopupDA;

/**
 * 关卡信息提示浮窗 — 轻量独立 Widget，不依赖 TutorialManager。
 * 特性：背景图 + 模糊 + 文字，计时自动关闭（或点击关闭按钮）。
 * 不暂停游戏，不锁定输入。
 *
 * WBP 布局见配置文档，必要控件：BodyText
 * HUD 配置：BP_YogHUD Details → InfoPopup → InfoPopupWidgetClass = WBP_InfoPopup
 */
UCLASS()
class DEVKIT_API UInfoPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 显示弹窗（由 YogHUD 调用） */
	void Show(const ULevelInfoPopupDA* DA);

	/** 请求关闭（触发淡出动画，动画结束后 Collapse） */
	UFUNCTION(BlueprintCallable)
	void RequestClose();

	/** 弹窗完全关闭后广播（供 LENode_ShowInfoPopup 等待） */
	FSimpleMulticastDelegate OnClosed;

	// ——— 必须绑定 ———
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonRichTextBlock> BodyText;

	// ——— 可选绑定 ———
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonRichTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBackgroundBlur> BlurPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnClose;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_FadeIn;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_FadeOut;

protected:
	virtual void NativeConstruct() override;

private:
	FTimerHandle AutoCloseTimer;

	UFUNCTION()
	void OnBtnCloseClicked();

	UFUNCTION()
	void OnFadeOutFinished();

	void DoClose();
};
