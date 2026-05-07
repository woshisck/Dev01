#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DataValidator.generated.h"

USTRUCT(BlueprintType)
struct DEVKITEDITOR_API FDataValidationReport
{
	GENERATED_BODY()

	/** 致命错误（如重复 RuneIdTag、Fragment 引用断链、Duration 非法），需修复才能 Cook */
	UPROPERTY(BlueprintReadOnly)
	int32 ErrorCount = 0;

	/** 警告（如 RuneIdTag 未配置/为占位 Tag、RuneName 为空、ChainDirections 空、Shape 空），策划判断是否修复 */
	UPROPERTY(BlueprintReadOnly)
	int32 WarningCount = 0;

	/** 扫描的 DA 总数 */
	UPROPERTY(BlueprintReadOnly)
	int32 ScannedCount = 0;

	/** 简要文本，可在 EUW 直接显示 */
	UPROPERTY(BlueprintReadOnly)
	FString Summary;
};

/**
 * 静态校验工具：批量扫描 DA 并报告问题（不修改资产）
 * 输出到 Output Log（LogDataValidator）+ 返回 FDataValidationReport
 *
 * 严重度划分（与 FDataValidationReport 字段一致）：
 *   ERROR   — 重复身份 Tag、引用断链、字段非法（< 0 / Duration <= 0）
 *   WARNING — 身份 Tag 未配置或为占位 Tag、RuneName 为空、ChainDirections 为空、Shape 为空
 */
UCLASS()
class DEVKITEDITOR_API UDataValidator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 校验所有 RuneDataAsset：身份 Tag 唯一性/占位检查、字段范围、Shape/GenericEffects 完整性 */
	UFUNCTION(BlueprintCallable, Category = "DataValidator")
	static FDataValidationReport ValidateAllRuneDAs();

	/** 校验所有 EffectDataAsset：EffectTag 重复/占位、Duration 合法性、Fragment 引用断链、Stack 配置自洽 */
	UFUNCTION(BlueprintCallable, Category = "DataValidator")
	static FDataValidationReport ValidateAllEffectDAs();

	/** 一键全量校验（合并报告） */
	UFUNCTION(BlueprintCallable, Category = "DataValidator")
	static FDataValidationReport ValidateAll();
};
