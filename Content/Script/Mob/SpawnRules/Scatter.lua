--
-- Spawn rule: buys mobs with the spawner's budget and scatters them across reachable NavMesh
-- points inside the spawner's SpawnRadius.
--

local SpawnBudget = require("Mob.SpawnBudget")

local M = {}

function M.Run(Spawner)
    local Picked = SpawnBudget.Purchase(Spawner)

    for _, Index in ipairs(Picked) do
        local Location = Spawner:PrepareSpawnLocation()
        -- PrepareSpawnLocation returns zero when no reachable point was found.
        if Location.X ~= 0 or Location.Y ~= 0 or Location.Z ~= 0 then
            Spawner:SpawnFromMobPool(Index, Location)
        end
    end
end

return M
