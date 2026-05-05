
#pragma once

#include "CoreMinimal.h"

#include "AbilitySystem/Abilities/YogAbilitySet.h"
#include "Data/AbilityData.h"
#include "Animation/YogAnimInstance.h"
#include "Component/BackpackGridComponent.h"
#include "GameplayTagContainer.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Item/Weapon/WeaponTypes.h"

#include "WeaponDefinition.generated.h"

class ULevelInfoPopupDA;

class UYogAbilitySet;
class AWeaponInstance;
class APlayerCharacterBase;
class UMaterialInterface;
class URuneDataAsset;
class UWeaponComboConfigDA;
class UGameplayAbilityComboGraph;
//class UYogAnimInstance;



USTRUCT(BlueprintType)
struct FBackpackConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "背包")
    int32 GridWidth = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "背包")
    int32 GridHeight = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "背包")
    FActivationZoneConfig ActivationZoneConfig;
};

USTRUCT(BlueprintType)
struct FWeaponSpawnData
{
	GENERATED_BODY()

	FWeaponSpawnData()
	{}

	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<AWeaponInstance> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FTransform AttachTransform;

	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	UPROPERTY(SaveGame)
	TObjectPtr<UAbilityData> WeaponAbilities;

	// Optional: Save game data for persistence
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldSaveToGame = false;
};

UCLASS(Blueprintable, BlueprintType, Const)
class UWeaponDefinition : public UPrimaryDataAsset
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UAbilityData> WeaponAbilityData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Combo")
	TObjectPtr<UWeaponComboConfigDA> WeaponComboConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Combo")
	TObjectPtr<UGameplayAbilityComboGraph> GameplayAbilityComboGraph;

	// 武器类型：决定装备时挂在 ASC 上的 Weapon.Type.* LooseTag。
	// 玩家专属攻击 GA 通过 ActivationRequiredTags 持有该 Tag → 自动隔离近战/远程激活路径。
	// 默认 Melee 保持向后兼容（旧武器 DA 不需要重新配）。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EWeaponType WeaponType = EWeaponType::Melee;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<FWeaponSpawnData> ActorsToSpawn;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FRotator WeaponRotation;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Anime")
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	// 热度阶段 Overlay 材质（带 Fresnel + EmissiveColor 参数）
	// 武器被拾取时由 WeaponSpawner 自动传给 WeaponInstance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat")
	TObjectPtr<UMaterialInterface> HeatOverlayMaterial;

	// 背包配置（格子尺寸 + 各热度阶段激活区）；装备时自动注入到 BackpackGridComponent
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "背包配置")
	FBackpackConfig BackpackConfig;

	// 武器展示信息（名称/描述/缩略图/激活区图像），驱动武器浮窗
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "武器信息")
	TObjectPtr<UWeaponInfoDA> WeaponInfo;

	// 初始符文列表：拾取武器时在浮窗展示，并预置到激活区起始格
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "武器信息")
	TArray<TObjectPtr<URuneDataAsset>> InitialRunes;

	// 512 战斗卡组：优先使用此列表初始化 1D 攻击卡组；为空时从 InitialRunes 中筛选 CombatCard 配置
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Deck")
	TArray<TObjectPtr<URuneDataAsset>> InitialCombatDeck;

	// 卡组打空后的装填时间。V1 默认 1 秒，武器可覆盖。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Deck", meta = (ClampMin = "0.0"))
	float ShuffleCooldownDuration = 1.0f;

	// 第一版默认展示完整卡组；大于 0 时限制 ActiveSequence 长度。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Deck", meta = (ClampMin = "0"))
	int32 MaxActiveSequenceSize = 0;

	// 勾选后武器仅作展示：玩家按 E 弹出 PreviewPopup 信息浮窗，不可实际拾取
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "预览模式")
	bool bPreviewOnly = false;

	// bPreviewOnly=true 时显示的 LevelInfoPopup DA（填标题/正文/自动关闭时长）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "预览模式")
	TObjectPtr<ULevelInfoPopupDA> PreviewPopup;

	UFUNCTION(BlueprintCallable)
	void SetupWeaponToCharacter(USkeletalMeshComponent* AttachTarget, APlayerCharacterBase* ReceivingChar);


private:
	void ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data);

};
