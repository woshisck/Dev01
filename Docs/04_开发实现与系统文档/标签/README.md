# GameplayTag 文档入口

本目录记录 DevKit 当前 GameplayTag 规则、迁移计划和清理报告。

## 当前结论

- `Character.State.*` 只表示角色动作/运行状态，例如攻击、技能、冲刺、受击窗口。
- 不新增正式 `Combat.*` 一级根。战斗动作状态归入 `Character.State.*`；战斗事件继续使用 `GameplayEvent.Combat.*`，战斗表现继续使用 `GameplayCue.Combat.*`。
- `Buff.*` 是正式的一线系统，覆盖 Buff、Debuff、持续状态，以及合并后的卡牌/符文效果语义。
- 卡牌/符文内部事件统一走 `Buff.Event.*`；`Action.Rune.*`、`Event.Rune.*` 只作为旧资产迁移来源，不作为运行时新入口。
- 卡牌/符文表现 Cue 统一走 `GameplayCue.Buff.*`；`GameplayCue.Rune.*` 只作为旧资产兼容来源。旧 `GameplayCue.Rune.FinisherCharge` 也迁移为 `GameplayCue.Buff.FinisherCharge`，但不代表恢复旧 QTE Finisher 运行时。
- 新内容不使用 `Buff.Status.*`、`Buff.ID.*`、`Buff.Keyword.*`、`Buff.Binding.*`、`Rune.ID.*`、`Rune.Effect.*`。
- Rune/Card 仍可作为资产、编辑器和模块名称存在，但 GameplayTag 语义合并到 `Buff.*`。身份、槽位、触发时机、流程角色、稀有度和数值放在 DA 字段/表格里。
- `PlayerState.AbilityCast.*`、`Card.*`、`Rune.*`、`Combo.CombatDeck.*`、`Buff.Rune.*` 只作为旧资产迁移来源或兼容读取，不作为新内容入口。
- Story/Tutorial/任务/固定房间/掉落覆盖后续归入 `Director.*` 系统；存档结果使用 `GameState.Flags.*`。

## 推荐阅读顺序

1. `GameplayTag_ReorgPlan_LOTF.md`：当前命名规范和 LOTF 参考后的项目化结论。
2. `PlayerActionRuneTag_Rules.md`：Attack/Skill/WeaponSkill/Dash 与 Buff 化卡牌规则。
3. `GameplayTag_MigrationMap_Phase1.md`：旧 tag 到新 tag 的迁移表和 Commandlet 使用说明。
4. `../../GeneratedReports/GameplayTagContentAssetScan.md`：Content 资产旧 tag 扫描结果。
5. `../../99_归档/GameplayTags_Analysis.md`：旧 LOTF/LOTF2 外部参考归档。

## 资产迁移流程

1. 先运行 `GameplayTagAssetMigrationCommandlet` dry-run，生成 `Docs/GeneratedReports/CommandletReports/GameplayTagAssetMigrationReport.md`。
2. 人工审查 report-only 项，尤其是 deprecated Special/Finisher、旧 `Rune.*` 元数据、Set 元素和 Map Key。
3. 确认可自动迁移后再使用 `-Apply`。
4. 在编辑器中重保存迁移过的资产。
5. 旧 tag 引用归零后，再删除旧 tag 定义并添加最终 `GameplayTagRedirects`。

不要手工编辑 `.uasset` 二进制文件。资产 tag 清理应通过 Commandlet、编辑器重保存和扫描报告闭环完成。
