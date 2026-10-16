--
-- Spawn rule: drops one mob per MobPool entry across reachable NavMesh points inside the
-- spawner's SpawnRadius.
--
-- Repeat an entry in the pool to spawn that enemy more than once.
--

local M = {}

function M.Run(Spawner)
    for Index = 1, Spawner:GetMobPoolSize() do
        local Location = Spawner:PrepareSpawnLocation()
        -- PrepareSpawnLocation returns zero when no reachable point was found.
        if Location.X ~= 0 or Location.Y ~= 0 or Location.Z ~= 0 then
            Spawner:SpawnFromMobPool(Index, Location)
        end
    end
end

return M
