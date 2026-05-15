# 远程武器子弹显示与生成门禁

## 目标

远程武器相关的弹药 HUD 和远程子弹生成都由 `Weapon.Type.Ranged` 管理：

- 未持有远程武器时，不显示弹药图标。
- 未持有远程武器时，即使远程投射物 Flow 被误触，也不生成远程子弹。
- 拾取远程武器并挂上 `Weapon.Type.Ranged` 后，弹药 HUD 和远程子弹逻辑恢复正常。

## 运行时入口

- 装备远程武器时，`WeaponDefinition::SetupWeaponToCharacter` 通过 `UYogAbilitySystemComponent::ApplyWeaponTypeTag(EWeaponType::Ranged)` 挂 `Weapon.Type.Ranged`。
- `UAmmoCounter` / `UWBP_AmmoCounter` 监听 `Weapon.Type.Ranged` 的 `NewOrRemoved` 事件，只有 Tag 存在且 `MaxAmmo > 0` 时显示。
- `UBFNode_SpawnRangedProjectiles` 默认 `bRequireRangedWeaponTag=true`，`RequiredWeaponTag=Weapon.Type.Ranged`；缺少该 Tag 时直接走 `Failed` 输出，不生成子弹。

## 配置项

`UBFNode_SpawnRangedProjectiles`：

- `bRequireRangedWeaponTag`：默认开启。
- `RequiredWeaponTag`：默认 `Weapon.Type.Ranged`。

如果未来有非武器来源的远程投射物，可以在对应 Flow 节点关闭 `bRequireRangedWeaponTag`，或替换成更具体的 RequiredTag。
