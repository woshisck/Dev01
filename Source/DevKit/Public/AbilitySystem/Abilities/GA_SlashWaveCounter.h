#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_SlashWaveCounter.generated.h"

class ASlashWaveProjectile;
class UAbilityTask_WaitGameplayEvent;

/**
 * 刀光波计数 GA（持久被动）。
 *
 * 工作原理：
 *   - 被授予时自动激活（OnGiveAbility → TryActivateAbility），永不主动结束。
 *   - 激活后创建 WaitGameplayEvent 任务持续监听攻击事件：
 *       命中模式（bHitRequired=true）：监听 Action.Rune.SlashWaveHit
 *           → FA_Rune_SlashWave 的 OnDamageDealt 触发后由 BFNode_SendGameplayEvent 发送
 *       挥刀模式（bHitRequired=false）：监听 Action.Attack.Swing
 *           → AN_MeleeDamage::Notify 每次挥刀时发送
 *           → 须配合 SwingModeGateTag（FA 激活时授予，停止时移除）防止无符文时误触发
 *   - 每收到事件后计数器 +1；达到 HitsRequired 后重置并生成 SlashWaveProjectile。
 *
 * 策划工作流：
 *   1. 编辑器创建 Blueprint 子类 BGA_SlashWaveCounter（Parent = GA_SlashWaveCounter）
 *   2. 配置 ProjectileClass（创建 BP_SlashWaveProjectile，Parent = ASlashWaveProjectile）
 *   3. 配置 SlashDamageEffect（创建 GE_SlashWaveDamage，Instant，SetByCaller Attribute.ActDamage）
 *   4. 在玩家 BP 的 BeginPlay → GiveAbility(BGA_SlashWaveCounter)
 *   5. 命中模式：创建 FA_Rune_SlashWave（见 TestRune_CreationGuide.md 符文 1008）
 *      挥刀模式：额外配置 SwingModeGateTag，FA 激活时授予该 Tag
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_SlashWaveCounter : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_SlashWaveCounter();

	/** 被授予时立即自我激活（被动 GA 标准模式）*/
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	                           const FGameplayAbilitySpec& Spec) override;

	// ── 计数配置 ────────────────────────────────────────────────────────────

	/** 触发刀光所需的事件次数（默认每 3 次触发一次）*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Counter")
	int32 HitsRequired = 3;

	// ── 模式配置 ────────────────────────────────────────────────────────────

	/**
	 * 命中模式（true）：监听 Action.Rune.SlashWaveHit，只在 FA_Rune_SlashWave 命中敌人时计数。
	 * 挥刀模式（false）：监听 Action.Attack.Swing，每次挥刀（无论是否命中）均计数。
	 *   ⚠️ 挥刀模式须配置 SwingModeGateTag，FA 激活时授予，停止时移除，防止无符文时误触发。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Counter")
	bool bHitRequired = true;

	/**
	 * 挥刀模式守卫 Tag（bHitRequired=false 时生效）。
	 * 仅当所有者持有此 Tag 时才计数。
	 * 对应 FA_Rune_SlashWave_Swing 激活时授予的 Tag（例如 Buff.Status.SlashWaveSwingActive）。
	 * 命中模式留空。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Counter")
	FGameplayTag SwingModeGateTag;

	// ── 刀光投射物配置 ──────────────────────────────────────────────────────

	/** 刀光投射物 Blueprint 子类（ASlashWaveProjectile → BP_SlashWaveProjectile）*/
	UPROPERTY(EditDefaultsOnly, Category = "SlashWave|Projectile")
	TSubclassOf<ASlashWaveProjectile> ProjectileClass;

	/** 刀光伤害量（通过 SetByCaller Attribute.ActDamage 传递给 GE）*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Projectile")
	float SlashDamage = 30.f;

	/**
	 * 刀光命中 GE（Instant，需含 SetByCaller Attribute.ActDamage Modifier）。
	 * 创建 GE_SlashWaveDamage（Blueprint GE，Instant，扣血 Modifier，SetByCaller Attribute.ActDamage）。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SlashWave|Projectile")
	TSubclassOf<UGameplayEffect> SlashDamageEffect;

	/** 生成投射物距角色中心的前向偏移（cm）*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Projectile")
	float SpawnOffset = 80.f;

protected:
	virtual void ActivateAbility(
	    const FGameplayAbilitySpecHandle Handle,
	    const FGameplayAbilityActorInfo* ActorInfo,
	    const FGameplayAbilityActivationInfo ActivationInfo,
	    const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
	    const FGameplayAbilitySpecHandle Handle,
	    const FGameplayAbilityActorInfo* ActorInfo,
	    const FGameplayAbilityActivationInfo ActivationInfo,
	    bool bReplicateEndAbility,
	    bool bWasCancelled) override;

private:
	/** 当前累积计数 */
	int32 CurrentCount = 0;

	TWeakObjectPtr<AActor> PendingSlashWaveInitialTarget;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;

	UFUNCTION()
	void OnAttackEventReceived(FGameplayEventData Payload);

	/** 生成刀光投射物（在角色前方 SpawnOffset 处，沿角色朝向发射）*/
	void SpawnSlashWave();
};
