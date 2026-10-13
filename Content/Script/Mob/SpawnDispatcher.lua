--
-- Entry point for AMobSpawner's Lua spawn rules.
--
-- AMobSpawner::RunLuaSpawnRule requires this module and calls Run(Spawner, ModuleName), where
-- ModuleName is the rule the spawner picked from its LuaSpawnRulePool at BeginPlay.
--
-- A rule module must return a table exposing Run(Spawner).
--

local M = {}

-- Spawner API available to rules (all implemented in C++):
--   Spawner.SpawnDifficultyBudget            number  budget in UEnemyData::DifficultyScore units
--   Spawner:GetMobPoolSize()                 number
--   Spawner:GetMobPoolDifficultyScore(i)     number  1-based, 0 when the entry is unusable
--   Spawner:SpawnFromMobPool(i, Location)    actor   1-based, nil when capped or invalid
--   Spawner:PrepareSpawnLocation()           FVector random reachable NavMesh point
--   Spawner:K2_GetActorLocation()            FVector

function M.Run(Spawner, ModuleName)
    if not Spawner or not ModuleName or ModuleName == "" then
        return
    end

    local Ok, Rule = pcall(require, ModuleName)
    if not Ok then
        UE.UKismetSystemLibrary.PrintString(Spawner,
            string.format("[SpawnDispatcher] require failed for '%s': %s", ModuleName, tostring(Rule)),
            true, true, UE.FLinearColor(1, 0, 0, 1), 10)
        return
    end

    if type(Rule) ~= "table" or type(Rule.Run) ~= "function" then
        UE.UKismetSystemLibrary.PrintString(Spawner,
            string.format("[SpawnDispatcher] '%s' does not return a table with Run()", ModuleName),
            true, true, UE.FLinearColor(1, 0, 0, 1), 10)
        return
    end

    -- pcall so a broken rule cannot take the spawner (or the frame) down with it.
    local RunOk, Err = pcall(Rule.Run, Spawner)
    if not RunOk then
        UE.UKismetSystemLibrary.PrintString(Spawner,
            string.format("[SpawnDispatcher] '%s' errored: %s", ModuleName, tostring(Err)),
            true, true, UE.FLinearColor(1, 0, 0, 1), 10)
    end
end

return M
