#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonInputTypeEnum.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "GameDialogWidget.generated.h"

class UYogCommonRichTextBlock;

// 单页数据
USTRUCT(BlueprintType)
struct FTutorialPage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Illustration = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText SubText;
};

// 教程弹窗 Widget — 支持多页翻页
// WBP 只需：正确命名控件 + 创建两个 UMG 动画（Anim_FadeIn / Anim_FadeOut）
// 不需要任何 Event Graph 节点
UCLASS()
class DEVKIT_API UTutorialPopupWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// bPauseGame=false 可显示不暂停游戏的纯信息浮窗
	void ShowPopup(const TArray<FTutorialPage>& InPages, bool bPauseGame = true);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void OnNextPressed();

	// 渐出动画结束后由 C++ 自动调用，无需在 WBP 手动调用
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ConfirmClose();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	int32 CurrentPage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	int32 TotalPages = 0;

	virtual void NativeConstruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;

	// ——— BindWidget 控件（名字必须与 WBP 中完全一致）———
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UYogCommonRichTextBlock> BodyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnConfirm;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BtnConfirmLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UYogCommonRichTextBlock> BtnConfirmInputHint;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PageHint;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UYogCommonRichTextBlock> BodySubText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IllustrationImage;

	// ——— BindWidgetAnim（WBP 中动画名必须与变量名完全一致）———
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_FadeIn;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_FadeOut;

private:
	TArray<FTutorialPage> Pages;
	bool bIsInteractable = false; // FadeIn 完成前禁止翻页/关闭，防止幽灵输入
	bool bPauseMe = true;         // ShowPopup 时传入，控制 NativeOnActivated 是否暂停游戏
	void RefreshPage();
	void RefreshConfirmInputHint(ECommonInputType NewInputType);

	UFUNCTION()
	void OnFadeInAnimFinished();

	UFUNCTION()
	void OnFadeOutAnimFinished();
};
