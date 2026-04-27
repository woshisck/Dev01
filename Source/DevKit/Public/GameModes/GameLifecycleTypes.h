#pragma once

#include "CoreMinimal.h"
#include "GameLifecycleTypes.generated.h"

/**
 * 游戏生命周期事件 — 由 AYogGameMode::TriggerLifecycleEvent 触发，
 * 在 BP_GameMode_Default.LifecycleEventFlows 里映射到 ULevelFlowAsset，
 * 用 LENode_ShowTutorial / LENode_Delay 等节点配置具体行为。
 *
 * 一次性事件（FirstRune* / HeatPhase* / GameStart / PlayerDeath）由 GameMode 内部去重。
 * 重复事件（LevelClear / LevelClearRevealed）每次触发都跑。
 */
UENUM(BlueprintType)
enum class EGameLifecycleEvent : uint8
{
    LevelClear            UMETA(DisplayName = "关卡完成（敌人全清）"),
    LevelClearRevealed    UMETA(DisplayName = "关卡揭幕完成（slow-mo + 黑屏退场后）"),
    FirstRuneAcquired     UMETA(DisplayName = "首次获得符文（任意来源）"),
    FirstRunePlaced       UMETA(DisplayName = "首次放置符文到背包"),
    HeatPhaseEntered      UMETA(DisplayName = "首次进入热度阶段（Phase>=1）"),
    PlayerDeath           UMETA(DisplayName = "玩家死亡"),
    GameStart             UMETA(DisplayName = "游戏开始（首关 BeginPlay）"),
};
