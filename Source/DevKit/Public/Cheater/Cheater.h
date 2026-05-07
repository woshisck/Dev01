// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "Cheater.generated.h"

class APlayerCharacterBase;

UCLASS()
class DEVKIT_API UYogCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// ── 热度 ──────────────────────────────────────────────────────────────────
	/** 直接设置热度值（0 ~ MaxHeat） */
	UFUNCTION(Exec) void Yog_SetHeat(float Value);
	/** 强制设置热度阶段（0=无，1/2/3=Phase1-3） */
	UFUNCTION(Exec) void Yog_SetPhase(int32 Phase);
	/** 将热度设为满值（MaxHeat），触发升阶判断 */
	UFUNCTION(Exec) void Yog_MaxHeat();
	/** 冻结/解冻热度衰减（通过授予/移除 Buff.Status.Heat.Active 实现） */
	UFUNCTION(Exec) void Yog_FreezeHeat(bool bFreeze);

	// ── 背包 / 符文 ───────────────────────────────────────────────────────────
	/** 按 RuneID 给予玩家一个符文（加入 PendingRunes，需手动放入格子） */
	UFUNCTION(Exec) void Yog_GiveRune(int32 RuneID);
	/** 清空背包中所有已放置符文 */
	UFUNCTION(Exec) void Yog_ClearRunes();
	/** 直接设置金币数量 */
	UFUNCTION(Exec) void Yog_SetGold(int32 Amount);

	// ── 玩家属性 ──────────────────────────────────────────────────────────────
	/** 切换无敌模式（HP & MaxHP 设为 99999） */
	UFUNCTION(Exec) void Yog_GodMode();
	/** 直接设置当前 HP */
	UFUNCTION(Exec) void Yog_SetHP(float Value);
	/** 将 HP 恢复到 MaxHP */
	UFUNCTION(Exec) void Yog_FullHP();
	/** 直接设置 Attack 属性基础值 */
	UFUNCTION(Exec) void Yog_SetAttack(float Value);

	// ── 敌人 ──────────────────────────────────────────────────────────────────
	/** 立即杀死场景内所有敌人 */
	UFUNCTION(Exec) void Yog_KillAll();
	/** 冻结/解冻所有敌人（CustomTimeDilation = 0/1） */
	UFUNCTION(Exec) void Yog_FreezeEnemies(bool bFreeze);

	// ── Debug 打印 ────────────────────────────────────────────────────────────
	/** 打印当前热度值、MaxHeat、热度阶段 */
	UFUNCTION(Exec) void Yog_PrintHeat();
	/** 打印玩家 ASC 所有激活 Tag */
	UFUNCTION(Exec) void Yog_PrintTags();
	/** 打印玩家所有属性当前值 */
	UFUNCTION(Exec) void Yog_PrintAttributes();
	/** 打印背包内所有已放置符文信息 */
	UFUNCTION(Exec) void Yog_PrintRunes();
	/** 列出所有可用 GM 命令 */
	UFUNCTION(Exec) void Yog_Help();

private:
	APlayerCharacterBase* GetPlayerChar() const;

	bool bGodModeActive = false;
};
