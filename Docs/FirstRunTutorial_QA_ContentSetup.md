# First-Run Tutorial QA / Content Setup

## Content Setup

- InitialRoom weapon spawners:
  - Tutorial weapon spawner: set `TutorialVisibility = FirstRunTutorialOnly`.
  - Normal weapon spawner: set `TutorialVisibility = NonTutorialOnly`.
  - Shared or debug weapon spawners can stay `Always`.
- Portal reward preview:
  - Card rewards can keep their real `RewardOptionsOverride` entries.
  - The portal UI now collapses any rune/card reward set to one generic card icon.
  - Gold uses the existing gold icon; material uses the question icon.
- First-run next-room plan:
  - Use `SetNextRoomPlan` for forced portal, room override, reward override, enemy buff override, and special enemy reward setup.
  - `PortalIndex` defaults to `0`; change it in the node if the target door should be different.
- Prayer room:
  - Fill `RoomDataOverride` with the real prayer room `RoomDataAsset` when that asset is ready.
  - Confirm the altar uses the sacrifice flow that triggers finisher grant.
- Moonlight special enemy:
  - Fill `SpecialRewardEnemyAuraFX` with the persistent blue aura Niagara asset.
  - Confirm `SpecialRewardEnemyLootOptions` contains only the Moonlight card.

## Manual QA Flow

- New save / first-run tutorial:
  - InitialRoom shows only the tutorial weapon.
  - Pick up tutorial weapon; the formal weapon should not show or interact.
  - Dummy kill still drops the Heavy card tutorial pickup.
  - First combat room portal previews gold with the gold icon.
- After first gold room:
  - Next portal opens only the configured `PortalIndex`.
  - BuffCardRoom preview shows the enemy buff plus one generic card icon.
  - It must not show Attack / Heavy / Split as three separate icons.
  - Clearing the room drops one reward pickup that opens the three-card choice.
- Moonlight room:
  - Last/special enemy has the persistent blue aura.
  - Killing it drops one Moonlight pickup.
  - The room clear reward pickup is suppressed if Moonlight is the special enemy reward.
- Transition rooms:
  - The next two rooms show and grant only gold/material rewards.
  - Picking gold/material should play the immediate pickup path and should not open card choice UI.
- Prayer / forced defeat:
  - Prayer room altar allows sacrificing one card.
  - Finisher card is granted and its tutorial popup appears.
  - Forced survival starts immediately after the finisher grant.
  - Player death skips the normal GameOver menu and returns to hub.
- Post tutorial:
  - InitialRoom shows only the formal weapon.
  - New run deck is `Attack, Attack, Moonlight, Finisher`.

## Useful Logs

- `[StoryRewardDebug]` verifies pending rewards, portal previews, and reward pickup options.
- `[FirstRunTutorialDirector]` verifies story stage transitions, next-room plan application, special enemy marking, and forced survival.
- `[WeaponSpawner]` verifies pickup/equip flow for the active weapon spawner.
- `[Frontend] Opening gameplay map ... (FirstRunTutorial=...)` verifies whether the run is using tutorial or normal flow.

## Common Checks

- If both weapons appear:
  - Check both weapon spawners in InitialRoom have opposite `TutorialVisibility` values.
  - Check the save slot has either active first-run tutorial or completed tutorial flags as expected.
- If card preview shows multiple icons:
  - Check the portal widget is using the latest code and not a stale packaged build.
  - Confirm the reward options are `ELootType::Rune`; non-rune options are intentionally shown separately.
- If the prayer room is not reached:
  - Check `RoomDataOverride` is assigned in the relevant `SetNextRoomPlan` node.
  - Check the forced `PortalIndex` points to the visible/open portal.
- If Moonlight drops twice:
  - Check `bSuppressRoomClearRewardPickup` is enabled for the Moonlight special enemy room plan.
