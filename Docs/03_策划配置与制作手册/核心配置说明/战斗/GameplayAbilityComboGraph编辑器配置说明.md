# GameplayAbilityComboGraph 兼容说明

## 当前状态

`GameplayAbilityComboGraph` 不再是玩家武器新制作的主配置入口。当前玩家动作使用 Attack / Skill / WeaponSkill / Dash 四个动作槽，武器动作数据配置在 `WeaponDefinition.AttackAbilityData`、`WeaponDefinition.WeaponSkillAbilityData` 和对应 `AbilityData.MontageConfigMap` 中。

旧 Graph 资产仅用于旧武器加载、迁移对照和编辑器工具兼容。

## 旧资产处理

| 场景 | 处理方式 |
| --- | --- |
| 新武器制作 | 不创建新的 `GameplayAbilityComboGraph` |
| 旧武器仍能战斗 | 保留旧 Graph 资产，不主动删除 |
| 需要继续迭代旧武器 | 优先把动作同步到 typed AbilityData 后再改 |
| 只需要查看旧节点 | 可打开 Graph 对照旧 NodeId、Montage、AttackData |

## 迁移映射

| 旧 Graph 字段 | 当前配置位置 |
| --- | --- |
| `GameplayAbilityClass` / `AbilityTagOverride` | Attack 或 WeaponSkill GA 的 Ability Tag |
| `MontageConfig` | `AbilityData.MontageConfigMap` |
| `AttackDataOverride` | `MontageConfigDA.Entries.Hit Detection Window.AttackDataCandidates` |
| `RootInputAction` / `InputAction` | 不再作为主设计条件；用动作槽 `RequiredActionSlot` |
| `bIsComboFinisher` | 卡牌 `RequiredFlowRole = Finisher` 或 WeaponSkill 上下文 |
| `ComboWindowStartFrame` / `ComboWindowEndFrame` | 旧连招窗口资料；当前主设计不依赖连招续段 |

## 注意事项

- 不要为新内容配置旧的输入分支树。
- 不要把卡牌联动写在旧节点的输入条件里；用卡牌顺序、动作槽、FlowRole 和 Link 条件。
- 如果必须保留旧 Graph，请在验收里明确它是兼容路径，不作为新玩法依据。
