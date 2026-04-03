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
