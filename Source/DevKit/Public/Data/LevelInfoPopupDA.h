#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelInfoPopupDA.generated.h"

/**
 * 关卡信息提示弹窗数据资产。
 * 只需填写文字和显示时长，不依赖 DialogContentDA。
 *
 * 创建方式：Content Browser → 右键 → Data Asset → 搜 LevelInfoPopupDA
 * 命名建议：DA_InfoPopup_XXX
 */
UCLASS(BlueprintType)
class DEVKIT_API ULevelInfoPopupDA : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 标题（可留空，留空时 TitleText 控件自动隐藏） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "内容")
	FText Title;

	/** 正文 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "内容", meta = (MultiLine = true))
	FText Body;

	/** HUD 简短摘要。用于底部信息区等紧凑浮窗；建议控制在 1-2 行，留空时继续显示 Body。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "内容", meta = (MultiLine = true))
	FText HUDSummaryText;

	/**
	 * 自动关闭时长（秒）。
	 * 0 = 不自动关闭，需玩家点击关闭按钮（WBP 里需有 BtnClose 控件）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "行为", meta = (ClampMin = "0"))
	float DisplayDuration = 4.0f;
};
