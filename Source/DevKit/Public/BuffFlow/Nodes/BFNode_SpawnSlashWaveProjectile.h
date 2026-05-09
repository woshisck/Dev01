#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayTagContainer.h"
#include "BFNode_SpawnSlashWaveProjectile.generated.h"

class ASlashWaveProjectile;
class ACharacter;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * 生成一个可配置的斩波投射物。
 * 表现层分离原则：飞行/命中/消亡的 Niagara 特效可直接在本节点配置（因斩波是独立投射物 Actor，
 * 与远程弹幕不同，无法通过"等待命中→表现节点"模式处理），但命中回调事件仍走 EventTag 通知流程图。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Slash Wave Projectile", Category = "BuffFlow|Projectile"))
class DEVKIT_API UBFNode_SpawnSlashWaveProjectile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// ---- 基础 ----

	// 投射物类 — 斩波 Actor 类（BP 子类或默认 C++ 类）
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "投射物类"))
	TSubclassOf<ASlashWaveProjectile> ProjectileClass;

	// 伤害效果类 — 命中时施加的 GameplayEffect
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "伤害效果类"))
	TSubclassOf<UGameplayEffect> DamageEffect;

	// 发射来源 — 斩波从哪个角色位置生成
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "发射来源"))
	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;

	// 伤害值 — 斩波命中时的基础伤害
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "伤害值"))
	float Damage = 10.f;

	// 伤害日志类型 — 用于调试日志标识此次伤害来源
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "伤害日志类型"))
	FName DamageLogType = TEXT("Rune_SlashWave");

	// 飞行速度（cm/s）— 斩波的移动速度
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1.0", DisplayName = "飞行速度（cm/s）"))
	float Speed = 1400.f;

	// 最大飞行距离（cm）— 超过此距离后斩波自动消亡
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0", DisplayName = "最大飞行距离（cm）"))
	float MaxDistance = 800.f;

	// 最大命中目标数 — <=0 表示无限制；每个目标可被命中 DamageApplicationsPerTarget 次
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "最大命中目标数"))
	int32 MaxHitCount = 2;

	// 每目标命中次数 — 同一目标首次接触后允许被施加伤害的总次数
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1", ClampMax = "20", DisplayName = "每目标命中次数"))
	int32 DamageApplicationsPerTarget = 1;

	// 命中间隔（秒）— 对同一目标重复施加伤害的最小时间间隔；<=0 表示立即重复
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0", DisplayName = "命中间隔（秒）"))
	float DamageApplicationInterval = 0.25f;

	// 碰撞盒半尺寸 — 斩波碰撞检测的半尺寸（XYZ 各半）
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "碰撞盒半尺寸"))
	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);

	// ---- 外观 ----

	// 随碰撞盒缩放外观 — 勾选后碰撞盒变大时，投射物的视觉比例同步缩放
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "随碰撞盒缩放外观"))
	bool bScaleVisualWithCollisionExtent = true;

	// 外观额外缩放 — 在碰撞缩放基础上叠加的额外视觉缩放（不影响碰撞区域）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "外观额外缩放"))
	FVector VisualScaleMultiplier = FVector(1.f, 1.f, 1.f);

	// 飞行粒子系统 — 斩波飞行中的主体视觉 Niagara 效果
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "飞行粒子系统"))
	TObjectPtr<UNiagaraSystem> ProjectileVisualNiagaraSystem = nullptr;

	// 飞行粒子缩放
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "飞行粒子缩放"))
	FVector ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);

	// 隐藏默认外观 — 勾选后隐藏投射物蓝图上的默认网格/组件，只显示飞行粒子系统
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "隐藏默认外观"))
	bool bHideDefaultProjectileVisuals = false;

	// 命中粒子系统 — 斩波命中目标时播放的 Niagara 效果
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "命中粒子系统"))
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem = nullptr;

	// 命中粒子缩放
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "命中粒子缩放"))
	FVector HitNiagaraScale = FVector(1.f, 1.f, 1.f);

	// 消亡粒子系统 — 斩波飞行结束/超出距离时播放的 Niagara 效果
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "消亡粒子系统"))
	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem = nullptr;

	// 消亡粒子缩放
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "消亡粒子缩放"))
	FVector ExpireNiagaraScale = FVector(1.f, 1.f, 1.f);

	// ---- 事件 ----

	// 命中事件Tag — 斩波命中时向源 ASC 发送的 Gameplay 事件 Tag
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events", meta = (DisplayName = "命中事件Tag"))
	FGameplayTag HitGameplayEventTag;

	// 消亡事件Tag — 斩波消亡时向源 ASC 发送的 Gameplay 事件 Tag
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events", meta = (DisplayName = "消亡事件Tag"))
	FGameplayTag ExpireGameplayEventTag;

	// ---- 弹幕模式 ----

	// 发射数量 — 单次发射的基础斩波枚数
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "1", DisplayName = "发射数量"))
	int32 ProjectileCount = 1;

	// 连击追加发射数 — 勾选后：连击层数 × 每层追加数 = 额外多发枚数
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (DisplayName = "连击追加发射数"))
	bool bAddComboStacksToProjectileCount = false;

	// 每层连击追加数 — 每增加一层连击额外多发几枚
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", DisplayName = "每层连击追加数",
		EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
	int32 ProjectilesPerComboStack = 1;

	// 最大追加上限 — 连击追加的斩波总数上限（0=无上限）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", DisplayName = "最大追加上限",
		EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
	int32 MaxBonusProjectiles = 0;

	// 散布锥角（度）— 多枚斩波的水平展开角；0=全部正前方
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "散布锥角（度）"))
	float ProjectileConeAngleDegrees = 0.f;

	// 顺序发射 — 勾选后多枚斩波沿同一路径依次生成而非扇形展开
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (DisplayName = "顺序发射"))
	bool bSpawnProjectilesSequentially = false;

	// 顺序发射间隔（秒）— 顺序模式下每枚斩波的生成时间间隔
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", DisplayName = "顺序发射间隔（秒）",
		EditCondition = "bSpawnProjectilesSequentially", EditConditionHides))
	float SequentialProjectileSpawnInterval = 0.12f;

	// ---- 碰撞 ----

	// 碰到静态物体销毁 — 勾选后斩波碰到 WorldStatic 时立即销毁（否则穿透）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Collision", meta = (DisplayName = "碰到静态物体销毁"))
	bool bDestroyOnWorldStaticHit = false;

	// ---- 伤害 ----

	// 强制纯伤害 — 勾选后跳过护甲计算，直接对生命值扣血
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "强制纯伤害"))
	bool bForcePureDamage = false;

	// 护甲伤害加成倍率 — 目标护甲值 × 此倍率 叠加到伤害（0=不启用）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0", DisplayName = "护甲伤害加成倍率"))
	float BonusArmorDamageMultiplier = 0.f;

	// 叠加来源护甲到伤害 — 勾选后将发射者的护甲值转化为额外伤害
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "叠加来源护甲到伤害"))
	bool bAddSourceArmorToDamage = false;

	// 护甲转伤害倍率 — 来源护甲 × 此倍率 = 额外伤害
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0", DisplayName = "护甲转伤害倍率"))
	float SourceArmorToDamageMultiplier = 1.f;

	// 生成时消耗来源护甲 — 勾选后发射时消耗发射者一部分护甲值
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "生成时消耗来源护甲"))
	bool bConsumeSourceArmorOnSpawn = false;

	// 护甲消耗倍率 — 消耗的护甲量 = 伤害 × 此倍率
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0", DisplayName = "护甲消耗倍率"))
	float SourceArmorConsumeMultiplier = 1.f;

	// 附加命中效果 — 命中时额外施加的 GameplayEffect（如附加灼烧）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "附加命中效果"))
	TSubclassOf<UGameplayEffect> AdditionalHitEffect;

	// 附加效果Tag — 附加命中效果使用的 SetByCaller Tag
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "附加效果Tag"))
	FGameplayTag AdditionalHitSetByCallerTag;

	// 附加效果数值 — 传入附加命中效果的 SetByCaller 数值
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "附加效果数值"))
	float AdditionalHitSetByCallerValue = 0.f;

	// ---- 分裂 ----

	// 首次命中分裂 — 勾选后斩波首次命中目标时分裂为多枚子弹
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (DisplayName = "首次命中分裂"))
	bool bSplitOnFirstHit = false;

	// 最大分裂代数 — 子弹可继续分裂的最大层数（1=只有主弹分裂，子弹不再分裂）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0", DisplayName = "最大分裂代数"))
	int32 MaxSplitGenerations = 1;

	// 分裂数量 — 每次分裂生成的子弹枚数
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "1", DisplayName = "分裂数量"))
	int32 SplitProjectileCount = 3;

	// 分裂锥角（度）— 分裂子弹的水平展开角
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "分裂锥角（度）"))
	float SplitConeAngleDegrees = 45.f;

	// 随机分裂方向 — 勾选后分裂子弹方向在锥角内随机散布
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (DisplayName = "随机分裂方向",
		EditCondition = "bSplitOnFirstHit", EditConditionHides))
	bool bRandomizeSplitDirections = false;

	// 偏航随机范围（度）— 随机模式下水平方向的最大抖动角度
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "偏航随机范围（度）",
		EditCondition = "bSplitOnFirstHit && bRandomizeSplitDirections", EditConditionHides))
	float SplitRandomYawJitterDegrees = 0.f;

	// 俯仰随机范围（度）— 随机模式下垂直方向的最大抖动角度
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "45.0", DisplayName = "俯仰随机范围（度）",
		EditCondition = "bSplitOnFirstHit && bRandomizeSplitDirections", EditConditionHides))
	float SplitRandomPitchDegrees = 0.f;

	// 分裂伤害倍率 — 子弹伤害 = 主弹伤害 × 此倍率
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", DisplayName = "分裂伤害倍率"))
	float SplitDamageMultiplier = 0.5f;

	// 分裂速度倍率 — 子弹速度 = 主弹速度 × 此倍率
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.01", DisplayName = "分裂速度倍率"))
	float SplitSpeedMultiplier = 2.f;

	// 分裂最大距离倍率 — 子弹最大飞行距离 = 主弹距离 × 此倍率
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", DisplayName = "分裂最大距离倍率"))
	float SplitMaxDistanceMultiplier = 0.6f;

	// 分裂碰撞盒倍率 — 子弹碰撞盒 = 主弹碰撞盒 × 此向量
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (DisplayName = "分裂碰撞盒倍率"))
	FVector SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);

	// 分裂子弹命中弹跳 — 仅对分裂产生的子弹启用敌人命中后弹跳（主弹不受影响）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split|Bounce", meta = (DisplayName = "分裂子弹命中弹跳",
		EditCondition = "bSplitOnFirstHit", EditConditionHides))
	bool bBounceSplitChildrenOnEnemyHit = false;

	// 子弹最大弹跳次数 — 分裂子弹可弹跳的最大次数（0=不限）
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split|Bounce", meta = (ClampMin = "0", DisplayName = "子弹最大弹跳次数",
		EditCondition = "bSplitOnFirstHit && bBounceSplitChildrenOnEnemyHit", EditConditionHides))
	int32 SplitChildMaxEnemyBounces = 0;

	// ---- 生成位置 ----

	// 生成位置偏移 — 相对于来源角色的局部偏移（X=前方，Z=上方）
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "生成位置偏移"))
	FVector SpawnOffset = FVector(80.f, 0.f, 45.f);

	// ---- 发射特效 ----

	// 发射粒子系统 — 斩波生成瞬间在来源位置播放的 Niagara 效果
	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子系统"))
	TObjectPtr<UNiagaraSystem> LaunchNiagaraSystem = nullptr;

	// 发射粒子效果名 — 供 DestroyNiagara 节点按名称精确销毁
	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子效果名"))
	FName LaunchNiagaraEffectName = NAME_None;

	// 发射粒子附着到来源 — 勾选后发射粒子跟随来源角色移动；否则固定在世界坐标
	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子附着到来源"))
	bool bAttachLaunchNiagaraToSource = false;

	// 发射粒子位置偏移 — 相对于生成点的额外偏移
	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子位置偏移"))
	FVector LaunchNiagaraOffset = FVector::ZeroVector;

	// 发射粒子旋转偏移 — 发射粒子的额外旋转
	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子旋转偏移"))
	FRotator LaunchNiagaraRotationOffset = FRotator::ZeroRotator;

	// 发射粒子缩放
	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子缩放"))
	FVector LaunchNiagaraScale = FVector(1.f, 1.f, 1.f);

	// FA结束时销毁发射粒子 — 勾选后 FA 停止时立即销毁发射粒子实例
	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "FA结束时销毁发射粒子"))
	bool bDestroyLaunchNiagaraWithFlow = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	float ResolveDamage(ACharacter* SourceCharacter) const;
	void ConsumeSourceArmor(ACharacter* SourceCharacter) const;
	void SpawnLaunchNiagara(ACharacter* SourceCharacter, const FVector& SpawnLocation, const FRotator& SpawnRotation) const;
};
