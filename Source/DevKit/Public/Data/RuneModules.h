#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RuneModules.generated.h"

class AMusketBullet;
class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;

// ============================================================
//  模块总开关
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneModuleFlags
{
	GENERATED_BODY()

	/** 飞行物模块 — 配置投射物类型、速度、数量、散布等物理行为 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modules")
	bool bProjectile = false;

	/** 光环/场地模块 — 配置区域形状、持续时间、贴花与粒子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modules")
	bool bAura = false;

	/** 状态效果模块 — 配置施加到目标的 GE、叠层与时限 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modules")
	bool bStatus = false;
};

// ============================================================
//  飞行物模块（物理表现层）
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneProjectileModule
{
	GENERATED_BODY()

	/** 投射物类 — 决定外观与碰撞（留空则由 FA 节点的兜底类决定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	TSubclassOf<AMusketBullet> ProjectileClass;

	/** 飞行速度（cm/s）— 在投射物 BP 的 ProjectileMovement.InitialSpeed 中设置；此处仅作配置参考 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "100.0"))
	float Speed = 1400.f;

	/** 基础发射数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** 散布锥角（度）— 0 = 全部正前方，90 = 90° 扇形 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile",
		meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ConeAngleDegrees = 0.f;

	/** 穿透飞行 — 开启后投射物使用 Sweep 碰撞（适合月牙、斩波等需要扫描体的投射物） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool bSweepCollision = false;

	/** 命中事件 Tag — 投射物命中时向源 ASC 发送的 Gameplay 事件，供 FA 内等待事件节点接收 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FGameplayTag HitEventTag;
};

// ============================================================
//  光环/场地模块（空间数据层）
// ============================================================

UENUM(BlueprintType)
enum class ERuneGroundAreaShape : uint8
{
	Rectangle UMETA(DisplayName = "矩形"),
	Sector    UMETA(DisplayName = "扇形"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneAuraModule
{
	GENERATED_BODY()

	/** 区域形状 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
	ERuneGroundAreaShape Shape = ERuneGroundAreaShape::Rectangle;

	/** 区域长度（cm）— 沿前方的延伸距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "1.0"))
	float Length = 520.f;

	/** 区域宽度（cm）*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "1.0"))
	float Width = 220.f;

	/** 区域高度（cm）*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "1.0"))
	float Height = 120.f;

	/** 效果持续时间（秒）*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "0.01"))
	float Duration = 3.f;

	/** 触发间隔（秒）— 区域每隔多少秒检测并施加效果 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "0.01"))
	float TickInterval = 1.f;

	/** 贴花材质 — 显示在地面的区域指示贴花 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Visual")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	/** Niagara 粒子系统 — 区域持续特效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Visual")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;
};

// ============================================================
//  状态效果模块
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneStatusModule
{
	GENERATED_BODY()

	/** 状态 GE 类 — 命中目标时施加的 GameplayEffect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	TSubclassOf<UGameplayEffect> StatusGE;

	/** 持续时间（秒）— 状态效果的存活时长；0 = 永久 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status", meta = (ClampMin = "0.0"))
	float Duration = 4.f;

	/** 叠层上限 — 同目标可叠加的最大层数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status", meta = (ClampMin = "1"))
	int32 StackLimit = 3;

	/** 状态标签 — 用于 UI 显示和条件判断（如 Buff.Status.Poison） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FGameplayTag StatusTag;
};
