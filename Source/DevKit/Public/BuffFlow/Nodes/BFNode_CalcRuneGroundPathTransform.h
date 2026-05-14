#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Actors/RuneGroundPathEffectActor.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_CalcRuneGroundPathTransform.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Calc Rune Ground Path Transform", Category = "BuffFlow|Area"))
class DEVKIT_API UBFNode_CalcRuneGroundPathTransform : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	// 效果来源 — 路径中心计算的参考角色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "效果来源"))
	EBFTargetSelector Source = EBFTargetSelector::BuffOwner;

	// 朝向模式 — 路径朝向的计算依据
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "朝向模式"))
	ERuneGroundPathFacingMode FacingMode = ERuneGroundPathFacingMode::SourceActorForward;

	// 路径长度（cm）— 用于计算中心点偏移，建议与生成节点填写相同的值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "路径长度（cm）",
		ToolTip = "Path length used only for center calculation. Link this to the same length as the spawn node when possible."))
	FFlowDataPinInputProperty_Float Length = FFlowDataPinInputProperty_Float(520.0f);

	// 生成位置偏移 — 相对于来源角色的局部偏移（X=前方，Y=右方，Z=上方）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "生成位置偏移",
		ToolTip = "Local offset from source actor. X is forward, Y is right, Z is up."))
	FVector SpawnOffset = FVector(45.0f, 0.0f, 6.0f);

	// 以路径长度为中心 — 勾选后输出位置为路径区域中心点；否则为路径起点/顶点
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "以路径长度为中心",
		ToolTip = "When true, output location is the center of the path area. When false, output location is the path start/apex."))
	bool bCenterOnPathLength = true;

	// 偏航角偏移 — 在计算朝向后额外旋转的角度（度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "偏航角偏移"))
	float RotationYawOffset = 0.0f;

	// 投影到地面 — 勾选后通过 LineTrace 将位置投影到地面表面
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Ground", meta = (DisplayName = "投影到地面"))
	bool bProjectToGround = true;

	// 地面追踪向上距离（cm）— 追踪起点向上的偏移，避免从地下开始追踪
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Ground", meta = (ClampMin = "0.0", DisplayName = "地面追踪向上距离（cm）"))
	float GroundTraceUp = 240.0f;

	// 地面追踪向下距离（cm）— 追踪终点向下的距离，适配地形起伏
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Ground", meta = (ClampMin = "0.0", DisplayName = "地面追踪向下距离（cm）"))
	float GroundTraceDown = 900.0f;

	// 生成位置（输出）— 计算完成的世界坐标，连接到生成节点的 SpawnLocationOverride
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Data", meta = (DisplayName = "生成位置（输出）"))
	FFlowDataPinOutputProperty_Vector SpawnLocation;

	// 生成朝向（输出）— 计算完成的世界旋转，连接到生成节点的 SpawnRotationOverride
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Data", meta = (DisplayName = "生成朝向（输出）"))
	FFlowDataPinOutputProperty_Rotator SpawnRotation;

	// 前向量（输出）— 路径朝向的单位前向量，供其他节点参考方向
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Data", meta = (DisplayName = "前向量（输出）"))
	FFlowDataPinOutputProperty_Vector ForwardVector;

	virtual void ExecuteBuffFlowInput(const FName& PinName) override;

private:
	bool ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const;
};
