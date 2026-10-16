--
-- Spawn rule: drops a single boss on the spawner and nothing else.
--
-- The boss is whichever MobPool entry is authored as EEnemyCombatTier::Boss, so designers only have
-- to put the boss DA in the pool. Unlike the other rules this one ignores the rest of the pool: a
-- boss encounter is sized by the boss itself, and escort adds belong on a second spawner.
--

local M = {}

local function Warn(Spawner, Message)
    UE.UKismetSystemLibrary.PrintString(Spawner, "[BossSpawn] " .. Message,
        true, true, UE.FLinearColor(1, 0, 0, 1), 10)
end

--- @return table list of 1-based MobPool indices whose EnemyData is tiered as Boss
local function FindBossEntries(Spawner)
    local Pool = Spawner.MobPool
    local Found = {}

    for i = 1, Pool:Length() do
        local Data = Pool:Get(i).EnemyData
        if Data and Data.CombatTier == UE.EEnemyCombatTier.Boss then
            Found[#Found + 1] = i
        end
    end

    return Found
end

function M.Run(Spawner)
    local Bosses = FindBossEntries(Spawner)
    if #Bosses == 0 then
        -- Silence here would read as a cleared room: the director opens the portals the moment
        -- nothing is alive, so the player would walk straight through the boss arena.
        Warn(Spawner, "MobPool has no entry with CombatTier = Boss; nothing spawned")
        return
    end

    local Index = Bosses[math.random(#Bosses)]
    if not Spawner:SpawnFromMobPool(Index, Spawner:K2_GetActorLocation()) then
        Warn(Spawner, string.format("pool entry %d refused the spawn (check MaxCountPerLevel and EnemyClass)", Index))
    end
end

return M
