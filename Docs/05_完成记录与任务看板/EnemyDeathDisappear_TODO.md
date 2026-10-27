# 敌人死亡消失 — 待办清单

| 项目 | 内容 |
| --- | --- |
| 适用范围 | 敌人死亡流程（`GA_Dead` → `FinishDying` → `Destroy`） |
| 适用人群 | 策划（配置）、程序（代码） |
| 配套文档 | 无 |
| 最后更新 | 2026-10-26 |

## 背景

敌人尸体消失由 `AYogCharacterBase::FinishDying()` 里的 `Destroy()` 执行
（`Source/DevKit/Private/Character/YogCharacterBase.cpp:568`）。触发时机由
`GA_Dead` 的延迟计时器决定，延迟值来自
`GetDeathDisappearDelayAfterAnimation()`。

2026-10-26 起统一为唯一配置源：**只有** `UEnemyData::DeathDisappearDelayAfterAnimation`
决定敌人尸体停留时间。尸体总停留时间 = 死亡蒙太奇时长 + 该延迟 + 1 帧。

## 已完成

- [x] `UEnemyData` 的 `Enemy|Death` 分组只保留 `DeathDisappearDelayAfterAnimation`
      （默认 0.15 秒），删除 `bOverrideDeathDisappearDelay` 开关
- [x] 删除 `AEnemyCharacterBase::DeathDisappearDelayAfterAnimation`（Blueprint 上不再能改）
- [x] `AEnemyCharacterBase::GetDeathDisappearDelayAfterAnimation()` 只读 EnemyData；
      无 EnemyData 时读 `UEnemyData` CDO 默认值
- [x] `GA_Dead` 死亡蒙太奇被取消 / 打断时也走同一延迟，不再立即销毁
- [x] `Die()` 兜底 `SetLifeSpan` 改为 `8 秒 + EnemyData 延迟`，不会提前截断长延迟

## 待办

### 1. 策划：在 EnemyData 上配置延迟

- [ ] 打开 `Content/Docs/Data/Enemy/<敌人名>/DA_<敌人名>.uasset`
- [ ] 展开 **Enemy | Death** 分组
- [ ] 填写 **死亡消失延迟**（秒）
- [ ] 保存资产

不填 = 默认 0.15 秒。

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

- [ ] 为"EnemyData 延迟生效 / 无 EnemyData 时取 CDO 默认值"补自动化测试

## 风险与注意事项

- **必须整关编辑器重编译**：新增反射属性（`UPROPERTY`）无法用 Live Coding 热更，
  需要关闭编辑器后跑 UBT。重编译会断开 MCP，之后要重新 `/mcp` 连接。
- **生成器不会覆盖新字段**：`EnemyAITemplateGeneratorCommandlet` 的
  `ConfigureDefaultEnemyData()` 不涉及 `Enemy|Death` 分组，重跑生成器不会
  擦掉策划在 DA 上的配置。
- **兜底销毁仍在**：`AEnemyCharacterBase::Die()` 里的 `SetLifeSpan(8 + 延迟)` 是
  `GA_Dead` 未激活时的保险。
- **BuffFlow 死亡流程可提前结束**：敌人配置了 Death Lifecycle Flow 且
  `BFNode_FinishLifecycle::bFinishDyingOnDeathLifecycle` 为 true 时，该节点会直接
  调用 `FinishDying()`，不经过 EnemyData 延迟。这属于流程资产主动结束，不是计时配置。
