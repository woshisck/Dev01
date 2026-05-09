#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_PlayFlipbookVFX.generated.h"

class ARune512FlipbookVFXActor;
class UMaterialInterface;
class UStaticMesh;
class UTexture2D;

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Flipbook VFX", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_PlayFlipbookVFX : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 纹理贴图 — Flipbook 序列帧纹理（与 Material 二选一）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "纹理贴图"))
	TObjectPtr<UTexture2D> Texture = nullptr;

	// 材质 — 自定义材质（优先级高于 Texture，可使用 Material 内置 Flipbook 参数）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "材质"))
	TObjectPtr<UMaterialInterface> Material = nullptr;

	// 平面网格 — 用于显示 Flipbook 的 StaticMesh（默认为引擎内置平面）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "平面网格"))
	TObjectPtr<UStaticMesh> PlaneMesh = nullptr;

	// 行数 — Flipbook 纹理的行数（与 Columns 共同决定帧数）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "1", DisplayName = "行数"))
	int32 Rows = 4;

	// 列数 — Flipbook 纹理的列数
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "1", DisplayName = "列数"))
	int32 Columns = 4;

	// 播放时长（秒）— 完整播放一遍所需的时间
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0.01", DisplayName = "播放时长（秒）"))
	float Duration = 0.45f;

	// Actor存活时长（秒）— 0 使用 Duration；配合 bLoop 用于状态效果时可填更长时间
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0.0", DisplayName = "Actor存活时长（秒）"))
	float Lifetime = 0.f;

	// 循环播放 — 勾选后持续循环，直到 Actor 被销毁或 FA 停止（需配合 Lifetime 或 bDestroyWithFlow）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "循环播放"))
	bool bLoop = false;

	// 尺寸（cm）— Flipbook 平面的世界空间尺寸
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "1.0", DisplayName = "尺寸（cm）"))
	float Size = 80.f;

	// 附着目标 — Flipbook 附着到哪个角色身上
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "附着目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// 插槽名 — 附着到目标骨骼的指定插槽
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "插槽名"))
	FName Socket = NAME_None;

	// 备选插槽名列表 — 主插槽不存在时按顺序尝试这些备选插槽
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "备选插槽名列表"))
	TArray<FName> SocketFallbackNames;

	// 位置偏移 — 相对于插槽/附着点的额外偏移
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "位置偏移"))
	FVector Offset = FVector::ZeroVector;

	// 投影到可见表面 — 勾选后将 Sprite 从骨骼中心向摄像机方向投影到目标可见表面
	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface", meta = (DisplayName = "投影到可见表面"))
	bool bProjectToVisibleSurface = false;

	// 表面偏移（cm）— 投影到表面后额外向外偏移的距离，防止 Z-fighting
	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface", meta = (ClampMin = "0.0", DisplayName = "表面偏移（cm）"))
	float SurfaceOffset = 6.f;

	// 表面备选半径缩放 — 追踪失败时用角色胶囊半径乘以此值作为备选距离
	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface", meta = (ClampMin = "0.0", DisplayName = "表面备选半径缩放"))
	float SurfaceFallbackRadiusScale = 0.45f;

	// 表面追踪额外距离（cm）— 追踪终点在骨骼位置基础上额外延伸的距离
	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface", meta = (ClampMin = "1.0", DisplayName = "表面追踪额外距离（cm）"))
	float SurfaceTraceExtraDistance = 120.f;

	// 朝向摄像机 — 勾选后 Sprite 始终朝向玩家摄像机（Billboard）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "朝向摄像机"))
	bool bFaceCamera = true;

	// FA停止时销毁 — 勾选后 FA 提前终止时立即销毁此 Flipbook Actor
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "FA停止时销毁"))
	bool bDestroyWithFlow = false;

	// 自发光颜色 — Sprite 的自发光颜色（HDR，可超过1.0以增强亮度）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "自发光颜色"))
	FLinearColor EmissiveColor = FLinearColor::White;

	// 透明度缩放 — 整体透明度倍率（0=全透明，1=完全不透明）
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0.0", DisplayName = "透明度缩放"))
	float AlphaScale = 1.f;

	// 效果名称 — 供 DestroyNiagara/DestroyFlipbook 节点按名称精确销毁此实例
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (DisplayName = "效果名称"))
	FName EffectName = NAME_None;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FVector ProjectToVisibleSurface(AActor* TargetActor, USceneComponent* AttachComp, const FVector& BaseLocation) const;

	UPROPERTY()
	TArray<TObjectPtr<ARune512FlipbookVFXActor>> ActiveActors;
};
