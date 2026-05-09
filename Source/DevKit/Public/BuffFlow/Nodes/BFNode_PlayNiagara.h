#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_PlayNiagara.generated.h"

class UNiagaraSystem;

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Niagara", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_PlayNiagara : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// Niagara 系统资产 — 要播放的粒子效果
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "Niagara 资产"))
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	// 效果名称 — 唯一标识符，供 DestroyNiagara 节点按名称销毁此效果
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "效果名称"))
	FName EffectName = NAME_None;

	// 挂载插槽名 — 挂载到目标骨骼的哪个插槽上（留空 = 挂到根节点）
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "挂载插槽名"))
	FName AttachSocketName = NAME_None;

	// 插槽备选列表 — 主插槽不存在时依次尝试的备选插槽名列表
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "插槽备选列表"))
	TArray<FName> AttachSocketFallbackNames;

	// 挂载目标 — 将 Niagara 效果挂载到哪个 Actor 上
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "挂载目标"))
	EBFTargetSelector AttachTarget = EBFTargetSelector::BuffOwner;

	// 挂载到目标 — 勾选后跟随目标移动；取消勾选则在目标位置生成后脱离
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "挂载到目标"))
	bool bAttachToTarget = true;

	// 位置偏移 — 相对于挂载点的本地坐标偏移
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

	// 旋转偏移 — 相对于挂载点的本地旋转偏移
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "旋转偏移"))
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 缩放 — Niagara 效果的整体缩放值
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "缩放"))
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	// 生命时间（秒）— 0 = 由 Niagara 系统自动完成；> 0 = 强制在 N 秒后停止
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (ClampMin = "0.0", DisplayName = "生命时间（秒）"))
	float Lifetime = 0.f;

	// FA停止时销毁 — 勾选后 FA 停止时立即销毁此 Niagara 效果
	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (DisplayName = "FA停止时销毁"))
	bool bDestroyWithFlow = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
