# Active Skill Setup Report
- Mode: Apply

- Found `/Game/Code/Core/ActiveSkills/DA_ActiveSkill_ShieldBurst`.
- `DA_ActiveSkill_ShieldBurst` uses `UGA_ActiveSkill_ShieldBurst`, cooldown 120s, buff duration 60s.
- Found `/Game/Code/Core/Hub/BP_HubActiveSkillTerminal`.
- Configured `BP_HubActiveSkillTerminal`: widget `UActiveSkillLoadoutWidget`, required feature `Feature.Combat.ActiveSkill`.
- Verified active skill terminal placement in `/Game/World/Hub/L_HubTown`.
- Placed `BP_HubActiveSkillTerminal` in `L_HubTown` at (320, -260, 90).
- Updated `DT_MetaUpgradeNodes`: `Node.Skill.Unlock` costs 30 MysticPoint and unlocks `Feature.Combat.ActiveSkill`; `Node.Skill.Slot2` costs 50 MysticPoint, depends on unlock, and unlocks `Feature.Combat.ActiveSkill.Slot2`.