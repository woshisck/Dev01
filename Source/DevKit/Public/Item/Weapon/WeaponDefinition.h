
#pragma once

#include "CoreMinimal.h"

#include "AbilitySystem/Abilities/YogAbilitySet.h"
#include "Animation/YogAnimInstance.h"
#include "Component/BackpackGridComponent.h"
#include "Data/AbilityData.h"
#include "GameplayTagContainer.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Item/Weapon/WeaponTypes.h"

#include "WeaponDefinition.generated.h"

class ULevelInfoPopupDA;
class URangedProjectileDefinition;
class UWeaponSkillDataAsset;

class UYogAbilitySet;
class AWeaponInstance;
class APlayerCharacterBase;
class UMaterialInterface;
class UGameplayEffect;
class URuneDataAsset;
class USoundBase;
//class UYogAnimInstance;



USTRUCT(BlueprintType)
struct FBackpackConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backpack", meta = (DisplayName = "网格宽度"))
    int32 GridWidth = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backpack", meta = (DisplayName = "网格高度"))
    int32 GridHeight = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backpack", meta = (DisplayName = "激活区配置"))
    FActivationZoneConfig ActivationZoneConfig;
};

USTRUCT(BlueprintType)
struct FWeaponSpawnData
{
	GENERATED_BODY()

	FWeaponSpawnData()
	{}

	UPROPERTY(EditAnywhere, Category = "Equip Spawn", meta = (DisplayName = "武器 Actor 类"))
	TSubclassOf<AWeaponInstance> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = "Equip Spawn", meta = (DisplayName = "挂接插槽"))
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = "Equip Spawn", meta = (DisplayName = "挂接变换"))
	FTransform AttachTransform;

	UPROPERTY(EditAnywhere, Category = "Equip Spawn", meta = (DisplayName = "武器动画层"))
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	// Optional: Save game data for persistence
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip Spawn", meta = (DisplayName = "写入存档"))
	bool bShouldSaveToGame = false;
};

UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// Class to spawn
	//UPROPERTY(EditDefaultsOnly, Category = Equipment)
	//TSubclassOf<AWeaponInstance> InstanceType;

	// Gameplay ability sets to grant when this is equipped
	//UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	//TArray<TObjectPtr<UYogAbilitySet>> AbilitySetsToGrant;
	UWeaponDefinition(){};

	// Deprecated ComboGraph references. Transient so resaving migrated weapons drops
	// serialized graph object refs; player combat uses the typed AbilityData fields below.
	UPROPERTY(Transient)
	TObjectPtr<UObject> GameplayAbilityComboGraph;

	// Deprecated ComboGraph reference. Kept as a native name only for old asset load.
	UPROPERTY(Transient)
	TObjectPtr<UObject> WeaponSkillComboGraph;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Action Data", meta = (DisplayName = "普通攻击动作数据"))
	TObjectPtr<UWeaponAttackAbilityMontageData> AttackAbilityData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Action Data|Deprecated", meta = (DeprecatedProperty, DisplayName = "旧战技动作数据", DeprecationMessage = "请使用“可装备战技列表”和战技 DA 内的“战技动作数据”。"))
	TObjectPtr<UWeaponSkillAbilityMontageData> WeaponSkillAbilityData;

	/**
	 * Weapon skills this weapon is allowed to equip. Each entry owns a distinct
	 * GA implementation and DA configuration. Runtime permits exactly one
	 * selected entry per weapon slot.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon Skill", meta = (DisplayName = "可装备战技列表"))
	TArray<TObjectPtr<UWeaponSkillDataAsset>> AvailableWeaponSkills;

	/** Defaults to the first valid AvailableWeaponSkills entry when unset/invalid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon Skill", meta = (DisplayName = "默认装备战技"))
	TObjectPtr<UWeaponSkillDataAsset> DefaultWeaponSkill;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Action Data|Deprecated", meta = (DeprecatedProperty, DisplayName = "旧特殊技能动作数据", DeprecationMessage = "请改用当前装备战技的专属动作数据。"))
	TObjectPtr<USpecialAbilityMontageData> SpecialAbilityData;

	// Optional weapon-specific reaction/passive data. Merged after action data so
	// hit react/death passive rows can override the character's base fallbacks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Action Data", meta = (DisplayName = "被动反应动作数据"))
	TObjectPtr<UWeaponPassiveAbilityMontageData> PassiveAbilityData;

	// 武器类型：决定装备时挂在 ASC 上的 Weapon.Type.* LooseTag。
	// 玩家专属攻击 GA 通过 ActivationRequiredTags 持有该 Tag → 自动隔离近战/远程激活路径。
	// 默认 Melee 保持向后兼容（旧武器 DA 不需要重新配）。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (DisplayName = "武器类型"))
	EWeaponType WeaponType = EWeaponType::Melee;

	// Hit-impact intensity level (1+) this weapon produces on victims. Selects which Levels entry
	// is read from each matching UEnemyHitImpactData entry. Read by GA_MeleeAttack and passed to
	// the victim's UHitImpactVisualComponent. Non-player attackers default to level 1.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit Feedback", meta = (DisplayName = "命中冲击等级", ClampMin = "1"))
	int32 HitImpactLevel = 1;

	// Attacker-side layer of the composite hit sound: the short dry transient identifying WHICH
	// weapon struck (blade shing / hammer thock). The victim's UEnemyHitImpactData tier supplies
	// the complementary body-and-tail layer identifying WHAT was struck; the two mix into one
	// impact. Played once per swing by GA_MeleeAttack no matter how many targets were hit — one
	// swing only rings the weapon once, so stacking it per victim would spike amplitude and comb
	// filter. Author it short (30-80ms) and free of reverb tail so it does not mask the victim
	// layer. Optional; leave empty to keep tier-sound-only behaviour.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit Feedback", meta = (DisplayName = "命中瞬态音效"))
	TObjectPtr<USoundBase> ImpactTransientSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit Feedback", meta = (DisplayName = "命中瞬态音量", ClampMin = "0.0"))
	float ImpactTransientVolume = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit Feedback", meta = (DisplayName = "命中瞬态音调", ClampMin = "0.0"))
	float ImpactTransientPitch = 1.f;

	// Projectile data used by AN_FireProjectile to spawn bullets via UYogBulletManagerSubsystem.
	// Only relevant when WeaponType == Ranged.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",
		meta = (EditCondition = "WeaponType == EWeaponType::Ranged", EditConditionHides, DisplayName = "远程弹丸定义"))
	TObjectPtr<URangedProjectileDefinition> ProjectileDefinition;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = "Equipment", meta = (DisplayName = "装备后生成的武器 Actor"))
	TArray<FWeaponSpawnData> ActorsToSpawn;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Pickup|Display Mesh", meta = (DisplayName = "模型位置偏移"))
	FVector WeaponMeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Pickup|Display Mesh", meta = (DisplayName = "模型旋转"))
	FRotator WeaponRotation;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Pickup|Display Mesh", meta = (DisplayName = "模型缩放"))
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Pickup|Display Mesh", meta = (DisplayName = "场景显示网格"))
	TObjectPtr<UStaticMesh> DisplayMesh;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Pickup|Animation", meta = (DisplayName = "武器动画层"))
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	// 热度阶段 Overlay 材质（带 Fresnel + EmissiveColor 参数）
	// 武器被拾取时由 WeaponSpawner 自动传给 WeaponInstance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat Effect", meta = (DisplayName = "热度叠加材质"))
	TObjectPtr<UMaterialInterface> HeatOverlayMaterial;

	// Deprecated compatibility data for the old heat/backpack rune grid.
	UPROPERTY()
	FBackpackConfig BackpackConfig;

	// 武器展示信息（名称/描述/缩略图/激活区图像），驱动武器浮窗
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display Info", meta = (DisplayName = "武器显示信息"))
	TObjectPtr<UWeaponInfoDA> WeaponInfo;

	// Deprecated compatibility data for old backpack rune seeding. Do not use as a combat deck fallback.
	UPROPERTY()
	TArray<TObjectPtr<URuneDataAsset>> InitialRunes;

	// Combat deck attack sequence. Skill, WeaponSkill, and Dash cards route to their own single slots.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Deck", meta = (DisplayName = "初始战斗卡组"))
	TArray<TObjectPtr<URuneDataAsset>> InitialCombatDeck;

	/**
	 * Applied to the player's ASC at the start of any attack that successfully consumed a Just Combo
	 * input (attack pressed during Character.State.Window.JustCombo). Removed in EndAbility.
	 * Must be Infinite or Has Duration — the GA always removes it by handle on ability end.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Precision Combo", meta = (DisplayName = "精准连击效果"))
	TSubclassOf<UGameplayEffect> JustComboEffect;

	// Deprecated compatibility data. Current cards loop in sequence without shuffle downtime.
	UPROPERTY()
	float ShuffleCooldownDuration = 0.0f;

	// Deprecated compatibility data. Current attack sequence always uses all equipped attack cards.
	UPROPERTY()
	int32 MaxActiveSequenceSize = 0;

	// 勾选后武器仅作展示：玩家按 E 弹出 PreviewPopup 信息浮窗，不可实际拾取
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview Mode", meta = (DisplayName = "仅用于预览"))
	bool bPreviewOnly = false;

	// bPreviewOnly=true 时显示的 LevelInfoPopup DA（填标题/正文/自动关闭时长）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview Mode", meta = (DisplayName = "预览提示数据"))
	TObjectPtr<ULevelInfoPopupDA> PreviewPopup;

	UFUNCTION(BlueprintCallable)
	void SetupWeaponToCharacter(USkeletalMeshComponent* AttachTarget, APlayerCharacterBase* ReceivingChar);

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon Skill", meta = (DisplayName = "能否装备战技"))
	bool CanEquipWeaponSkill(const UWeaponSkillDataAsset* WeaponSkill) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon Skill", meta = (DisplayName = "解析默认战技"))
	UWeaponSkillDataAsset* ResolveDefaultWeaponSkill() const;


private:
	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);

};
