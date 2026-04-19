# GM 调试命令指南

> 项目：星骸降临 Dev01  
> 系统版本：2026-04-19  
> 适用人群：程序 + 策划（测试/调试阶段）

---

## 一、接入方式

### 1.1 注册 CheatManager

在 **PlayerController 蓝图**（或 GameMode 蓝图）的 Class Defaults 中，将 `Cheat Manager Class` 设为 `UYogCheatManager`。

> 路径：`BP_YogPlayerController` → Details → Cheat Manager Class → 选 `YogCheatManager`

### 1.2 打开控制台

PIE 或打包（Development/Debug 模式）运行时，按 **`~`**（反引号）打开 UE 控制台，直接输入命令回车。

### 1.3 启用 Cheat

默认 PIE 中 Cheat 已启用。打包版本若需要，在 DefaultEngine.ini 中添加：
```ini
[/Script/Engine.CheatManager]
bAllowCheatsInShipping=True
```

---

## 二、命令速查表

### 2.1 热度系统

| 命令 | 参数 | 说明 | 示例 |
|------|------|------|------|
| `Yog_SetHeat` | `float` | 直接设置热度值（自动 Clamp 到 0~MaxHeat） | `Yog_SetHeat 200` |
| `Yog_SetPhase` | `int (0–3)` | 强制跳转热度阶段（0=无热度，1/2/3=Phase1-3） | `Yog_SetPhase 3` |
| `Yog_MaxHeat` | 无 | 热度设为满值，可触发自然升阶流程 | `Yog_MaxHeat` |
| `Yog_FreezeHeat` | `bool (0/1)` | 冻结热度衰减（通过授予 `Buff.Status.Heat.Active`） | `Yog_FreezeHeat 1` |

**说明：**
- `Yog_SetPhase` 直接写入阶段值，**不走升阶 FlowGraph**（不触发升阶特效）；若需触发升阶效果，用 `Yog_MaxHeat` 让系统自然升阶。
- `Yog_FreezeHeat 1` 利用 `Buff.Status.Heat.Active` 阻断 `GE_HeatDecay`，原理与战斗状态保热一致。输入 `Yog_FreezeHeat 0` 解冻。

---

### 2.2 背包 / 符文

| 命令 | 参数 | 说明 | 示例 |
|------|------|------|------|
| `Yog_GiveRune` | `int RuneID` | 将对应符文加入**待放置列表**（需手动打开背包放入格子） | `Yog_GiveRune 1001` |
| `Yog_ClearRunes` | 无 | 清空背包所有**非永久**符文，停止其 BuffFlow | `Yog_ClearRunes` |
| `Yog_SetGold` | `int` | 直接设置金币总量（正负均可） | `Yog_SetGold 999` |

**说明：**
- `Yog_GiveRune` 通过遍历所有已加载 `URuneDataAsset` 匹配 RuneID，**必须确保对应 DA 已被加载**（在编辑器 PIE 中通常已加载；打包版本需确认资产是否被引用）。
- RuneID 对照见 [符文制作主指南](RuneMaster_ProductionGuide.md)（1001–1021）。
- 永久符文（`bIsPermanent=true`，如 `FA_Rune_HeatPhase`）不会被 `Yog_ClearRunes` 移除。

---

### 2.3 玩家属性

| 命令 | 参数 | 说明 | 示例 |
|------|------|------|------|
| `Yog_GodMode` | 无 | 切换无敌（开启：HP & MaxHP 设为 99999） | `Yog_GodMode` |
| `Yog_SetHP` | `float` | 直接设置当前 HP（自动 Clamp 到 0~MaxHP） | `Yog_SetHP 50` |
| `Yog_FullHP` | 无 | HP 恢复至 MaxHP | `Yog_FullHP` |
| `Yog_SetAttack` | `float` | 设置 Attack 属性基础值 | `Yog_SetAttack 100` |

**说明：**
- `Yog_GodMode` 每次调用切换状态（奇数次开启，偶数次关闭）。关闭后 HP 仍为 99999，需手动用 `Yog_SetHP` 调回。

---

### 2.4 敌人控制

| 命令 | 参数 | 说明 | 示例 |
|------|------|------|------|
| `Yog_KillAll` | 无 | 立即对场景内所有存活 `AEnemyCharacterBase` 调用 `Die()` | `Yog_KillAll` |
| `Yog_FreezeEnemies` | `bool (0/1)` | 通过 `CustomTimeDilation=0/1` 冻结/恢复所有敌人的 Tick 与 AI | `Yog_FreezeEnemies 1` |

**说明：**
- `Yog_KillAll` 会触发正常死亡流程（消解特效、掉落奖励）。
- `Yog_FreezeEnemies` 使用 `CustomTimeDilation` 冻结，AI 行为树和动画均停止；敌人不会死亡，切换回 1 后恢复正常行为。

---

### 2.5 Debug 打印

| 命令 | 说明 | 输出位置 |
|------|------|----------|
| `Yog_PrintHeat` | 打印当前热度值、MaxHeat、热度阶段 | Output Log + 屏幕 5 秒 |
| `Yog_PrintTags` | 打印玩家 ASC 所有激活 GameplayTag | Output Log（含计数）+ 屏幕 |
| `Yog_PrintAttributes` | 打印全部属性值（HP/Attack/Heat/…） | Output Log + 屏幕提示 |
| `Yog_PrintRunes` | 打印背包所有符文（名称/ID/等级/位置/激活状态） | Output Log + 屏幕计数 |
| `Yog_Help` | 列出所有可用命令 | Output Log + 屏幕提示 |

---

## 三、常用测试场景

### 场景 A：测试符文激活区（不同热度阶段）

```
Yog_SetPhase 0        # 重置到零阶段
Yog_GiveRune 1001     # 给予符文（打开背包放置）
Yog_SetPhase 1        # 升阶，观察激活区格子变化
Yog_SetPhase 2
Yog_SetPhase 3
```

### 场景 B：测试升阶完整流程（含特效）

```
Yog_SetHeat 0         # 重置热度
Yog_SetPhase 0        # 重置阶段
Yog_MaxHeat           # 热度满（需在 LastHit 动画窗口触发升阶，或直接让 BGC 自然升阶）
```

### 场景 C：测试符文 BuffFlow 效果

```
Yog_ClearRunes        # 清空背包
Yog_GiveRune 1005     # 给指定符文
Yog_SetPhase 1        # 确保激活区覆盖放置位置（放置后自动激活）
Yog_FreezeHeat 1      # 冻结热度，方便观察效果
Yog_KillAll           # 触发 OnKill 类符文
```

### 场景 D：快速还原状态

```
Yog_FreezeHeat 0      # 解冻
Yog_FreezeEnemies 0   # 解冻敌人
Yog_FullHP            # 恢复血量
```

---

## 四、C++ 实现细节

### 文件

| 文件 | 路径 |
|------|------|
| 头文件 | `Source/DevKit/Public/Cheater/Cheater.h` |
| 实现 | `Source/DevKit/Private/Cheater/Cheater.cpp` |
| 基类 | `GameFramework/CheatManager.h`（UE 内置） |

### 关键接口

| 功能 | 实现方式 |
|------|----------|
| 属性读写 | `ASC->SetNumericAttributeBase(UBaseAttributeSet::GetXxxAttribute(), Value)` |
| 热度通知 | `BGC->OnHeatValueChanged(Value)` — 触发边沿检测和 UI 广播 |
| 阶段跳转 | `BGC->RestorePhase(Phase)` — 跳过 FlowGraph，直接写入 |
| 热度冻结 | `ASC->AddLooseGameplayTag(Buff.Status.Heat.Active)` |
| 符文查找 | `TObjectIterator<URuneDataAsset>` 遍历，按 `RuneID` 匹配 |
| 符文给予 | `DA->CreateInstance()` → `Character->AddRuneToInventory(Instance)` |
| 敌人迭代 | `TActorIterator<AEnemyCharacterBase>` |
| 敌人冻结 | `Enemy->CustomTimeDilation = 0.f / 1.f` |

---

## 五、扩展指南

新增命令只需在 `UYogCheatManager` 中添加 `UFUNCTION(Exec)` 函数，重新编译即可，无需注册或配置。

```cpp
// 示例：添加 Yog_SetMoveSpeed
UFUNCTION(Exec) void Yog_SetMoveSpeed(float Value);

// cpp 实现
void UYogCheatManager::Yog_SetMoveSpeed(float Value)
{
    if (APlayerCharacterBase* Char = GetPlayerChar())
    {
        Char->GetASC()->SetNumericAttributeBase(UBaseAttributeSet::GetMoveSpeedAttribute(), Value);
    }
}
```
