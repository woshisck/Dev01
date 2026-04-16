#include "Map/Portal.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "Engine/GameInstance.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Data/RoomDataAsset.h"

APortal::APortal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	BillBoard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Root_BillBoard"));
	RootComponent = BillBoard;

	CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitBoxExtent(FVector(80, 80, 120));
	CollisionVolume->SetupAttachment(RootComponent);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);

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
}

void APortal::Open(FName InSelectedLevel, URoomDataAsset* InSelectedRoom)
{
	SelectedLevel = InSelectedLevel;
	SelectedRoom  = InSelectedRoom;
	bIsOpen = true;
	EnablePortal();
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

	if (NeverOpenArt.Mesh)
	{
		ApplyArtConfig(NeverOpenArt, false);
	}
	else
	{
		// 没配 NeverOpenArt 就直接隐藏门体
		if (PortalMesh) PortalMesh->SetVisibility(false);
	}

	// 禁用碰撞，玩家无法穿越
	CollisionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APortal::YogOpenLevel(FName LevelName)
{
	UGameplayStatics::OpenLevel(GetWorld(), LevelName, true);
}

void APortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (!bIsOpen) return;

	APlayerCharacterBase* OverlappingPawn = Cast<APlayerCharacterBase>(OtherActor);
	if (!OverlappingPawn) return;

	UYogSaveSubsystem* SaveSubsystem = UGameInstance::GetSubsystem<UYogSaveSubsystem>(GetGameInstance());
	EnterPortal(OverlappingPawn, SaveSubsystem);
}

void APortal::EnterPortal_Implementation(APlayerCharacterBase* ReceivingChar, UYogSaveSubsystem* SaveSubsystem)
{
	if (!bIsOpen || SelectedLevel.IsNone()) return;

	if (SaveSubsystem)
	{
		SaveSubsystem->WriteSaveGame();
	}

	// 通知 GameMode 保存跑局状态（含本门选定的房间配置）后切关
	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->TransitionToLevel(SelectedLevel, SelectedRoom);
	}
}
