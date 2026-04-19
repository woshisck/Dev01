#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/LevelFlowTypes.h"
#include "RuneRewardFloatWidget.generated.h"

class UVerticalBox;
class UTextBlock;

/**
 * 符文奖励拾取浮窗 — 玩家靠近 ARewardPickup 时自动显示可选符文列表。
 *
 * WBP 控件（BindWidgetOptional）：
 *   RuneListBox    VerticalBox  动态填充每个可选符文行（图标 + 名称）
 *   PickupHintText TextBlock    静态提示文字（如 "按 E 选择符文"，直接在 WBP 中填写）
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API URuneRewardFloatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RuneRewardFloat")
	void SetLootOptions(const TArray<FLootOption>& Options);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RuneListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PickupHintText;
};
