
#pragma once

#include "CoreMinimal.h"

#include "AbilitySystem/Abilities/YogAbilitySet.h"
#include "Data/AbilityData.h"
#include "Animation/YogAnimInstance.h"
#include "Component/BackpackGridComponent.h"
#include "Item/Weapon/WeaponInfoDA.h"

#include "WeaponDefinition.generated.h"

class ULevelInfoPopupDA;

class UYogAbilitySet;
class AWeaponInstance;
class APlayerCharacterBase;
class UMaterialInterface;
class URuneDataAsset;
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
