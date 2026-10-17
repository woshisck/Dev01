# Action Rules 示例（默认未挂载）

本目录是开发模板，不是正式游戏策略。`player_combat.example.json` 的四条规则全部 `enabled:false`；没有配置自动挂载，不会改变当前冲刺、普攻或战技关系。

`conditions.lua` 提供纯条件函数样例。`has_stamina` 需要调用方主动提供 `ctx.values.stamina` 和 `requested_cost`，不是替代 GAS 成本验证。`in_cancel_window` 使用当前项目已有兼容窗口 Tag，不用于组合连招计数。

在工作台选择候选源、检查诊断和关系差异，再按框架的显式挂载流程使用。导出 JSON 不等于挂载，不会提交 P4 或发布 UGS。

作者工具和字段说明：`Tools/ActionRules/README.md`。Excel 在 `Tools/ActionRules/Templates/ActionRules_Template.xlsx`。
