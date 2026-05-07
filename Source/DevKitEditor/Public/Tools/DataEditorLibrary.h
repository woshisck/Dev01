#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Data/RuneDataAsset.h"
#include "DataEditorLibrary.generated.h"

class URuneDataAsset;
class UEffectDataAsset;
class UCharacterData;
class UMontageConfigDA;
class UMontageAttackDataAsset;
class UMusketActionTuningDataAsset;

/**
 * 数值平衡 EUW 用的编辑器工具函数库
 * 业务规则（详见 Docs/Conventions/DataAuthoring.md）：
 *  - 业务代码读取 DA 字段必须走访问器（GetGoldCost / GetRarity 等）
 *  - 编辑器工具内的批量赋值必须包裹 FScopedTransaction，便于策划用 Ctrl+Z 撤销
 *  - 资产写入后调用 MarkPackageDirty 触发保存提示
 */
UCLASS()
class DEVKITEDITOR_API UDataEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ── 资产收集 ───────────────────────────────────────────────

	/**
	 * 收集项目内全部 URuneDataAsset。
	 * **注意**：内部通过 AssetRegistry + GetAsset() 同步加载，资产量大时会卡编辑器并占内存。
	 * 仅供编辑器工具（EUW / Validator / 一次性导出）使用，运行时勿调。
	 */
	UFUNCTION(BlueprintCallable, Category = "DataEditor|Collect")
	static TArray<URuneDataAsset*> GetAllRuneDAs();

	/** 同 GetAllRuneDAs，对 UEffectDataAsset。*/
	UFUNCTION(BlueprintCallable, Category = "DataEditor|Collect")
	static TArray<UEffectDataAsset*> GetAllEffectDAs();

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Collect")
	static TArray<UCharacterData*> GetAllCharacterDAs();

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Collect")
	static TArray<UMontageConfigDA*> GetAllMontageConfigDAs();

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Collect")
	static TArray<UMontageAttackDataAsset*> GetAllMontageAttackDataDAs();

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Collect")
	static TArray<UMusketActionTuningDataAsset*> GetAllMusketActionTuningDAs();

	// ── 批量赋值（带 Undo）─────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Batch")
	static void BatchSetRuneGoldCost(const TArray<URuneDataAsset*>& Targets, int32 NewGoldCost);

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Batch")
	static void BatchSetRuneRarity(const TArray<URuneDataAsset*>& Targets, ERuneRarity NewRarity);

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Batch")
	static void BatchSetRuneTriggerType(const TArray<URuneDataAsset*>& Targets, ERuneTriggerType NewType);

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Batch")
	static void BatchSetEffectDuration(const TArray<UEffectDataAsset*>& Targets, float NewDuration);

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Batch")
	static void BatchSetEffectMaxStack(const TArray<UEffectDataAsset*>& Targets, int32 NewMaxStack);

	// ── 迁移工具：RuneID(int32) → RuneIdTag(GameplayTag) ────────

	/**
	 * 第 1 阶段：扫描所有 RuneDA，对 RuneID > 0 但 RuneIdTag 未配置的，
	 * 把对应的 Rune.ID.Legacy_<RuneID> 写入 Config/Tags/RuneIDs.ini（去重，不重复追加）。
	 * 不修改任何 DA 资产。
	 * @return 写入的新 Tag 行数（0 表示 .ini 已是最新）
	 *
	 * 完成后必须重启编辑器，让 GameplayTagsManager 加载新 Tag，再调用 ApplyRuneIdTagsAfterRestart
	 */
	UFUNCTION(BlueprintCallable, Category = "DataEditor|Migrate")
	static int32 PrepareRuneIdTagIni();

	/**
	 * 第 2 阶段：在编辑器重启后调用。再次扫描所有 RuneDA，
	 * 对每条 RuneID > 0 / RuneIdTag 未配置的：
	 *   - 通过 RequestGameplayTag 拿到（此时 .ini 已加载）有效 Tag
	 *   - 写回 DA 并标 dirty
	 * 找不到对应 Tag 的会跳过并输出 Warning（说明 .ini 没准备好）
	 * @return 成功写回的 DA 数量
	 */
	UFUNCTION(BlueprintCallable, Category = "DataEditor|Migrate")
	static int32 ApplyRuneIdTagsAfterRestart();

	// ── 双源比对（P0 验证）─────────────────────────────────────

	/**
	 * 遍历所有 RuneDA，对每个字段同时调用访问器与直接读字段，diff 后记录差异
	 * 输出到 Output Log（LogDataEditor 类别）
	 * @return 发现差异的 DA 数量（0 表示访问器与字段语义一致）
	 */
	UFUNCTION(BlueprintCallable, Category = "DataEditor|Verify")
	static int32 VerifyAccessorParity();

	// ── 只读 CSV 快照导出 ───────────────────────────────────────

	/**
	 * 把所有 RuneDA 的关键数值导出到 CSV，便于策划在 Excel 横向比对
	 * @param OutFilePath 完整文件路径，留空则导出到 Saved/Balance/RuneNumbers_<时间戳>.csv
	 * @return 实际写入文件路径，失败返回空字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "DataEditor|Export")
	static FString ExportRuneDAsToCSV(const FString& OutFilePath);

	UFUNCTION(BlueprintCallable, Category = "DataEditor|Export")
	static FString ExportEffectDAsToCSV(const FString& OutFilePath);
};
