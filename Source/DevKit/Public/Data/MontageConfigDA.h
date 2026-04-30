// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/MontageNotifyEntry.h"
#include "MontageConfigDA.generated.h"

class UMontageAttackDataAsset;

/**
 * 单个蒙太奇的完整配置 DataAsset
 *
 * 用法：
 *   1. 填写 Montage 和 TotalFrames
 *   2. 在 Entries 数组中按需添加条目类型（ComboWindow / EarlyExit / TagWindow / 自定义...）
 *   3. 将此 DA 引用到 AbilityData 或直接挂在 GA 上
 *
 * 未来可扩展：添加新条目类型只需新建 UMontageNotifyEntry 子类，不改此 DA
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UMontageConfigDA : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ─── 蒙太奇 ──────────────────────────────────────────────────────────────

	/** 关联的蒙太奇资产 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage;

	/** Optional tags describing this montage config, for tools and branch selection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
	FGameplayTagContainer ConfigTags;

	/**
	 * 蒙太奇总帧数（与动画编辑器中的帧数一致）
	 * 所有 Entry 中填写的帧数均以此为基准归一化
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage", meta = (ClampMin = "1"))
	int32 TotalFrames = 30;

	// ─── Notify 条目 ─────────────────────────────────────────────────────────

	/**
	 * Notify 配置列表
	 * 点 [+] 选择条目类型后展开填参数，支持多个同类条目（如两段命中窗口）
	 *
	 * 内置类型：
	 *   - Combo Window          连击输入窗口
	 *   - Early Exit            后摇移动取消
	 *   - Gameplay Tag Window   Tag 区间（替代 ANS_AddGameplayTag）
	 *   - Hit Detection Window  命中判定区间
	 *   - Gameplay Event        单帧 GameplayEvent 触发
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Notifies")
	TArray<TObjectPtr<UMontageNotifyEntry>> Entries;

	// ─── 工具函数 ─────────────────────────────────────────────────────────────

	/** 获取指定类型的所有条目（供 GA_PlayMontage 遍历） */
	template<typename T>
	TArray<T*> GetEntriesOfType() const
	{
		TArray<T*> Result;
		for (UMontageNotifyEntry* Entry : Entries)
		{
			if (T* Typed = Cast<T>(Entry))
				Result.Add(Typed);
		}
		return Result;
	}

	/** 帧数 → 归一化时间（0~1），供内部计时器使用 */
	UFUNCTION(BlueprintPure, Category = "MontageConfig")
	float FrameToNormalized(int32 Frame) const
	{
		return TotalFrames > 0 ? FMath::Clamp((float)Frame / TotalFrames, 0.f, 1.f) : 0.f;
	}

	/** 帧数 → 实际时间（秒），需要传入蒙太奇实际时长 */
	UFUNCTION(BlueprintPure, Category = "MontageConfig")
	float FrameToTime(int32 Frame, float MontageDuration) const
	{
		return FrameToNormalized(Frame) * MontageDuration;
	}

	/** Returns the best attack data referenced by the first Hit Window that has a matching candidate. */
	UFUNCTION(BlueprintPure, Category = "MontageConfig")
	UMontageAttackDataAsset* ResolveAttackData(const FGameplayTagContainer& ContextTags) const;
};
