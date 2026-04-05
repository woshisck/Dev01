#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "RuneGameplayAbility.generated.h"

class URuneDataAsset;

/**
 * 符文被动 GA 基类。
 *
 * 所有由符文系统（BFNode_GrantGA）授予的 GA 应继承此类，而非直接继承 YogGameplayAbility。
 *
 * 与普通 GA 的区别：
 *   - 语义上标识"这是一个符文效果 GA"（被动、由 FA 控制生命周期）
 *   - 提供 GetGrantingDA() 辅助函数，方便从 SourceObject 读取授予此 GA 的符文 DA
 *   - GA 的生命周期由 BFNode_GrantGA 管理：FA 启动时授予，FA 停止时自动撤销
 *
 * 配置步骤（以击退 GA 为例）：
 *   1. 创建 GA_Knockback，父类选 RuneGameplayAbility
 *   2. Class Defaults → Ability Triggers → Tag = Event.Combat.Knockback, Source = GameplayEvent
 *   3. 实现 ActivateAbilityFromEvent：读 KnockbackForce 属性，调用 LaunchCharacter
 *   4. 在 FA_Knockback 中，起点连 [GrantGA](GA_Knockback)
 */
UCLASS(Blueprintable, meta = (DisplayName = "Rune Gameplay Ability"))
class DEVKIT_API URuneGameplayAbility : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * 获取授予此 GA 的符文 DA。
	 *
	 * 仅在 BFNode_GrantGA 通过 SourceObject 传递 DA 时有效（默认未设置，返回 nullptr）。
	 * 若 GA 需要读取 DA 配置，可在 BFNode_GrantGA 中设置 SourceObject，
	 * 然后通过此函数读取。
	 *
	 * 通常不需要此函数——GA 应直接读取 AttributeSet 中的属性值。
	 */
	UFUNCTION(BlueprintPure, Category = "Rune")
	URuneDataAsset* GetGrantingDA() const;
};
