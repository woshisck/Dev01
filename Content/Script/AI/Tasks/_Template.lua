--
-- Template for a FStateTreeTask_CallLua module.
--
-- Point a "Call Lua" task's LuaModule at a copy of this file (dot path relative to
-- Content/Script, e.g. "AI.Tasks.Taunt"). The task requires the module fresh on every
-- EnterState/Tick/ExitState and calls whichever of these three functions it finds.
--
-- OnEnter(Owner) -> bool|nil   Called once on EnterState. Return false to fail the task
--                              immediately; anything else (including nothing) keeps it
--                              Running. Optional — omit it if the module only ticks.
-- OnTick(Owner, DeltaTime) -> bool|nil
--                              Called every StateTree tick while Running. Return true to
--                              succeed, false to fail, nil/nothing to keep Running.
--                              Presence is checked once per EnterState: adding OnTick to a
--                              module that was already missing it this state won't take
--                              effect until the state is re-entered.
-- OnExit(Owner)                Called once on ExitState, regardless of how the task ended.
--                              Optional.
--
-- Owner is whatever actor the task's Owner property is bound to (usually the AIController
-- or its pawn). All three calls are pcall-guarded on the C++ side, so an error here fails
-- the task rather than taking the owner's frame down with it.
--

local M = {}

function M.OnEnter(Owner)
end

-- function M.OnTick(Owner, DeltaTime)
-- end

function M.OnExit(Owner)
end

return M
