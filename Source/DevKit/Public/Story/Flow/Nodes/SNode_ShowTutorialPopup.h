#pragma once

#include "CoreMinimal.h"
#include "UI/GameDialogWidget.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_ShowTutorialPopup.generated.h"

/**
 * 触发教程弹窗后立即触发 Out（非阻塞）。
 * TutorialEventId 与 InlinePages 二选一：EventId 优先。
 */
UCLASS(meta = (DisplayName = "Show Tutorial Popup"))
class DEVKIT_API USNode_ShowTutorialPopup : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** 注册表中的 EventId（与 InlinePages 二选一）。 */
	UPROPERTY(EditAnywhere, Category = "Popup")
	FName TutorialEventId;

	/** 内联页面列表（TutorialEventId 为空时使用）。 */
	UPROPERTY(EditAnywhere, Category = "Popup")
	TArray<FTutorialPage> InlinePages;

	UPROPERTY(EditAnywhere, Category = "Popup")
	bool bPauseGame = true;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
