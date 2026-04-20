# 编辑器配置指南：LevelFlow + Tutorial 弹窗动画 + WeaponSpawner 浮窗

> 对应代码版本：2026-04-19 第二次会话（FEAT-012 / FIX-008 / FIX-009）  
> 前提：代码已编译通过，编辑器已热重载  
> 不需要写任何 C++ 代码，全部在编辑器操作

---

## 概览

| # | 任务 | 类型 | 预计时间 |
|---|------|------|---------|
| 1 | 保存 BP_WeaponSpawner（固化浮窗引用） | 打开+保存 | 1 min |
| 2 | WBP_TutorialPopup 实现渐入渐出动画 | 动画 + 蓝图节点 | 15 min |
| 3 | DialogContentDA 添加 EventID 内容 | 填数据 | 5 min/条 |
| 4 | 创建 DA_LevelEvent_XXX（关卡事件 Flow） | 新建资产 + 连节点 | 10 min |
| 5 | 创建 BP_LevelEventTrigger | 新建蓝图 | 3 min |
| 6 | 关卡中放置触发器并配置 | 拖放 + Details | 3 min |

---

## 任务 1：保存 BP_WeaponSpawner（避免 merge 再次丢引用）

编辑器启动后 C++ 已自动兜底加载 WBP_WeaponFloat（路径硬编码），  
但需要把引用固化进 BP，防止将来路径改变或资产移动导致兜底失效。

1. Content Browser → `Content/Code/Weapon/BP_WeaponSpawner`，双击打开
2. Details 面板 → 分类 **浮窗**
3. 确认 `Weapon Float Widget Class` = `WBP_WeaponFloat`（编辑器加载后应自动显示）
4. 点 **Compile** → **Save**

> 若字段仍为空：手动下拉选择 `WBP_WeaponFloat`，再保存。

---

## 任务 2：WBP_TutorialPopup — 实现渐入渐出动画

C++ 暴露了两个钩子，WBP 需要实现动画并正确调用 `ConfirmClose()`。

### 2.1 创建动画

打开 `WBP_TutorialPopup`，在 **Animations** 面板点 **+ Add**：

| 动画名 | 时长 | 作用对象 | 属性 | 起→终 |
|--------|------|----------|------|-------|
| `Anim_FadeIn` | 0.15s | 根 CanvasPanel 或最外层控件 | Render Opacity | 0 → 1 |
| `Anim_FadeOut` | 0.15s | 同上 | Render Opacity | 1 → 0 |

### 2.2 实现 BP_OnPopupShown（渐入）

Event Graph → 右键搜索 `BP On Popup Shown`（Event）：

```
Event BP_OnPopupShown
    └─→ Play Animation (Anim_FadeIn)
```

### 2.3 实现 BP_OnPopupClosing（渐出 + 关闭）

右键搜索 `BP On Popup Closing`（Event）：

```
Event BP_OnPopupClosing
    └─→ Play Animation (Anim_FadeOut)
            └─→ [绑定 OnAnimationFinished]
                    └─→ Confirm Close   ← 此节点由 C++ 暴露，BlueprintCallable
```

**绑定方式：**
- 在 Play Animation 节点后拖出 Return Value
- 右键 → Bind Event to OnAnimationFinished
- 在 Bind 的事件里调用 `Confirm Close`

> ⚠️ 必须等动画结束再调 `Confirm Close`，不能直接在 Play Animation 后面接，  
> 否则渐出动画未播完弹窗就关闭。

### 2.4 验证

运行游戏靠近武器 → 确认弹窗渐入出现 → 点"知道了"→ 确认渐出后弹窗消失、游戏恢复正常操控。

---

## 任务 3：DialogContentDA — 添加 Tutorial 内容

DialogContentDA 是一个 Data Asset，里面用 EventID 作为 Key，存放弹窗页面内容。

### 3.1 找到或创建 DialogContentDA

- 通常在 `Content/Data/` 或 `Content/Tutorial/` 下
- 若不存在：Content Browser 右键 → Data Asset → 搜索 `DialogContentDA` → 创建

### 3.2 添加 EventID 条目

打开 DA，Details → **Pages Map**（或类似字段），点 **+**：

| EventID（Key，必须与代码一致） | 页数 | 填写内容 |
|-------------------------------|------|---------|
| `WeaponTutorial` | 1+ 页 | 标题 / 正文 / 可选插图 |
| `PostCombatTutorial` | 1+ 页 | 标题 / 正文 / 可选插图 |
| 自定义 EventID | 任意 | 自定义内容 |

**每页字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `Title` | FText | 弹窗标题（粗体大字） |
| `Body` | FText | 正文内容 |
| `SubText` | FText | 次要说明（留空则自动隐藏） |
| `Illustration` | Texture2D | 插图（留空则显示黑色背景） |

### 3.3 确认 HUD 已引用该 DA

打开 `BP_YogHUD` → Details → **Tutorial** → `Dialog Content DA` = 刚才的 DA 资产。

---

## 任务 4：创建关卡事件 Flow（DA_LevelEvent_XXX）

LevelFlowAsset 是关卡触发器使用的 Flow 图谱，和符文 BuffFlow 完全独立。

### 4.1 创建资产

Content Browser 右键 → **Miscellaneous → Data Asset**  
→ 父类搜索 `Level Event Flow`（即 `ULevelFlowAsset`）  
→ 命名建议：`DA_LevelEvent_WeaponIntro`，放在 `Content/LevelFlow/`

### 4.2 打开 Flow 编辑器连线

双击资产打开 Flow 编辑器。右键空白处只会出现 **LevelEvent 分类**（BFNode 不会显示）。

**武器教程触发示例（推荐连法）：**

```
[Start] ──→ [Time Dilation]       ──→ [Show Tutorial Popup] ──→ [Delay] ──→ [End]
              DilationScale: 0.08        EventID: WeaponTutorial   Duration: 0.5
              Duration: 0.35             ↑ OnClosed 引脚接 Delay
```

**节点参数说明：**

| 节点 | 关键字段 | 推荐值 | 说明 |
|------|----------|--------|------|
| Time Dilation | `Dilation Scale` | 0.08 | 慢动作 8%，营造"世界暂停"感 |
| Time Dilation | `Duration` | 0.35 | 真实时间 0.35s 后恢复正常并弹出弹窗 |
| Show Tutorial Popup | `Event ID` | `WeaponTutorial` | 对应 DialogContentDA 里的 Key |
| Delay | `Duration` | 0.5 | 弹窗关闭后延迟 0.5s 再触发 End（可选） |

> **OnClosed 引脚**：Show Tutorial Popup 的 OnClosed 会等玩家关闭弹窗后才触发，  
> 适合在弹窗后接后续关卡事件（对话 / 下一阶段触发 / 解锁门等）。

### 4.3 纯区域触发（无时间膨胀）示例

```
[Start] ──→ [Show Tutorial Popup] ──→ [End]
               EventID: WeaponTutorial
```

---

## 任务 5：创建 BP_LevelEventTrigger

1. Content Browser 右键 → **Blueprint Class**
2. 父类搜索 `LevelEventTrigger`（`ALevelEventTrigger`）
3. 命名 `BP_LevelEventTrigger`，放在 `Content/LevelFlow/`
4. 打开蓝图，无需添加任何节点（逻辑全在 C++）
5. **Compile → Save**

---

## 任务 6：在关卡中放置触发器

### 6.1 放置

将 `BP_LevelEventTrigger` 从 Content Browser 拖入关卡，放到触发位置。

### 6.2 配置 Details

| 字段 | 值 |
|------|----|
| **LevelFlow → Level Flow** | 选择 `DA_LevelEvent_WeaponIntro`（任务 4 创建的） |
| **LevelFlow → bTriggerOnce** | ✅ 勾选（默认开启，玩家只触发一次） |

### 6.3 调整触发范围

选中 `BP_LevelEventTrigger` → 在视口点击 `TriggerVolume`（蓝色 Box）→  
拖拽控制点调整 Box 大小，覆盖你想要的触发区域。

> **范围建议：**  
> 武器区域教程：Box 略大于 WeaponSpawner 的胶囊体（WeaponSpawner 默认半径 80cm），  
> 建议 300×300×200 cm，让玩家"走进区域"时就触发，而不是需要贴近武器。

### 6.4 测试

运行游戏 → 走进 Box 区域 → 确认事件序列按顺序执行（慢动作 → 弹窗渐入 → 游戏暂停）。

---

## 附：触发器常见搭配方案

| 场景 | Flow 连法 | 说明 |
|------|-----------|------|
| 区域进入显示提示 | Start → ShowTutorial → End | 最简，OnClosed 后结束 |
| 区域进入 + 慢动作 | Start → TimeDilation → ShowTutorial → End | 氛围感更强 |
| 依次显示多条信息 | Start → Show(A) → Show(B) → Show(C) → End | OnClosed 串联，逐页展示 |
| 显示后延迟触发下一事件 | …→ ShowTutorial → Delay(2s) → [其他节点] → End | Delay 为真实时间 |

---

## 检查清单

完成以上步骤后，逐项验证：

- [ ] BP_WeaponSpawner 的 WeaponFloatWidgetClass 已设置并保存
- [ ] WBP_TutorialPopup 渐入正常（弹出时有淡入效果）
- [ ] WBP_TutorialPopup 渐出正常（点"知道了"后淡出，游戏控制恢复）
- [ ] 武器浮窗在 Tutorial 弹窗显示期间不出现
- [ ] 武器浮窗在弹窗关闭后恢复显示
- [ ] 武器拾取后浮窗不再显示，Spawner 网格颜色正常（非纯黑）
- [ ] DA_LevelEvent_XXX 中节点连线正确，Flow 编辑器无报错
- [ ] BP_LevelEventTrigger 放置到关卡后触发正常（走进 Box → 事件启动）
- [ ] bTriggerOnce 有效：离开再进入不再触发第二次
