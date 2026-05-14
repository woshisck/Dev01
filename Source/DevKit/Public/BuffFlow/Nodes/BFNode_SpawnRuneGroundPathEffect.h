#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Actors/RuneGroundPathEffectActor.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_SpawnRuneGroundPathEffect.generated.h"

class UMaterialInterface;
class UNiagaraSystem;

UCLASS(DisplayName = "Spawn Rune Ground Path Effect")
class DEVKIT_API UBFNode_SpawnRuneGroundPathEffect : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	// 施加的效果类 — 路径区域内命中目标时施加的 GameplayEffect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "施加的效果类"))
	TSubclassOf<UGameplayEffect> Effect;

	// 命中目标策略 — 只命中敌人/只命中友方/全部
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "命中目标策略"))
	ERuneGroundPathTargetPolicy TargetPolicy = ERuneGroundPathTargetPolicy::EnemiesOnly;

	// 区域形状 — 矩形/扇形等碰撞区域形状
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "区域形状"))
	ERuneGroundPathShape Shape = ERuneGroundPathShape::Rectangle;

	// 朝向模式 — 无覆盖引脚时的默认朝向计算方式（推荐用 CalcRuneGroundPathTransform 节点替代）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "朝向模式",
		ToolTip = "Fallback facing mode when Spawn Location Override / Spawn Rotation Override are not linked. Prefer using Calc Rune Ground Path Transform for authored flows."))
	ERuneGroundPathFacingMode FacingMode = ERuneGroundPathFacingMode::SourceActorForward;

	// 生成位置覆盖 — 可选数据引脚；连线后直接使用此世界坐标，不再自动计算
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "生成位置覆盖",
		ToolTip = "Optional data pin. If linked, this exact world location is used instead of the internal position calculation."))
	FFlowDataPinInputProperty_Vector SpawnLocationOverride;

	// 生成朝向覆盖 — 可选数据引脚；连线后直接使用此世界旋转，不再自动计算
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "生成朝向覆盖",
		ToolTip = "Optional data pin. If linked, this exact world rotation is used instead of the internal facing calculation."))
	FFlowDataPinInputProperty_Rotator SpawnRotationOverride;

	// 以路径长度为中心 — 勾选后生成位置为路径区域的中心点
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "以路径长度为中心"))
	bool bCenterOnPathLength = true;

	// 偏航角偏移 — 在计算朝向后额外旋转的角度（度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "偏航角偏移"))
	float RotationYawOffset = 0.0f;

	// 持续时间（秒）— 路径效果的存活时长；若数据引脚已连线则被覆盖
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01", DisplayName = "持续时间（秒）"))
	float Duration = 3.0f;

	// 持续时间（数据引脚）— 连线后覆盖上方「持续时间」；可从 GetAuraModule 节点接入
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "持续时间（数据引脚）"))
	FFlowDataPinInputProperty_Float DurationPin;

	// 触发间隔（秒）— 路径区域每隔多少秒检测并施加效果一次；若数据引脚已连线则被覆盖
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01", DisplayName = "触发间隔（秒）"))
	float TickInterval = 1.0f;

	// 触发间隔（数据引脚）— 连线后覆盖上方「触发间隔」；可从 GetAuraModule 节点接入
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "触发间隔（数据引脚）"))
	FFlowDataPinInputProperty_Float TickIntervalPin;

	// 路径长度（cm）— 区域沿前方的延伸长度；若数据引脚已连线则被覆盖
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0", DisplayName = "路径长度（cm）"))
	float Length = 520.0f;

	// 路径长度（数据引脚）— 连线后覆盖上方「路径长度」；可从 GetAuraModule 节点接入
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "路径长度（数据引脚）"))
	FFlowDataPinInputProperty_Float LengthPin;

	// 路径宽度（cm）— 区域的横向宽度；若数据引脚已连线则被覆盖
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0", DisplayName = "路径宽度（cm）"))
	float Width = 220.0f;

	// 路径宽度（数据引脚）— 连线后覆盖上方「路径宽度」；可从 GetAuraModule 节点接入
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "路径宽度（数据引脚）"))
	FFlowDataPinInputProperty_Float WidthPin;

	// 路径高度（cm）— 区域的垂直高度；若数据引脚已连线则被覆盖
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0", DisplayName = "路径高度（cm）"))
	float Height = 120.0f;

	// 路径高度（数据引脚）— 连线后覆盖上方「路径高度」；可从 GetAuraModule 节点接入
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "路径高度（数据引脚）"))
	FFlowDataPinInputProperty_Float HeightPin;

	// 贴花投影深度（cm）— 地面贴花的投影厚度，保持较浅可防止贴花投影到角色身上
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1.0", DisplayName = "贴花投影深度（cm）",
		ToolTip = "Decal projection depth in cm. Keep this shallow so the path decal stays on the floor instead of projecting up onto characters."))
	float DecalProjectionDepth = 18.0f;

	// 贴花平面旋转（度）— 只旋转贴花纹理方向，不影响碰撞区域；试试 0/90/180/270
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "贴花平面旋转（度）",
		ToolTip = "Rotates only the decal texture/mask on the floor. This does not rotate the damage/collision area. Try 0/90/180/270 if the decal visual direction does not match the yellow debug area."))
	float DecalPlaneRotationDegrees = 0.0f;

	// 生成位置偏移 — 相对于来源角色的局部偏移（X=前方，Y=右方，Z=上方）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "生成位置偏移"))
	FVector SpawnOffset = FVector(45.0f, 0.0f, 6.0f);

	// 效果来源 — 路径中心计算的参考角色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "效果来源"))
	EBFTargetSelector Source = EBFTargetSelector::BuffOwner;

	// 贴花材质 — 显示在地面上的区域指示贴花
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "贴花材质"))
	TObjectPtr<UMaterialInterface> DecalMaterial;

	// Niagara 粒子系统 — 路径区域的持续粒子表现
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "Niagara 粒子系统"))
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	// Niagara 缩放 — 粒子系统的缩放比例
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "Niagara 缩放"))
	FVector NiagaraScale = FVector(0.5f, 0.5f, 0.35f);

	// Niagara 实例数量 — 沿路径分布的粒子实例数（火焰路径通常需要多个小实例）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1", ClampMax = "12", DisplayName = "Niagara 实例数量",
		ToolTip = "Number of Niagara instances distributed along the path. Fire paths use multiple small instances to read as a ground strip."))
	int32 NiagaraInstanceCount = 1;

	// 伤害Tag1（SetByCaller）— GE执行时使用的Tag（如 Data.Damage.Burn）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "伤害Tag1（SetByCaller）",
		ToolTip = "GameplayTag used by the GameplayEffect execution. Burn paths should use Data.Damage.Burn."))
	FGameplayTag SetByCallerTag1;

	// 每Tick伤害数值1 — 传入 SetByCallerTag1 对应槽的实际伤害值（每次触发）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "每Tick伤害数值1",
		ToolTip = "Designer-facing damage value passed to SetByCallerTag1. For UGE_RuneBurn this is the burn damage per periodic tick."))
	FFlowDataPinInputProperty_Float SetByCallerValue1 = FFlowDataPinInputProperty_Float(0.0f);

	// 伤害Tag2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "伤害Tag2"))
	FGameplayTag SetByCallerTag2;

	// 伤害数值2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "伤害数值2"))
	FFlowDataPinInputProperty_Float SetByCallerValue2 = FFlowDataPinInputProperty_Float(0.0f);

	// 施加次数 — 每次触发时对目标施加GE的次数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (ClampMin = "1", DisplayName = "施加次数"))
	int32 ApplicationCount = 1;

	// 每目标只施加一次 — 勾选后同一目标在路径存活期间只会被施加一次GE
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "每目标只施加一次"))
	bool bApplyOncePerTarget = false;

	virtual void ExecuteBuffFlowInput(const FName& PinName) override;

private:
	bool ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const;
};
