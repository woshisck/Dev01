#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GenericEffectListWidget.generated.h"

class UVerticalBox;
class UImage;
class UTextBlock;
class UCommonRichTextBlock;
class UGenericRuneEffectDA;

/**
 * 单条通用效果条目（C++ 全自动填充，BP 只配布局）
 *
 * 蓝图制作步骤：
 *   1. 新建 Widget Blueprint，父类选 GenericEffectEntryWidget
 *   2. Designer 里放：
 *        Image                 命名 "EffectIcon"
 *        TextBlock             命名 "EffectName"
 *        CommonRichTextBlock   命名 "EffectDesc"（配 BP_InputActionDecorator）
 *   3. 不需要写 Event Graph — C++ SetData 自动赋值
 */
UCLASS(Blueprintable)
class DEVKIT_API UGenericEffectEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Effect")
	void SetData(UGenericRuneEffectDA* Effect);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> EffectIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EffectName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonRichTextBlock> EffectDesc;
};


/**
 * 通用效果列表小窗
 *
 * 蓝图制作步骤：
 *   1. 新建 Widget Blueprint，父类选 GenericEffectListWidget
 *   2. Designer 放一个 VerticalBox，命名 "EntriesBox"
 *   3. 在 Class Defaults → EntryClass 选择 WBP_GenericEffectEntry
 *   4. 父容器（如 RuneInfoCard）通过 BindWidgetOptional 绑定本 widget；空数组时自动 Collapsed
 */
UCLASS(Blueprintable)
class DEVKIT_API UGenericEffectListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Effect")
	void SetEffects(const TArray<UGenericRuneEffectDA*>& Effects);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TSubclassOf<UGenericEffectEntryWidget> EntryClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> EntriesBox;
};
