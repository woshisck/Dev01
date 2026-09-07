# Dev01 动作与锁管理方案

日期：2026-09-08

状态：设计提案，未实施、未启用任何新规则
适用范围：玩家、敌人、GAS 能力、输入、移动、转向、AI、对话/剧情临时控制

## 1. 结论与本轮边界

建议采用“框架注册默认契约 + 一个全局策略资产 + ASC 内的锁账本 + 各系统适配器”。继续使用 GAS 管理能力生命周期、OwnedTags、消耗、冷却和取消；不再建一个与 GAS 并行的角色状态机。

策划从一个编辑器入口管理动作准入、block 白名单、资源占用、互斥和中断窗口；程序负责注册动作与资源类型、生命周期接入和校验。一个界面可以有多张视图，但只有一个可编辑的策略来源。运行时必须能回答：“这个动作为什么不能做？谁持有锁？何时取得？等待什么条件释放？”

本轮只设计，不修改 C++、配置、蓝图、数据资产，不编译或发布。暂缓冲刺优先级调整。本文出现的 `Dash > WeaponSkill > Attack` 只是后续待确认、待启用的需求示例，不代表当前规则，也不随框架落地自动生效。Guard 移动问题仅列出可观测性要求，不在此文宣称已定因。

## 2. 当前真实入口与缺口

以下为本轮读取 Git 工作区源文件得到的现状；源码路径均相对于 `X:\Project\Dev01`，不能代替蓝图 CDO、数据资产与实际运行验收。

| 当前入口 | 目前承担的职责 | 统一方案需要补足的内容 |
| --- | --- | --- |
| `Source/DevKit/Private/AbilitySystem/YogAbilitySystemComponent.cpp`：`InitConflictTable` / `ProcessStateConflict` | 按 ActiveTag 精确索引规则，增加/减少 GAS Ability Block，取消匹配能力 | 明确来源、稳定 RuleId、重复规则校验、重入事件排队、按持有者释放 |
| 同文件：`OnTagUpdated` | `Block.Movement`、`Block.AI`、默认受击/击退/死亡移动处理及部分表现反应 | 把副作用移交资源适配器，避免多个入口互相恢复移动或 AI |
| 同文件：`InitTagReactionTable` / `ProcessTagReactions` | 全局+角色表合并，Tag 变化时启动 Flow、施加 GE、激活 GA，记录部分回收句柄 | 保持“反应”与“许可”分离；反应启动的能力也经过准入；嵌套回收不能静默丢失 |
| `Source/DevKit/Private/Character/YogPlayerControllerBase.cpp`：`IsGameplayInputBlocked` / `SetBlockGameInput` / `ToggleInput` | 输入布尔门禁、死亡检查、输入模式和 Enable/DisableInput | 输入所有者令牌、输入组划分；UI 焦点不能充当战斗状态 |
| 同文件：`EnterConversationMode` / `ExitConversationMode` | 幂等 bool，调用输入门禁并遍历当前敌人设置对话 Hold | 与 UI/死亡/受击锁叠加；新生成 AI 接入；退出仅释放自己的锁 |
| `Source/DevKit/Private/Controller/YogAIController.cpp`：`SetConversationHold` | StopMovement，然后 PauseLogic/ResumeLogic；不是世界暂停 | 不直接用一次 Resume 覆盖其他系统仍存在的 AI 锁 |
| `Source/DevKit/Private/Character/YogCharacterBase.cpp`：`BlockMovementControl` / `DisableMovement` / `EnableMovement` | 控制输入与硬停移动，现有受击路径允许动画 Root Motion | 分清控制移动、Root Motion 和物理移动；聚合后恢复而非盲目还原旧状态 |
| `Source/DevKit/Private/AbilitySystem/Abilities/GA_PlayMontage.cpp` 及各 GA | OwnedTags、蒙太奇、成本/冷却、取消回调 | 全部失败/结束出口走生命周期清理；无法接入的 GA 必须被 lint 找出 |
| `Source/DevKit/Private/Character/PlayerCharacterBase.cpp`：`TryActivateEquippedWeaponSkill` | 使用当前装备战技精确 Spec Handle 激活 | 策略只决定资格和中断，不能改回按广义 Tag 随机挑 GA |

当前不是“缺少一个 Priority 数值”这么简单：新动作的许可、旧动作的取消、输入接收、移动限制、AI 暂停是不同问题。StateConflict 的 `Priority` 字段目前没有参与准入裁决；TagReaction 的 Priority 用于反应执行顺序，二者均不能当全局动作等级。

需要设计覆盖的现有风险包括：取消能力同步触发 Tag 删除时，递归保护直接 return 会漏处理后续解锁；某些激活失败出口未结束能力；对话退出直接解除输入/恢复 AI 无法表达其他持有者仍在阻断。这些是方案要覆盖的代码路径，实际发生频率仍需运行验证。

## 3. 三种概念必须分开

| 概念 | 回答的问题 | 表达方式 |
| --- | --- | --- |
| 能力/动作身份 | “请求执行的是谁？” | `ActionId` 枚举或稳定 FName、GA 类、精确 Spec Handle；可附已有 AbilityTags 作为筛选条件 |
| 当前动作/效果状态 | “角色现在发生什么？” | GAS ActivationOwnedTags、GE Tags，例如 `Character.State.Skill.Attack`、`Character.State.Movement.Dash`、`Buff.HitReact` |
| 阻断资源 | “现在不允许哪一种控制？” | 注册的 ResourceId，例如 Movement.Control、Movement.RootMotion、Rotation.Control、Input.Combat、AI.Decision；名称是提案字段，不自动新增 GameplayTag |

保留现有正式命名 `Character.State.Skill.Attack.Combo1` 至 `.Combo4`；不改成 `.Attack.1`，不新增 `Combat.State.*`，不重新引入 LightAttack/HeavyAttack/SpecialAttack 正式入口。Combo 子标签是动作阶段/蒙太奇映射，不等于另一个全局动作身份。

既有 `Block.Movement` / `Block.AI` 可以在迁移期间作为只读兼容投影，但 Tag 本身不保存 owner，不能作为锁账本。`PlayerState.*`、`Buff.Status.*` 通过适配/正式迁移处理，不因为命名旧就直接删除。

## 4. 架构与唯一配置来源

建议的名字均为拟新增接口，不是已存在功能。

1. **框架注册表**：注册四个玩家动作、敌人动作族、系统持有者（对话/UI/死亡等）、资源类型和生命周期适配器。基础类注册稳定的默认契约，包含动作所属资源域、受管范围和默认准入策略。注册不得从字符串相似性推断优先级。
2. **全局策略资产 `UActionLockPolicyDataAsset`**：由项目配置只指定一份。内含动作 Profile、状态到锁 Profile、准入规则、定向中断规则和窗口契约。它是唯一业务编辑来源，角色/武器 DA 只引用其中的 ProfileId，不再复制一份 block/cancel 表。
3. **编辑器策略页**：同一资产显示为“动作矩阵”“资源白名单”“中断窗口”“默认值与覆盖差异”四个视图。框架默认行只读可见，显式覆盖写在本资产；编译成一个确定版本的有效策略，不在运行时临时拼接多份覆盖表。
4. **ASC 锁服务**：可实现为 `UYogAbilitySystemComponent` 内部对象，保存当前角色的 Token、能力激活事务和决策记录。锁是生命周期元数据，不负责代替 GAS 播放技能或维护另一份攻击状态。
5. **系统适配器**：输入、CharacterMovement、转向、AI Brain、PathFollowing 只执行聚合后的结果。对话/剧情拥有世界域会话，向参与者 ASC/Controller 申请令牌；没有 ASC 的 UI/Controller 使用同一令牌协议的适配端，不另设规则表。

### 4.1 框架默认注册不等于默认放行

- 新受管动作必须有注册 Profile，明确资源声明和默认策略；缺 Profile 返回 `UnregisteredAction`，开发环境报错，受管发布模式拒绝启动。
- 正式模式每个受管请求必须命中显式规则，或命中该 Profile 已声明的基础规则；没有规则返回 `NoAdmissionRule`。编辑器矩阵中空白表示配置缺失，不表示 Allow。
- 基础规则可显式声明“Idle 时允许”“无冲突的被动效果允许共存”，减少手填组合；这些也是可查看、可测试的命名规则，不是隐藏的 fail-open。
- 迁移初期的 Legacy/Observe 模式可以保留旧行为，但必须显示 `LegacyFallback` 及未覆盖项；它是受控过渡模式，不是最终默认规则。
- 玩家/敌人公共契约复用；身份差异用 Profile 条件。禁止每个角色蓝图私有重写全局规则。

### 4.2 建议的表格字段

| 区域 | 最小字段 |
| --- | --- |
| Action Profile | ProfileId、ActionId、GA 类约束、请求来源（Input/AI/Event/Script）、资源声明、默认准入、支持的实例化/取消模型 |
| 状态/系统锁 Profile | ProfileId、来源状态或系统会话、ResourceId、限制方式、该锁的 AllowList、释放条件、是否允许被特定中断规则解除 |
| 准入 | RuleId、请求 Action/Profile、当前状态查询、目标/角色过滤、所需窗口、Allow/Deny/Buffer、原因码 |
| 中断 | RuleId、发起 Action/Profile、目标 Action/Profile、允许窗口、CanCancel 前提、取消对象解析方式、超时策略 |
| 窗口 | WindowId、允许请求集合、作用域、montage/activation owner、开启/关闭来源、兜底关闭时机 |
| 管理 | 版本、启用状态、负责人、变更说明、关联回归用例、迁移来源 |

业务优先级不用一个万能 Priority。规则选择采用可审计的精确覆盖关系：注册默认值 → 全局资产中对指定默认 RuleId 的显式替代。若两个有效规则在同一条件下互相矛盾，lint 报错，而不是悄悄取数值更大者；需要组合条件时写成一条明确规则。

## 5. 裁决语义：准入、白名单和取消

### 5.1 Allow 不等于 Cancel

- **Allow**：允许进入后续检查，不保证已成功激活，也不取消任何已有能力。
- **Deny**：拒绝本次启动，保留当前能力与锁，不扣成本、不启动冷却、不播放新蒙太奇。
- **Buffer**：记录有时限的意图，尚未激活；窗口到来时重新检查全部条件，不能复用旧的 Allow。
- **Cancel**：对已解析的能力实例/Spec 发起取消，是有副作用的生命周期操作。必须由明确的定向中断规则授权，并检查目标当前可取消。
- **Resource AllowList**：仅说明“这一个锁不阻止这些请求”；不是豁免所有锁。例如对话允许 UI.Confirm，不会使死亡锁允许 Attack。

一个请求只有同时满足以下条件才能启动：已注册，所有适用硬性前提成立，存在明确准入授权，所有尚未解除的资源锁均允许该请求或可按已授权中断事务处理，GAS 成本/冷却/标签等检查通过。任一不可解除锁拒绝就拒绝。不能用一个 Allow 覆盖另一 owner 的 Deny。

技能自己的成本、冷却、目标有效性继续由 GAS/技能实现负责。全局表无权“Allow 后忽略 GAS 冷却”。持续 Buff 也不因带 `Buff.*` 自动被当成受保护动作；保护范围应按 Profile 及能力生命周期明确配置。

### 5.2 中断窗口不能跨技能串用

窗口令牌绑定 `Owner + ActivationSerial + MontageInstance/Phase`。沿用 CanCombo/JustCombo 等现有正式 Tag 为兼容显示，但裁决使用当前 owner 的窗口记录；旧攻击残留的窗口不能解锁新战技。蒙太奇中断、跳段、能力结束、Spec 移除均关闭其窗口；NotifyEnd 丢失不应永久留窗。

输入的 Started/Triggered 与 Completed/Released/Cancelled 分流。锁可以拒绝新的 Attack，却必须让已经按下的蓄力技能收到 Release/Cancel，或在锁取得时执行明确的输入终止清理。禁止把所有 Release 一起 return，造成“永远按住”。Buffer 同样记录原始输入序列、过期时间和消费状态，切场景/死亡/取消会话时按策略清理。

### 5.3 待后续启用的示例矩阵

下表只演示如何表达此前提出的需求，全部标为 `Proposed/Disabled`，不用于本次框架迁移的行为基线。行是**新请求**，列是**正在执行的动作**。

| 新请求 \ 当前动作 | Attack | WeaponSkill | Dash |
| --- | --- | --- | --- |
| Attack | 使用已确认的连段/窗口规则，待核定 | Deny，待核定 | Deny：后续需求 |
| WeaponSkill | Allow + 定向 Cancel Attack：后续需求 | 精确战技内部阶段规则，待核定 | Deny：后续需求 |
| Dash | Allow + 定向 Cancel Attack：后续需求 | 是否允许打断战技及窗口，待核定 | 是否重入/消耗充能，待核定 |

因此“Dash > WeaponSkill > Attack”也不能直接推导每一格：例如“冲刺是否打断战技”、充能重入、不可取消的演出段、主动 Skill 的关系仍需要单独规则。当前目标只是准备能表达这些关系的框架。

## 6. 锁账本与资源聚合

### 6.1 每把锁都有身份

令牌建议字段：`TokenId`、`Subject`、`OwnerWeakPtr`、`OwnerGeneration`、`SourceKind`、`RuleId/ProfileId`、`AbilitySpecHandle`、`ActivationSerial`、`ResourceId`、`AcquiredAt`、`ReleaseCondition`、`Reason`、`CorrelationId`、`PolicyVersion`。

- SpecHandle 本身不足以区分同一技能的多次激活，必须加 ActivationSerial；武器切换不能让旧回调释放新技能的锁。
- 同一 owner 对同一业务操作重复 Acquire，使用稳定 RequestKey 返回已有令牌；真正独立的并行占用生成不同令牌。
- `Release(Token)`幂等；重复释放记录诊断但不使计数变负。禁止 `ReleaseAll(Resource)` 作为日常解锁接口。
- 资源计数由活跃令牌集合推导。最后一个令牌释放时才改变底层状态；不能在“某个 Tag 消失”时假定资源完全空闲。
- 生命周期结束释放该激活拥有的令牌、窗口和输入占用；不能无差别删掉共享 GameplayTag 的全部计数。
- 监听 EndAbility、SpecRemoved、ActorEndPlay、UnPossess、会话结束、WorldCleanup；另做轻量失效 owner 扫描。有效 owner 的长锁仅告警，不按随意超时自动解锁死亡或剧情。

### 6.2 资源分层

| 资源 | 实际限制 | 不应误伤 |
| --- | --- | --- |
| Input.Combat / Input.Move / Input.Camera | 是否接受新的对应输入意图 | UI 确认/取消与已开始输入的 Release |
| Movement.Control | 玩家/AI 控制加速度或路径驱动 | 明确允许的蒙太奇 Root Motion、击退、平台移动 |
| Movement.RootMotion | 是否接收动作 Root Motion | 普通控制移动是否可用由独立锁决定 |
| Movement.Physical | 特定模式的硬停/冻结 | 不应由普通攻击锁顺手关闭物理；死亡等单独声明 |
| Rotation.Control / Rotation.Action | 输入/AI 朝向和动作定向各自的权限 | 摄像机与角色身体旋转不混为一谈 |
| AI.Decision | 是否允许 Brain/StateTree 发出新决策 | 世界 Tick、动画、VFX |
| AI.PathFollowing | 是否继续当前路径请求 | 决策暂停与已有攻击是否结束需另外声明 |
| Ability.ExecutionDomain | 同一动作域的互斥租约 | 非冲突的持续 Buff 和表现任务 |

底层 `SetIgnoreMoveInput`、DisableMovement、PauseLogic 等只由适配器按聚合状态变化执行，不能每帧重复加锁/减锁。对不同物理限制使用明确的合成规则（如“禁止控制”和“硬停”不同），不是按动作 Priority 谁高谁赢。

恢复时重新评估活着/复活等待、移动模式、控制器、其他锁及最新物理状态。不无条件恢复 `MOVE_Walking`、清空所有 IgnoreInput 计数，或用过期的“加锁前快照”覆盖后来发生的掉落/击退。迁移期间底层存在旧调用者时要记录 LegacyOwner；无法可靠归属的调用列为迁移阻塞项，不伪造归属。

### 6.3 Conversation Mode 的组合规则

保留 BlueprintCallable 的 `EnterConversationMode/ExitConversationMode` 入口及同一会话重复进入/退出无副作用的体验；内部映射到 `ConversationSession` 令牌。新接口可显式携带 owner/session handle，旧接口以 Controller 的 legacy session 做兼容，不能把两个独立对话拥有者合成一个 bool。

会话取得玩家输入锁、敌人决策与路径锁；不调用世界暂停。已有蒙太奇/投射物/伤害是否继续是单独待确认的演出策略，不把 PauseLogic 误当成取消已激活 GA。默认迁移保持现有动画与 VFX 继续运行的语义。

退出只释放当前会话令牌：若背包 UI 仍锁住输入，输入仍受限；若敌人还受击/死亡，AI 不恢复。会话期间新增/重生/重新 Possess 的 AI 也获得相应锁；销毁/离场的参与者安全释放。多人或多本地玩家场景需明确会话影响集合，不能默认一个 Controller 的对话锁住所有玩家。

## 7. 激活事务与取消重入

### 7.1 建议流程

1. **解析请求**：得到 ActionProfile、精确目标 Spec/实例、目标角色、输入来源及请求序号。装备战技继续走 `TryActivateEquippedWeaponSkill` 的当前 Handle；替换装备后旧请求失效。
2. **无副作用预检**：检查规则版本、窗口归属、资源账本、GAS 可激活条件和可取消目标。生成 Decision/CancelPlan，但不取消旧技能、不扣消耗。
3. **临时预留**：为本次事务取得域预留，防止同帧两个相互冲突的请求都看到“空闲”。预留不是 OwnedTag，不对 HUD 宣称技能已经执行。
4. **授权取消并等待确认**：仅取消 CancelPlan 指定的激活；其 EndAbility/令牌释放确认后推进。拒绝取消、异步取消超时或目标版本改变都中止新请求，不绕过旧锁播放新蒙太奇。
5. **最终重验与 GAS 激活**：重查 Spec、窗口、存活、成本/冷却和资源；调用 GAS。PreActivate/Commit/Activate 的接入点必须形成显式成功/失败/同步完成回执，不能把 `TryActivateAbility == true` 等同于整个技能已提交成功。
6. **确认或补偿**：成功后预留转为正式租约；失败调用正确的 EndAbility 清理与本事务 Release，清除临时窗口/资源。瞬时能力在同一调用栈完成也是合法完成，不能被当作“失去活动状态”失败。

### 7.2 回滚保证的边界

能保证的是**本事务新增锁、临时状态和未完成资源预留可回滚**，不是把已取消攻击的蒙太奇、已发生伤害、投射物或已广播事件倒放。取消旧动作属于不可逆边界；越过后新能力仍可能因外部状态变化失败，此时结果是“旧动作已取消、新动作未启动、双方不遗留锁”，必须给出明确原因并记录。

成本/冷却是否补偿由各 GA 的提交契约决定，不能全局盲目退还或删除所有冷却 GE。工程实现前需为受管 GA 定义“可预检、真正提交、终止”的接入契约；无法拆分的旧能力先留在 Legacy 模式。框架不得承诺仅包装一次 TryActivate 就获得完整原子事务。

### 7.3 重入事件不能丢弃

当前“正在处理就 return”的递归保护应在后续实现中替换为有序队列：TagEdge、AbilityEnded、TokenReleased、ReactionStarted/Stopped、OwnerInvalidated 均携带事件序号、owner/activation 与策略版本。外层阶段提交后排空，再做下一轮裁决。

- 同一逻辑事件可幂等去重，但不能把不同激活的添加/删除合并为一个布尔值。
- 先完成阻断账本更新，再执行 TagReaction 启动行为；Reaction 激活 GA 一样经过统一准入。
- 保持每条锁 Acquire/Release 配对。GE/Flow 有自己的句柄，清理只能撤销该记录所拥有的对象。
- 环路检测使用因果链、每帧处理预算和重复规则签名；超过预算保留队列并告警/拒绝该因果链的新激活，不能直接丢失必要的 Release。
- 清理阶段优先保证资源释放；异步回调需检查 ActivationSerial/OwnerGeneration，过期回调记录并忽略，不碰新持有者。

## 8. GAS、蓝图与 TagReaction 的边界

GAS 始终是 GA 激活/结束、成本、冷却、OwnedTags 和网络生命周期的权威。新服务只在能力请求及生命周期钩子上做准入与资源租约，不能同时维护 `CurrentAttackState` 去替代 GAS 的活动实例。

- 原生与蓝图 GA 通过公共基础类/ASC 接口接入；提供 Blueprint 可调用的 RequestAction、AcquireScopedLock、ReleaseLock、ExplainDecision。蓝图传 owner handle，不能只传 bool 解锁。
- `GiveAbility`/Spec 注册时检查 Profile；纯蓝图 GA、第三方非项目基类 GA、直接事件触发 GA 都要列入覆盖扫描，不能只管 Controller 四个函数。
- AbilityTags 的兼容匹配用于解析与迁移；最终执行/取消定位到 Spec+Activation，不随意用广义 `Character.State.Skill` 取消整类能力。
- 当前武器专属 GA 保留精确 Spec 与 SourceObject DA；Profile 不得替换武器值、蒙太奇选择和卡牌结算逻辑。
- GAS 的 ActivationBlockedTags/RequiredTags、成本和冷却仍是约束。迁移后需要明确哪些 block 由全局策略拥有，哪些是技能本身的硬前提；相同业务规则不得在两边各计一次锁。
- TagReaction 继续负责“出现状态后产生什么效果”，不负责“谁有权开始”。对 Flow/GE 的 Auto/Persist 回收策略保留；Persist 仅表示效果生命周期独立，不代表永久持有动作锁。
- 若有网络预测，正式锁以服务端权威生命周期为准，预测租约与 PredictionKey/ActivationSerial 绑定；拒绝预测释放本地租约。当前设计不假设项目已经完成多人同步。

## 9. 编辑器与运行时可观察性

### 9.1 编辑器

动作矩阵同时显示 Allow/Deny/Buffer 与独立 Cancel 箭头；点击单元格可查看命中 RuleId、资源锁、窗口和继承来源。未知配置显示红色“未定义”，不是空白。支持按玩家/敌人 Profile、输入来源和阶段过滤。

资源页显示“持锁原因 → 被阻断动作 → 该锁允许列表”。默认行、覆盖行和最终有效结果并排显示；保存前 lint，导出稳定排序的文本快照用于 Git/P4 diff。修改者能看到受影响的动作和必须重跑的用例。

### 9.2 运行时

提供选中角色的只读 Debug 面板/控制台查询：

| 字段 | 示例含义 |
| --- | --- |
| 请求与裁决 | WeaponSkill / Deny / ResourceDenied |
| 命中规则 | RuleId、PolicyVersion、具体条件值 |
| 谁锁住 | Owner 名称与路径、GA Spec、ActivationSerial、TokenId |
| 锁住什么 | Input.Combat、AI.Decision 等资源及允许列表 |
| 从何时开始 | 游戏时间、持续时间、取得调用来源 |
| 为什么未释放 | 等待 Montage End/Ability End/Session Exit，或 owner 已失效 |
| 取消状态 | 计划目标、已请求/已确认/拒绝/超时 |

按 CorrelationId 记录小型环形时间线：请求 → 决策 → 预留 → Cancel → End → Release → Activate/Fail。默认不每帧刷日志，可按角色/失败原因开启采样。正常重复Release可统计，负计数、孤儿锁、过期回调命中新owner、准入覆盖缺失必须显著告警。

## 10. Guard 的易懂解释与采样边界

当前敌人主链路已使用 StateTree。`YogStateTreeTasks.cpp` 的 EnemyCombatMove 会在目标变化或重算间隔到期时发出移动请求，进入攻击范围/退出该状态时停止移动；`YogAIController.cpp` 的计算中仍保留稳定战斗站位与转向诊断信息。

可这样理解：**移动请求像给导航换目的地，移动锁像暂时不准司机踩油门。** 车一顿一顿可能是反复换目的地、抵达半径过大、攻击距离来回越界、AI状态切换，也可能是移动/AI锁不断取得释放。仅看到 MoveTo 重发或一个未采用的平滑计算结果，不能断言就是根因。

新锁面板需与现有 EnemyMoveSmooth 采样按时间关联：速度、PathStatus、请求间隔、目标变化、实际攻击范围、StateTree状态、所有 Movement/AI Token。若速度下降时没有锁变化，就继续查导航/状态切换；若有锁变化，查看 owner与RuleId。本文不修改 Guard 调参，也不直接启用平滑目标计算的返回值。

## 11. lint 与验收清单

### 11.1 静态/资产检查

- 动作/资源/ProfileId/RuleId唯一；引用有效；受管能力和请求来源都有覆盖；未定义矩阵单元不能发布。
- 重叠且冲突的规则报错；AllowList不能越过其他owner锁；默认覆盖必须指出被覆盖RuleId。
- 中断规则有明确方向、合法目标和窗口；循环关系要求专门用例，不能仅靠任意Priority解决。
- 正式Tag可解析；区分Exact/ParentMatch，禁止未经声明用父Tag扩大取消目标。
- GA生命周期覆盖成功、失败、取消、同步完成、Spec移除、蓝图终止；每个锁Profile有释放条件。
- 扫描直接Enable/DisableMovement、SetIgnoreInput、Pause/ResumeLogic、CancelAbilities等调用；适配器之外允许项必须有可审计理由。
- 同一来源不得同时经新旧路径重复加锁；窗口绑定正确激活；派生蓝图的覆盖差异可导出审查。

### 11.2 必须自动化或明确手测的回归

| 用例 | 预期 |
| --- | --- |
| UI锁+Conversation锁，先后任意释放 | 最后一个相关owner释放才恢复对应资源 |
| HitReact+Knockback+Dead叠加 | 任一结束不提前恢复；死亡状态不恢复AI或步行 |
| 激活预检/Commit失败、蒙太奇缺失、同步结束 | 无孤儿令牌/OwnedTags/窗口；不误清其他技能冷却 |
| A取消B，B.End产生TagReaction或启动C | 事件排队且配对完整；不会丢B解锁，不会越权启动C |
| 同一Spec连续重触发、装备替换后旧回调 | 旧ActivationSerial只能释放旧令牌 |
| Blueprint GA直接激活、GameplayEvent和AI激活 | 与输入触发同样受管，不存在旁路 |
| 蓄力期间打开UI/对话/死亡 | 收到Release/Cancel或显式清理，不永久保持按下 |
| 同帧两个互斥请求 | 只有符合稳定裁决规则的请求获得预留 |
| 窗口NotifyEnd丢失、蒙太奇跳段/中断 | 随owner结束关闭，不影响下一次激活 |
| 对话期间新生成AI、重Possess、世界切换 | 新参与者继承会话限制；销毁清理不碰新角色 |
| Actor销毁、Spec移除、异常异步取消超时 | 释放所属资源，保留可审计失败记录 |
| 无Profile、无规则、策略冲突 | 正式受管模式拒绝，开发环境明确告警 |
| Guard追逐/进出射程/受击/对话交叉 | 能用时间线区分导航停顿和锁停顿；不先承诺调参改善 |

动作优先级示例只有在用户另行确认启用后才加入行为验收；框架迁移首先验证原行为等价与锁生命周期可靠性。

## 12. 分阶段迁移和回滚

| 阶段 | 工作 | 放行标准 |
| --- | --- | --- |
| 0 盘点 | 导出实际GA CDO、StateConflict、TagReaction、输入/AI调用；列出全部直接控制点 | 当前行为与资产来源可追踪；只读报告，无重写资产 |
| 1 观察 | 注册表、账本事件和ExplainDecision，模式Observe；旧系统仍唯一执行 | 不改变玩法；未知/重复来源可见，基线日志可复现 |
| 2 收口资源 | 先把UI/Conversation/AI/移动恢复改为owner令牌适配，按域逐一接管 | 多owner交错、销毁及对话回归通过；每个域只有一个实际执行者 |
| 3 接入GA | 逐类接入请求、预留、提交回执、取消与结束清理，先保留当前规则 | 所有失败出口/重入/精确战技Spec测试通过 |
| 4 策略资产 | 把旧表导入候选全局资产，dry-run diff；明确每条默认/覆盖和兼容映射 | 审核现有资产覆盖，lint零阻塞，观察模式差异解释完毕 |
| 5 受管发布 | 按批准Profile开启Enforce，完成本地回归、项目构建与同事端验证 | 未定义不放行；无孤儿锁；不夹带待定冲刺行为改变 |
| 6 清理 | 已迁移旧入口只作委托/兼容；旧Tag按项目正式迁移工具重保存 | 引用审计、可回滚包和发布记录齐全，再移除旧规则 |

拟提供 `Legacy / Observe / Enforce` 模式与资源域/ActionProfile迁移开关。开关切换需要安全点：拒绝新请求、结束/排空在途事务、释放或显式转移新系统持有的租约，再重建兼容状态；不能在攻击正执行时直接换bool留下无人拥有的锁。重大回滚优先通过受控重启/关卡重载，并保留上一版策略资产和模块。

新旧系统可以同时**计算并对比**，不能同时执行同一资源的block/unblock。保存到存档的是玩法结果，不保存瞬态Token或SpecHandle；载入后由有效状态与系统会话重新取得租约。回滚不能删除用户资产、旧兼容Tag或 unrelated opened 文件。

实现前须再次检查 Git状态、P4 opened/pending/resolve，单独划定代码与资产范围。只涉及项目代码/插件时走项目编译与PCB流程；本方案不要求修改或重新发布引擎，更不涉及角色渲染功能。

## 13. 实施前待确认项

1. 第一批受管范围建议为 Conversation/UI 输入、AI Hold、Movement恢复，还是先从GA失败清理开始；两者都先保持当前动作准入行为。
2. 对话期间已经激活的攻击、投射物和伤害是否继续：现有“不暂停世界”不自动给出这几个答案。
3. 技能的可取消阶段、Buffer时限与失败成本补偿契约需要逐Profile批准。
4. 冲刺、攻击、战技、主动Skill的最终定向矩阵另开行为变更验收；本文只提供表达框架。

建议先评审数据结构、准入语义和owner/token协议，再开始编码。若仅补几条 `CancelTags` 或在多个地方添加新的bool，会继续扩大目前难以解释和安全释放的问题。
