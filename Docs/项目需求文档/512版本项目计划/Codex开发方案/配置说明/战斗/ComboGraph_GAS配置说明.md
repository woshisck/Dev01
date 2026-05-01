# ComboGraph GAS 配置说明

## 作用

本项目当前战斗系统不依赖 ComboGraph，`DevKit.uproject` 中已显式禁用 `ComboGraph` 插件。

只有后续重新启用 ComboGraph 节点式连招编辑器时，才需要配置 ComboGraph 提供的 GAS 全局类。否则不要配置 `ComboGraphAbilitySystemGlobals`，避免项目无意依赖 ComboGraph。

## 配置位置

插件启用位置：

`DevKit.uproject`

当前配置：

```json
{
  "Name": "ComboGraph",
  "Enabled": false
}
```

GAS 配置文件：

`Config/DefaultGame.ini`

配置段：

```ini
[/Script/GameplayAbilities.AbilitySystemGlobals]
```

## 当前项目配置

当前不使用 ComboGraph，因此 `DefaultGame.ini` 不应配置：

```ini
AbilitySystemGlobalsClassName="/Script/ComboGraph.ComboGraphAbilitySystemGlobals"
```

当前 GAS 配置应保持为：

```ini
[/Script/GameplayAbilities.AbilitySystemGlobals]
bUseDebugTargetFromHud=true
+GameplayCueNotifyPaths="/Game/Code/GAS/GameplayCueNotifies"
```

## 仅在重新启用 ComboGraph 时

如果后续明确要使用 ComboGraph，需要同时做两件事：

1. 在 `DevKit.uproject` 中把 `ComboGraph` 改为启用。
2. 在 `DefaultGame.ini` 的 GAS 配置段加入：

```ini
AbilitySystemGlobalsClassName="/Script/ComboGraph.ComboGraphAbilitySystemGlobals"
```

## 报错现象

如果 ComboGraph 被启用，但缺少 GAS 全局类配置，引擎启动时可能出现：

```text
Combo Graph: AbilitySystemGlobals settings do not include an entry for class of type ComboGraphAbilitySystemGlobals
```

## 验收方式

1. 保存 `DevKit.uproject` 和 `DefaultGame.ini`。
2. 重新启动编辑器。
3. 当前不使用 ComboGraph 时，确认启动日志中不再出现 `ComboGraphAbilitySystemGlobals` 或 `Combo Graph` 相关 LoadErrors。
4. 确认现有战斗系统、Gameplay Cue 相关资源能正常加载。

## 注意事项

- 不要同时配置多个 `AbilitySystemGlobalsClassName`。
- 当前项目禁用 ComboGraph 时，不要保留 ComboGraph 的 `AbilitySystemGlobalsClassName`。
- 该配置属于项目级 GAS 初始化配置，不是某个角色或武器 DA 的字段。
