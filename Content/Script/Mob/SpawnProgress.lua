--
-- Shared progress of spawn rules that finish over time instead of all at once.
--
-- A rule that keeps spawning after Run returns calls Begin(Spawner) before it returns and Finish(Spawner)
-- when its last mob is out. The room director reads IsBusy so it does not count a room as cleared while
-- a spawner still has mobs to produce, and Notify tells it to rescan for the mobs that just appeared.
--
-- The module table is shared by every spawner and director in the Lua env, so it holds no per-room
-- state beyond the weak set of spawners that are still working.
--

local M = {}

-- Weak keys: a spawner destroyed mid-sequence drops out on its own instead of holding the room shut.
local Active = setmetatable({}, { __mode = "k" })

--- Called by the room director; receives no arguments. Replaced, not stacked.
M.Listener = nil

function M.Reset()
    Active = setmetatable({}, { __mode = "k" })
    M.Listener = nil
end

function M.Begin(Spawner)
    Active[Spawner] = true
end

function M.IsBusy()
    return next(Active) ~= nil
end

--- Tells the director new mobs may exist; call after every spawn.
function M.Notify()
    if M.Listener then
        M.Listener()
    end
end

function M.Finish(Spawner)
    Active[Spawner] = nil
    M.Notify()
end

return M
