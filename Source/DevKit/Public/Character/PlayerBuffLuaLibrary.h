#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlayerBuffLuaLibrary.generated.h"

class APlayerCharacterBase;
class UGameplayEffect;
class UYogAbilitySystemComponent;

/**
 * The engine-side calls a Lua-scripted player buff (devour offers) needs: gameplay effects, loose tags,
 * attributes and the combat delegates. Tags and attributes are taken as FName so Lua can pass plain strings.
 * Attribute names are the bare property names of any attribute set on the player ("Health", "Attack", ...).
 */
UCLASS()
class DEVKIT_API UPlayerBuffLuaLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The player's ASC, for binding OnKilledTarget / DealtDamage / OnDashExecuted / ReceivedDamage in Lua. */
	UFUNCTION(BlueprintPure, Category = "Player|Buff|Lua")
	static UYogAbilitySystemComponent* GetPlayerASC(APlayerCharacterBase* Player);

	/** Applies Effect to the player at Level. Returns an invalid handle when it could not be applied. */
	UFUNCTION(BlueprintCallable, Category = "Player|Buff|Lua")
	static FActiveGameplayEffectHandle ApplyPlayerEffect(APlayerCharacterBase* Player, TSubclassOf<UGameplayEffect> Effect, float Level);

	UFUNCTION(BlueprintCallable, Category = "Player|Buff|Lua")
	static void RemovePlayerEffect(APlayerCharacterBase* Player, FActiveGameplayEffectHandle Handle);

	/** Loose tags are counted, so every add needs a matching remove. */
	UFUNCTION(BlueprintCallable, Category = "Player|Buff|Lua")
	static void AddPlayerLooseTag(APlayerCharacterBase* Player, FName TagName);

	UFUNCTION(BlueprintCallable, Category = "Player|Buff|Lua")
	static void RemovePlayerLooseTag(APlayerCharacterBase* Player, FName TagName);

	UFUNCTION(BlueprintPure, Category = "Player|Buff|Lua")
	static bool HasPlayerTag(APlayerCharacterBase* Player, FName TagName);

	/** Current value of the named attribute, 0 when the player has no such attribute. */
	UFUNCTION(BlueprintPure, Category = "Player|Buff|Lua")
	static float GetPlayerAttribute(APlayerCharacterBase* Player, FName AttributeName);

	/** Adds Delta to the attribute's base value. Use an Instant GE instead when other effects must react. */
	UFUNCTION(BlueprintCallable, Category = "Player|Buff|Lua")
	static void AddPlayerAttributeBase(APlayerCharacterBase* Player, FName AttributeName, float Delta);

	/** Restores Amount health, clamped to MaxHealth. */
	UFUNCTION(BlueprintCallable, Category = "Player|Buff|Lua")
	static void HealPlayer(APlayerCharacterBase* Player, float Amount);
};
