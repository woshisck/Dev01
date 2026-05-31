#include "Map/Portal.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/BackpackGridComponent.h"
#include "NiagaraComponent.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "UI/YogHUD.h"

namespace
{
	FText GetPortalRewardTypeDisplayName(ELootType LootType)
	{
		switch (LootType)
		{
		case ELootType::Gold:
			return NSLOCTEXT("Portal", "PreviewRewardGold", "金币");
		case ELootType::Rune:
			return NSLOCTEXT("Portal", "PreviewRewardCard", "卡牌");
		case ELootType::Material:
		default:
			return NSLOCTEXT("Portal", "PreviewRewardMaterial", "材料");
		}
	}

	FLootOption MakePortalRewardTypePreviewOption(ELootType LootType)
	{
		FLootOption Option;
		Option.LootType = LootType;
		Option.DisplayName = GetPortalRewardTypeDisplayName(LootType);
		return Option;
	}

	FString DescribePortalEnumValueForRewardDebug(const UEnum* Enum, int64 Value)
	{
		return Enum ? Enum->GetNameStringByValue(Value) : FString::Printf(TEXT("%lld"), Value);
	}

	FString DescribePortalLootOptionsForRewardDebug(const TArray<FLootOption>& Options)
	{
		if (Options.IsEmpty())
		{
			return TEXT("Count=0 []");
		}

		TArray<FString> Parts;
		Parts.Reserve(Options.Num());
		for (int32 Index = 0; Index < Options.Num(); ++Index)
		{
			const FLootOption& Option = Options[Index];
			Parts.Add(FString::Printf(
				TEXT("#%d{Type=%s,Amount=%d,Display=%s,Rune=%s,Icon=%s,Meta=%s}"),
				Index,
				*DescribePortalEnumValueForRewardDebug(StaticEnum<ELootType>(), static_cast<int64>(Option.LootType)),
				Option.Amount,
				*Option.DisplayName.ToString(),
				*GetNameSafe(Option.RuneAsset.Get()),
				*GetNameSafe(Option.Icon.Get()),
				*Option.MetaCurrencyTag.ToString()));
		}

		return FString::Printf(TEXT("Count=%d [%s]"), Options.Num(), *FString::Join(Parts, TEXT("; ")));
	}

	void AddPortalRewardTypePreviewOption(
		TArray<FLootOption>& OutOptions,
		TSet<ELootType>& AddedTypes,
		ELootType LootType)
	{
		if (AddedTypes.Contains(LootType))
		{
			return;
		}

		AddedTypes.Add(LootType);
		OutOptions.Add(MakePortalRewardTypePreviewOption(LootType));
	}

	TArray<FLootOption> BuildPortalRewardPreviewOptions(const URoomDataAsset* Room)
	{
		TArray<FLootOption> PreviewOptions;
		if (!Room)
		{
			return PreviewOptions;
		}

		TSet<ELootType> AddedTypes;
		if (Room->bUseFixedRewardOptions && !Room->FixedRewardOptions.IsEmpty())
		{
			for (const FLootOption& FixedOption : Room->FixedRewardOptions)
			{
				AddPortalRewardTypePreviewOption(PreviewOptions, AddedTypes, FixedOption.LootType);
			}
			return PreviewOptions;
		}

		if (!Room->LootPool.IsEmpty())
		{
			AddPortalRewardTypePreviewOption(PreviewOptions, AddedTypes, ELootType::Rune);
		}

		return PreviewOptions;
	}
}

APortal::APortal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	BillBoard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Root_BillBoard"));
	RootComponent = BillBoard;

	CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitBoxExtent(FVector(80, 80, 120));
	CollisionVolume->SetupAttachment(RootComponent);
	// 仅对 Pawn 产生 Overlap，不产生物理阻挡
	CollisionVolume->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);
	CollisionVolume->OnComponentEndOverlap.AddDynamic(this, &APortal::OnOverlapEnd);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);

	OpenVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OpenVFX"));
	OpenVFXComp->SetupAttachment(RootComponent);
	OpenVFXComp->bAutoActivate = false;

	IdleVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IdleVFX"));
	IdleVFXComp->SetupAttachment(RootComponent);
	IdleVFXComp->bAutoActivate = false;
}

void APortal::BeginPlay()
{
	Super::BeginPlay();
	// 关卡开始时门是关闭的
	DisablePortal();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &APortal::TriggerLinkedDoorCloseEvents));
	}
}

void APortal::Open(FName InSelectedLevel, URoomDataAsset* InSelectedRoom,
                   const TArray<FBuffEntry>& InPreRolledBuffs)
{
	SelectedLevel   = InSelectedLevel;
	SelectedRoom    = InSelectedRoom;
	PreRolledBuffs  = InPreRolledBuffs;
	bWillNeverOpen  = false;
	bIsOpen         = true;

	if (CollisionVolume)
	{
		CollisionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionVolume->SetGenerateOverlapEvents(true);
	}

	RefreshPreviewInfo();
	EnablePortal();
	K2_OnPortalOpened();
	TriggerLinkedDoorOpenEvents();

	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] Portal Open Portal=%s Index=%d SelectedLevel=%s SelectedRoom=%s PreviewRevision=%d CachedRewardOptions=%s"),
		*GetNameSafe(this),
		Index,
		*SelectedLevel.ToString(),
		*GetNameSafe(SelectedRoom),
		PreviewRevision,
		*DescribePortalLootOptionsForRewardDebug(CachedPreviewInfo.RewardPreviewOptions));

	// 调试：列出每个 PreRolled Buff 的资产名 + RuneName，定位 RuneName 漏填
	for (int32 i = 0; i < PreRolledBuffs.Num(); ++i)
	{
		const FBuffEntry& E = PreRolledBuffs[i];
		const FName RuneName = E.RuneDA ? E.RuneDA->GetRuneName() : NAME_None;
		UE_LOG(LogTemp, Log, TEXT("Portal[%d] PreRolled[%d]: Asset=%s RuneName=%s"),
			Index, i,
			E.RuneDA ? *E.RuneDA->GetName() : TEXT("null"),
			*RuneName.ToString());
	}
}

TArray<FLootOption> APortal::BuildRewardPreviewOptionsForRoom(
	const URoomDataAsset* Room,
	const UYogGameInstanceBase* GameInstance)
{
	TArray<FLootOption> PendingOptions;
	if (GameInstance && GameInstance->GetPendingRoomRewardOptionsOverride(PendingOptions))
	{
		UE_LOG(LogTemp, Log,
			TEXT("[StoryRewardDebug] Portal BuildRewardPreviewOptionsForRoom uses pending override Room=%s GI=%s Options=%s"),
			*GetNameSafe(Room),
			*GetNameSafe(GameInstance),
			*DescribePortalLootOptionsForRewardDebug(PendingOptions));
		return PendingOptions;
	}

	TArray<FLootOption> RoomOptions = BuildPortalRewardPreviewOptions(Room);
	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] Portal BuildRewardPreviewOptionsForRoom uses RoomData Room=%s GI=%s UseFixed=%d FixedCount=%d LootPoolCount=%d Options=%s"),
		*GetNameSafe(Room),
		*GetNameSafe(GameInstance),
		(Room && Room->bUseFixedRewardOptions) ? 1 : 0,
		Room ? Room->FixedRewardOptions.Num() : 0,
		Room ? Room->LootPool.Num() : 0,
		*DescribePortalLootOptionsForRewardDebug(RoomOptions));
	return RoomOptions;
}

void APortal::BuildPreviewInfo()
{
	CachedPreviewInfo = FPortalPreviewInfo{};
	CachedPreviewInfo.RoomLevelName  = SelectedLevel;
	CachedPreviewInfo.PreRolledBuffs = PreRolledBuffs;
	CachedPreviewInfo.LootCount      = 3;

	if (!SelectedRoom)
	{
		// 兜底：玩家可见名直接拿关卡名
		CachedPreviewInfo.RoomDisplayName = FText::FromName(SelectedLevel);
		return;
	}

	// DisplayName 为空时回退用 RoomName
	CachedPreviewInfo.RoomDisplayName = SelectedRoom->DisplayName.IsEmptyOrWhitespace()
		? FText::FromName(SelectedRoom->RoomName)
		: SelectedRoom->DisplayName;

	const UYogGameInstanceBase* GI = GetWorld()
		? Cast<UYogGameInstanceBase>(GetWorld()->GetGameInstance())
		: nullptr;
	CachedPreviewInfo.RewardPreviewOptions = BuildRewardPreviewOptionsForRoom(SelectedRoom, GI);
	CachedPreviewInfo.LootCount = CachedPreviewInfo.RewardPreviewOptions.Num();

	// 提取首个 Room.Type.* Tag 作为类型徽章依据
	static const FGameplayTag RoomTypeRoot = FGameplayTag::RequestGameplayTag(
		FName("Room.Type"), false);
	if (RoomTypeRoot.IsValid())
	{
		FGameplayTagContainer TypeTags = SelectedRoom->RoomTags.Filter(
			FGameplayTagContainer(RoomTypeRoot));
		if (TypeTags.Num() > 0)
		{
			CachedPreviewInfo.RoomTypeTag = TypeTags.First();
		}
	}
}

void APortal::RefreshPreviewInfo()
{
	BuildPreviewInfo();
	++PreviewRevision;
	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] Portal RefreshPreviewInfo Portal=%s Index=%d Revision=%d SelectedLevel=%s SelectedRoom=%s RewardOptions=%s"),
		*GetNameSafe(this),
		Index,
		PreviewRevision,
		*SelectedLevel.ToString(),
		*GetNameSafe(SelectedRoom),
		*DescribePortalLootOptionsForRewardDebug(CachedPreviewInfo.RewardPreviewOptions));
}

// =========================================================
// 美术驱动：ApplyArtConfig
// =========================================================

void APortal::ApplyArtConfig(const FPortalArtConfig& Config, bool bPlayOpenVFX)
{
	// 切换网格
	if (PortalMesh)
	{
		if (Config.Mesh)
		{
			PortalMesh->SetStaticMesh(Config.Mesh);
			PortalMesh->SetVisibility(true);
		}
	}

	// 持续待机特效
	if (IdleVFXComp)
	{
		IdleVFXComp->Deactivate();
		if (Config.IdleVFX)
		{
			IdleVFXComp->SetAsset(Config.IdleVFX);
			IdleVFXComp->Activate(true);
		}
	}

	// 开启时一次性特效（仅在 bPlayOpenVFX 时触发）
	if (OpenVFXComp)
	{
		OpenVFXComp->Deactivate();
		if (bPlayOpenVFX && Config.OpenVFX)
		{
			OpenVFXComp->SetAsset(Config.OpenVFX);
			OpenVFXComp->Activate(true);
		}
	}
}

// =========================================================
// BlueprintNativeEvent 默认实现
// =========================================================

void APortal::EnablePortal_Implementation()
{
	if (const FPortalArtConfig* Art = DestinationArtMap.Find(SelectedLevel))
	{
		ApplyArtConfig(*Art, true);
	}
	// 若 DestinationArtMap 里没配对应关卡，保持原样（兼容旧 BP）
}

void APortal::DisablePortal_Implementation()
{
	// 关闭状态：应用 ClosedArt，停止所有特效
	if (OpenVFXComp) OpenVFXComp->Deactivate();
	if (IdleVFXComp) IdleVFXComp->Deactivate();
	ApplyArtConfig(ClosedArt, false);
}

void APortal::NeverOpen_Implementation()
{
	// 停止特效
	if (OpenVFXComp) OpenVFXComp->Deactivate();
	if (IdleVFXComp) IdleVFXComp->Deactivate();
	TriggerLinkedDoorCloseEvents();

	if (NeverOpenArt.Mesh || NeverOpenArt.IdleVFX || NeverOpenArt.OpenVFX)
	{
		ApplyArtConfig(NeverOpenArt, false);
	}
	else if (ClosedArt.Mesh || ClosedArt.IdleVFX || ClosedArt.OpenVFX)
	{
		ApplyArtConfig(ClosedArt, false);
	}
	else
	{
		// 没配 NeverOpenArt 就直接隐藏门体
		if (PortalMesh) PortalMesh->SetVisibility(true);
	}

	// 禁用碰撞，玩家无法穿越
	if (CollisionVolume)
	{
		CollisionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionVolume->SetGenerateOverlapEvents(false);
	}
	bIsOpen = false;
}

void APortal::MarkUnavailable()
{
	bWillNeverOpen = true;
	NeverOpen();
}

void APortal::MarkUnavailableForPreview(FName InSelectedLevel, URoomDataAsset* InSelectedRoom)
{
	SelectedLevel = InSelectedLevel;
	SelectedRoom = InSelectedRoom;
	PreRolledBuffs.Reset();
	bWillNeverOpen = true;
	bIsOpen = false;

	if (OpenVFXComp) OpenVFXComp->Deactivate();
	if (IdleVFXComp) IdleVFXComp->Deactivate();
	TriggerLinkedDoorCloseEvents();

	const FPortalArtConfig& UnavailableArt =
		(NeverOpenArt.Mesh || NeverOpenArt.IdleVFX || NeverOpenArt.OpenVFX)
			? NeverOpenArt
			: ClosedArt;
	ApplyArtConfig(UnavailableArt, false);

	if (CollisionVolume)
	{
		CollisionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionVolume->SetGenerateOverlapEvents(false);
	}
}

void APortal::YogOpenLevel(FName LevelName)
{
	UGameplayStatics::OpenLevel(GetWorld(), LevelName, true);
}

// v3：Overlap 不再自动切关，仅维护 PendingPortal + BP 视觉钩
void APortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (!bIsOpen) return;
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		HandlePlayerEnterRange(Player);
	}
}

void APortal::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		HandlePlayerExitRange(Player);
	}
}

void APortal::HandlePlayerEnterRange(APlayerCharacterBase* Player)
{
	if (!Player) return;
	Player->PendingPortal = this;

	// TODO(Stage C)：调 HUD->NotifyPlayerInPortalRange(this) 切单例浮窗

	K2_OnHighlightChanged(true);
	K2_OnPortalRangeEntered();
	UE_LOG(LogTemp, Log, TEXT("Portal[%d]: 玩家进入交互范围（按 E 进入）"), Index);
}

void APortal::HandlePlayerExitRange(APlayerCharacterBase* Player)
{
	if (!Player) return;
	if (Player->PendingPortal == this)
	{
		Player->PendingPortal = nullptr;
	}

	// TODO(Stage C)：调 HUD->NotifyPlayerExitedPortalRange(this)

	K2_OnHighlightChanged(false);
	K2_OnPortalRangeExited();
	UE_LOG(LogTemp, Log, TEXT("Portal[%d]: 玩家离开交互范围"), Index);
}

// v3 完整过场：Portal 主导业务流程；HUD 仅负责视觉 BlackoutFade
void APortal::TriggerLinkedDoorOpenEvents()
{
	TriggerLinkedDoorEvent(DoorOpenEventName);
}

void APortal::TriggerLinkedDoorCloseEvents()
{
	TriggerLinkedDoorEvent(DoorCloseEventName);
}

void APortal::TriggerLinkedDoorEvent(FName EventName)
{
	if (EventName.IsNone())
	{
		return;
	}

	for (AActor* Target : DoorOpenEventTargets)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		UFunction* Function = Target->FindFunction(EventName);
		if (!Function)
		{
			UE_LOG(LogTemp, Warning, TEXT("Portal[%d]: Door event '%s' not found on %s"),
				Index, *EventName.ToString(), *GetNameSafe(Target));
			continue;
		}

		if (Function->NumParms != 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Portal[%d]: Door event '%s' on %s must not have parameters"),
				Index, *EventName.ToString(), *GetNameSafe(Target));
			continue;
		}

		Target->ProcessEvent(Function, nullptr);
	}
}

void APortal::TryEnter(APlayerCharacterBase* Player)
{
	if (bEntryInProgress)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Portal[%d]: TryEnter 重入被忽略"), Index);
		return;
	}

	// 前置校验（任一失败 → log + return；此时尚未锁输入，无需恢复）
	if (!bIsOpen || SelectedLevel.IsNone() || !Player || !SelectedRoom)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Portal[%d]: TryEnter 前置失败 bIsOpen=%d Level=%s Room=%s Player=%s"),
			Index, bIsOpen ? 1 : 0, *SelectedLevel.ToString(),
			SelectedRoom ? *SelectedRoom->GetName() : TEXT("null"),
			Player ? *Player->GetName() : TEXT("null"));
		return;
	}

	APlayerController* PC = Player->GetController<APlayerController>();
	AYogGameMode*      GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode());
	if (!PC || !GM)
	{
		UE_LOG(LogTemp, Warning, TEXT("Portal[%d]: TryEnter 拿不到 PC=%s GM=%s"),
			Index, PC ? TEXT("OK") : TEXT("null"), GM ? TEXT("OK") : TEXT("null"));
		return;
	}

	bEntryInProgress = true;
	EntryPlayer      = Player;
	K2_OnEntrySequenceStart();

	// 锁输入 + 锁背包，避免过场中玩家干扰
	PC->DisableInput(PC);
	if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
	{
		Backpack->SetLocked(true);
	}

	// 触发 HUD 渐黑（视觉接口；HUD 不感知业务流程）
	if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
	{
		HUD->HidePortalGuidance();   // 收起浮窗 + 箭头
		HUD->BeginBlackoutFade(HUD->PortalBlackoutDuration);
	}

	// 启动每帧 AddMovementInput Tick + Walk 结束 Timer
	GetWorld()->GetTimerManager().SetTimer(
		EntryWalkTickTimer, this, &APortal::TickEntryMovement, 0.05f, true);

	const float FinishDelay = PortalEntryWalkDuration + PortalEntryFailSafeBuffer;
	GetWorld()->GetTimerManager().SetTimer(
		EntryFinishTimer, this, &APortal::FinishEntry, FinishDelay, false);

	UE_LOG(LogTemp, Log, TEXT("Portal[%d]: TryEnter 启动过场 walk=%.2fs failSafe=%.2fs"),
		Index, PortalEntryWalkDuration, FinishDelay);
}

void APortal::TickEntryMovement()
{
	APlayerCharacterBase* Player = EntryPlayer.Get();
	if (!Player) return;

	const FVector ToDoor = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
	if (!ToDoor.IsNearlyZero())
	{
		Player->AddMovementInput(ToDoor, 1.0f, false);
	}
}

void APortal::FinishEntry()
{
	GetWorld()->GetTimerManager().ClearTimer(EntryWalkTickTimer);
	GetWorld()->GetTimerManager().ClearTimer(EntryFinishTimer);

	APlayerCharacterBase* Player = EntryPlayer.Get();
	if (!Player) { AbortEntry(TEXT("Player 已失效")); return; }

	AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode());
	if (!GM)        { AbortEntry(TEXT("GameMode 失效")); return; }

	if (SelectedLevel.IsNone() || !SelectedRoom)
	{
		AbortEntry(TEXT("SelectedLevel/Room 失效"));
		return;
	}

	// v3：一次性同点写齐 GI 跨关字段，保证下一关数据一致
	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->PendingRoomBuffs      = PreRolledBuffs;
		GI->bPlayLevelIntroFadeIn = true;
		// PendingRoomData / PendingNextFloor / PendingRunState 由 TransitionToLevel 内同点写
	}

	// 写存档（可选，TransitionToLevel 内也会处理 RunState）
	if (UYogSaveSubsystem* SaveSubsystem = UGameInstance::GetSubsystem<UYogSaveSubsystem>(GetGameInstance()))
	{
		SaveSubsystem->WriteSaveGame();
	}

	UE_LOG(LogTemp, Log, TEXT("Portal[%d]: FinishEntry → TransitionToLevel(%s)"),
		Index, *SelectedLevel.ToString());

	// 切关（OpenLevel 后本 actor 销毁，bEntryInProgress 不需要复位）
	GM->TransitionToLevel(SelectedLevel, SelectedRoom);
}

void APortal::AbortEntry(const TCHAR* Reason)
{
	UE_LOG(LogTemp, Error, TEXT("Portal[%d]: AbortEntry — %s"), Index, Reason);

	GetWorld()->GetTimerManager().ClearTimer(EntryWalkTickTimer);
	GetWorld()->GetTimerManager().ClearTimer(EntryFinishTimer);

	if (APlayerCharacterBase* Player = EntryPlayer.Get())
	{
		if (APlayerController* PC = Player->GetController<APlayerController>())
		{
			PC->EnableInput(PC);
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->EndBlackoutFade(0.2f);   // 快速恢复画面
			}
		}
		if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
		{
			Backpack->SetLocked(false);
		}
	}

	EntryPlayer = nullptr;
	bEntryInProgress = false;
}

// v3：保留 EnterPortal_Implementation 作为 BP override 钩子；新流程实际不调用
// （旧调用方 OnOverlapBegin 已删除；外部 BP 若仍调 EnterPortal 仍能工作）
void APortal::EnterPortal_Implementation(APlayerCharacterBase* ReceivingChar, UYogSaveSubsystem* SaveSubsystem)
{
	if (!bIsOpen || SelectedLevel.IsNone()) return;

	if (SaveSubsystem)
	{
		SaveSubsystem->WriteSaveGame();
	}

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->PendingRoomBuffs      = PreRolledBuffs;
		GI->bPlayLevelIntroFadeIn = true;
	}
	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->TransitionToLevel(SelectedLevel, SelectedRoom);
	}
}
