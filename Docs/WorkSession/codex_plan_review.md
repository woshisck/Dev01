ain## Codex 审查结果

### 发现的问题
- `AddLooseGameplayTag/RemoveLooseGameplayTag` 默认不复制；当前输入在 `YogPlayerControllerBase::LightAtack/HeavyAtack` 中用 `TryActivateAbilitiesByTag(..., true)` 发起，若项目启用客户端预测或多人场景，客户端 ASC 没有同步的 `Weapon.Type.*` 会导致本地激活失败或预测不一致。
- 方案只在 `SetupWeaponToCharacter` 中清理旧 `Weapon.Type.*`，没有覆盖“卸下武器但不立刻装备新武器”的路径；如果存在销毁、死亡、切关、调试清空、预览武器等非替换流程，旧武器类型 Tag 可能残留。
- 直接 `RemoveLooseGameplayTag` 两个类型 Tag 只会减少一次计数；如果同一类型 Tag 因重复装备、恢复流程或调试命令被加了多次，仍可能残留，导致近战/远程 RequiredTag 同时通过。
- “在 C++ 父类构造函数加 RequiredTag 后不需要改任何 BP”这个结论过强。已有 Blueprint GA 的 CDO 可能已经序列化了 `ActivationRequiredTags`，不一定自动继承新的 C++ 默认值，必须在编辑器中验证或批量重存/检查资产默认值。
- 方案没有明确 `Config/Tags/Equip.ini` 的写入格式。该目录现有 Tag 文件使用 `[/Script/GameplayTags.GameplayTagsList]` + `GameplayTagList=(...)`，实现时若只写裸 Tag 名，GameplayTag 不会被注册。
- `HeavyAttackReleased` 直接发送 `GameplayEvent.Musket.HeavyRelease`，不是通过 `TryActivateAbilitiesByTag`。虽然远程重攻击 GA 的初始激活会被 RequiredTag 拦住，但方案应明确“释放事件只影响已激活的火枪 GA”，避免误以为所有远程行为都统一由 RequiredTag 入口控制。

### 潜在风险
- 如果后续需要多人或 Listen Server 以外的网络形态，武器类型状态最好使用 Replicated Loose Tag、持续 GameplayEffect、或可复制的装备状态驱动；单纯 LooseTag 只适合本地/服务器单点逻辑。
- `UGA_PlayerMeleeAttack` 是玩家近战中间基类，但文档也显示 `UGA_MeleeAttack` 仍允许策划直接创建玩家 BP；若资产父类不统一，隔离会漏掉部分玩家近战 GA。
- `UWeaponData` 与 `UWeaponDefinition` 双轨并存，方案只覆盖 `UWeaponDefinition`。只要还有旧武器、测试武器、掉落器或存档恢复路径使用 `UWeaponData`，就会绕过武器类型 Tag。
- 无武器时禁止所有近战/远程 GA 会改变当前输入体验；Controller 仍会记录攻击输入 Buffer，可能产生“无武器输入被缓存，装备后异常连招/触发”的边缘行为。
- `UGA_MusketBase` 统一加 `Weapon.Type.Ranged` 会影响火枪换弹、冲刺换弹等非伤害 GA；这符合“远程武器专属”的意图，但如果未来有通用换弹、弹药 UI 或调试 GA 继承该基类，会被一起限制。
- 新增 `EWeaponType` 后，所有现有 `WeaponDefinition` 资产会默认近战；火枪 DA 若漏配为 Ranged，问题不会报错，只会表现为火枪 GA 全部无法激活。

### 改进建议
- 把武器类型 Tag 维护封装成 helper，例如 `ClearWeaponTypeTags()` / `ApplyWeaponTypeTag()`，并使用 `SetLooseGameplayTagCount(Tag, 0)` 清零类型 Tag，避免计数残留。
- 若考虑网络同步，优先使用 `AddReplicatedLooseGameplayTag/RemoveReplicatedLooseGameplayTag`，或用一个 Infinite GameplayEffect 持有 `Weapon.Type.*`。
- 在 `SetupWeaponToCharacter` 之外补一个明确的卸武器/清武器入口，并在死亡、切关前清理、调试清空、无武器恢复等路径调用。
- 实现后增加运行时日志或断言：装备完成后输出当前 `Weapon.Type.*` 计数；激活失败时能看到是缺少 `Weapon.Type.Melee/Ranged`。
- 给火枪 DA、近战 DA 增加一次资产检查清单：确认 `WeaponType` 配置、玩家近战 GA 父类、火枪 GA RequiredTag 是否实际生效。
- 文档步骤中补充 `Equip.ini` 的完整格式示例，避免 Tag 注册失败。

### 需要向用户确认的问题
- 是否需要支持多人/客户端预测？如果需要，不能只用普通 LooseTag。
- 无武器时是否允许徒手攻击？如果允许，需要 `Weapon.Type.Unarmed` 或独立徒手 GA。
- 当前所有玩家近战 GA 是否都继承 `UGA_PlayerMeleeAttack`？是否存在直接继承 `UGA_MeleeAttack` 的玩家 BP？
- 是否还有武器走旧的 `UWeaponData` 流程？如果有，是否一起迁移或也补 Tag 守卫？
- 未来是否会有主副武器、双持、临时召唤武器等场景？如果有，单一 `Weapon.Type.*` 可能需要扩展为槽位维度。