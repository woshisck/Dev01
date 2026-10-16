--
-- Spawn rule: drops one mob per MobPool entry, all at once, on a ring centred on the spawner.
--
-- Pool order is spawn order, so repeating an entry both raises the count and places that enemy
-- again further around the ring.
--

local RING_RADIUS = 400.0

local M = {}

function M.Run(Spawner)
    local PoolSize = Spawner:GetMobPoolSize()
    if PoolSize == 0 then
        return
    end

    local Origin = Spawner:K2_GetActorLocation()
    for Index = 1, PoolSize do
        local Angle = (Index / PoolSize) * math.pi * 2
        Spawner:SpawnFromMobPool(Index, UE.FVector(
            Origin.X + math.cos(Angle) * RING_RADIUS,
            Origin.Y + math.sin(Angle) * RING_RADIUS,
            Origin.Z))
    end
end

return M
