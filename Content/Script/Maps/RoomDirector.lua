--
-- Room flow for one map. Bound to AYogRoomDirector through its LuaModule property.
--
-- Owns the whole loop: run every AMobSpawner placed in the map, follow the mobs they produce, and
-- once nothing is left alive open the map's portals and reveal the reward pickups placed with
-- bPickupEnabled off. AYogGameMode is not consulted at any point.
--
-- Copy this file per map when a room needs different behaviour and point that map's director at
-- the copy (e.g. "Maps.Room_Forest01").
--

local Progress = require("Mob.SpawnProgress")

local M = UnLua.Class()

local function Log(Director, Message)
    if Director.bVerbose then
        UE.UKismetSystemLibrary.PrintString(Director, "[RoomDirector] " .. Message,
            true, true, UE.FLinearColor(0, 1, 1, 1), 5)
    end
end

local function CollectActors(Director, Class)
    local Found = UE.TArray(UE.AActor)
    UE.UGameplayStatics.GetAllActorsOfClass(Director, Class, Found)
    return Found
end

-- Portals are shut here rather than in OnRoomStart so the player never sees a frame of an open
-- door on a room that still has to be fought.
function M:ReceiveBeginPlay()
    self.Mobs = {}        -- [actor name] = { Actor = mob, bAlive = bool }
    self.AliveCount = 0
    self.Portals = {}
    self.Pickups = {}
    self.bPortalsOpen = false

    -- A previous PIE session can leave spawners marked busy if the Lua env outlives the world.
    Progress.Reset()
    Progress.Listener = function()
        self:OnSpawnProgress()
    end

    local FoundPortals = CollectActors(self, UE.APortal)
    for i = 1, FoundPortals:Length() do
        local Portal = FoundPortals:Get(i)
        self.Portals[#self.Portals + 1] = Portal
        Portal:DisablePortal()
    end

    -- Only pickups that were placed disabled belong to the room-clear reward; enabled ones are left alone.
    local FoundPickups = CollectActors(self, UE.ARewardPickup)
    for i = 1, FoundPickups:Length() do
        local Pickup = FoundPickups:Get(i)
        if not Pickup:IsPickupEnabled() then
            self.Pickups[#self.Pickups + 1] = Pickup
        end
    end
end

-- Raised by AYogRoomDirector one tick after BeginPlay, so every spawner has already picked its rule.
function M:OnRoomStart()
    -- A rule spawns at least its first mob inside RunLuaSpawnRule; one that keeps going afterwards
    -- registers with Mob.SpawnProgress, which holds the room open and reports each later spawn.
    local Spawners = CollectActors(self, UE.AMobSpawner)
    for i = 1, Spawners:Length() do
        Spawners:Get(i):RunLuaSpawnRule()
    end

    self:TrackMobs()
    Log(self, string.format("%d spawner(s) produced %d mob(s), %d portal(s) held closed",
        Spawners:Length(), self.AliveCount, #self.Portals))

    -- A room with no combat in it should not trap the player.
    if self.AliveCount == 0 and not Progress.IsBusy() then
        self:OpenPortals()
    end
end

--- Raised by Mob.SpawnProgress after a timed rule spawns a mob or finishes.
function M:OnSpawnProgress()
    self:TrackMobs()

    -- Every mob out so far may already be dead, so the last spawn is also a possible clear.
    if self.AliveCount == 0 and not Progress.IsBusy() then
        self:OpenPortals()
    end
end

--- Subscribes to every AEnemyCharacterBase in the map that is not tracked yet.
function M:TrackMobs()
    local Found = CollectActors(self, UE.AEnemyCharacterBase)
    for i = 1, Found:Length() do
        local Mob = Found:Get(i)
        local Key = Mob:GetName()
        if not self.Mobs[Key] and not Mob.bIsDead then
            self.Mobs[Key] = { Actor = Mob, bAlive = true }
            self.AliveCount = self.AliveCount + 1
            Mob.OnCharacterDied:Add(self, self.HandleMobDied)
        end
    end
end

function M:HandleMobDied(Mob)
    local Entry = Mob and self.Mobs[Mob:GetName()]
    -- The death pipeline can broadcast more than once; a second pass must not push the count below
    -- zero and strand the room.
    if not Entry or not Entry.bAlive then
        return
    end

    Entry.bAlive = false
    self.AliveCount = self.AliveCount - 1

    -- A death is the only moment a respawn or demand-spawn can have appeared, and there are only
    -- as many deaths as mobs, so rescanning here costs less than ticking every frame.
    self:TrackMobs()

    Log(self, string.format("%s died, %d left", Mob:GetName(), self.AliveCount))

    if self.AliveCount == 0 and not Progress.IsBusy() then
        self:OpenPortals()
    end
end

function M:OpenPortals()
    if self.bPortalsOpen then
        return
    end
    self.bPortalsOpen = true

    for _, Portal in ipairs(self.Portals) do
        self:OpenPortal(Portal)
    end

    for _, Pickup in ipairs(self.Pickups) do
        Pickup:SetPickupEnabled(true)
        Pickup:PlaySpawnFocusCue()
    end

    Log(self, string.format("room clear, %d portal(s) opened, %d pickup(s) revealed", #self.Portals, #self.Pickups))
end

function M:ReceiveEndPlay()
    Progress.Listener = nil
    for _, Entry in pairs(self.Mobs) do
        if Entry.bAlive then
            Entry.Actor.OnCharacterDied:Remove(self, self.HandleMobDied)
        end
    end
    self.Mobs = {}
end

return M
