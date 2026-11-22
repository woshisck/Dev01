#include "Component/DevourComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "Data/DevourOfferDataAsset.h"
#include "UnLua.h"
#include "UnLuaModule.h"

static constexpr int32 DevourComponent_DefaultMaxDevoursPerRoom = 2;
static constexpr int32 DevourComponent_DefaultMaxDevourStacks = 2;

// Every character of the module string becomes part of Lua's require path, so surrounding whitespace
// or a ".lua" suffix silently resolves to a file that does not exist.
static FString DevourComponent_NormalizeModule(const FString& LuaModule)
{
	FString Module = LuaModule.TrimStartAndEnd();
	Module.RemoveFromEnd(TEXT(".lua"));
	return Module;
}

static int32 DevourComponent_Duration(const UDevourOfferDataAsset* Offer)
{
	return Offer ? FMath::Max(1, Offer->DurationRooms) : 1;
}

UDevourComponent::UDevourComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UDevourComponent::GetOwnerASC() const
{
	const IAbilitySystemInterface* ASCOwner = Cast<IAbilitySystemInterface>(GetOwner());
	return ASCOwner ? ASCOwner->GetAbilitySystemComponent() : nullptr;
}

int32 UDevourComponent::ReadCapAttribute(const FGameplayAttribute& Attribute, int32 DefaultValue) const
{
	const UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !ASC->HasAttributeSetForAttribute(Attribute))
	{
		return DefaultValue;
	}

	return FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(Attribute)));
}

int32 UDevourComponent::GetMaxDevoursPerRoom() const
{
	return ReadCapAttribute(UPlayerAttributeSet::GetMaxDevoursPerRoomAttribute(), DevourComponent_DefaultMaxDevoursPerRoom);
}

int32 UDevourComponent::GetMaxDevourStacks() const
{
	return ReadCapAttribute(UPlayerAttributeSet::GetMaxDevourStacksAttribute(), DevourComponent_DefaultMaxDevourStacks);
}

int32 UDevourComponent::GetActiveStackCount() const
{
	int32 Count = 0;
	for (const FActiveDevour& Entry : Devours)
	{
		if (!Entry.bMatured)
		{
			++Count;
		}
	}
	return Count;
}

bool UDevourComponent::CanDevour() const
{
	return DevoursThisRoom < GetMaxDevoursPerRoom() && GetActiveStackCount() < GetMaxDevourStacks();
}

bool UDevourComponent::TryDevour(UDevourOfferDataAsset* Offer)
{
	if (!Offer || !CanDevour())
	{
		return false;
	}

	if (DevourComponent_NormalizeModule(Offer->LuaModule).IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Devour] %s has no LuaModule; the devour only counts rooms."), *GetNameSafe(Offer));
	}

	FActiveDevour& Entry = Devours.AddDefaulted_GetRef();
	Entry.Offer = Offer;
	Entry.Id = NextDevourId++;
	++DevoursThisRoom;

	UE_LOG(LogTemp, Log, TEXT("[Devour] Devoured %s #%d (stacks %d/%d, this room %d/%d)"),
		*GetNameSafe(Offer), Entry.Id, GetActiveStackCount(), GetMaxDevourStacks(), DevoursThisRoom, GetMaxDevoursPerRoom());

	// Copy: the Lua call may re-enter and grow Devours.
	const FActiveDevour Started = Entry;
	CallOfferLua(Started, "OnTempBegin", Started.RoomsEntered);

	OnDevourStacksChanged.Broadcast();
	return true;
}

void UDevourComponent::Mature(FActiveDevour& Entry, bool bEndTempOnLivePlayer)
{
	Entry.bMatured = true;
	const FActiveDevour Matured = Entry;

	if (bEndTempOnLivePlayer)
	{
		CallOfferLua(Matured, "OnTempEnd");
	}
	CallOfferLua(Matured, "OnPermanentBegin");

	UE_LOG(LogTemp, Log, TEXT("[Devour] %s #%d matured into its permanent phase"), *GetNameSafe(Matured.Offer), Matured.Id);
	OnDevourMatured.Broadcast(Matured.Offer);
}

void UDevourComponent::OnRoomEntered()
{
	DevoursThisRoom = 0;

	bool bChanged = false;
	for (int32 Index = 0; Index < Devours.Num(); ++Index)
	{
		if (Devours[Index].bMatured)
		{
			continue;
		}

		bChanged = true;
		FActiveDevour& Entry = Devours[Index];
		++Entry.RoomsEntered;
		if (Entry.RoomsEntered > DevourComponent_Duration(Entry.Offer))
		{
			Mature(Entry, true);
		}
		else
		{
			const FActiveDevour Aged = Entry;
			CallOfferLua(Aged, "OnRoomEntered", Aged.RoomsEntered);
		}
	}

	if (bChanged)
	{
		OnDevourStacksChanged.Broadcast();
	}
}

void UDevourComponent::CaptureRunState(TArray<FDevourRunEntry>& OutEntries) const
{
	OutEntries.Reset(Devours.Num());
	for (const FActiveDevour& Entry : Devours)
	{
		if (!Entry.Offer)
		{
			continue;
		}

		FDevourRunEntry& Out = OutEntries.AddDefaulted_GetRef();
		Out.Offer = Entry.Offer;
		Out.Id = Entry.Id;
		Out.RoomsEntered = Entry.RoomsEntered;
		Out.bMatured = Entry.bMatured;
	}
}

void UDevourComponent::RestoreFromRunState(const TArray<FDevourRunEntry>& Entries)
{
	Devours.Reset(Entries.Num());
	DevoursThisRoom = 0;
	NextDevourId = 1;

	for (const FDevourRunEntry& In : Entries)
	{
		if (!In.Offer)
		{
			continue;
		}

		FActiveDevour& Entry = Devours.AddDefaulted_GetRef();
		Entry.Offer = In.Offer;
		Entry.Id = In.Id;
		Entry.RoomsEntered = In.RoomsEntered;
		Entry.bMatured = In.bMatured;
		NextDevourId = FMath::Max(NextDevourId, In.Id + 1);
	}

	// The temp phase never started on this new player, so a devour maturing here has no OnTempEnd.
	for (int32 Index = 0; Index < Devours.Num(); ++Index)
	{
		if (Devours[Index].bMatured)
		{
			const FActiveDevour Permanent = Devours[Index];
			CallOfferLua(Permanent, "OnPermanentBegin");
			continue;
		}

		FActiveDevour& Entry = Devours[Index];
		++Entry.RoomsEntered;
		if (Entry.RoomsEntered > DevourComponent_Duration(Entry.Offer))
		{
			Mature(Entry, false);
		}
		else
		{
			const FActiveDevour Temp = Entry;
			CallOfferLua(Temp, "OnTempBegin", Temp.RoomsEntered);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Devour] Restored %d devours (%d temp)"), Devours.Num(), GetActiveStackCount());
	OnDevourStacksChanged.Broadcast();
}

void UDevourComponent::ResetForNewRun()
{
	const TArray<FActiveDevour> Removed = MoveTemp(Devours);
	Devours.Reset();
	DevoursThisRoom = 0;
	NextDevourId = 1;

	for (const FActiveDevour& Entry : Removed)
	{
		CallOfferLua(Entry, "OnRemoved");
	}

	if (!Removed.IsEmpty())
	{
		OnDevourStacksChanged.Broadcast();
	}
}

void UDevourComponent::CallOfferLua(const FActiveDevour& Entry, const char* FunctionName, int32 RoomsEntered) const
{
	AActor* Owner = GetOwner();
	const FString Module = Entry.Offer ? DevourComponent_NormalizeModule(Entry.Offer->LuaModule) : FString();
	if (!IsValid(Owner) || Module.IsEmpty())
	{
		return;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Devour][Lua] %s has no Lua env; '%s' skipped."), *GetNameSafe(Owner), *Module);
		return;
	}

	const int32 Top = lua_gettop(L);

	lua_pushcfunction(L, UnLua::ReportLuaCallError);
	const int32 MsgHandlerIdx = lua_gettop(L);

	lua_getglobal(L, "require");
	lua_pushstring(L, TCHAR_TO_UTF8(*Module));
	if (lua_pcall(L, 1, 1, MsgHandlerIdx) != LUA_OK || !lua_istable(L, -1))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Devour][Lua] could not require '%s'."), *Module);
		lua_settop(L, Top);
		return;
	}

	// Every hook is optional, so a module that only needs OnTempBegin is valid.
	if (lua_getfield(L, -1, FunctionName) != LUA_TFUNCTION)
	{
		lua_settop(L, Top);
		return;
	}

	UnLua::PushUObject(L, Owner);
	UnLua::PushUObject(L, Entry.Offer.Get());
	lua_pushinteger(L, Entry.Id);
	int32 NumArgs = 3;
	if (RoomsEntered >= 0)
	{
		lua_pushinteger(L, RoomsEntered);
		++NumArgs;
	}

	if (lua_pcall(L, NumArgs, 0, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Devour][Lua] %s '%s'.%hs errored."), *GetNameSafe(Owner), *Module, FunctionName);
	}
	lua_settop(L, Top);
}
