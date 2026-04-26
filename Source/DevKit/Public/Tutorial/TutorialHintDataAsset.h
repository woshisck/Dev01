#pragma once

#include "CoreMinimal.h"
#include "TutorialHintDataAsset.generated.h"

// 教程引导状态机
// 新存档默认从 NeedWeaponTutorial 开始，引导完成后持久化为 Completed
UENUM(BlueprintType)
enum class ETutorialState : uint8
{
	None                    UMETA(DisplayName = "None"),
	NeedWeaponTutorial      UMETA(DisplayName = "等待拾取武器"),                // 旧/兼容；新流程 ① 走 LevelFlow
	WeaponTutorialDone      UMETA(DisplayName = "武器引导完成"),                // 旧/兼容
	NeedFirstRuneTutorial   UMETA(DisplayName = "等待第一颗符文掉落"),          // 新 ②
	NeedBackpackTutorial    UMETA(DisplayName = "等待第一次打开背包"),          // 新 ③
	NeedHeatPhaseTutorial   UMETA(DisplayName = "等待热度首次入相"),            // 新 ④
	NeedPostCombatTutorial  UMETA(DisplayName = "等待战斗后三选一（已废弃）"),  // 旧/已废弃
	Completed               UMETA(DisplayName = "引导完成"),
};
