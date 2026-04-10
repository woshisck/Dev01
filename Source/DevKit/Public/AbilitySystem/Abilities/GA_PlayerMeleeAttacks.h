#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "GA_PlayerMeleeAttacks.generated.h"

/**
 * 玩家近战攻击中间基类
 * 构造函数自动绑定：
 *   StatBeforeATKEffect = GE_StatBeforeATK（攻击前临时 Buff，动作结束后自动移除）
 *   StatAfterATKEffect  = GE_StatAfterATK（动作正常结束后施加，GE 自身 Duration 到期）
 * 所有玩家攻击 GA 继承此类，无需手动填写这两个字段。
 */
UCLASS(Abstract)
class DEVKIT_API UGA_PlayerMeleeAttack : public UGA_MeleeAttack
{
	GENERATED_BODY()
public:
	UGA_PlayerMeleeAttack();
};

// ── Light Attack Combo ────────────────────────────────────────────────────

UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_LightAtk1 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_LightAtk1(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_LightAtk2 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_LightAtk2(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_LightAtk3 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_LightAtk3(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_LightAtk4 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_LightAtk4(); };

// ── Heavy Attack Combo ────────────────────────────────────────────────────

UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_HeavyAtk1 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_HeavyAtk1(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_HeavyAtk2 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_HeavyAtk2(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_HeavyAtk3 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_HeavyAtk3(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_HeavyAtk4 : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_HeavyAtk4(); };

// ── Dash Attack ───────────────────────────────────────────────────────────

UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Player_DashAtk  : public UGA_PlayerMeleeAttack { GENERATED_BODY() public: UGA_Player_DashAtk();  };
