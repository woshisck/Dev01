#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "GA_EnemyMeleeAttacks.generated.h"

/**
 * 敌人近战攻击 GA — 轻攻击系列
 * AbilityTags / AbilityData Key：Enemy.Melee.LAtk1~4
 * ActivationOwnedTags：空（敌人攻击不广播状态）
 * ActivationBlockedTags：Buff.Status.Dead
 *
 * 配表：给每个敌人类型的 AbilityData.AbilityMap 添加对应 Key 行，填写蒙太奇 + 命中框。
 * GASTemplate：给敌人 Grant 该角色实际使用的攻击 GA（如只用 LAtk1~2 就只给这两个）。
 */

UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_LAtk1 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_LAtk1(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_LAtk2 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_LAtk2(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_LAtk3 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_LAtk3(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_LAtk4 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_LAtk4(); };

/**
 * 敌人近战攻击 GA — 重攻击系列
 * AbilityTags / AbilityData Key：Enemy.Melee.HAtk1~4
 */

UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_HAtk1 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_HAtk1(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_HAtk2 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_HAtk2(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_HAtk3 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_HAtk3(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_HAtk4 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_HAtk4(); };
