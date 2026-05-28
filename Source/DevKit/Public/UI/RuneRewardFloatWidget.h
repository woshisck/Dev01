#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameModes/LevelFlowTypes.h"
#include "RuneRewardFloatWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UWidget;

/**
 * 符文奖励拾取浮窗 — 玩家靠近 ARewardPickup 时自动显示可选符文列表。
 *
 * WBP 控件（BindWidgetOptional）：
 *   RuneListBox    VerticalBox  动态填充每个可选符文行（图标 + 名称）
 *   PickupHintText RichText/TextBlock    推荐显示 `<input action="Interact"/> 拾取`
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API URuneRewardFloatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RuneRewardFloat")
	void SetLootOptions(const TArray<FLootOption>& Options);

	UFUNCTION(BlueprintCallable, Category = "RuneRewardFloat")
	void PlayPromptHighlightPulse(float DurationSeconds);

	static float ComputePromptHighlightScale(float ElapsedSeconds, float DurationSeconds);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RuneListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PickupHintText;

	void RefreshPickupHint();
	void AddRewardRow(const FText& Name, UTexture2D* IconTexture, const FLinearColor& FallbackColor);

private:
	float PromptHighlightElapsed = -1.f;
	float PromptHighlightDuration = 0.f;
};
