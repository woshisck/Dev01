# DataEditor 数值编辑器测试说明

## 作用

用于测试 `DevKitEditor` 数值工具链：RuneDA / EffectDA 收集、DataValidator 校验、访问器双源比对、CSV 导出、RuneID 到 RuneIdTag 迁移，以及批量数值操作。

当前日常入口是 UE 顶部菜单 `Tools > DataEditor`。它是 C++ Slate 面板，不依赖 `EUW_RuneBalanceEditor.uasset` 蓝图资产；`Tools/DataEditor` 脚本作为自动化回归和 headless 运维入口保留。

## 配置位置

| 对象 | 位置 |
| --- | --- |
| UE 菜单入口 | `Tools > DataEditor` |
| Slate 面板 | `Source/DevKitEditor/Private/Tools/SDataEditorWidget.*` |
| 脚本入口 | `Tools/DataEditor/README.md` |
| PowerShell 公共环境 | `Tools/DataEditor/_common.ps1` |
| C++ 工具库 | `Source/DevKitEditor/Public/Tools/DataEditorLibrary.h` |
| C++ 校验器 | `Source/DevKitEditor/Public/Tools/DataValidator.h` |
| Rune Tag 配置 | `Config/Tags/RuneIDs.ini` |
| Effect Tag 配置 | `Config/Tags/EffectIDs.ini` |
| 输出报告 | `Saved/Balance/SmokeTest_<时间戳>/report.txt` |

## 推荐测试顺序

1. 编译 `DevKitEditor`，确保最新 C++ 面板和函数库已加载。
2. 启动 `DevKit.uproject`。
3. 点击 UE 顶部菜单 `Tools > DataEditor`。
4. 在 DataEditor 窗口确认 Rune 列表显示约 73 条，`EffectDA` 统计为 0。
5. 点 `Refresh`，确认列表不丢数据。
6. 点 `Open` 打开任意 RuneDA，点 `Copy Tag` 确认能复制当前 `RuneIdTag`。
7. 选中 1-2 条 Rune，批量修改 `GoldCost` 或 `Rarity`，确认资产 dirty 后可用 `Ctrl+Z` 撤销；这一步不需要保存资产。
8. 点 `Validate All`，确认 Output Log 有 DataValidator 结果。
9. 点 `Export CSV`，确认 `Saved/Balance/` 生成 Rune / Effect CSV。
10. 关闭编辑器或确认没有正在编辑的 UE 窗口；如需要脚本自动关闭，命令追加 `-CloseEditor`。
11. 跑只读 smoke test：

```powershell
pwsh Tools/DataEditor/run_smoke_test.ps1
```

如果本机没有 `pwsh`，使用 Windows PowerShell：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\DataEditor\run_smoke_test.ps1
```

12. 打开最新 `Saved/Balance/SmokeTest_*/report.txt`。
13. 只在需要迁移旧 `RuneID` 时跑 migration；只在需要全量 headless 批量改数值时跑 batch ops。
14. 做最少玩法回归：Rune Tools Detail Panel、PIE 买符文/看背包/触发符文/看传送门浮窗。

## 验收标准

| 项 | 通过标准 |
| --- | --- |
| DataEditor 菜单 | `Tools > DataEditor` 存在并能打开窗口 |
| Rune 主表 | 显示约 73 条 RuneDA，列包含 Asset / RuneIdTag / GoldCost / Rarity 等 |
| 面板批量修改 | 选中 Rune 后能改 `GoldCost` / `Rarity`，资产 dirty，不自动保存，可 `Ctrl+Z` |
| 面板导出 | `Export CSV` 后 `Saved/Balance/` 有 CSV |
| Smoke test 退出码 | PowerShell 返回 `0` |
| Validation | `Errors: 0` |
| Accessor Parity | `Accessor/field diff count: 0` |
| CSV 导出 | Rune CSV 和 Effect CSV 路径非空 |
| RuneDA 基线 | 当前可接受 `RuneDA count: 73` |
| EffectDA 基线 | 当前 `EffectDA count: 0` 是资产现状，不算失败 |
| Detail Panel | 任意 `DA_Rune_*` 顶部出现 **Rune Tools** |
| 回归玩法 | 商店、背包、符文触发、传送门浮窗无新增阻断问题 |

## 报告解读

`report.txt` 包含 5 段：

- `Asset Collection`：确认扫描到的 RuneDA / EffectDA 数量。
- `Validation`：只要 `Errors` 为 0，Warning 可先进入人工判断。
- `Accessor Parity`：必须为 0；非 0 表示访问器和原字段语义不一致。
- `CSV Export`：给策划横向对比用，默认在 `Saved/Balance/`。
- `Overall`：`PASS` 才算 smoke test 通过。

当前常见 Warning 多为 `RuneIdTag 未配置`、`RuneName 为空`、`Shape.Cells 为空`。其中 `RuneIdTag 未配置` 会在迁移后下降。

## 迁移测试

```powershell
pwsh Tools/DataEditor/run_migration.ps1 -Phase prepare
pwsh Tools/DataEditor/run_migration.ps1 -Phase apply
```

注意：

- `prepare` 写 `Config/Tags/RuneIDs.ini`，不改 DA。
- `apply` 会修改并保存 RuneDA。
- `prepare` 后必须重启编辑器或重新跑 headless 进程，让 GameplayTagsManager 加载新 Tag。
- 迁移后重新跑 smoke test，确认 RuneIdTag 相关 Warning 下降。

## 批量操作测试

日常手动测试优先用 `Tools > DataEditor` 面板选中少量 Rune 修改。面板路径只标 dirty、不自动保存，支持 `Ctrl+Z`。

下面的脚本是全量 headless 运维入口，会修改并保存资产，测试前建议确认工作区干净或准备好回滚。

```powershell
pwsh Tools/DataEditor/batch_ops.ps1 -Op set_rune_gold -Value 99
pwsh Tools/DataEditor/batch_ops.ps1 -Op set_rune_rarity -Value Rare
```

当前不把 Effect 批量操作列为必测项，因为基线里 `EffectDA count = 0`。

## 注意事项

- 默认不自动关闭 UE；遇到 `UnrealEditor is already running` 时，先手动保存并关闭，或确认安全后加 `-CloseEditor`。
- `Tools > DataEditor` 是本轮正式日常入口；不要求创建 EUW 蓝图资产。
- Headless 修改类脚本会保存目标资产，不能依赖编辑器里的 `Ctrl+Z` 回退。
- 批量操作出错后用 source control 或 `git restore` 回滚 `.uasset`。
- `DATA-P1-EUW` 仍是后续任务；本轮测试不要求创建 `EUW_RuneBalanceEditor.uasset`。
# DataEditor Multi-Panel Test Update (2026-05)

Use these UE menu entries for manual testing:

- `Tools > DevKit Data > Character Balance`: confirm CharacterData rows load, edit one copied/test row value such as `MoveSpeed` or `Attack`, then verify dirty state and `Ctrl+Z`.
- `Tools > DevKit Data > Action Balance`: confirm MontageConfig / AttackData rows load, musket Light/Heavy/Sprint rows load, and generated tuning assets are explicit dirty assets.
- `Tools > DevKit Data > Rune Balance`: confirm RuneDA count remains around 73, edit `GoldCost` / `Rarity` / `TriggerType`, and add or update one `Tuning Scalar`.

The compatibility entry `Tools > DataEditor` opens `Rune Balance`. Automated regression remains `pwsh Tools/DataEditor/run_smoke_test.ps1`; it now also reports CharacterData, MontageConfigDA, MontageAttackDataAsset, and MusketActionTuningDataAsset counts.
