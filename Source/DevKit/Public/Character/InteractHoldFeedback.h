#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractHoldFeedback.generated.h"

UINTERFACE(MinimalAPI)
class UInteractHoldFeedback : public UInterface
{
	GENERATED_BODY()
};

/**
 * IInteractHoldFeedback — 按住 E 蓄力交互的进度反馈接口
 *
 * AYogPlayerControllerBase 拥有蓄力计时与取消规则；实现者只负责把归一化进度
 * 转发给自己已有的浮窗 / 提示 Widget。两个方法都有默认实现，目标可按需选择性覆写。
 */
class DEVKIT_API IInteractHoldFeedback
{
	GENERATED_BODY()

public:
	/** 0..1 during a hold, 0 on cancel or completion. */
	virtual void SetInteractHoldProgress(float Normalized) {}

	/** Negative = fall back to the controller's InteractHoldDuration. */
	virtual float GetInteractHoldDuration() const { return -1.f; }
};
