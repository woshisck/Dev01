#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "Data/EnemyData.h"   // FBuffEntry
#include "Portal.generated.h"

class APlayerCharacterBase;
class UBillboardComponent;
class UBoxComponent;
class UNiagaraComponent;
class UStaticMeshComponent;
class UYogSaveSubsystem;
class URoomDataAsset;
class URuneDataAsset;

/**
 * FPortalPreviewInfo —— 单个传送门提供给 HUD 浮窗 / 方位指引的预览数据。
 * 由 APortal::BuildPreviewInfo 在 Open() 时一次性构建并缓存。
 *
 * 注意：字段直接保存 FBuffEntry（而非 URuneDataAsset*），避免丢失 DifficultyScore 等元数据；
 * 也使 TryEnter 时能直接拷贝 PreRolledBuffs 到 GI->PendingRoomBuffs，无需额外转换。
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FPortalPreviewInfo
{
    GENERATED_BODY()

    /** 玩家可见的房间名（DisplayName 兜底为 RoomName.ToString()） */
    UPROPERTY(BlueprintReadOnly)
    FText RoomDisplayName;

    /** 关卡资产名，用于调试与 fallback */
    UPROPERTY(BlueprintReadOnly)
    FName RoomLevelName;

    /** 房间类型 Tag（Room.Type.Normal/Elite/Shop/Event），UI 用于徽章颜色 */
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag RoomTypeTag;

    /** 已确定要施加给下一关敌人的 Buff 列表（与 GI->PendingRoomBuffs 同型） */
    UPROPERTY(BlueprintReadOnly)
    TArray<FBuffEntry> PreRolledBuffs;

    /** 战利品个数（当前固定 3） */
    UPROPERTY(BlueprintReadOnly)
    int32 LootCount = 3;
};

/**
 * 传送门单个状态的美术配置。
 * 在蓝图 Details 面板里填写资产即可，无需写蓝图逻辑。
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FPortalArtConfig
{
	GENERATED_BODY()

	// 门的静态网格体
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UStaticMesh> Mesh;

	// 开启瞬间播放一次的特效（激活后自动结束）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UNiagaraSystem> OpenVFX;

	// 开启后持续循环的特效（待机状态）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
	TObjectPtr<UNiagaraSystem> IdleVFX;
};

UCLASS()
class DEVKIT_API APortal : public AActor
{
	GENERATED_BODY()

public:

	APortal(const FObjectInitializer& ObjectInitializer);

	/**
	 * 开启传送门：C++ 根据 SelectedLevel 查 DestinationArtMap 自动切换美术。
	 * 蓝图可 override 追加表现（如相机抖动、声音等），但无需实现美术切换逻辑。
	 */
	UFUNCTION(BlueprintNativeEvent)
	void EnablePortal();
	virtual void EnablePortal_Implementation();

	/**
	 * 关闭传送门（关卡开始时）：C++ 自动应用 ClosedArt。
	 */
	UFUNCTION(BlueprintNativeEvent)
	void DisablePortal();
	virtual void DisablePortal_Implementation();

	/**
	 * 关卡开始时由 GameMode 确定该门永不开启：C++ 应用 NeverOpenArt 并禁用碰撞。
	 */
	UFUNCTION(BlueprintNativeEvent)
	void NeverOpen();
	virtual void NeverOpen_Implementation();

	// GameMode 在关卡结束时调用，分配目标关卡 / 房间配置 / 已预骰的关卡 Buff 列表并开启门。
	// PreRolledBuffs 与 GI->PendingRoomBuffs 同型；本门各自缓存，仅在玩家确认进入时由 TryEnter 写入 GI。
	UFUNCTION(BlueprintCallable)
	void Open(FName InSelectedLevel, URoomDataAsset* InSelectedRoom,
	          const TArray<FBuffEntry>& InPreRolledBuffs);

	// 直接通过名字切换关卡（保留旧接口，BP 可调）
	UFUNCTION(BlueprintCallable)
	void YogOpenLevel(FName LevelName);

protected:
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintNativeEvent)
	void EnterPortal(APlayerCharacterBase* ReceivingChar, UYogSaveSubsystem* SaveSubsystem);

	/**
	 * 玩家按 E 时由 PlayerController::Interact 调用。v3：替代旧的 Overlap 自动入门。
	 * B 阶段最小实现：直接走 EnterPortal 流程；C 阶段升级为带角色自走 + 渐黑过场的完整序列。
	 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void TryEnter(APlayerCharacterBase* Player);

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// =========================================================
	// 视觉强化 BP 接口（C++ 不实现，BP_Portal 自由 override）
	// 全部加 K2_ 前缀，避免与 C++ 内部函数命名冲突
	// =========================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|FX", meta = (DisplayName = "On Highlight Changed"))
	void K2_OnHighlightChanged(bool bHighlighted);

	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|FX", meta = (DisplayName = "On Portal Range Entered"))
	void K2_OnPortalRangeEntered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|FX", meta = (DisplayName = "On Portal Range Exited"))
	void K2_OnPortalRangeExited();

	UFUNCTION(BlueprintImplementableEvent, Category = "Portal|FX", meta = (DisplayName = "On Entry Sequence Start"))
	void K2_OnEntrySequenceStart();

	// 场景中唯一标识（与 CampaignData.PortalDestinations[i].PortalIndex 对应）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	int32 Index = 0;

	// GameMode 写入的目标关卡名（关卡结束时由随机池选定）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FName SelectedLevel;

	// GameMode 写入的下一关房间配置（骰子决定类型后从类型池中选定）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<URoomDataAsset> SelectedRoom;

	// 是否已开启（BeginPlay 时为 false，GameMode 调 Open() 后变 true）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bIsOpen = false;

	// 关卡开始时确定永不开启（未登记在 PortalDestinations 中）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bWillNeverOpen = false;

	// =========================================================
	// 预骰子的关卡 Buff（玩家确认进入时由 TryEnter 拷贝到 GI->PendingRoomBuffs）
	// =========================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Preview")
	TArray<FBuffEntry> PreRolledBuffs;

	// HUD 单例浮窗 / 方位指引读取的预览数据（由 BuildPreviewInfo 在 Open 时构建）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Preview")
	FPortalPreviewInfo CachedPreviewInfo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> CollisionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UBillboardComponent> BillBoard;

	// =========================================================
	// 美术组件（C++ 驱动，蓝图只需在 Details 填入资产）
	// =========================================================

	// 门体网格。DisablePortal/EnablePortal 时由 C++ 自动切换资产。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	// 门开启瞬间的一次性特效（播完自动停止）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	TObjectPtr<UNiagaraComponent> OpenVFXComp;

	// 门开启后的持续待机特效
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Components")
	TObjectPtr<UNiagaraComponent> IdleVFXComp;

	// =========================================================
	// 美术配置（在蓝图 Details 面板填写即可）
	// =========================================================

	// 关卡开始时（关闭状态）的美术
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Art")
	FPortalArtConfig ClosedArt;

	// 永不开启时的美术（留空则直接隐藏网格）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Art")
	FPortalArtConfig NeverOpenArt;

	/**
	 * 目标关卡 → 对应美术配置。
	 * Key 填关卡资产名称（FName），如 "L1_Forest"、"L1_Desert"。
	 * GameMode 调用 Open() 时已确定 SelectedLevel，EnablePortal 会自动查此 Map。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Art")
	TMap<FName, FPortalArtConfig> DestinationArtMap;

public:
	// === 入门过场配置（在 BP_Portal Details 面板可调）===

	/** 角色自走入门时长（秒）。0.7 默认；用户可调。不必抵达门位置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Entry")
	float PortalEntryWalkDuration = 0.7f;

	/** 兜底超时余量（秒）。WalkDuration + 此值后强制切关，防卡墙 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Entry")
	float PortalEntryFailSafeBuffer = 0.5f;

private:
	// 统一应用一套 FPortalArtConfig：切换网格、激活/停止特效
	void ApplyArtConfig(const FPortalArtConfig& Config, bool bPlayOpenVFX = false);

	// 根据 SelectedRoom + PreRolledBuffs 填充 CachedPreviewInfo（Open 时调用）
	void BuildPreviewInfo();

	// Overlap 内部处理函数（与 BP 钩 K2_* 区分命名）
	void HandlePlayerEnterRange(APlayerCharacterBase* Player);
	void HandlePlayerExitRange(APlayerCharacterBase* Player);

	// === Entry 过场状态机 ===
	void TickEntryMovement();   // Timer 回调：每帧驱动玩家走向门
	void FinishEntry();         // Walk 结束 / 兜底超时：写 GI + TransitionToLevel
	void AbortEntry(const TCHAR* Reason);  // 异常分支：恢复输入 + 解锁背包

	// 防重入：TryEnter 触发后置 true，本 actor 销毁前不再接受第二次按 E
	bool bEntryInProgress = false;

	FTimerHandle EntryWalkTickTimer;
	FTimerHandle EntryFinishTimer;
	TWeakObjectPtr<APlayerCharacterBase> EntryPlayer;
};
