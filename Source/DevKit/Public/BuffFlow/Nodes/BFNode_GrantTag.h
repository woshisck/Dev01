#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_GrantTag.generated.h"

class UAbilitySystemComponent;

/**
 * 临时授予目标一个 Loose GameplayTag。
 *
 * 与 BFNode_AddTag 的区别：
 *   - AddTag  — 永久添加，只在 FA 停止时（Cleanup）才移除
 *   - GrantTag — 支持 Duration：N 秒后自动 Expire；FA 提前停止时 Cleanup 也会移除
 *
 * 输入引脚：
 *   In     — 授予 Tag，启动计时（若 Duration > 0）
 *   Remove — 手动提前移除（触发 Removed 输出）
 *
 * 输出引脚：
 *   Out     — Tag 成功授予后立即触发
 *   Expired — Duration 倒计时结束，Tag 自动移除后触发
 *   Removed — Remove 引脚手动移除后触发
 *   Failed  — 目标无效或无 ASC 时触发
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Grant Tag (Timed)", Category = "BuffFlow|Tag"))
class DEVKIT_API UBFNode_GrantTag : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要授予的 Tag — FA 停止或 Duration 到期时自动移除
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "授予 Tag"))
	FGameplayTag Tag;

	// 持续时间（秒）— 0 = 不自动到期；> 0 = N 秒后自动移除并触发 Expired 引脚
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (ClampMin = "0.0",
		ToolTip = "0 = 不自动到期（只在 FA 停止时移除）；> 0 = N 秒后自动到期", DisplayName = "持续时间（秒）"))
	float Duration = 5.0f;

	// 授予目标 — 将 Tag 添加到哪个 Actor 的 ASC，默认 BuffOwner
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "授予目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void RemoveTagFromTarget();

	FTimerHandle ExpiryTimer;

	/** 累计授予的层数（可能多次触发 In 引脚） */
	int32 TotalCountGranted = 0;
	TWeakObjectPtr<UAbilitySystemComponent> StoredASC;
};
