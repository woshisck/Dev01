--
-- Default devour buff. Used through a UDevourOfferDataAsset whose LuaModule is "Devour.Default".
--
-- Temp phase (the devour room plus the next DurationRooms rooms): TempEffect is on and every kill heals
-- TempHealOnKill. Permanent phase (rest of the run): PermanentEffect is on and every kill heals
-- PermanentHealOnKill. Leave any of them empty / 0 to skip that part.
--
-- Tuning on the offer asset:
--   NumberParams
--     TempHealOnKill        health restored per kill during the temp phase      (default 0)
--     PermanentHealOnKill   health restored per kill during the permanent phase (default 0)
--     TempEffectLevel       level TempEffect is applied at                      (default 1)
--     PermanentEffectLevel  level PermanentEffect is applied at                 (default 1)
--   EffectParams
--     TempEffect            Infinite GameplayEffect for the temp phase
--     PermanentEffect       Infinite GameplayEffect for the permanent phase
--
-- C++ calls the hooks (see UDevourOfferDataAsset). The player is respawned every room, so OnTempBegin and
-- OnPermanentBegin run again on each new player; the old player's state is dropped with it (weak keys).
-- The module table is shared, so all state lives in States[Player][Id].
--

local M = {}

local Lib = UE.UPlayerBuffLuaLibrary

-- [Player] = { [Id] = { EffectHandle = FActiveGameplayEffectHandle|nil, OnKill = function|nil } }
local States = setmetatable({}, { __mode = "k" })

local function Param(Offer, Name, Default)
    local Value = Offer.NumberParams:Find(Name)
    if Value == nil then
        return Default
    end
    return Value
end

local function GetState(Player, Id)
    local PerPlayer = States[Player]
    if not PerPlayer then
        PerPlayer = {}
        States[Player] = PerPlayer
    end

    local State = PerPlayer[Id]
    if not State then
        State = {}
        PerPlayer[Id] = State
    end
    return State
end

-- Removes whatever the current phase put on the player, so the next phase starts clean.
local function ClearPhase(Player, Id)
    local PerPlayer = States[Player]
    local State = PerPlayer and PerPlayer[Id]
    if not State then
        return
    end

    if State.EffectHandle then
        Lib.RemovePlayerEffect(Player, State.EffectHandle)
    end

    if State.OnKill then
        local ASC = Lib.GetPlayerASC(Player)
        if ASC then
            ASC.OnKilledTarget:Remove(Player, State.OnKill)
        end
    end

    PerPlayer[Id] = nil
end

local function BeginPhase(Player, Offer, Id, EffectName, LevelName, HealName)
    ClearPhase(Player, Id)
    local State = GetState(Player, Id)

    local Effect = Offer.EffectParams:Find(EffectName)
    if Effect then
        State.EffectHandle = Lib.ApplyPlayerEffect(Player, Effect, Param(Offer, LevelName, 1))
    end

    local Heal = Param(Offer, HealName, 0)
    if Heal > 0 then
        local ASC = Lib.GetPlayerASC(Player)
        if ASC then
            -- UnLua calls a delegate callback with the bound object first, then the delegate params.
            State.OnKill = function(BoundPlayer, Target, DeathLocation)
                Lib.HealPlayer(BoundPlayer, Heal)
            end
            ASC.OnKilledTarget:Add(Player, State.OnKill)
        end
    end
end

function M.OnTempBegin(Player, Offer, Id, RoomsEntered)
    BeginPhase(Player, Offer, Id, "TempEffect", "TempEffectLevel", "TempHealOnKill")
end

function M.OnTempEnd(Player, Offer, Id)
    ClearPhase(Player, Id)
end

function M.OnPermanentBegin(Player, Offer, Id)
    BeginPhase(Player, Offer, Id, "PermanentEffect", "PermanentEffectLevel", "PermanentHealOnKill")
end

function M.OnRemoved(Player, Offer, Id)
    ClearPhase(Player, Id)
end

return M
