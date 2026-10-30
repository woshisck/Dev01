// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyTraitLuaLibrary.generated.h"

class AEnemyCharacterBase;
class UGameplayEffect;

/**
 * The engine-side calls a Lua-scripted enemy trait needs and cannot reach otherwise: the enemy registry,
 * loose tags, StateTree events and gameplay effects. Tags are taken as FName so Lua can pass plain strings.
 */
UCLASS()
class DEVKIT_API UEnemyTraitLuaLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Every living registered enemy in the world of WorldContext. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Trait|Lua", meta = (WorldContext = "WorldContext"))
	static TArray<AEnemyCharacterBase*> GetAliveEnemies(const UObject* WorldContext);

	/** Adds or removes a loose gameplay tag on the enemy's ASC. Idempotent, so a repeated call never stacks. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Trait|Lua")
	static void SetEnemyLooseTag(AEnemyCharacterBase* Enemy, FName TagName, bool bPresent);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Trait|Lua")
	static bool HasEnemyTag(const AEnemyCharacterBase* Enemy, FName TagName);

	/** True while the enemy's AI is in the Combat state. */
	UFUNCTION(BlueprintPure, Category = "Enemy|Trait|Lua")
	static bool IsEnemyInCombat(const AEnemyCharacterBase* Enemy);

	/** Puts the enemy into the Combat AI state against the player. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Trait|Lua")
	static bool ForceEnemyCombat(AEnemyCharacterBase* Enemy);

	/** Sends a gameplay-tag event to the enemy's running StateTree. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Trait|Lua")
	static void SendEnemyStateTreeEvent(AEnemyCharacterBase* Enemy, FName EventTagName);

	/**
	 * Applies the effect to the enemy at the given level and returns its handle; an invalid handle means
	 * nothing was applied. Level is how a stack count reaches the effect, via scalable-float curves.
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Trait|Lua")
	static FActiveGameplayEffectHandle ApplyEnemyEffect(AEnemyCharacterBase* Enemy, TSubclassOf<UGameplayEffect> Effect, float Level);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Trait|Lua")
	static void RemoveEnemyEffect(AEnemyCharacterBase* Enemy, FActiveGameplayEffectHandle Handle);

	/** True when both enemies share EnemyId, falling back to the same EnemyData asset when EnemyId is None. */
	UFUNCTION(BlueprintPure, Category = "Enemy|Trait|Lua")
	static bool IsSameEnemyType(const AEnemyCharacterBase* A, const AEnemyCharacterBase* B);
};
