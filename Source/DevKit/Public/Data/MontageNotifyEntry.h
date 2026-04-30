// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "MontageNotifyEntry.generated.h"

class UMontageAttackDataAsset;

USTRUCT(BlueprintType)
struct DEVKIT_API FTaggedMontageAttackData
{
	GENERATED_BODY()

	/** Tags that must be present on the attacker or current combo context. Empty means always allowed. */
	UPROPERTY(EditAnywhere, Category = "Config")
	FGameplayTagContainer RequiredTags;

	/** Tags that reject this candidate when present. */
	UPROPERTY(EditAnywhere, Category = "Config")
	FGameplayTagContainer BlockedTags;

	/** Priority used when multiple candidates match. Larger wins. */
	UPROPERTY(EditAnywhere, Category = "Config")
	int32 Priority = 0;

	/** Attack numbers, hitboxes, hit stop, hit events, and extra rune effects. */
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UMontageAttackDataAsset> AttackData;

	bool Matches(const FGameplayTagContainer& ContextTags) const
	{
		const bool bRequiredOk = RequiredTags.IsEmpty() || ContextTags.HasAll(RequiredTags);
		const bool bBlockedOk = BlockedTags.IsEmpty() || !ContextTags.HasAny(BlockedTags);
		return bRequiredOk && bBlockedOk && AttackData != nullptr;
	}
};

/**
 * 蒙太奇 Notify 条目基类
 *
 * 用法：在 MontageConfigDA 的 Entries 数组中点 [+] 选择具体子类，
 * 展开后填写该类型的参数。
 * 新增行为只需继承此类并添加 UPROPERTY 字段，无需修改 DA 本身。
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType,
	meta = (DisplayName = "Montage Notify Entry"))
class DEVKIT_API UMontageNotifyEntry : public UObject
{
	GENERATED_BODY()
public:
	/** 帧数转归一化时间（由 DA 调用，不需要手动填） */
	float ToNormalized(int32 Frame, int32 TotalFrames) const
	{
		return TotalFrames > 0 ? FMath::Clamp((float)Frame / TotalFrames, 0.f, 1.f) : 0.f;
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// 内置条目类型
// ─────────────────────────────────────────────────────────────────────────────

/**
 * 连击输入窗口
 * 在 [StartFrame, EndFrame] 区间内开启 CanCombo tag，允许接下一击
 */
UCLASS(meta = (DisplayName = "Combo Window"))
class DEVKIT_API UMNE_ComboWindow : public UMontageNotifyEntry
{
	GENERATED_BODY()
public:
	/** 连击窗口开始帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 StartFrame = 18;

	/** 连击窗口结束帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 EndFrame = 27;
};

/**
 * 后摇移动取消
 * 从 Frame 开始，若检测到移动输入则以 BlendTime 快速混出技能
 */
UCLASS(meta = (DisplayName = "Early Exit (Move Cancel)"))
class DEVKIT_API UMNE_EarlyExit : public UMontageNotifyEntry
{
	GENERATED_BODY()
public:
	/** 允许取消的起始帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 Frame = 22;

	/** 混出时长（秒），0 = 瞬切 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float BlendTime = 0.1f;
};

/**
 * Gameplay Tag 窗口
 * 在 [StartFrame, EndFrame] 区间内为角色 ASC 添加指定 Tag（替代 ANS_AddGameplayTag）
 */
UCLASS(meta = (DisplayName = "Gameplay Tag Window"))
class DEVKIT_API UMNE_TagWindow : public UMontageNotifyEntry
{
	GENERATED_BODY()
public:
	/** 要添加的 Tag（可多个） */
	UPROPERTY(EditAnywhere, Category = "Config")
	FGameplayTagContainer Tags;

	/** Tag 添加起始帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 StartFrame = 0;

	/** Tag 移除帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 EndFrame = 5;
};

/**
 * 命中判定窗口
 * 在 [StartFrame, EndFrame] 区间内开启 HitDetection（替代 ANS 命中范围检测）
 */
UCLASS(meta = (DisplayName = "Hit Detection Window"))
class DEVKIT_API UMNE_HitWindow : public UMontageNotifyEntry
{
	GENERATED_BODY()
public:
	/** 判定开始帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 StartFrame = 8;

	/** 判定结束帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 EndFrame = 14;

	/**
	 * Optional DA candidates for this hit window.
	 * V1 uses the first/best matching candidate as configuration data; the
	 * montage notify can still be the actual frame trigger.
	 */
	UPROPERTY(EditAnywhere, Category = "Config")
	TArray<FTaggedMontageAttackData> AttackDataCandidates;

	UMontageAttackDataAsset* ResolveAttackData(const FGameplayTagContainer& ContextTags) const
	{
		const FTaggedMontageAttackData* Best = nullptr;
		for (const FTaggedMontageAttackData& Candidate : AttackDataCandidates)
		{
			if (!Candidate.Matches(ContextTags))
			{
				continue;
			}

			if (!Best || Candidate.Priority > Best->Priority)
			{
				Best = &Candidate;
			}
		}

		return Best ? Best->AttackData.Get() : nullptr;
	}
};

/**
 * 自定义帧事件
 * 在指定帧触发一个 GameplayEvent，由 ASC 路由到对应 GA 处理（One-shot）
 */
UCLASS(meta = (DisplayName = "Gameplay Event at Frame"))
class DEVKIT_API UMNE_GameplayEvent : public UMontageNotifyEntry
{
	GENERATED_BODY()
public:
	/** 触发帧 */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = "0"))
	int32 Frame = 10;

	/** 发送的 EventTag */
	UPROPERTY(EditAnywhere, Category = "Config")
	FGameplayTag EventTag;
};
