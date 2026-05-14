#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_SpawnRangedProjectiles.generated.h"

class AMusketBullet;
class UGameplayEffect;

/**
 * 向目标方向发射一组远程弹丸。
 * 弹丸继承当前战斗卡攻击实例 GUID，不额外消耗卡牌资源。
 *
 * 表现层分离原则：
 *   - 弹丸飞行特效（轨迹/粒子）由弹丸 BP 类自身持有，不在本节点配置。
 *   - 命中特效与命中回调请在流程图中通过「等待事件 → 特效表现」节点链实现。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Ranged Projectiles", Category = "BuffFlow|Projectile"))
class DEVKIT_API UBFNode_SpawnRangedProjectiles : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// ---- 投射物基础 ----

	// 发射来源 — 投射物从哪个角色的枪口位置发射（通常选 BuffOwner 即玩家自身）
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "发射来源"))
	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;

	// 弹丸类（兜底）— 未启用「优先卡牌投射物类」时，或卡牌 DA 未配置投射物时使用此类
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "弹丸类（兜底）"))
	TSubclassOf<AMusketBullet> BulletClass;

	// 伤害效果类（兜底）— 未启用「优先卡牌伤害效果类」时，或卡牌 DA 未配置 GE 时使用此类
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "伤害效果类（兜底）"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 优先卡牌投射物类 — 勾选后使用战斗卡 DA 上配置的弹丸类（推荐保持勾选）
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "优先卡牌投射物类"))
	bool bPreferCombatCardProjectileClass = true;

	// 优先卡牌伤害效果类 — 勾选后使用战斗卡 DA 上配置的命中伤害 GE（推荐保持勾选）
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "优先卡牌伤害效果类"))
	bool bPreferCombatCardDamageEffectClass = true;

	// 枪口插槽名 — 发射位置优先取角色骨骼/武器模型上同名插槽的世界坐标
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "枪口插槽名"))
	FName MuzzleSocketName = TEXT("Muzzle");

	// 手动偏航角列表 — 每枚投射物的水平偏转角（度）；仅在不启用「数量模式」时生效
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "手动偏航角列表"))
	TArray<float> YawOffsets;

	// ---- 弹幕模式 ----

	// 启用数量模式 — 勾选后改用「发射数量 + 散布锥角」均匀分布，取代手动偏航角列表
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (DisplayName = "启用数量模式"))
	bool bUseProjectileCountPattern = false;

	// 发射数量 — 启用数量模式时，单次发射的基础弹丸枚数（最小 1）；若下方数据引脚已连线则被覆盖
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (ClampMin = "1", DisplayName = "发射数量"))
	int32 ProjectileCount = 1;

	// 发射数量（数据引脚）— 连线后覆盖上方「发射数量」；可从 GetProjectileModule 节点接入
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (DisplayName = "发射数量（数据引脚）"))
	FFlowDataPinInputProperty_Int32 ProjectileCountPin;

	// 散布锥角（度）— 多枚弹丸的水平展开角；0 = 全部正前方，90 = 90 度扇形；若下方数据引脚已连线则被覆盖
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "散布锥角（度）"))
	float ProjectileConeAngleDegrees = 0.f;

	// 散布锥角（数据引脚）— 连线后覆盖上方「散布锥角」；可从 GetProjectileModule 节点接入
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (DisplayName = "散布锥角（数据引脚）"))
	FFlowDataPinInputProperty_Float ProjectileConeAnglePinDegrees;

	// ---- 伤害 ----

	// 使用卡牌攻击力 — 勾选后伤害值来自战斗卡攻击力属性；否则使用下方「固定伤害值」
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "使用卡牌攻击力"))
	bool bUseCombatCardAttackDamage = true;

	// 固定伤害值 — 不使用卡牌攻击力时生效；可连接 Pure 数据节点（读取数值）的输出引脚
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "!bUseCombatCardAttackDamage", DisplayName = "固定伤害值"))
	FFlowDataPinInputProperty_Float Damage;

	// ---- 连击追加 ----

	// 连击追加发射数 — 勾选后：当前连击层数 × 每层追加数 = 额外多发弹丸枚数
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|连击追加", meta = (DisplayName = "连击追加发射数"))
	bool bAddComboStacksToProjectileCount = false;

	// 每层连击追加数 — 每增加一层连击，额外多发几枚弹丸
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|连击追加", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides, DisplayName = "每层连击追加数"))
	int32 ProjectilesPerComboStack = 1;

	// 最大追加上限 — 连击追加的弹丸总数上限（0 = 无上限）
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|连击追加", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides, DisplayName = "最大追加上限"))
	int32 MaxBonusProjectiles = 0;

	// ---- 攻击实例 ----

	// 共享攻击 GUID — 勾选后与主攻击共享同一 GUID，额外弹丸不额外消耗卡牌次数
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "共享攻击 GUID"))
	bool bShareAttackInstanceGuid = true;

	// ---- 武器检查 ----

	// 需要远程武器 Tag — 勾选后角色必须拥有指定 Tag 才可发射，否则触发 Failed 引脚
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "需要远程武器 Tag"))
	bool bRequireRangedWeaponTag = true;

	// 所需武器 Tag — 勾选「需要远程武器 Tag」后，具体检查的 Tag（默认 Weapon.Type.Ranged）
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "bRequireRangedWeaponTag", DisplayName = "所需武器 Tag"))
	FGameplayTag RequiredWeaponTag;

	// ---- 命中事件（用于流程图回调） ----

	// 命中事件 Tag — 弹丸命中时向 ASC 发送的 Gameplay 事件 Tag，供同 FA 内「等待事件」节点接收
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (DisplayName = "命中事件 Tag"))
	FGameplayTag HitGameplayEventTag;

	// 事件发给攻击来源 — 勾选后事件发给发射者（玩家）ASC，等待事件节点设置 Target = BuffOwner 即可接收
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (EditCondition = "HitGameplayEventTag.IsValid()", EditConditionHides, DisplayName = "事件发给攻击来源"))
	bool bSendHitGameplayEventToSourceASC = true;

	// 用伤害量作为事件强度 — Magnitude = 命中实际伤害值，供下游节点通过事件读取伤害数值
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (EditCondition = "HitGameplayEventTag.IsValid()", EditConditionHides, DisplayName = "用伤害量作为事件强度"))
	bool bUseDamageAsHitGameplayEventMagnitude = true;

	// 命中事件强度 — 不用伤害量时，手动指定的 Magnitude 值；可连接 Pure 数据节点输出
	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (EditCondition = "HitGameplayEventTag.IsValid() && !bUseDamageAsHitGameplayEventMagnitude", EditConditionHides, DisplayName = "命中事件强度"))
	FFlowDataPinInputProperty_Float HitGameplayEventMagnitude;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;

private:
	TArray<float> BuildResolvedYawOffsets(int32 ComboBonusStacks, int32 OverrideCount, float OverrideConeAngle) const;
	FVector ResolveMuzzleLocation(ACharacter* SourceCharacter) const;
};
