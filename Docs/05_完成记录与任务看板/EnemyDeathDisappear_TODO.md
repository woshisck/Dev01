# 敌人死亡消失 — 待办清单

| 项目 | 内容 |
| --- | --- |
| 适用范围 | 敌人死亡流程（`GA_Dead` → `FinishDying` → `Destroy`） |
| 适用人群 | 策划（配置）、程序（代码） |
| 配套文档 | 无 |
| 最后更新 | 2026-09-20 |

## 背景

敌人尸体消失由 `AYogCharacterBase::FinishDying()` 里的 `Destroy()` 执行
（`Source/DevKit/Private/Character/YogCharacterBase.cpp:582`）。触发时机由
`GA_Dead` 的延迟计时器决定，延迟值来自
`GetDeathDisappearDelayAfterAnimation()`。

原先该延迟只能在敌人 Blueprint 上改（`AEnemyCharacterBase` 默认 0.15 秒），
本次改为可在 `UEnemyData` 资产上按敌人定义覆盖。

## 已完成

- [x] `UEnemyData` 新增 `Enemy|Death` 分组：`bOverrideDeathDisappearDelay` +
      `DeathDisappearDelayAfterAnimation`（`Source/DevKit/Public/Data/EnemyData.h`）
- [x] `AEnemyCharacterBase::GetDeathDisappearDelayAfterAnimation()` 优先读取
      EnemyData，未勾选覆盖时回退到 Actor 自身值
      （`Source/DevKit/Private/Character/EnemyCharacterBase.cpp:224`）
- [x] UBT 重新编译，反射数据已进入 `UnrealEditor-DevKit.dll`

## 待办

### 1. 策划：在 EnemyData 上配置延迟

- [ ] 打开 `Content/Docs/Data/Enemy/<敌人名>/DA_<敌人名>.uasset`
- [ ] 展开 **Enemy | Death** 分组
- [ ] 勾选 **覆盖死亡消失延迟**，填写 **死亡消失延迟**（秒）
- [ ] 保存资产

不勾选 = 沿用敌人 Blueprint 上的旧值，行为不变。

涉及资产（已核对存在）：

| 敌人 | EnemyData 路径 |
| --- | --- |
| Nemesis | `Content/Docs/Data/Enemy/Rat/DA_Nemesis.uasset` |
| Rat | `Content/Docs/Data/Enemy/Rat/DA_Rat.uasset` |
| RottenGuard | `Content/Docs/Data/Enemy/RottenGuard/DA_RottenGuard.uasset` |
| RottenGuard（无武器） | `Content/Docs/Data/Enemy/RottenGuard/DA_RottenGuard_UnArm.uasset` |
| GuardCaptain | `Content/Docs/Data/Enemy/GuardCaptain/DA_GuardCaptain.uasset` |
| AlarmBellJailer | `Content/Docs/Data/Enemy/AlarmBellJailer/DA_AlarmBellJailer.uasset` |
| Shadow | `Content/Docs/Data/Enemy/Shadow/DA_Shadow_01.uasset` |
| Boss | `Content/Docs/Data/Enemy/Boss/DA_Boss.uasset` |
| Dummy | `Content/Docs/Data/Enemy/Dummy/DA_Dummy.uasset` |

新目录 `Content/Code/Enemy/Definitions/Normal/AlarmBellJailer/DA_EN_AlarmBellJailer.uasset`
（未入版本库）看起来是 EnemyData 的新存放位置，配置前先确认刷怪实际引用的是哪一份。

### 2. 待确认：死亡消解特效缺失

当前 0.15 秒延迟过短，尸体基本是"瞬间消失"而非淡出。淡出需要消解
GameplayCue，配置位置在：

```
DA_<敌人名>            (UEnemyData)
  └─ AbilityData       → DA_AbilityMontage_<敌人名>_01
       └─ PassiveMap[ Action.Dead ]
            ├─ 反应蒙太奇            = 死亡动画
            └─ 消散 GameplayCue 标签 = 消解特效
```

- [ ] 确认是否需要消解特效
- [ ] 若需要：新建 GameplayCue Notify 资产（目前
      `Content/Code/GAS/GameplayCueNotifies/` 下只有 `GCN_Burn_OldGameplayCueNotifyActor`，
      没有死亡消解 Cue）
- [ ] 在 `PassiveMap[Action.Dead]` 填入对应 Cue 标签
- [ ] 将延迟调整为覆盖消解动画时长

> 注意：Cue 内的粒子必须在世界坐标生成（非附加模式），否则 Actor 销毁后
> 粒子会一起消失。见 `Source/DevKit/Public/Data/AbilityData.h:271`。

### 3. 可选：补测试

- [ ] 为"EnemyData 覆盖生效 / 未勾选时回退"补自动化测试。现有测试
      `Source/DevKit/Private/Tests/TutorialMobSpawnerTests.cpp:68` 只覆盖了 CDO
      默认值 0.15，未覆盖覆盖路径。

## 风险与注意事项

- **必须整关编辑器重编译**：新增反射属性（`UPROPERTY`）无法用 Live Coding 热更，
  需要关闭编辑器后跑 UBT。重编译会断开 MCP，之后要重新 `/mcp` 连接。
- **生成器不会覆盖新字段**：`EnemyAITemplateGeneratorCommandlet` 的
  `ConfigureDefaultEnemyData()` 不涉及 `Enemy|Death` 分组，重跑生成器不会
  擦掉策划在 DA 上的配置。
- **兜底销毁仍在**：`AEnemyCharacterBase::Die()` 里的 `SetLifeSpan(8.0f)`
  （`EnemyCharacterBase.cpp:495`）是 `GA_Dead` 未激活时的保险，不受本次改动影响。
