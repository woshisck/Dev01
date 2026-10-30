--
-- Swarm Frenzy as a Lua trait. Used through a UEnemyTrait_Lua asset listed on UEnemyData.Traits.
--
-- While enough enemies of the owner's type are alive the owner is frenzied and carries one or more
-- stacks. A frenzied owner gets the Enemy.State.Frenzy tag.
--
-- Frenzied enemies fight as one: as soon as any of them is in Combat, every other frenzied enemy of the same
-- type is pulled into Combat too (ForceEnemyCombat plus the Enemy.Event.Frenzy StateTree event, which ST_*
-- can turn into a jump to Combat > Attack). A frenzied enemy that is already in Combat never de-aggros
-- (AYogAIController keeps it there while the tag is present); one that is not in Combat follows normal
-- awareness until a mate engages. An enemy that becomes frenzied while a frenzied mate is already fighting
-- joins the fight at once.
--
-- Stacks = 1 once Count >= MinCount, plus 1 for every further CountPerStack enemies, capped at MaxStacks:
--   MinCount 5, CountPerStack 5  ->  5 enemies = 1 stack, 10 = 2, 15 = 3, ...
--
-- Tuning on the trait asset:
--   NumberParams
--     MinCount        enemies needed for the first stack            (default 5)
--     CountPerStack   extra enemies per additional stack; 0 = the trait never goes past 1 stack (default 0)
--     MaxStacks       stack cap; 0 = uncapped                       (default 0)
--     CountSelf       0 leaves the owner out of the count           (default 1)
--     ReattackPerStack  1 = every gained stack after the first also forces Combat and re-sends the event (default 0)
--     ForceCombatOnFrenzy  1 = becoming frenzied forces Combat at once, without waiting for a mate to engage (default 0)
--   EffectParams
--     StackEffect     optional Infinite, Stack Limit 1 GameplayEffect. It is applied once, at Level =
--                     stack count, so scale its magnitudes with a curve table (or scalable float).
--
-- This module table is shared by every enemy that uses the trait, so per-enemy state lives in States,
-- keyed by the owner.
--

local M = {}

local FrenzyTag = "Enemy.State.Frenzy"
local FrenzyEvent = "Enemy.Event.Frenzy"

-- [Owner] = { Stacks = integer, EffectHandle = FActiveGameplayEffectHandle|nil }
local States = setmetatable({}, { __mode = "k" })

local function Param(Trait, Name, Default)
    local Value = Trait.NumberParams:Find(Name)
    if Value == nil then
        return Default
    end
    return Value
end

local function CountMatching(Owner, Trait)
    local bCountSelf = Param(Trait, "CountSelf", 1)
    local Alive = UE.UEnemyTraitLuaLibrary.GetAliveEnemies(Owner)
    local Count = 0
    for i = 1, Alive:Length() do
        local Other = Alive:Get(i)
        if (Other ~= Owner or bCountSelf ~= 0) and UE.UEnemyTraitLuaLibrary.IsSameEnemyType(Owner, Other) then
            Count = Count + 1
        end
    end
    return Count
end

local function ComputeStacks(Count, Trait)
    local MinCount = Param(Trait, "MinCount", 5)
    if Count < MinCount then
        return 0
    end

    local CountPerStack = Param(Trait, "CountPerStack", 0)
    local Stacks = 1
    if CountPerStack > 0 then
        Stacks = 1 + math.floor((Count - MinCount) / CountPerStack)
    end

    local MaxStacks = Param(Trait, "MaxStacks", 0)
    if MaxStacks > 0 and Stacks > MaxStacks then
        Stacks = MaxStacks
    end
    return Stacks
end

-- True for the enemies a frenzy engagement spreads between: same type, frenzied, alive, and not the owner.
local function FrenziedMates(Owner)
    local Mates = {}
    local Alive = UE.UEnemyTraitLuaLibrary.GetAliveEnemies(Owner)
    for i = 1, Alive:Length() do
        local Other = Alive:Get(i)
        if Other ~= Owner
            and UE.UEnemyTraitLuaLibrary.IsSameEnemyType(Owner, Other)
            and UE.UEnemyTraitLuaLibrary.HasEnemyTag(Other, FrenzyTag) then
            Mates[#Mates + 1] = Other
        end
    end
    return Mates
end

-- Combat first: the jump to Attack re-tests Combat's enter condition (Enemy AI State Is = Combat).
local function Engage(Enemy)
    if UE.UEnemyTraitLuaLibrary.ForceEnemyCombat(Enemy) then
        UE.UEnemyTraitLuaLibrary.SendEnemyStateTreeEvent(Enemy, FrenzyEvent)
    end
end

-- Engaging a mate re-enters this through OnCombatEntered, but only enemies that were not yet in Combat
-- fire it, so the spread ends once everyone is fighting.
local function PullMatesIntoCombat(Owner)
    for _, Mate in ipairs(FrenziedMates(Owner)) do
        if not UE.UEnemyTraitLuaLibrary.IsEnemyInCombat(Mate) then
            Engage(Mate)
        end
    end
end

local function AnyMateInCombat(Owner)
    for _, Mate in ipairs(FrenziedMates(Owner)) do
        if UE.UEnemyTraitLuaLibrary.IsEnemyInCombat(Mate) then
            return true
        end
    end
    return false
end

local function ClearEffect(Owner, State)
    if State.EffectHandle then
        UE.UEnemyTraitLuaLibrary.RemoveEnemyEffect(Owner, State.EffectHandle)
        State.EffectHandle = nil
    end
end

local function SetStacks(Owner, State, Trait, NewStacks)
    local OldStacks = State.Stacks
    if NewStacks == OldStacks then
        return
    end

    ClearEffect(Owner, State)
    State.Stacks = NewStacks

    if NewStacks == 0 then
        UE.UEnemyTraitLuaLibrary.SetEnemyLooseTag(Owner, FrenzyTag, false)
        return
    end

    UE.UEnemyTraitLuaLibrary.SetEnemyLooseTag(Owner, FrenzyTag, true)

    local Effect = Trait.EffectParams:Find("StackEffect")
    if Effect then
        State.EffectHandle = UE.UEnemyTraitLuaLibrary.ApplyEnemyEffect(Owner, Effect, NewStacks)
    end

    if OldStacks == 0 then
        if Param(Trait, "ForceCombatOnFrenzy", 0) ~= 0 then
            Engage(Owner)
        elseif UE.UEnemyTraitLuaLibrary.IsEnemyInCombat(Owner) then
            -- Frenzy can land after this enemy was already pulled into a fight, so its mates missed the spread.
            PullMatesIntoCombat(Owner)
        elseif AnyMateInCombat(Owner) then
            Engage(Owner)
        end
    elseif NewStacks > OldStacks and Param(Trait, "ReattackPerStack", 0) ~= 0 then
        -- Losing a stack never re-engages; only a gained one can.
        Engage(Owner)
    end
end

function M.OnActivate(Owner, Trait)
    States[Owner] = { Stacks = 0 }
end

--- Raised by C++ when this enemy's AI moves into Combat from another state.
function M.OnCombatEntered(Owner, Trait)
    local State = States[Owner]
    if State and State.Stacks > 0 then
        PullMatesIntoCombat(Owner)
    end
end

function M.OnRosterChanged(Owner, Trait)
    local State = States[Owner]
    if not State then
        return
    end

    SetStacks(Owner, State, Trait, ComputeStacks(CountMatching(Owner, Trait), Trait))
end

function M.OnDeactivate(Owner, Trait)
    local State = States[Owner]
    if not State then
        return
    end

    ClearEffect(Owner, State)
    if State.Stacks > 0 then
        UE.UEnemyTraitLuaLibrary.SetEnemyLooseTag(Owner, FrenzyTag, false)
    end
    States[Owner] = nil
end

return M
