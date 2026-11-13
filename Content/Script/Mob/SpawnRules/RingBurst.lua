--
-- Spawn rule: spawns SPAWN_COUNT mobs from one MobPool entry, one every SPAWN_INTERVAL seconds, each
-- at its own place on a ring centred on the spawner. The first mob appears immediately.
--
-- The rule keeps spawning after Run returns, so it registers with Mob.SpawnProgress; the room director
-- will not treat the room as cleared until the last mob is out. A mob the pool entry refuses
-- (MaxCountPerLevel reached) still takes its slot in the sequence.
--

local Progress = require("Mob.SpawnProgress")

local RING_RADIUS = 400.0
local SPAWN_COUNT = 5
local SPAWN_INTERVAL = 1.5
local POOL_INDEX = 1 -- 1-based

local M = {}

function M.Run(Spawner)
    if Spawner:GetMobPoolSize() < POOL_INDEX then
        return
    end

    local Origin = Spawner:K2_GetActorLocation()
    local Spawned = 0

    Progress.Begin(Spawner)

    local function SpawnNext()
        -- The spawner can be destroyed between ticks (level change), which would leave the director waiting.
        if not UE.UKismetSystemLibrary.IsValid(Spawner) then
            Progress.Finish(Spawner)
            return
        end

        Spawned = Spawned + 1
        local Angle = (Spawned / SPAWN_COUNT) * math.pi * 2
        Spawner:SpawnFromMobPool(POOL_INDEX, UE.FVector(
            Origin.X + math.cos(Angle) * RING_RADIUS,
            Origin.Y + math.sin(Angle) * RING_RADIUS,
            Origin.Z))

        if Spawned >= SPAWN_COUNT then
            Progress.Finish(Spawner)
            return
        end

        Progress.Notify()
        UE.UKismetSystemLibrary.K2_SetTimerDelegate({ Spawner, SpawnNext }, SPAWN_INTERVAL, false)
    end

    SpawnNext()
end

return M
