#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "GameDialogWidget.generated.h"

// 单页数据
USTRUCT(BlueprintType)
struct FTutorialPage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText Body;

	// 左侧插图（留空则插图区域保持黑色占位）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Illustration = nullptr;

	// 补充说明（留空则 BodySubText 自动隐藏）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText SubText;
};

// 教程弹窗 Widget — 支持多页翻页
// C++ 控制全部逻辑，WBP 放四个命名控件：
//   TitleText / BodyText / BtnConfirm / BtnConfirmLabel（按钮内的文字）
UCLASS()
class DEVKIT_API UTutorialPopupWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// 显示弹窗（由 TutorialManager 调用，支持单页或多页）
	void ShowPopup(const TArray<FTutorialPage>& InPages);

	// BtnConfirm 点击：非末页则翻页，末页则关闭
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void OnNextPressed();

	// 当前页索引（0-based），蓝图可读
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	int32 CurrentPage = 0;

	// 总页数，蓝图可读
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	int32 TotalPages = 0;

protected:
	virtual void NativeConstruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	// 四个命名控件，WBP 中名字必须完全一致
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnConfirm;

	// 按钮内的文字控件（非末页显示"下一页"，末页显示"知道了"）
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BtnConfirmLabel;

	// 页码显示（可选，显示"1 / 3"；只有一页时自动隐藏）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PageHint;

	// 补充说明文字（可选，SubText 为空时自动 Collapsed）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodySubText;

	// 左侧插图（可选，有贴图时显示，无贴图时保持黑色背景）
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IllustrationImage;

private:
	TArray<FTutorialPage> Pages;

	void RefreshPage();
};
