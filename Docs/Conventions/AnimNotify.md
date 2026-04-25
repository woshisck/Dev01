# AnimNotify 规范

> Claude 在创建 AN / ANS 类前必须阅读。

---

## 核心规则

1. **单帧事件用 `UAnimNotify`（AN）**，持续区间用 `UAnimNotifyState`（ANS）
2. 获取角色：`Cast<AXxxCharacter>(MeshComp->GetOwner())`，**必须判空**
3. `GetNotifyName_Implementation()` 必须返回有意义的编辑器标签
4. **DisplayName** 用 `meta = (DisplayName = "AN Xxx")` 让编辑器显示友好名称
5. AN/ANS 是纯 C++ 类，**没有 Blueprint 子类**，用户直接在蒙太奇时间轴上拖用

---

## 分工

### Claude 写：
- AN / ANS C++ 类（.h + .cpp）
- 逻辑：调用角色 C++ 接口（不在 Notify 里写复杂逻辑）

### 用户在引擎里做：
- 打开蒙太奇资产
- 在 Notifies 轨道拖入对应 AN / ANS
- 设置 UPROPERTY 参数（如果 AN 暴露了配置项）

---

## 已有 AN / ANS 类速查

| 类名 | 类型 | 用途 | 放置位置 |
|---|---|---|---|
| `UANS_AutoTarget` | ANS | 攻击吸附最近敌人，旋转角色朝向 | 攻击蒙太奇挥击区间 |
| `UANS_AttackRotate` | ANS | 攻击期间持续朝摇杆方向旋转 | 攻击蒙太奇挥击区间 |
| `UANS_PreAttackFlash` | ANS | 触发角色攻击前摇闪红 | 敌人蒙太奇**前摇帧**到命中帧 |
| `UAN_MeleeDamage` | AN | 攻击判定 + 伤害参数 + HitStop 模式 + 命中事件 | 攻击蒙太奇**命中帧**（单帧） |
| `UAN_EnemyComboSection` | AN | 触发敌人下段连击 | 敌人蒙太奇段落衔接点 |

---

## AN 标准模板

```cpp
// Header
UCLASS(meta = (DisplayName = "AN Xxx"))
class DEVKIT_API UAN_Xxx : public UAnimNotify
{
    GENERATED_BODY()
public:
    // 策划可配置参数
    UPROPERTY(EditAnywhere, Category = "Xxx")
    float Duration = 0.05f;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override;
};

// CPP
void UAN_Xxx::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (!MeshComp) return;
    AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
    if (!Character) return;
    // 调用角色接口
}

FString UAN_Xxx::GetNotifyName_Implementation() const
{
    return FString::Printf(TEXT("Xxx %.0fms"), Duration * 1000.f);
}
```

---

## ANS 标准模板

```cpp
// Header
UCLASS(meta = (DisplayName = "ANS Xxx"))
class DEVKIT_API UANS_Xxx : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override;
};

// CPP：NotifyBegin / NotifyEnd 里 Cast 获取角色，调接口
```

---

## AN_HitStop 配置参考值

| 攻击类型 | FrozenDuration | SlowDuration | SlowTimeDilation |
|---|---|---|---|
| 轻攻击 | 50ms | — | — |
| 重攻击 | 80ms | 120ms | 0.25 |
| 暴击/终结 | 60ms | 150ms | 0.20 |
