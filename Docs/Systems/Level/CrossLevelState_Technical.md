# 跨关状态持久化系统技术文档

> 适用范围：Roguelite 局内跑局跨关卡数据传递  
> 适用人群：程序  
> 配套文档：[传送门系统设计](Portal_Design.md)、[关卡系统技术文档](LevelSystem_ProgrammerDoc.md)  
> 最后更新：2026-04-10

---

## 一、问题背景

UE 的 `OpenLevel` 会完整销毁当前 World 下的所有 Actor（GameMode、PlayerController、Character 等），导致运行时数据全部丢失。

对于 Roguelite 游戏，玩家在每关之间需要保留：
- 当前 HP
- 金币数量
- 背包中的符文及其位置
- 背包热度阶段（Phase）

**解决方案**：在切关前将这些数据写入 `UYogGameInstanceBase`（GameInstance 跨关卡生存），新关卡加载后读取恢复。

---

## 二、数据结构

### FRunState（定义于 YogGameInstanceBase.h）

| 字段 | 类型 | 说明 |
|---|---|---|
| `bIsValid` | `bool` | 是否有有效存档；false 时不恢复（首次进入 / 死亡后）|
| `CurrentHP` | `float` | 切关前玩家 HP |
| `CurrentGold` | `float` | 切关前金币数量 |
| `CurrentPhase` | `int32` | 背包热度阶段（0-3）|
| `PlacedRunes` | `TArray<FPlacedRune>` | 非永久符文列表（含位置和激活状态）|

### UYogGameInstanceBase 相关字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `PendingRunState` | `FRunState` | 当前跑局状态快照 |
| `PendingNextFloor` | `int32` | 下一关楼层编号（FloorTable 下标 + 1）|

---

## 三、数据流

### 3.1 切关时写入（TransitionToLevel）

```
AYogGameMode::TransitionToLevel(FName NextLevel)
  ├─ CurrentPhase = ELevelPhase::Transitioning
  ├─ Backpack->SetLocked(true)
  ├─ GI->PendingNextFloor = CurrentFloor + 1
  ├─ 构建 FRunState：
  │    NewState.bIsValid    = true
  │    NewState.CurrentHP   = ASC->GetNumericAttribute(Health)
  │    NewState.CurrentGold = Player->GetGold()
  │    NewState.CurrentPhase = Backpack->GetCurrentPhase()
  │    NewState.PlacedRunes  = 过滤掉 bIsPermanent 的符文
  ├─ GI->PendingRunState = NewState
  └─ OpenLevel(NextLevel)
```

⚠️ `ConfirmArrangementAndTransition`（旧系统入口）也执行相同的写入逻辑，两条切关路径均安全。

### 3.2 新关卡加载时恢复

```
AYogGameMode::HandleStartingNewPlayer_Implementation(NewPlayer)
  ├─ Possess(LoadedChar)          ← 必须先完成，ASC 初始化后才能设属性
  └─ LoadedChar->RestoreRunStateFromGI()
       ├─ 检查 GI->PendingRunState.bIsValid，false 则跳过
       ├─ ASC->SetNumericAttributeBase(Health, NewState.CurrentHP)
       ├─ Player->AddGold(NewState.CurrentGold)       ← 含 OnGoldChanged 广播
       ├─ Backpack->RestorePhase(NewState.CurrentPhase)
       └─ for each PR in NewState.PlacedRunes:
            Backpack->TryPlaceRune(PR.Rune, PR.Pivot)
```

### 3.3 玩家死亡时清除

```
APlayerCharacterBase::Die()
  └─ GI->ClearRunState()          ← bIsValid = false
```

下一次从大厅进入新跑局时，`bIsValid` 为 false，`RestoreRunStateFromGI` 直接跳过，角色从初始状态开始。

---

## 四、关键函数速查

| 函数 | 文件 | 说明 |
|---|---|---|
| `TransitionToLevel(FName)` | `YogGameMode.cpp` | Portal 切关入口，写入 RunState + OpenLevel |
| `ConfirmArrangementAndTransition()` | `YogGameMode.cpp` | 旧切关入口，同样写入 RunState |
| `RestoreRunStateFromGI()` | `PlayerCharacterBase.cpp` | 新关卡恢复入口 |
| `ClearRunState()` | `YogGameInstanceBase.cpp` | 死亡时清除跑局数据 |
| `RestorePhase(int32)` | `BackpackGridComponent.cpp` | 直接设置热度阶段（跳过正常升降级逻辑）|

---

## 五、永久符文的特殊处理

**永久符文**（`FPlacedRune::bIsPermanent = true`）不写入 RunState。

原因：永久符文由 `BeginPlay` 自动重放（`UBackpackGridComponent::PermanentRunes` 在初始化时放置），如果也写入 RunState 会导致重复放置冲突。

规则：
- `PlacedRunes` 数组写入时过滤掉 `bIsPermanent == true` 的条目
- 恢复时只恢复非永久符文

---

## 六、调试方法

在 Output Log 中搜索以下关键词：

| 关键词 | 含义 |
|---|---|
| `[RunState] SAVE` | 切关时写入（来自 TransitionToLevel 或 ConfirmArrangementAndTransition）|
| `[RunState] SAVE (Portal)` | 由传送门触发的切关写入 |
| `[RunState] RESTORE` | 新关卡恢复时触发 |
| `[RunState] RESTORE skipped` | GI 中无有效状态，跳过恢复（首次进入或死亡后）|

日志格式：
```
[RunState] SAVE (Portal) — HP=80.0 Gold=150 Phase=2 Runes=3
[RunState] RESTORE — HP=80.0 Gold=150 Phase=2 Runes=3
```

---

## 七、注意事项

| 情况 | 风险 | 解决方案 |
|---|---|---|
| 在 Possess() 前调用 `RestoreRunStateFromGI` | ASC 未初始化，SetNumericAttributeBase 崩溃 | 已通过在 Possess() 之后调用避免 |
| `OpenLevel` 后 GameInstance 被替换 | 数据丢失 | 检查 GameInstance Class 是否在 Project Settings 中正确配置 |
| 背包组件未初始化就 TryPlaceRune | 无效操作 | `RestoreRunStateFromGI` 内部检查 `BackpackGridComponent` 指针有效性 |
| 多次调用 RestoreRunStateFromGI | 符文重复放置 | 恢复成功后 `bIsValid` 应置为 false（待验证是否已实现）|

---

## ⚠️ Claude 编写注意事项

- **SaveAll 必须在 OpenLevel 之前同步完成**：`YogSaveSubsystem::SaveAll` 是同步操作，调用后立即 `UGameplayStatics::OpenLevel`，不要用异步回调（UE5 的 AsyncSaveGame 在切关时可能未完成）
- **RestoreAll 在 BeginPlay 的时机**：恢复调用必须在 `PlayerCharacter::BeginPlay` 里、ASC 初始化之后、AbilitySet 授予之后，否则弹药 Attribute 恢复会被默认值覆盖
- **背包恢复顺序**：先 RestoreRunes（恢复符文格子）→ 再 RestoreHeat（恢复热度 Tag）→ 最后 RestoreWeapon（恢复武器 + 追赶广播），顺序错误会导致热度发光状态错误
- **SaveGame 对象不能存 UObject 指针**：`UYogSaveGame` 里只能存基础类型（int/float/FString/FGameplayTag/TArray），不能存 Actor* 或 UObject*，切关后指针全部失效
- **调试用 GM 命令**：开发期可用 `SaveSubsystem.Debug 1` 在每次 Save/Restore 时打印详细日志，发布前关闭
- **符文格子用 FIntPoint 序列化**：背包格坐标存为 `FIntPoint`，反序列化后用 `BackpackGridComponent::TryRestoreRune(Coord, RuneTag)` 恢复，不要用格子 Index（宽度变化后 Index 会错位）
