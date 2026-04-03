#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayEffect/PowerUpEffect.h"
#include "RuneStatEffect.generated.h"

/**
 * 符文数值提升 GE 基类
 * Duration: Infinite，装备符文时 Apply，卸下时 Remove
 * 所有具体数值 GE 继承此类
 */
UCLASS()
class DEVKIT_API URuneStatEffect_Base : public UPowerUpEffect
{
	GENERATED_BODY()
public:
	URuneStatEffect_Base();
};

// ─── Attack（攻击）───────────────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_Attack : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_Attack();
};

// ─── AttackPower（攻击力 / 伤害倍率）────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_AttackPower : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_AttackPower();
};

// ─── MaxHealth（最大生命）────────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_MaxHealth : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_MaxHealth();
};

// ─── AttackSpeed（攻击速度）──────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_AttackSpeed : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_AttackSpeed();
};

// ─── MoveSpeed（移动速度）────────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_MoveSpeed : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_MoveSpeed();
};

// ─── Crit_Rate（暴击率）──────────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_CritRate : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_CritRate();
};

// ─── Crit_Damage（暴击伤害）──────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_CritDamage : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_CritDamage();
};

// ─── DmgTaken（受伤加成）─────────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_DmgTaken : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_DmgTaken();
};

// ─── Dodge（闪避）────────────────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_Dodge : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_Dodge();
};

// ─── AttackRange（攻击范围）──────────────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API URuneStatEffect_AttackRange : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	URuneStatEffect_AttackRange();
};

// ═══════════════════════════════════════════════════════════════
//  4.15 符文 GE 类
// ═══════════════════════════════════════════════════════════════

// ─── 振奋（好战每次命中叠加，+1 Attack，3s，最多5层）──────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Rune_ZhenFen : public UPowerUpEffect
{
	GENERATED_BODY()
public:
	UGE_Rune_ZhenFen();
};

// ─── 奋力一击 BehaviorGE（授予 Rune.FenLiYiJi.Active）─────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Rune_FenLiYiJi : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	UGE_Rune_FenLiYiJi();
};

// ─── 突袭 BehaviorGE（授予 Rune.TuXi.Active）────────────────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Rune_TuXi : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	UGE_Rune_TuXi();
};

// ─── 双重打击 BehaviorGE（授予 Rune.ShuangChongDaJi.Active）─────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Rune_ShuangChongDaJi : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	UGE_Rune_ShuangChongDaJi();
};

// ─── 风行者 BehaviorGE（授予 Rune.FengXingZhe.Active）──────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Rune_FengXingZhe : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	UGE_Rune_FengXingZhe();
};

// ─── 滑行速度增益（3s，MoveSpeed +120 ≈ 20% of 600）────────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Buff_SlideSpeed : public UPowerUpEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_SlideSpeed();
};

// ─── 蛇咬中毒（5s 每秒 -MaxHP×2%）──────────────────────────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Buff_Poison_Rune : public UPowerUpEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_Poison_Rune();
};

// ─── 战斗渴望 BehaviorGE（授予 Rune.ZhanDouKewang.Active）───────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Rune_ZhanDouKewang : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	UGE_Rune_ZhanDouKewang();
};

// ─── 战斗渴望动态攻速 GE（Infinite，AttackSpeed SetByCaller）────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Buff_ZhanDouKewang : public UPowerUpEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_ZhanDouKewang();
};

// ─── 全能 BehaviorGE（授予 Rune.QuanNeng.Active）────────────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Rune_QuanNeng : public URuneStatEffect_Base
{
	GENERATED_BODY()
public:
	UGE_Rune_QuanNeng();
};

// ─── 全能动态暴击率 GE（5s，Crit_Rate SetByCaller）──────────────
UCLASS(BlueprintType)
class DEVKIT_API UGE_Buff_QuanNeng : public UPowerUpEffect
{
	GENERATED_BODY()
public:
	UGE_Buff_QuanNeng();
};
