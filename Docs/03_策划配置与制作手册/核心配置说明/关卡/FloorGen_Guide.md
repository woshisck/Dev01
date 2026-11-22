# Floor Generation (M0) — Usage Guide

| Item | Content |
| --- | --- |
| Scope | Per-run floor layout generation: room graph, grid placement, debug view (milestone M0) |
| Audience | Designers (room data, generation rules), programmers (integration) |
| Related docs | [商店房间配置说明](商店房间配置说明.md), [PrayRoom献祭事件关配置说明](PrayRoom献祭事件关配置说明.md), [故事事件引擎接口说明](故事事件引擎接口说明.md) |
| Last updated | 2026-11-18 |
| Status | **M0 nearly done.** Code, data assets and `L_FloorGenTest` are in and the first run works. Console commands and the acceptance run are still open. See section 9 for progress and the roadmap |

---

## 1. What it does

Each run builds one floor of about 20 rooms laid out on a grid, Castlevania-style: rooms connect
through doors and the player can walk back to rooms already visited. Room shapes and their placement
are shuffled every run; a fixed set of room types (Start, Boss, Elite, Shop, Pray, Treasure, Story) is
guaranteed on every floor.

M0 covers **layout only**. It produces a data layout and draws it as debug boxes. It loads no levels,
spawns no enemies and opens no doors.

```
UFloorGenConfig (DA) ──┐
Room pool (Room DAs) ──┼─► [1] Graph ──► [2] Grid placement ──► [3] Validate ──► FFloorLayout
Seed ──────────────────┘   main path +    footprints on cells,   reachability,     (pure data)
                           side branches,  doors aligned          rules, overlap         │
                           required rooms                                                ▼
                                                                         AYogFloorDirector debug draw
```

| Step | What happens | Fails when |
| --- | --- | --- |
| 1. Graph | Builds the main path Start → Boss, hangs side branches off it, assigns each required room to a node that matches its rule | Room count too small for the required rooms |
| 2. Grid placement | Places each node's footprint next to its parent so a door socket on each side lines up; backtracks on dead ends | Grid too small, footprints too big, too few door sockets |
| 3. Validate | Checks every room is reachable, every rule is met, nothing overlaps | Should never fail if 1 and 2 succeeded — report the seed if it does |

If a seed fails, the generator retries with a derived seed up to `MaxSeedRetries` times. The same seed
and the same config always produce the same layout.

### Not in M0

| Feature | Milestone |
| --- | --- |
| Streaming room levels, doors, room cleared state, camera bounds | M1 |
| Room Director integration, minimap | M2 |
| Filler pass (optional WFC), biomes | M3 |
| Ability / key gated areas | M4 (design pending) |

---

## 2. Concepts

| Term | Meaning |
| --- | --- |
| Cell | One grid square. Size is `CellSize` on the config, set in the editor |
| Footprint | The set of cells a room occupies, relative to its origin cell `(0,0)` |
| Door socket | A possible doorway: one cell of the footprint plus the edge it sits on |
| Depth | A room's position along the main path, `0.0` = Start, `1.0` = Boss. Side-branch rooms take the depth of the main-path room they branch from |
| Main path | The shortest route Start → Boss |
| Side branch | A chain of rooms hanging off the main path, ending in a dead end |

### Grid and direction convention

Directions use the world axes, not compass names, so they stay unambiguous under the top-down camera.

```
        +X  (screen up with the default camera)
         ▲
         │   cell (0,1) │ cell (1,1)
         │  ───────────┼───────────
         │   cell (0,0) │ cell (1,0)
         └──────────────────────────► +Y
```

| Direction | Edge |
| --- | --- |
| `PosX` | The cell's +X edge |
| `NegX` | The cell's -X edge |
| `PosY` | The cell's +Y edge |
| `NegY` | The cell's -Y edge |

A footprint cell `(X, Y)` covers world X from `X * CellSize.X` to `(X+1) * CellSize.X`, same for Y.
Doors sit at the **centre** of an edge.

> **Authoring rule for M1 (follow it now so rooms don't need rework):** the room level's world origin
> is the min corner of cell `(0,0)`, and no geometry extends outside the footprint.

---

## 3. What to prepare

| # | Item | Who | Notes |
| --- | --- | --- | --- |
| 1 | Room type tags | Programmer | Added to `Config/Tags/RoomTag.ini` with the code (see section 4.3) |
| 2 | `DA_FloorGen_L1` | Designer | One floor-generation config, section 5 |
| 3 | Test room data assets | Designer | 15 placeholder `URoomDataAsset`s, no level needed for M0, section 4.4 |
| 4 | `L_FloorGenTest` | Designer | Empty map with one `AYogFloorDirector`, section 6 |

Suggested location: `Content/Docs/Map/FloorGen/` for the config and `Content/Docs/Map/FloorGen/TestRooms/`
for the test rooms.

---

## 4. Room data (`URoomDataAsset`)

### 4.1 New fields

Existing fields (`LevelAsset`, `RoomName`, `DisplayName`, `RoomTags`) are unchanged. New fields are in
the **Generation** category. Existing room assets keep working: an empty footprint is treated as 1×1.

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `FootprintCells` | `TArray<FIntPoint>` | empty (= `(0,0)` only) | Cells the room occupies. Must include `(0,0)` and be 4-connected |
| `DoorSockets` | `TArray<FRoomDoorSocket>` | empty (= every outer edge) | Where doors may be. Leave empty unless the room must restrict its doorways |
| `GenWeight` | `float` | `1.0` | Relative chance to be picked among rooms of the same type. `0` = never picked |
| `MaxPerFloor` | `int32` | `0` | Max copies on one floor. `0` = unlimited |

`FRoomDoorSocket`:

| Field | Type | Meaning |
| --- | --- | --- |
| `Cell` | `FIntPoint` | A footprint cell |
| `Direction` | `ERoomDoorDir` | `PosX` / `NegX` / `PosY` / `NegY`. Must be an outer edge (no footprint cell on the other side) |

`RoomTags` must contain exactly one `Room.Type.*` tag. The generator uses it to match rooms to rules.

### 4.2 Footprint examples

| Shape | `FootprintCells` |
| --- | --- |
| 1×1 | *(empty)* or `(0,0)` |
| 2×1 (long in X) | `(0,0) (1,0)` |
| 1×2 (long in Y) | `(0,0) (0,1)` |
| 2×2 | `(0,0) (1,0) (0,1) (1,1)` |
| L-shape | `(0,0) (1,0) (0,1)` |

Room rotation is not supported, on purpose: a top-down room plays differently when rotated. Author a
separate room asset for each orientation you want.

### 4.3 Room type tags

| Tag | Status |
| --- | --- |
| `Room.Type.Normal` | Exists |
| `Room.Type.Elite` | Exists |
| `Room.Type.Shop` | Exists |
| `Room.Type.Event` | Exists (not used by M0 rules) |
| `Room.Type.Start` | New |
| `Room.Type.Boss` | New |
| `Room.Type.Treasure` | New |
| `Room.Type.Pray` | New |
| `Room.Type.Story` | New |

### 4.4 Test room set for M0

| Asset | Type tag | Footprint |
| --- | --- | --- |
| `DA_Room_Test_1x1_A` / `_B` / `_C` | Normal | 1×1 |
| `DA_Room_Test_2x1_A` / `_B` | Normal | 2×1 |
| `DA_Room_Test_1x2_A` / `_B` | Normal | 1×2 |
| `DA_Room_Test_2x2_A` | Normal | 2×2 |
| `DA_Room_Test_Start` | Start | 1×1 |
| `DA_Room_Test_Boss` | Boss | 2×2 |
| `DA_Room_Test_Elite` | Elite | 2×1 |
| `DA_Room_Test_Shop` | Shop | 1×1 |
| `DA_Room_Test_Pray` | Pray | 1×1 |
| `DA_Room_Test_Treasure` | Treasure | 1×1 |
| `DA_Room_Test_Story` | Story | 1×1 |

Leave `LevelAsset` empty on all of them for M0.

---

## 5. Floor config (`UFloorGenConfig`)

Create via **Content Browser → Miscellaneous → Data Asset → FloorGenConfig**. Name: `DA_FloorGen_<Layer>`,
e.g. `DA_FloorGen_L1`.

### 5.1 Fields

| Field | Type | Recommended | Meaning |
| --- | --- | --- | --- |
| `CellSize` | `FVector2D` | `2400, 2400` | World size of one cell. Match the size of your smallest room |
| `GridSize` | `FIntPoint` | `10, 10` | Max floor area in cells. Larger = fewer failures, more sprawling floors |
| `RoomCountMin` / `RoomCountMax` | `int32` | `18` / `22` | Total rooms, including required ones |
| `MainPathMin` / `MainPathMax` | `int32` | `8` / `10` | Rooms on Start → Boss, both ends included |
| `BranchMaxLength` | `int32` | `3` | Longest side branch, in rooms |
| `RoomPool` | `TArray<URoomDataAsset*>` | all test rooms | Rooms the generator may use. Normal rooms fill everything not covered by a rule |
| `RequiredRooms` | `TArray<FFloorRoomRule>` | see 5.3 | Rooms that must appear every floor |
| `RoomPopulation` | `TArray<FFloorRoomPopulation>` | empty | Actors spawned into rooms by type, see 5.4 |
| `MaxPlacementAttempts` | `int32` | `200` | Backtracking budget per seed in step 2 |
| `MaxSeedRetries` | `int32` | `20` | Derived seeds tried before giving up |

### 5.2 `FFloorRoomRule`

| Field | Type | Meaning |
| --- | --- | --- |
| `RoomType` | `FGameplayTag` | A `Room.Type.*` tag. A room from `RoomPool` with this tag is picked by `GenWeight` |
| `OverrideRoom` | `URoomDataAsset*` | Optional. Use this exact room instead of picking from the pool |
| `Count` | `int32` | How many to place |
| `Placement` | `EFloorRoomPlacement` | `Start` / `End` / `MainPath` / `SideBranch` / `Any` |
| `DepthRange` | `FVector2D` | Allowed depth, `0..1`. Ignored for `Start` and `End` |
| `bDeadEnd` | `bool` | The room gets exactly one door |

`Start` and `End` must appear exactly once each, with `Count = 1`.

### 5.3 Recommended rules for `DA_FloorGen_L1`

| RoomType | Count | Placement | DepthRange | bDeadEnd |
| --- | --- | --- | --- | --- |
| `Room.Type.Start` | 1 | Start | — | false |
| `Room.Type.Boss` | 1 | End | — | true |
| `Room.Type.Elite` | 1 | MainPath | 0.4 – 0.7 | false |
| `Room.Type.Shop` | 1 | SideBranch | 0.3 – 0.8 | false |
| `Room.Type.Pray` | 1 | SideBranch | 0.2 – 0.9 | false |
| `Room.Type.Treasure` | 1 | SideBranch | 0.0 – 1.0 | true |
| `Room.Type.Story` | 1 | SideBranch | 0.0 – 1.0 | false |

### 5.4 Room population (`RoomPopulation`)

Spawns actors into every generated room that carries a given type tag, e.g. weapon spawners in the
Start room. It runs once per generation, after walls and doors, and the actors are destroyed with the
floor on regenerate.

`FFloorRoomPopulation`:

| Field | Type | Meaning |
| --- | --- | --- |
| `RoomType` | `FGameplayTag` | A `Room.Type.*` tag. Every room with this tag gets the entries |
| `Entries` | `TArray<FFloorSpawnEntry>` | What to spawn |

`FFloorSpawnEntry`:

| Field | Type | Meaning |
| --- | --- | --- |
| `ActorClass` | `TSubclassOf<AActor>` | Class to spawn. For weapons use `BP_WeaponSpawner` |
| `WeaponDefinition` | `UWeaponDefinition*` | Used only when `ActorClass` is a weapon spawner. Set before the actor finishes spawning, so its mesh and info card are built from it |
| `Offset` | `FVector` | From the room centre, world axes. Z is the height above the floor surface |
| `Yaw` | `float` | Rotation around Z |

The room centre is the average of the room's cell centres. In the Start room it is also the player
spawn point, so give weapons an `Offset` a few hundred units away from `(0,0)`, otherwise the player
spawns inside the interact volume.

Example, three weapons in the Start room:

| Entry | `ActorClass` | `WeaponDefinition` | `Offset` |
| --- | --- | --- | --- |
| 0 | `BP_WeaponSpawner` | `DA_WPN_THSword` | `(500, -300, 0)` |
| 1 | `BP_WeaponSpawner` | `DA_WPN_UnArm` | `(500, 0, 0)` |
| 2 | `BP_WeaponSpawner` | *(another weapon)* | `(500, 300, 0)` |

Toggle with `bPopulateRooms` on the director. For logic the config cannot express, implement
`OnFloorGenerated` (Blueprint or Lua) and call `RegisterSpawnedPart` on anything it spawns so the
director cleans it up.

---

## 6. Running it

### 6.1 Test map setup

1. Create an empty level `L_FloorGenTest` (already exists in `Content/Docs/Map/FloorGen/`).
2. Place **`AYogFloorDirector`** anywhere (its location is the world position of grid cell `(0,0)`). The test map uses the Blueprint subclass `B_YOGFloorDirector` from the same folder.
3. Set its fields:

| Field | Value |
| --- | --- |
| `Config` | `DA_FloorGen_L1` |
| `Seed` | `0` (random each Play) or a fixed number to reproduce a layout |
| `bGenerateOnBeginPlay` | `true` |
| `bDrawDebug` | `true` |

4. Press Play and look down from above (the debug camera or `ToggleDebugCamera` works well).

### 6.2 Reading the debug view

| Element | Drawn as |
| --- | --- |
| Grid bounds | Thin grey outline |
| Room footprint | Filled box in the type colour, label = room asset name + depth |
| Door link | Line between the two door positions |
| Main path | Thick yellow line through room centres |
| Seed and stats | On-screen text: seed, room count, main-path length, attempts used |

| Type | Colour |
| --- | --- |
| Start | Green |
| Boss | Red |
| Elite | Orange |
| Shop | Cyan |
| Pray | Purple |
| Treasure | Gold |
| Story | Pink |
| Normal | Grey |

### 6.3 Console commands

| Command | Effect |
| --- | --- |
| `Yog.FloorGen.Regen` | Regenerate with a new random seed |
| `Yog.FloorGen.Regen <Seed>` | Regenerate with a specific seed |
| `Yog.FloorGen.Draw 0/1` | Hide / show the debug view |
| `Yog.FloorGen.Stress <Count>` | Generate `<Count>` floors with consecutive seeds, no drawing; logs success rate, average and worst time, and every failing seed with its reason |

Log category: `LogYogFloorGen`.

---

## 7. M0 acceptance

| Check | Target |
| --- | --- |
| `Yog.FloorGen.Stress 1000` success rate | 100% (after seed retries) |
| Average generation time | Under 5 ms |
| Same seed, same config | Identical layout |
| Every required room present | Matches its rule on every floor |

---

## 8. Troubleshooting

| Log reason | Meaning | Fix |
| --- | --- | --- |
| `NotEnoughRooms` | `RoomCountMin` is less than main path + required side-branch rooms | Raise `RoomCountMin` or lower `MainPathMin` |
| `NoRoomForRule <Tag>` | No room in `RoomPool` carries that type tag (or all have `GenWeight` 0 / hit `MaxPerFloor`) | Add a room with that tag, or set `OverrideRoom` |
| `PlacementBudgetExceeded` | Step 2 ran out of attempts | Increase `GridSize`, add more 1×1 rooms, or lower `RoomCountMax` |
| `DeadEndHasNoSingleSocketFit` | A `bDeadEnd` room could not be attached with one door | Give that room more door sockets, or leave `DoorSockets` empty |
| `InvalidFootprint <Room>` | Footprint missing `(0,0)` or not connected | Fix `FootprintCells` |
| `InvalidSocket <Room>` | A socket is not on an outer edge | Fix `DoorSockets` |
| `ValidationFailed` | Generator bug | Report the seed and the config asset name |

When reporting a bad layout, always include the **seed** shown on screen and the config asset name;
that is enough to reproduce it exactly.

---

## 9. Progress and roadmap

Last checked: 2026-11-18 (against the repo).

### 9.1 M0 progress

| # | Item | Status |
| --- | --- | --- |
| 1 | Room type tags (`Start`, `Boss`, `Treasure`, `Pray`, `Story`) in `RoomTag.ini` | Done |
| 2 | `URoomDataAsset` generation fields (`FootprintCells`, `DoorSockets`, `GenWeight`, `MaxPerFloor`) | Done |
| 3 | `UFloorGenConfig`, `FFloorRoomRule`, `EFloorRoomPlacement` | Done |
| 4 | Generator: graph, grid placement, validate, seed retry, `FFloorLayout` | Done, first run works |
| 5 | `AYogFloorDirector` debug view | Done, first run works |
| 6 | Console commands `Yog.FloorGen.Regen` / `Draw` / `Stress` (section 6.3) | **Not started** |
| 7 | `DA_FloorGen_L1` and the 15 `DA_Room_Test_*` assets (section 3, items 2 and 3) | Done, in `Content/Docs/Map/FloorGen/` and `TestRooms/` |
| 8 | `L_FloorGenTest` map with one `AYogFloorDirector` (section 6.1) | Done, uses `B_YOGFloorDirector` with `Config = DA_FloorGen_L1` |
| 9 | Acceptance run (section 7) | **Not run**, needs item 6 for the stress test |

### 9.2 To close M0

| # | Step | Who | Notes |
| --- | --- | --- | --- |
| 1 | Console commands `Regen` / `Draw` (section 6.3) | Programmer | Find the `AYogFloorDirector` in the PIE world and call `GenerateFloor` / `SetDebugDrawEnabled` |
| 2 | Console command `Stress` and its `LogYogFloorGen` report | Programmer | Calls `FFloorGenerator::Generate` directly with consecutive seeds, no actor and no drawing; logs success rate, average and worst time, and each failing seed with its reason |
| 3 | Acceptance run (section 7) | Programmer + designer | `Stress 1000` in `L_FloorGenTest`; check the same seed twice for an identical layout |
| 4 | Fix failures from step 3 | Programmer / designer | Generator bug → code fix; tuning failure → `DA_FloorGen_L1` or more test rooms (section 8) |
| 5 | Correct sections 4 to 6 of this guide against what the runs showed | Programmer | Then mark M0 done in the header |

### 9.3 Later milestones

| Milestone | Scope | Notes |
| --- | --- | --- |
| M1 | Streaming room levels, doors, room-cleared state, camera bounds | See 9.4 |
| M2 | Room Director integration, minimap | After M1 |
| M3 | Filler pass (optional WFC), biomes | After M2 |
| M4 | Ability / key gated areas | Design pending |

### 9.4 M1 plan: seamless room loading

M1 replaces the full-map reload (`AYogGameMode::TransitionToLevel` calling `OpenLevel`) with one persistent floor map that streams room levels in and out at runtime.

```
L_Floor (persistent: GameMode, player, camera, AYogFloorDirector)
   ├─ Room A   streamed at cell origin * CellSize
   ├─ Room B   streamed at cell origin * CellSize   <- player is here
   └─ Room C   preloaded, door still closed
```

| Decision | Choice |
| --- | --- |
| Loading API | `ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr` per room `LevelAsset`, transform from `FFloorLayout` |
| World Partition / Data Layers | Not used. Both the floor map and the room maps stay plain levels |
| Loaded set | Current room plus its door neighbours; rooms two or more doors away unload |
| Between floors | `OpenLevel` stays; `TransitionToLevel` and the run-state snapshot are kept for that case only |

Work M1 pulls in:

| Area | Change |
| --- | --- |
| `AYogRoomDirector` + Lua | Each loaded room starts its script only when it becomes the active room |
| Room cleared state | Per-room state kept in the layout so a revisited room does not respawn its mobs |
| `APortal` | `DestinationLevel` replaced by the destination in the generated layout |
| `AYogCameraPawn` | Polygon bounds switch to the active room |
| Navmesh | Runtime generation or per-room nav inside each streamed level |
| Lighting and fog | Streamed levels need runtime lighting; the global Ninja fog must work across rooms |
| Room maps | No GameMode override and no player start in a room map |

Suggested order: first the director streams blank test rooms in and out with simple door triggers, to prove loading and positioning. Only then the Lua room flow, cleared state and camera bounds.

**Authoring rule to follow now:** each room level's world origin is the min corner of cell `(0,0)`, and no geometry extends outside its footprint (section 2). Rooms built this way need no rework in M1.
