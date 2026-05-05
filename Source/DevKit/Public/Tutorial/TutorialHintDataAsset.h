#pragma once

#include "CoreMinimal.h"
#include "TutorialHintDataAsset.generated.h"

// Tutorial flow state machine.
// Do not reorder values: this enum is persisted in save data.
UENUM(BlueprintType)
enum class ETutorialState : uint8
{
	None                    UMETA(DisplayName = "None"),
	NeedWeaponTutorial      UMETA(DisplayName = "等待拾取武器"),
	WeaponTutorialDone      UMETA(DisplayName = "武器引导完成"),
	NeedFirstRuneTutorial   UMETA(DisplayName = "等待首次奖励卡牌"),
	NeedBackpackTutorial    UMETA(DisplayName = "等待首次打开卡组编排"),
	NeedHeatPhaseTutorial   UMETA(DisplayName = "等待首次连携教学"),
	NeedPostCombatTutorial  UMETA(DisplayName = "旧战斗后教程兼容"),
	Completed               UMETA(DisplayName = "引导完成"),
};
