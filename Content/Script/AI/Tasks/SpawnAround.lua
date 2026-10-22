--
-- Spawns Params.Count enemies of Params.EnemyData on reachable NavMesh points within
-- Params.Radius of Owner. Used by a "Call Lua Spawn" StateTree task.
--
-- Params (authored on the task): EnemyData (UEnemyData), Radius (cm), Count (integer).
--

local M = {}

local MIN_DISTANCE = 0
local Z_OFFSET = 96
local COUNTS_FOR_LEVEL_CLEAR = true

local function Log(Owner, Message)
    UE.UKismetSystemLibrary.PrintString(Owner, Message, true, true, UE.FLinearColor(1, 0, 0, 1), 10)
end

function M.OnEnter(Owner, Params)
    if not Params or not Params.EnemyData then
        Log(Owner, "[SpawnAround] EnemyData is not set on the Call Lua Spawn task")
        return false
    end

    local Spawned = 0
    for _ = 1, Params.Count do
        local Enemy = UE.UYogLuaSpawnLibrary.SpawnEnemyAroundActor(
            Owner, Params.EnemyData, Params.Radius, MIN_DISTANCE, Z_OFFSET, COUNTS_FOR_LEVEL_CLEAR)
        if Enemy then
            Spawned = Spawned + 1
        end
    end

    if Spawned == 0 then
        Log(Owner, string.format("[SpawnAround] no enemy could be placed within radius %.0f", Params.Radius))
        return false
    end

    return true
end

return M
