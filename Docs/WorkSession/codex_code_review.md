## Codex 代码审查

### 代码问题
- `Source/DevKit/Private/Item/Weapon/WeaponDefinition.cpp:102-104` 和 `Source/DevKit/Private/Item/Weapon/WeaponSpawner.cpp:421-423` 在未确认 `LastSpawnedWeapon` / `NewWeapon` 成功生成的情况下直接挂 `Weapon.Type.*`。如果 `ActorsToSpawn` 为空或 Spawn 失败，角色实际无武器但 ASC 仍持有武器类型 Tag，违背“无武器时 LMB 无反应”。
- `SetupWeaponToCharacter` 与 `TryPickupWeapon` 各自手写“清旧 Tag + 应用新 Tag”流程，逻辑重复，后续新增装备入口时容易漏掉 Tag 守卫。建议收敛成统一的装备完成/卸装接口。
- `YogAbilitySystemComponent.cpp:336-353` 重复声明 `Weapon.Type.Melee/Ranged` 静态 Tag，建议集中成一个私有常量/工具函数，避免字符串漂移。

### 潜在 Bug
- `WeaponDefinition.h:95` 默认 `WeaponType = EWeaponType::Melee`。若任意火枪/远程武器 DA 未迁移为 `Ranged`，装备后会启用近战 GA、阻止火枪 GA，且问题只表现为数据配置错误，代码层面没有校验。
- `YogAbilitySystemComponent.cpp:352-363` 使用 `SetLooseGameplayTagCount` 写 LooseTag；LooseTag 不复制。若拾取/读档装备在服务器执行，而客户端用 `TryActivateAbilitiesByTag(..., true)` 做预测激活，客户端可能没有 `Weapon.Type.*`，导致本地无法激活或和服务器判定不一致。
- `GA_PlayerMeleeAttacks.cpp:22`、`GA_MusketBase.cpp:34` 在 C++ 父类构造函数追加 `ActivationRequiredTags`，但已有 Blueprint GA 子类如果序列化覆盖了 `ActivationRequiredTags`，可能不会继承新增守卫。需要逐个检查现有 BP 类默认值或用运行时断言验证。
- `WeaponSpawner.cpp:378-423` 在 `NewWeapon == nullptr` 后仍继续写 `EquippedWeaponDef` 并挂类型 Tag；远程 GA 里 `GetMuzzleLocation` 还有无武器 fallback，可能出现“没枪也能从角色前方开火”的情况。

### 性能隐患
- 未见明显性能热点。
- `TryPickupWeapon` / `SetupWeaponToCharacter` 先显式 `ClearWeaponTypeTags()`，随后 `ApplyWeaponTypeTag()` 内又清一次，会产生重复 TagMap 更新/广播；影响很小，但可以顺手去掉外层或内层重复清理。
- `ApplyWeaponTypeTag` 每次装备都 `UE_LOG(LogTemp, Log, ...)`，频率不高；若装备流程未来变频繁，建议改为 `Verbose` 或受调试开关控制。

### 改进建议
- 仅在武器实例成功装备后应用类型 Tag：`LastSpawnedWeapon` / `NewWeapon` 有效时 `ApplyWeaponTypeTag`，失败时保持 `ClearWeaponTypeTags` 后直接返回或报错。
- 增加显式卸装路径，例如 `UnequipWeapon()`：统一销毁武器、清 `EquippedWeaponDef/Instance`、清 `Weapon.Type.*`，保证“无武器态”可被稳定进入。
- 对数据资产做校验：远程武器的 `WeaponAbilityData` 包含 Musket GA 时强制 `WeaponType == Ranged`，近战武器反之；可放在编辑器校验或装备时 Warning。
- 如果项目目标包含联网，尽早把这两个 Tag 改成 Replicated LooseTag 或用服务器权威状态同步给客户端，否则当前守卫机制只适合单机/本地判定。