#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StateConflictDataAsset.generated.h"

/**
 * 单条状态冲突规则
 * 当 ActiveTag 出现在 ASC 上时，对 BlockTags / CancelTags 执行对应操作
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FStateConflictRule
{
	GENERATED_BODY()

	// 触发本条规则的状态 Tag（出现在 ASC 上时生效）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateConflict")
	FGameplayTag ActiveTag;

	// 阻止激活：AbilityTags 含有这些 Tag 的 GA 无法激活
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateConflict")
	FGameplayTagContainer BlockTags;

	// 立即取消：AbilityTags 含有这些 Tag 的 GA 立即被取消
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateConflict")
	FGameplayTagContainer CancelTags;

	// 优先级（数值越高越强势，-1 = 不参与优先级判断）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateConflict")
	int32 Priority = 0;
};

/**
 * 状态冲突表 DataAsset
 * 游戏启动 / ASC 初始化时读取，构建全局冲突索引
 */
UCLASS(BlueprintType)
class DEVKIT_API UStateConflictDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateConflict")
	TArray<FStateConflictRule> Rules;
};