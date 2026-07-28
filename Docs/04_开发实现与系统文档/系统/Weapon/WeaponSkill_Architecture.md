# 可装备武器战技架构

## 核心规则

- 每种战技使用一个独立的原生 C++ `UGA_WeaponSkill` 子类。
- 每种战技使用一个独立的 `UWeaponSkillDataAsset` 资产。
- 武器 `UWeaponDefinition::AvailableWeaponSkills` 是该武器允许装备的战技容器。
- 每个武器槽运行时只保存一个已装备战技；主武器和备用武器分别保存选择。
- 战技 DA 配置 `AbilityClass`、该战技自己的 `AbilityData` 和展示信息。
- 特有数值应由对应战技的 C++ DA 子类扩展；GA 可通过 `GetEquippedWeaponSkillData()` 读取当前 DA。

## 运行链

1. 武器装备时从 `DefaultWeaponSkill` 初始化；无有效默认值时使用容器第一个有效项。
2. 玩家更换战技时调用 `APlayerCharacterBase::EquipWeaponSkill`。
3. 角色移除旧战技的 Ability Spec，授予新 DA 指定的 GA，并把新战技 AbilityData 合并到运行时 CharacterData。
4. `WeaponSkill` 输入通过保存的 Ability Spec Handle 精确激活当前战技 GA。
5. 换武器时主/备用武器定义、战技选择、Actor 和卡组状态一起交换。
6. 切关和检查点分别保存主/备用武器的战技 DA。
7. 默认空手武器仍使用同一套战技容器和专用 GA；为了保持首次拾取武器逻辑，运行时可以不把它记为真实主武器。
8. GA 始终从当前 Ability Spec 的 SourceObject 读取战技 DA，避免切换时旧 GA 读到新战技数据。

## 兼容规则

尚未迁移的武器如果 `AvailableWeaponSkills` 为空，会继续读取旧的
`WeaponSkillAbilityData` 并使用旧通用 WeaponSkill GA。迁移完成后可逐步移除该回退。

## 新增战技

1. 新建原生 GA，例如 `UGA_WeaponSkill_Uppercut`，继承 `UGA_WeaponSkill`。
2. 如果需要独有配置，新建 `UUppercutWeaponSkillDataAsset`，继承 `UWeaponSkillDataAsset`。
3. 在 GA 中通过 `GetEquippedWeaponSkillData()` 获取并转换为对应 DA 类型。
4. 创建战技 DA 资产，填写 GA 类和独立 AbilityData。
5. 将战技 DA 加入允许使用它的武器 `AvailableWeaponSkills`，并按需设为默认战技。
