#pragma once

#include "CoreMinimal.h"
#include "TutorialHintDataAsset.generated.h"

// 教程引导状态机
// 新存档默认从 NeedWeaponTutorial 开始，引导完成后持久化为 Completed
UENUM(BlueprintType)
enum class ETutorialState : uint8
{
	None                    UMETA(DisplayName = "None"),
	NeedWeaponTutorial      UMETA(DisplayName = "等待拾取武器"),
	WeaponTutorialDone      UMETA(DisplayName = "武器引导完成"),
	NeedPostCombatTutorial  UMETA(DisplayName = "等待战斗后三选一"),
	Completed               UMETA(DisplayName = "引导完成"),
};
