#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
// Complete type, not a forward declare: consumers pass FWeaponSpawnData::WeaponLayer to
// LinkAnimClassLayers, which needs the UYogAnimInstance -> UAnimInstance derivation visible.
#include "Animation/YogAnimInstance.h"

#include "WeaponDefinitionBase.generated.h"

class AWeaponInstance;
class UNiagaraSystem;
class USoundBase;

USTRUCT(BlueprintType)
struct FWeaponSpawnData
{
	GENERATED_BODY()

	FWeaponSpawnData()
	{}

	UPROPERTY(EditAnywhere, Category = "装备生成", meta = (DisplayName = "武器 Actor 类"))
	TSubclassOf<AWeaponInstance> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = "装备生成", meta = (DisplayName = "挂接插槽"))
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = "装备生成", meta = (DisplayName = "挂接变换"))
	FTransform AttachTransform;

	UPROPERTY(EditAnywhere, Category = "装备生成", meta = (DisplayName = "武器动画层"))
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	// Optional: Save game data for persistence
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备生成", meta = (DisplayName = "写入存档"))
	bool bShouldSaveToGame = false;
};

/**
 * Shared base for player and enemy weapon definitions.
 *
 * Holds only what both sides genuinely need: the actors a weapon spawns on its wielder, and the
 * presentation identity used to resolve swing and hit feedback. Player-facing systems (skills,
 * combat deck, backpack, scene pickup) live on UWeaponDefinition; AI-facing systems (attack
 * profile, attribute modifiers, passive effects) live on UEnemyWeaponDefinition.
 *
 * Existing beyond a tidier hierarchy: AYogCharacterBase can name one weapon-definition type that
 * both a player and an enemy satisfy, so combat VFX/SFX resolution has a single code path instead
 * of branching on the concrete character class.
 *
 * Not marked Const on purpose — that specifier is inherited, and UEnemyWeaponDefinition is
 * mutable. UWeaponDefinition keeps Const on itself.
 */
UCLASS(Blueprintable, BlueprintType, DisplayName = "武器定义基类")
class DEVKIT_API UWeaponDefinitionBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = "装备", meta = (DisplayName = "装备后生成的武器 Actor"))
	TArray<FWeaponSpawnData> ActorsToSpawn;

	// Hit-impact intensity level (1+) this weapon produces on victims. Selects which Levels entry
	// is read from each matching UEnemyHitImpactData entry. Read by GA_MeleeAttack and passed to
	// the victim's UHitImpactVisualComponent.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit Feedback", meta = (DisplayName = "命中冲击等级", ClampMin = "1"))
	int32 HitImpactLevel = 1;

	// Air-swing sound identifying WHICH weapon is being swung. Fires on every swing regardless of
	// whether the swing connects — a miss is otherwise silent. Independent of the victim-side
	// contact sound in UEnemyHitImpactData, which resolves later and only when something is hit.
	// Author it 150-400ms with an audible air swell and its own decay tail baked in: playback is
	// fire-and-forget, so nothing fades it out if the montage is cancelled mid-arc.
	// Optional; leave empty for a silent swing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Whiff", meta = (DisplayName = "挥击音效"))
	TObjectPtr<USoundBase> WhiffSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Whiff", meta = (DisplayName = "挥击音量", ClampMin = "0.0"))
	float WhiffSoundVolume = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Whiff", meta = (DisplayName = "挥击音调", ClampMin = "0.0"))
	float WhiffSoundPitch = 1.f;

	// Trail/swing VFX for this weapon. Supplies the base layer when UMontageVFXBindingComponent
	// resolves a whiff slot and UWhiffVFXData has no row matching this weapon, so a weapon always
	// has a look even before the table is authored. Optional; leave empty for no swing VFX.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Whiff", meta = (DisplayName = "挥击特效"))
	TObjectPtr<UNiagaraSystem> WhiffVFX;

	// Per-weapon identity Tag, finer than UWeaponDefinition::WeaponType (which only distinguishes
	// Melee/Ranged). UMontageVFXBindingComponent folds this into the tag context it resolves
	// UWhiffVFXData against, so a table row can key on one specific weapon plus a buff element.
	// Unlike Weapon.Type.*, this is never granted to the ASC — it is read straight off the
	// definition at swing time, so there is no equip/unequip lifecycle to keep in sync.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Whiff",
		meta = (DisplayName = "武器身份标签", Categories = "Weapon.Id"))
	FGameplayTag WeaponIdentityTag;
};
