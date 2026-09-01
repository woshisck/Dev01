
#pragma once

#include "CoreMinimal.h"

#include "AbilitySystem/Abilities/YogAbilitySet.h"
#include "Animation/YogAnimInstance.h"
#include "Component/BackpackGridComponent.h"
#include "Data/AbilityData.h"
#include "GameplayTagContainer.h"
#include "Item/Weapon/WeaponDefinitionBase.h"
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
class UYogGameplayEffect;
class URuneDataAsset;
class USoundBase;
class UNiagaraSystem;
//class UYogAnimInstance;



USTRUCT(BlueprintType)
struct FBackpackConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "背包", meta = (DisplayName = "网格宽度"))
    int32 GridWidth = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "背包", meta = (DisplayName = "网格高度"))
    int32 GridHeight = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "背包", meta = (DisplayName = "激活区配置"))
    FActivationZoneConfig ActivationZoneConfig;
};

UENUM(BlueprintType)
enum class EJustComboEffectLifetime : uint8
{
	// Applied the moment the Just Combo input lands. Behaves like any other GE:
	// stacks across procs and expires on its own duration. Never removed by an ability.
	Duration       UMETA(DisplayName = "持续时间"),

	// Applied at the start of the next attack and removed when that ability ends.
	// Captured at proc time, so it survives a weapon switch and keeps the effect
	// authored on the weapon that earned it.
	NextAttackOnly UMETA(DisplayName = "仅下次攻击"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FJustComboEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "战斗|精准连击", meta = (DisplayName = "效果"))
	TSubclassOf<UYogGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "战斗|精准连击", meta = (DisplayName = "生效方式"))
	EJustComboEffectLifetime Lifetime = EJustComboEffectLifetime::Duration;
};

UCLASS(Blueprintable, BlueprintType, Const)
class DEVKIT_API UWeaponDefinition : public UWeaponDefinitionBase
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗|动作数据", meta = (DisplayName = "普通攻击动作数据"))
	TObjectPtr<UWeaponAttackAbilityMontageData> AttackAbilityData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗|动作数据|已弃用", meta = (DeprecatedProperty, DisplayName = "旧战技动作数据", DeprecationMessage = "请使用“可装备战技列表”和战技 DA 内的“战技动作数据”。"))
	TObjectPtr<UWeaponSkillAbilityMontageData> WeaponSkillAbilityData;

	/**
	 * Weapon skills this weapon is allowed to equip. Each entry owns a distinct
	 * GA implementation and DA configuration. Runtime permits exactly one
	 * selected entry per weapon slot.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗|战技", meta = (DisplayName = "可装备战技列表"))
	TArray<TObjectPtr<UWeaponSkillDataAsset>> AvailableWeaponSkills;

	/** Defaults to the first valid AvailableWeaponSkills entry when unset/invalid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗|战技", meta = (DisplayName = "默认装备战技"))
	TObjectPtr<UWeaponSkillDataAsset> DefaultWeaponSkill;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗|动作数据|已弃用", meta = (DeprecatedProperty, DisplayName = "旧特殊技能动作数据", DeprecationMessage = "请改用当前装备战技的专属动作数据。"))
	TObjectPtr<USpecialAbilityMontageData> SpecialAbilityData;

	// Optional weapon-specific reaction/passive data. Merged after action data so
	// hit react/death passive rows can override the character's base fallbacks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗|动作数据", meta = (DisplayName = "被动反应动作数据"))
	TObjectPtr<UWeaponPassiveAbilityMontageData> PassiveAbilityData;

	// 武器类型：决定装备时挂在 ASC 上的 Weapon.Type.* LooseTag。
	// 玩家专属攻击 GA 通过 ActivationRequiredTags 持有该 Tag → 自动隔离近战/远程激活路径。
	// 默认 Melee 保持向后兼容（旧武器 DA 不需要重新配）。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "装备", meta = (DisplayName = "武器类型"))
	EWeaponType WeaponType = EWeaponType::Melee;

	// Projectile data used by AN_FireProjectile to spawn bullets via UYogBulletManagerSubsystem.
	// Only relevant when WeaponType == Ranged.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "装备",
		meta = (EditCondition = "WeaponType == EWeaponType::Ranged", EditConditionHides, DisplayName = "远程弹丸定义"))
	TObjectPtr<URangedProjectileDefinition> ProjectileDefinition;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "场景拾取|显示模型", meta = (DisplayName = "模型位置偏移"))
	FVector WeaponMeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "场景拾取|显示模型", meta = (DisplayName = "模型旋转"))
	FRotator WeaponRotation;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "场景拾取|显示模型", meta = (DisplayName = "模型缩放"))
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "场景拾取|显示模型", meta = (DisplayName = "场景显示网格"))
	TObjectPtr<UStaticMesh> DisplayMesh;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "场景拾取|动画", meta = (DisplayName = "武器动画层"))
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	// 热度阶段 Overlay 材质（带 Fresnel + EmissiveColor 参数）
	// 武器被拾取时由 WeaponSpawner 自动传给 WeaponInstance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "热度效果", meta = (DisplayName = "热度叠加材质"))
	TObjectPtr<UMaterialInterface> HeatOverlayMaterial;

	// Deprecated compatibility data for the old heat/backpack rune grid.
	UPROPERTY()
	FBackpackConfig BackpackConfig;

	// 武器展示信息（名称/描述/缩略图/激活区图像），驱动武器浮窗
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "显示信息", meta = (DisplayName = "武器显示信息"))
	TObjectPtr<UWeaponInfoDA> WeaponInfo;

	// Deprecated compatibility data for old backpack rune seeding. Do not use as a combat deck fallback.
	UPROPERTY()
	TArray<TObjectPtr<URuneDataAsset>> InitialRunes;

	// Combat deck attack sequence. Skill, WeaponSkill, and Dash cards route to their own single slots.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗卡组", meta = (DisplayName = "初始战斗卡组"))
	TArray<TObjectPtr<URuneDataAsset>> InitialCombatDeck;

	/**
	 * Effects awarded when the player presses attack during Character.State.Window.JustCombo.
	 * Each entry picks its own lifetime; see EJustComboEffectLifetime.
	 *
	 * NextAttackOnly entries must be Infinite or Has Duration — the consuming ability always
	 * removes them by handle on ability end.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "战斗|精准连击", meta = (DisplayName = "精准连击效果"))
	TArray<FJustComboEffectEntry> JustComboEffects;

	// Deprecated compatibility data. Current cards loop in sequence without shuffle downtime.
	UPROPERTY()
	float ShuffleCooldownDuration = 0.0f;

	// Deprecated compatibility data. Current attack sequence always uses all equipped attack cards.
	UPROPERTY()
	int32 MaxActiveSequenceSize = 0;

	// 勾选后武器仅作展示：玩家按 E 弹出 PreviewPopup 信息浮窗，不可实际拾取
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "预览模式", meta = (DisplayName = "仅用于预览"))
	bool bPreviewOnly = false;

	// bPreviewOnly=true 时显示的 LevelInfoPopup DA（填标题/正文/自动关闭时长）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "预览模式", meta = (DisplayName = "预览提示数据"))
	TObjectPtr<ULevelInfoPopupDA> PreviewPopup;

	UFUNCTION(BlueprintCallable)
	void SetupWeaponToCharacter(USkeletalMeshComponent* AttachTarget, APlayerCharacterBase* ReceivingChar);

	UFUNCTION(BlueprintPure, Category = "战斗|战技", meta = (DisplayName = "能否装备战技"))
	bool CanEquipWeaponSkill(const UWeaponSkillDataAsset* WeaponSkill) const;

	UFUNCTION(BlueprintPure, Category = "战斗|战技", meta = (DisplayName = "解析默认战技"))
	UWeaponSkillDataAsset* ResolveDefaultWeaponSkill() const;


private:
	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);

};
