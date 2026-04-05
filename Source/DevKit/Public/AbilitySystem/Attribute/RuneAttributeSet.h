#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSetMacro.h"
#include "RuneAttributeSet.generated.h"

/**
 * URuneAttributeSet — 符文专属属性集
 *
 * 仅放置由符文系统引入的属性。
 * 与角色通用属性（Health/Attack/MoveSpeed 等）隔离，
 * 避免 BaseAttributeSet 因符文扩展而膨胀。
 *
 * 使用方式：
 *   在角色的 ASC 初始化时（或角色构造函数中）通过
 *   CreateDefaultSubobject<URuneAttributeSet>() 添加到 ASC。
 *
 * 当前属性：
 *   KnockbackForce — 攻击方施加的击退冲量，由击退符文 GE 增减，GA 读取并执行
 */
UCLASS()
class DEVKIT_API URuneAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    URuneAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ============================================================
    //  击退
    // ============================================================

    /**
     * 击退力（Knockback Force）
     * 攻击方施加给被击目标的冲量幅度。
     *
     * 数值流向：
     *   DA_Rune_Knockback.Effects[Add Attribute Modifier]
     *     → GE 将此值 +N（符文激活期间）
     *     → GA_Knockback 读取此值 × 方向向量 → LaunchCharacter
     *
     * 基础值为 0（无符文时不产生额外击退冲量）。
     */
    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_KnockbackForce, Category = "Attributes|Rune")
    FGameplayAttributeData KnockbackForce;
    ATTRIBUTE_ACCESSORS(URuneAttributeSet, KnockbackForce);

    // ── 后续符文专属属性在此扩展 ──────────────────────────────

protected:
    UFUNCTION()
    void OnRep_KnockbackForce(const FGameplayAttributeData& OldValue);
};
