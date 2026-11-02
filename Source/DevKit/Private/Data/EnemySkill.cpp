// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/EnemySkill.h"

#include "AbilitySystem/Abilities/GA_EnemyScriptedSkill.h"
#include "Character/EnemyCharacterBase.h"
#include "UnLua.h"
#include "UnLuaModule.h"

// Every character of the module string becomes part of Lua's require path, so surrounding whitespace
// or a ".lua" suffix silently resolves to a file that does not exist.
static FString EnemySkillLua_NormalizeModule(const FString& LuaModule)
{
	FString Module = LuaModule.TrimStartAndEnd();
	Module.RemoveFromEnd(TEXT(".lua"));
	return Module;
}

void UEnemySkill_Lua::OnActivate(UGA_EnemyScriptedSkill* Skill) const
{
	CallLua(Skill, "OnActivate", [](lua_State*) { return 0; });
}

void UEnemySkill_Lua::OnEvent(UGA_EnemyScriptedSkill* Skill, FName EventName, AActor* Other) const
{
	CallLua(Skill, "OnEvent", [&EventName, Other](lua_State* L)
	{
		lua_pushstring(L, TCHAR_TO_UTF8(*EventName.ToString()));
		UnLua::PushUObject(L, Other);
		return 2;
	});
}

void UEnemySkill_Lua::OnEnd(UGA_EnemyScriptedSkill* Skill, bool bWasCancelled) const
{
	CallLua(Skill, "OnEnd", [bWasCancelled](lua_State* L)
	{
		lua_pushboolean(L, bWasCancelled);
		return 1;
	});
}

void UEnemySkill_Lua::CallLua(UGA_EnemyScriptedSkill* Skill, const char* FunctionName, TFunctionRef<int32(lua_State*)> PushExtraArgs) const
{
	const FString Module = EnemySkillLua_NormalizeModule(LuaModule);
	AEnemyCharacterBase* Owner = Skill ? Skill->GetSkillOwner() : nullptr;
	if (!IsValid(Owner) || Module.IsEmpty())
	{
		return;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemySkill][Lua] %s has no Lua env; '%s' skipped."), *GetNameSafe(Owner), *Module);
		return;
	}

	const int32 Top = lua_gettop(L);

	lua_pushcfunction(L, UnLua::ReportLuaCallError);
	const int32 MsgHandlerIdx = lua_gettop(L);

	lua_getglobal(L, "require");
	lua_pushstring(L, TCHAR_TO_UTF8(*Module));
	if (lua_pcall(L, 1, 1, MsgHandlerIdx) != LUA_OK || !lua_istable(L, -1))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemySkill][Lua] could not require '%s'."), *Module);
		lua_settop(L, Top);
		return;
	}

	if (lua_getfield(L, -1, FunctionName) != LUA_TFUNCTION)
	{
		lua_settop(L, Top);
		return;
	}

	UnLua::PushUObject(L, Skill);
	UnLua::PushUObject(L, Owner);
	UnLua::PushUObject(L, const_cast<UEnemySkill_Lua*>(this));
	const int32 NumArgs = 3 + PushExtraArgs(L);
	if (lua_pcall(L, NumArgs, 0, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemySkill][Lua] %s '%s'.%hs errored."),
			*GetNameSafe(Owner), *Module, FunctionName);
	}
	lua_settop(L, Top);
}
