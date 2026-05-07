# Enemy AI Template Generator Report
- Mode: Apply
- Preset: DefaultMelee

## Blackboard
- Found `/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee`.
- Ensured default melee blackboard keys.

## Behavior Tree
- Combat branch order: Skill -> SpecialMovement -> CloseMelee -> MoveToCombatSlot.
- Found `/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee`.
- Rebuilt visual graph and runtime tree.

## Enemy Data
- Set `/Game/Docs/Data/Enemy/Rat/DA_Rat`.BehaviorTree -> `/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee`.
- Ensured `/Game/Docs/Data/Enemy/Rat/DA_Rat` default movement, awareness and attack profile.
- Set `/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard`.BehaviorTree -> `/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee`.
- Ensured `/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard` default movement, awareness and attack profile.