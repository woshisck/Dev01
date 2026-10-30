// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/EnemyTrait_Lua.h"

#include "Character/EnemyCharacterBase.h"
#include "Engine/World.h"
#include "GameModes/YogGameMode.h"
#include "TimerManager.h"
#include "UnLua.h"
#include "UnLuaModule.h"

// Every character of the module string becomes part of Lua's require path, so surrounding whitespace
// or a ".lua" suffix silently resolves to a file that does not exist.
static FString EnemyTraitLua_NormalizeModule(const FString& LuaModule)
{
	FString Module = LuaModule.TrimStartAndEnd();
	Module.RemoveFromEnd(TEXT(".lua"));
	return Module;
}

void UEnemyTrait_Lua::ActivateTrait(AEnemyCharacterBase* Owner) const
{
	if (!IsValid(Owner) || !Owner->GetWorld())
	{
		return;
	}

	AYogGameMode* GM = Cast<AYogGameMode>(Owner->GetWorld()->GetAuthGameMode());
	if (!GM)
	{
		return;
	}

	FEnemyTraitRuntime& Runtime = Owner->FindOrAddTraitRuntime(this);
	if (Runtime.EnemyRegisteredHandle.IsValid())
	{
		return;
	}

	const TWeakObjectPtr<const UEnemyTrait_Lua> WeakThis(this);
	const TWeakObjectPtr<AEnemyCharacterBase> WeakOwner(Owner);
	auto OnRegistryChanged = [WeakThis, WeakOwner](AEnemyCharacterBase* /*Changed*/)
	{
		if (WeakThis.IsValid() && WeakOwner.IsValid())
		{
			WeakThis->ScheduleRosterChanged(WeakOwner.Get());
		}
	};

	Runtime.EnemyRegisteredHandle = GM->OnEnemyRegisteredNative.AddWeakLambda(Owner, OnRegistryChanged);
	Runtime.EnemyUnregisteredHandle = GM->OnEnemyUnregisteredNative.AddWeakLambda(Owner, OnRegistryChanged);

	CallLua(Owner, "OnActivate");
	ScheduleRosterChanged(Owner);

	Super::ActivateTrait(Owner);
}

void UEnemyTrait_Lua::DeactivateTrait(AEnemyCharacterBase* Owner) const
{
	if (!IsValid(Owner))
	{
		return;
	}

	FEnemyTraitRuntime* Runtime = Owner->FindTraitRuntime(this);
	if (!Runtime)
	{
		return;
	}

	if (UWorld* World = Owner->GetWorld())
	{
		if (AYogGameMode* GM = Cast<AYogGameMode>(World->GetAuthGameMode()))
		{
			GM->OnEnemyRegisteredNative.Remove(Runtime->EnemyRegisteredHandle);
			GM->OnEnemyUnregisteredNative.Remove(Runtime->EnemyUnregisteredHandle);
		}
	}

	// Drop the runtime slot first so a roster call already queued for next tick becomes a no-op.
	Owner->RemoveTraitRuntime(this);
	CallLua(Owner, "OnDeactivate");

	Super::DeactivateTrait(Owner);
}

void UEnemyTrait_Lua::OnOwnerEnteredCombat(AEnemyCharacterBase* Owner) const
{
	// A trait that was deactivated (dead or leaving the world) has no runtime slot and must stay silent.
	if (!IsValid(Owner) || !Owner->FindTraitRuntime(this))
	{
		return;
	}

	CallLua(Owner, "OnCombatEntered");
}

void UEnemyTrait_Lua::ScheduleRosterChanged(AEnemyCharacterBase* Owner) const
{
	FEnemyTraitRuntime* Runtime = Owner ? Owner->FindTraitRuntime(this) : nullptr;
	if (!Runtime || Runtime->bRecountPending)
	{
		return;
	}
	Runtime->bRecountPending = true;

	// Deferred so the spawning enemy has finished BeginPlay and every AI controller has started its
	// StateTree; a StateTree event sent this frame would be dropped.
	const TWeakObjectPtr<const UEnemyTrait_Lua> WeakThis(this);
	const TWeakObjectPtr<AEnemyCharacterBase> WeakOwner(Owner);
	Owner->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Owner,
		[WeakThis, WeakOwner]()
		{
			if (!WeakThis.IsValid() || !WeakOwner.IsValid())
			{
				return;
			}

			FEnemyTraitRuntime* Pending = WeakOwner->FindTraitRuntime(WeakThis.Get());
			if (!Pending)
			{
				return;
			}

			Pending->bRecountPending = false;
			WeakThis->CallLua(WeakOwner.Get(), "OnRosterChanged");
		}));
}

void UEnemyTrait_Lua::CallLua(AEnemyCharacterBase* Owner, const char* FunctionName) const
{
	const FString Module = EnemyTraitLua_NormalizeModule(LuaModule);
	if (!IsValid(Owner) || Module.IsEmpty())
	{
		return;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyTrait][Lua] %s has no Lua env; '%s' skipped."), *GetNameSafe(Owner), *Module);
		return;
	}

	const int32 Top = lua_gettop(L);

	lua_pushcfunction(L, UnLua::ReportLuaCallError);
	const int32 MsgHandlerIdx = lua_gettop(L);

	lua_getglobal(L, "require");
	lua_pushstring(L, TCHAR_TO_UTF8(*Module));
	if (lua_pcall(L, 1, 1, MsgHandlerIdx) != LUA_OK || !lua_istable(L, -1))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyTrait][Lua] could not require '%s'."), *Module);
		lua_settop(L, Top);
		return;
	}

	// Every function is optional, so a module that only needs OnRosterChanged is valid.
	if (lua_getfield(L, -1, FunctionName) != LUA_TFUNCTION)
	{
		lua_settop(L, Top);
		return;
	}

	UnLua::PushUObject(L, Owner);
	UnLua::PushUObject(L, const_cast<UEnemyTrait_Lua*>(this));
	if (lua_pcall(L, 2, 0, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyTrait][Lua] %s '%s'.%hs errored."),
			*GetNameSafe(Owner), *Module, FunctionName);
	}
	lua_settop(L, Top);
}
