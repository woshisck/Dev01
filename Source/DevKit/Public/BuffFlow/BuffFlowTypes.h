#pragma once

#include "CoreMinimal.h"
#include "BuffFlowTypes.generated.h"

/**
 * 目标选择器 —— BuffFlow 节点中选择作用目标
 */
UENUM(BlueprintType)
enum class EBFTargetSelector : uint8
{
	BuffOwner    UMETA(DisplayName = "Buff拥有者"),
	BuffGiver    UMETA(DisplayName = "Buff施加者"),
};
