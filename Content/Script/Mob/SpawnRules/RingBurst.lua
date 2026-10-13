--
-- Spawn rule: buys mobs with the spawner's budget and drops them all at once on a ring
-- centred on the spawner.
--

local SpawnBudget = require("Mob.SpawnBudget")

local RING_RADIUS = 400.0

local M = {}

function M.Run(Spawner)
    local Picked = SpawnBudget.Purchase(Spawner)
    if #Picked == 0 then
        return
    end

    local Origin = Spawner:K2_GetActorLocation()
    for n, Index in ipairs(Picked) do
        local Angle = (n / #Picked) * math.pi * 2
        Spawner:SpawnFromMobPool(Index, UE.FVector(
            Origin.X + math.cos(Angle) * RING_RADIUS,
            Origin.Y + math.sin(Angle) * RING_RADIUS,
            Origin.Z))
    end
end

return M
