// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/EnemyTrait.h"
#include "EnemyTrait_Lua.generated.h"

class UGameplayEffect;

/**
 * A trait whose logic is a Lua module, so designers can script one without touching C++.
 *
 * The module is required fresh for every call and must return a table. All functions are optional and
 * receive the owning enemy and this trait asset (read tuning from Trait.NumberParams / Trait.EffectParams;
 * the names are whatever the script documents, nothing here fixes them):
 *
 *   OnActivate(Owner, Trait)       once, when the enemy begins play
 *   OnRosterChanged(Owner, Trait)  one tick after any enemy registers or unregisters, coalesced; also once
 *                                  one tick after activation
 *   OnCombatEntered(Owner, Trait)  when the enemy's AI moves into Combat from another state; fires once per
 *                                  entry, not while it stays in Combat
 *   OnDeactivate(Owner, Trait)     when the enemy dies or leaves the world
 *
 * The module table is shared by every enemy using this trait, so per-enemy state must be keyed by Owner
 * (for example in a weak-keyed table), never stored on the module itself.
 *
 * Every call is pcall-guarded: a Lua error is logged and skipped rather than breaking the enemy.
 */
UCLASS()
class DEVKIT_API UEnemyTrait_Lua : public UEnemyTrait
{
	GENERATED_BODY()

public:
	/** Module path relative to Content/Script, dot separated. e.g. "Traits.SwarmFrenzy". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString LuaModule;

	/** Designer tuning handed to the script through the Trait argument. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FName, float> NumberParams;

	/** GameplayEffects the script can look up by name, so effect assets are chosen in the editor, not by path in Lua. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FName, TSubclassOf<UGameplayEffect>> EffectParams;

	virtual void ActivateTrait(AEnemyCharacterBase* Owner) const override;
	virtual void DeactivateTrait(AEnemyCharacterBase* Owner) const override;
	virtual void OnOwnerEnteredCombat(AEnemyCharacterBase* Owner) const override;

private:
	void ScheduleRosterChanged(AEnemyCharacterBase* Owner) const;
	void CallLua(AEnemyCharacterBase* Owner, const char* FunctionName) const;
};
