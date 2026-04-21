# 武器系统 — 技术文档

> 更新：2026-04-16  
> 覆盖：WeaponSpawner 拾取交互 / WeaponInstance 热度发光 / WeaponDefinition 配置 / 切关恢复

---

## 一、系统概览

```
WeaponSpawner（场景中）
  │ 玩家进入 Overlap 范围
  │ → PendingWeaponSpawner 登记
  │ 按 E 键
  └─→ TryPickupWeapon()
         │
         ├─ 销毁旧武器（RemoveDynamic + Destroy）
         ├─ 生成 WeaponInstance（附着角色骨架）
         ├─ 赋 HeatOverlayMaterial
         ├─ AddDynamic → PlayerCharacterBase::OnHeatPhaseChanged
         └─ Broadcast(当前 Phase)   ← 追赶同步

PlayerCharacterBase（BeginPlay）
  └─ RegisterGameplayTagEvent
       Buff.Status.Heat.Phase.1/2/3 → OnHeatPhaseTagChanged
       Buff.Status.Heat.Phase（parent） → OnHeatPhaseParentTagChanged

热度升阶（GAS Tag 变化）
  └─ OnHeatPhaseTagChanged(Tag, NewCount)
       NewCount > 0 → OnHeatPhaseChanged.Broadcast(N)
       ↓
  WeaponInstance::OnHeatPhaseChanged(Phase)
       Phase > 0 → SetOverlayMaterial(HeatOverlayDynMat)
                    + SetVectorParameterValue("EmissiveColor", PhaseColor)
       Phase = 0 → SetOverlayMaterial(nullptr)
```

---

## 二、核心文件

| 文件 | 职责 |
|------|------|
| `WeaponSpawner.h/.cpp` | 场景中的武器展示 + Interact 拾取逻辑 |
| `WeaponDefinition.h/.cpp` | 武器数据资产（DA），`SetupWeaponToCharacter` 切关恢复路径 |
| `WeaponInstance.h/.cpp` | 装备到角色身上的武器 Actor，热度发光接口 |
| `PlayerCharacterBase.h/.cpp` | 持有 `OnHeatPhaseChanged` 委托，注册 GAS Tag 监听 |
| `YogPlayerControllerBase.cpp` | `Interact()` 中判断 `PendingWeaponSpawner` 并调 `TryPickupWeapon` |

---

## 三、拾取流程（WeaponSpawner）

### 3.1 Overlap 登记

```cpp
// OnOverlapBegin
Player->PendingWeaponSpawner = this;

// OnOverlapEnd
if (Player->PendingWeaponSpawner == this)
    Player->PendingWeaponSpawner = nullptr;
```

### 3.2 TryPickupWeapon()

1. **清理旧武器**
   - `OnHeatPhaseChanged.RemoveDynamic(旧 Instance)`
   - `旧 Spawner->RestoreSpawnerMesh()`（恢复原材质）
   - `旧 Instance->Destroy()`

2. **生成新武器**（走 `YogBlueprintFunctionLibrary::SpawnWeaponOnCharacter`）

3. **绑定热度委托**
   ```cpp
   NewWeapon->HeatOverlayMaterial = WeaponDefinition->HeatOverlayMaterial;
   Player->OnHeatPhaseChanged.AddDynamic(NewWeapon, &AWeaponInstance::OnHeatPhaseChanged);
   Player->EquippedWeaponInstance = NewWeapon;
   ```

4. **追赶同步**：查 ASC 当前 `Buff.Status.Heat.Phase.1/2/3` → `Broadcast(Phase)`

5. **Spawner 网格变黑**：遍历 `WeaponMesh` 材质槽替换 `BlackedOutMaterial`

### 3.3 碰撞配置（C++ 构造函数强制设置）

```cpp
CollisionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
CollisionVolume->SetGenerateOverlapEvents(true);
```

> 注意：不依赖 CDO 蓝图配置，C++ 写死以防蓝图覆盖失效。

---

## 四、热度发光（WeaponInstance）

### 4.1 委托接口

```cpp
// PlayerCharacterBase.h
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeatPhaseDelegate, int32, Phase);
UPROPERTY(BlueprintAssignable, Category = "Heat")
FHeatPhaseDelegate OnHeatPhaseChanged;
```

### 4.2 GAS Tag 监听（PlayerCharacterBase::BeginPlay）

```cpp
// 监听 Phase.1/2/3 新增 → Broadcast(N)
ASC->RegisterGameplayTagEvent(Phase1Tag, NewOrRemoved)
    .AddUObject(this, &APlayerCharacterBase::OnHeatPhaseTagChanged);

// 监听 parent tag 归零 → Broadcast(0)
ASC->RegisterGameplayTagEvent(PhaseParentTag, NewOrRemoved)
    .AddUObject(this, &APlayerCharacterBase::OnHeatPhaseParentTagChanged);
```

Tag 名称：
- `Buff.Status.Heat.Phase.1` / `.2` / `.3`
- `Buff.Status.Heat.Phase`（parent，用于归零检测）

### 4.3 发光颜色表

| Phase | 含义 | EmissiveColor（线性，HDR） |
|-------|------|--------------------------|
| 0 | 无 | — （清除 Overlay） |
| 1 | 白光 | `(2, 2, 2)` |
| 2 | 绿光 | `(0, 3, 0)` |
| 3 | 橙黄 | `(4, 2, 0)` |
| 4 | 过热红 | `(5, 0, 0)` |

### 4.4 动态材质创建

```cpp
if (!HeatOverlayDynMat)
    HeatOverlayDynMat = UMaterialInstanceDynamic::Create(HeatOverlayMaterial, this);

HeatOverlayDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), PhaseColors[Phase]);

for (UMeshComponent* Mesh : Meshes)
    Mesh->SetOverlayMaterial(HeatOverlayDynMat);   // Additive，不破坏原材质
```

`GetComponents<UMeshComponent>()` 可获取蓝图子类里添加的所有 Mesh，无需在 C++ 中显式引用。

---

## 五、切关恢复路径（WeaponDefinition）

`RestoreRunStateFromGI` → `WeaponDefinition::SetupWeaponToCharacter`

与 `TryPickupWeapon` 的对齐：

| 步骤 | TryPickupWeapon | SetupWeaponToCharacter |
|------|-----------------|----------------------|
| 清理旧武器 | ✓ | ✓（新增） |
| 赋 HeatOverlayMaterial | ✓ | ✓（新增） |
| AddDynamic | ✓ | ✓（新增） |
| 追赶同步 | ✓ | ✓（新增） |
| 设 EquippedWeaponInstance | ✓ | ✓（新增） |

> 切关前 Phase 已在 `FRunState.CurrentPhase` 中保存，恢复时 `BackpackGridComponent::RestorePhase` 会重新施加对应 GAS Tag，`SetupWeaponToCharacter` 末尾的追赶同步即可拿到正确值。

---

## 六、配置指南（编辑器）

### 6.1 创建 Overlay 材质

在 Content Browser 新建 Material：

| 设置 | 值 |
|------|-----|
| Blend Mode | Additive |
| Shading Model | Unlit |
| Two Sided | 可选（开启可避免背面缺失） |

节点结构：
```
VectorParameter "EmissiveColor"  ─┐
                                   Multiply ─→ Emissive Color
Fresnel(ExponentIn=3.0)          ─┘
```

Fresnel 参数建议：
- Normal = Pixel Normal WS
- ExponentIn：3.0（边缘锐利） / 1.5（柔和），可暴露为 ScalarParameter

### 6.2 WeaponDefinition DA 配置

| 字段 | 位置 | 填写内容 |
|------|------|---------|
| `HeatOverlayMaterial` | Heat 分类 | 上面创建的 Overlay 材质 |
| `ActorsToSpawn` | Equipment 分类 | WeaponInstance 蓝图类 + 插槽名 + Transform |
| `DisplayMesh` | Pickup\|Mesh 分类 | Spawner 展示网格 |

### 6.3 WeaponSpawner 蓝图配置

| 字段 | 位置 | 填写内容 |
|------|------|---------|
| `WeaponDefinition` | 顶层 | 对应的 DA |
| `BlackedOutMaterial` | 顶层 | 拾取后变黑的材质（纯黑不透明即可） |

---

## 七、常见问题

**武器没有发光效果**

1. 确认 DA `HeatOverlayMaterial` 已填
2. Output Log 查 `[WeaponSpawner] 武器已拾取，当前热度阶段=N` — N 若为 0 表示拾取时 Phase=0，属正常（等热度升阶后发光）
3. 确认 `Buff.Status.Heat.Phase.1` tag 被施加：Log 中应有 `[OnTagUpdated] Tag=Buff.Status.Heat.Phase.1 Exists=1`

**切关后发光消失**

检查 `WeaponDefinition::SetupWeaponToCharacter` 是否有 `[WeaponDefinition] 热度委托绑定，当前阶段=N` log。若无，说明旧代码未更新。

**WeaponSpawner Overlap 不触发**

`CollisionVolume` 的 Profile 和 `GenerateOverlapEvents` 在 C++ 构造函数中强制设置，蓝图继承时无需手动配置。若仍不触发，检查角色胶囊的碰撞通道是否与 `OverlapAllDynamic` 兼容。

---

## ⚠️ Claude 编写注意事项

- **WeaponInstance 不是 Actor 的子类**：`WeaponInstance` 是 `UObject`，附着逻辑靠 `AttachToComponent`，不能用 Actor 的 `SetActorLocation` 等接口
- **热度发光走 GAS Tag 事件**：不要轮询热度值，使用 `RegisterGameplayTagEvent(Buff.Status.Heat.Phase.N)` 监听 Tag 变化，NewCount>0 触发 Broadcast
- **切关恢复顺序**：`YogSaveSubsystem::RestoreWeapon` 必须在 `BeginPlay` 之后、`OnHeatPhaseChanged` Delegate 注册之后才调用，否则追赶广播无效
- **HeatOverlayMaterial 不能为 nullptr**：`WeaponInstance` 创建时如果 `WeaponDefinition` 里未填 HeatOverlayMaterial，动态材质创建会 Crash，C++ 里必须判空
- **拾取时销毁旧武器**：`TryPickupWeapon` 执行前必须先 `RemoveDynamic`（旧 WeaponInstance 的 OnHeatPhaseChanged），再 `Destroy` SpawnedActor，顺序不能反
- **WeaponDefinition 软引用**：蒙太奇等大资产用 `TSoftObjectPtr`，运行时按需 LoadSynchronous，不在构造时加载
