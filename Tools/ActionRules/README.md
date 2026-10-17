# 动作规则作者工具

使用 Excel、两份 CSV、结构化 Markdown 或自己的 Python 生成脚本定义动作关系。它们都生成同一个 JSON 契约，运行时由动作规则框架执行。**此工具不运行 Excel 内代码、Python 字符串或 Lua，也不挂载、不保存 UE 资产、不提交或发布。**

依赖：Python 3.10+ 标准库，无需 openpyxl。仅支持 `.xlsx`，不支持旧 `.xls`、宏或外部链接。作者文件上限 8 MiB，动作最多 1,024 个、规则最多 8,192 条；输出 JSON 不超过 1 MiB，挂载 Lua 源文件不超过 64 KiB UTF-8，与运行时限制一致。公式全部拒绝，即使有缓存值也必须先“复制 → 粘贴为值”，避免旧缓存悄悄改变动作逻辑。

## 快速开始

在项目根目录执行（`python` 替换为本机 Python 3.10+ 的路径）：

```powershell
python Tools/ActionRules/action_rules.py --input Tools/ActionRules/Templates/ActionRules_Template.xlsx --id PlayerCombat.Development --tags-file Config/Tags/CharacterTag.ini --output Content/ActionRules/player_combat.json
```

示例四条规则均 `FALSE`，不会自动改变正式冲刺。确认需要的策略后才在你的源文件启用相关行。转换后在工作台预览、校验、显式挂载；未经过运行时验证的 JSON 不能视为已生效。

```powershell
# Excel 的两份 CSV 导出；文件编码 UTF-8（允许 BOM）
python Tools/ActionRules/action_rules.py --actions-csv Tools/ActionRules/examples/Actions.csv --rules-csv Tools/ActionRules/examples/Rules.csv --id PlayerCombat.Example --output Saved/ActionRules/csv-example.json

# 同结构 Markdown 双表
python Tools/ActionRules/action_rules.py --input Tools/ActionRules/examples/actions.md --id PlayerCombat.Example --output Saved/ActionRules/md-example.json

# 自己维护的开发脚本：普通 Python 导入 build_document/write_document
python Tools/ActionRules/examples/build_rules.py --output Saved/ActionRules/python-example.json

# 回归
python -m unittest discover -s Tools/ActionRules/tests -v
```

默认不覆盖已有输出；需要替换时明确加 `--force`。先完整校验，再原子发布文件。校验失败即使带 `--force` 也不会清空已有输出。不要让输入和输出指向同一个作者文件。

## 双表约定

表头必须放第一行且名称、顺序一致。空白行忽略。数据区不能额外添加未知列。Excel 的筛选/隐藏行仍属于数据，会正常导出；删行或 `Enabled=FALSE` 才能移除/禁用规则。

- `Actions`：`ID, DisplayName, AbilityTags, StateTags`。ID 稳定不依赖行号；多个 Tag 用英文分号分隔。`AbilityTags` 用于匹配请求能力，`StateTags` 用于匹配角色当前状态。仅引用已注册 Tag，不创建 Tag。不同 Action 的 AbilityTags 不得重复或形成父子覆盖，以免一个请求匹配多个身份。
- `Rules`：`ID, Enabled, Subject, Current, Request, Effect, Condition, Reason`。`Enabled` 只接受 `TRUE/FALSE`。`Subject` 是 `any/player/enemy`。`Current` 与 `Request` 必须引用 Actions.ID。`Effect` 是 `block/interrupt`。
- `block`：Current 正在执行时，拒绝 Request。`interrupt`：Request 获准时，尝试打断 Current，不能指向自身。方向不可颠倒。规则不绕过死亡、GAS 成本/冷却和目标不可取消等硬约束。
- `Condition` 空白表示无需附加条件，否则只能填 1–64 个 ASCII 字符的 Lua 函数名，不填代码。`Reason` 是必须填写的非空诊断解释。

JSON 顶层固定为 `schemaVersion:1,id,actions,rules`，可选 `luaFile`。Action 字段固定 `id,displayName,abilityTags,stateTags`；Rule 字段固定 `id,enabled,subject,current,request,effect,condition,reason`。导入会拒绝未知字段、重复 ID/JSON key、坏枚举和悬空引用，避免拼错后静默失效。

## 条件脚本挂载

把真实 `.lua` 文件放在输出 JSON 同目录，指定 `--lua-file conditions.lua`。工具不会复制或创建占位函数：文件缺失或条件名未声明会拒绝输出。Lua 作者约定：

```lua
return {
    functions = {
        in_cancel_window = function(ctx)
            return ctx.owned_tags["PlayerState.AbilityCast.CanCombo"] == true
        end
    }
}
```

只使用 `ctx.owned_tags`、`ctx.request_tags`（Tag 名到 true）、`ctx.values`（键到 number）和 `ctx.subject`。转换器只检查这种直接返回 `functions` 字面量的写法，并排除注释/字符串/嵌套函数里的伪声明；不支持先声明局部表再动态组装返回值。它不是 Lua 编译器，不保证函数体可运行；实际语法、返回值、函数是否真的导出及执行预算由运行时 VM 在挂载/求值时验证。不要依靠此检查运行不可信代码。

Excel 不生成大量 Lua `if`，只引用函数名；常规关系放表格，复杂条件写一个可复用函数。若直接用 Python API，先 `build_document(...)`，再 `write_document(..., force=False)`；它和 CLI 使用同一验证器。

## Tag 字典与复现

`--tags-file` 接受 UE `GameplayTagList` INI、每行一个 Tag 的 UTF-8 文件或 JSON 字符串数组。跨多个字典时，提供由项目注册表导出的全集。未传时只校验 Tag 语法并明确告警；挂载端仍必须对实际注册 Tag 做最终检查。新 Tag 必须通过所属 `Config/Tags` 字典正式注册后重启/重新载入，不允许工具临时创造。

Excel 模板为 Actions/Rules 两张可扩展表，枚举下拉、冻结表头。新增数据直接追加，Tag 命名引用 `Config/Tags/CharacterTag.ini`。模板没有公式，列之间无隐藏计算依赖。
