#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"   // FDamageBreakdown
#include "CombatLogStatics.generated.h"

// ============================================================
//  过滤器枚举（EUW_CombatLog 使用）
// ============================================================

UENUM(BlueprintType)
enum class ECombatLogFilter : uint8
{
	All      UMETA(DisplayName = "全部"),
	Normal   UMETA(DisplayName = "普通"),
	Crit     UMETA(DisplayName = "暴击"),
	Rune     UMETA(DisplayName = "符文"),
	Bleed    UMETA(DisplayName = "流血"),
	// 512版本新增（追加到末尾，勿插入中间，否则蓝图枚举值错位）
	Card     UMETA(DisplayName = "卡牌"),
	Finisher UMETA(DisplayName = "终结技"),
	Link     UMETA(DisplayName = "连携"),
	Shuffle  UMETA(DisplayName = "洗牌"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatLogTextSegment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FLinearColor Color = FLinearColor::White;
};

/**
 * 战斗日志静态数据桥
 *
 * 使用静态存储，天然穿越 PIE / Editor 世界边界。
 * YogAbilitySystemComponent 写入，Editor Utility Widget 读取。
 *
 * 在编辑器中创建 EUW 步骤：
 *   1. Content Browser → 右键 → Editor Utilities → Editor Utility Widget
 *   2. 父类选默认 EditorUtilityWidget 即可
 *   3. 命名 EUW_CombatLog，放到 Content/UI/Debug/
 *   4. Designer 里放 ScrollBox + 过滤按钮 + SummaryText（同 WBP_DamageBreakdown）
 *   5. Graph 里用 EventTick：
 *        GetVersion != CachedVersion → 刷新显示 → 更新 CachedVersion
 *   6. 窗口菜单 → Tools → EUW_CombatLog 打开（可停靠）
 */
UCLASS()
class DEVKIT_API UCombatLogStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ── 写入（由 YogAbilitySystemComponent 调用） ─────────────────

	/** 追加一条伤害记录（内部维护最多 MaxEntries 条） */
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static void PushEntry(const FDamageBreakdown& Entry);

	// ── 读取（由 EUW Tick 调用） ──────────────────────────────────

	/**
	 * 版本号：每次 PushEntry / ClearEntries 时自增。
	 * EUW 缓存这个值，只有不同时才重建显示，避免每帧重绘。
	 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static int32 GetVersion();

	/** 返回所有记录（按时间从旧到新） */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static const TArray<FDamageBreakdown>& GetAllEntries();

	/** 返回记录总数 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static int32 GetEntryCount();

	// ── 管理 ──────────────────────────────────────────────────────

	/** 清空所有记录（可绑定到 EUW 的"重置"按钮） */
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static void ClearEntries();

	/**
	 * 返回过滤后的完整日志文字（一次性拿到，直接 SetText）
	 * 格式：[MM:SS] Source → Target  [动作]  25×1.2×1.0 ★CRIT = 30.0
	 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static FString GetFormattedLog(ECombatLogFilter Filter = ECombatLogFilter::All);

	/** 返回会话统计汇总文字（直接 SetText） */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static FString GetFormattedSummary();

	/** 格式化单条记录为显示文字（供 Blueprint 逐条创建行时使用） */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static FString GetEntryText(const FDamageBreakdown& Entry);

	/** 格式化单条记录为分段上色文本，供 Dota2 风格日志行使用 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static TArray<FCombatLogTextSegment> GetEntryTextSegments(const FDamageBreakdown& Entry, bool bDebugMode = false);

	/** 返回单条记录的颜色（普通=白，暴击=黄，符文=紫，流血=红） */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static FLinearColor GetEntryColor(const FDamageBreakdown& Entry);

	/** 判断单条记录是否通过过滤器 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static bool PassesFilter(const FDamageBreakdown& Entry, ECombatLogFilter Filter);

	/** 判断记录是否通过类型、攻击者、目标和时间窗口组合过滤 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static bool PassesAdvancedFilter(
		const FDamageBreakdown& Entry,
		ECombatLogFilter Filter,
		const FString& SourceFilter,
		const FString& TargetFilter,
		float CurrentTime,
		float TimeWindowSeconds);

	/** 清理蓝图生成名，转换为更适合日志阅读的显示名 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	static FString GetDisplayActorName(const FString& RawName);

	/** 单次会话最多保留的记录数（超出后移除最旧的） */
	static constexpr int32 MaxEntries = 500;

private:
	static TArray<FDamageBreakdown> Entries;
	static int32 Version;
};
