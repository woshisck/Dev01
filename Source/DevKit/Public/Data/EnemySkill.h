// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemySkill.generated.h"

class AActor;
class UAnimMontage;
class UGA_EnemyScriptedSkill;
class UGameplayEffect;
struct lua_State;

/**
 * An enemy skill an enemy gets by listing the asset in UEnemyData::Skills under an Enemy.Skill.SkillN slot.
 * Assets are stateless config shared by every enemy that lists them: per-activation state belongs on the
 * UGA_EnemyScriptedSkill instance, which is passed to every callback.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DEVKIT_API UEnemySkill : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Played by Skill->PlaySkillMontage. Optional for skills that only move or apply effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UAnimMontage> Montage;

	virtual void OnActivate(UGA_EnemyScriptedSkill* Skill) const {}
	virtual void OnEvent(UGA_EnemyScriptedSkill* Skill, FName EventName, AActor* Other) const {}
	virtual void OnEnd(UGA_EnemyScriptedSkill* Skill, bool bWasCancelled) const {}
};

/**
 * A skill whose logic is a Lua module, so designers can script one without touching C++.
 *
 * The module is required fresh for every call and must return a table. All functions are optional:
 *
 *   OnActivate(Skill, Owner, Asset)
 *   OnEvent(Skill, Owner, Asset, EventName, Other)   see UGA_EnemyScriptedSkill for the built-in event names
 *   OnEnd(Skill, Owner, Asset, bWasCancelled)
 *
 * Skill is the running UGA_EnemyScriptedSkill (call its PlaySkillMontage / StartChargeMove / EndSkill ...),
 * Owner the enemy, Asset this data asset (read tuning from Asset.NumberParams / Asset.EffectParams).
 * The module table is shared by every enemy, so per-activation state must be keyed by Skill.
 *
 * Every call is pcall-guarded. A Lua error is logged and skipped, but a skill that errors before it
 * calls EndSkill stays active until its montage ends, so keep MontageEnded handled.
 */
UCLASS()
class DEVKIT_API UEnemySkill_Lua : public UEnemySkill
{
	GENERATED_BODY()

public:
	/** Module path relative to Content/Script, dot separated. e.g. "Skills.Charge". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	FString LuaModule;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FName, float> NumberParams;

	/** GameplayEffects the script can look up by name, so effect assets are chosen in the editor, not by path in Lua. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lua")
	TMap<FName, TSubclassOf<UGameplayEffect>> EffectParams;

	virtual void OnActivate(UGA_EnemyScriptedSkill* Skill) const override;
	virtual void OnEvent(UGA_EnemyScriptedSkill* Skill, FName EventName, AActor* Other) const override;
	virtual void OnEnd(UGA_EnemyScriptedSkill* Skill, bool bWasCancelled) const override;

private:
	/** Pushes Skill, Owner, Asset, then whatever PushExtraArgs pushes, and calls FunctionName. */
	void CallLua(UGA_EnemyScriptedSkill* Skill, const char* FunctionName, TFunctionRef<int32(lua_State*)> PushExtraArgs) const;
};
