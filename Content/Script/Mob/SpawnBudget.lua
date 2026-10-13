--
-- Shared budget spending for AMobSpawner Lua rules.
--
-- Rules decide placement and timing; this decides *what* gets bought. Kept in one place so a new
-- rule does not have to re-implement the purchase loop.
--

local M = {}

--- Buys mobs from the spawner's pool until the budget cannot afford anything else.
--- @param Spawner AMobSpawner
--- @param Budget number|nil defaults to Spawner.SpawnDifficultyBudget
--- @return table list of 1-based MobPool indices, in purchase order
function M.Purchase(Spawner, Budget)
    local Remaining = Budget or Spawner.SpawnDifficultyBudget
    local PoolSize = Spawner:GetMobPoolSize()
    local Picked = {}

    if PoolSize == 0 then
        return Picked
    end

    while Remaining > 0 do
        local Affordable = {}
        for i = 1, PoolSize do
            local Cost = Spawner:GetMobPoolDifficultyScore(i)
            -- Zero cost means an unusable entry; buying it would loop forever.
            if Cost > 0 and Cost <= Remaining then
                Affordable[#Affordable + 1] = i
            end
        end

        if #Affordable == 0 then
            break
        end

        local Index = Affordable[math.random(#Affordable)]
        Remaining = Remaining - Spawner:GetMobPoolDifficultyScore(Index)
        Picked[#Picked + 1] = Index
    end

    return Picked
end

return M
