#include "Map/Portal.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "Engine/GameInstance.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"

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
}

void APortal::BeginPlay()
{
	Super::BeginPlay();
	// 关卡开始时门是关闭的
	DisablePortal();
}

void APortal::Open(FName InSelectedLevel)
{
	SelectedLevel = InSelectedLevel;
	bIsOpen = true;
	EnablePortal();
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

	// 通知 GameMode 保存跑局状态后切关
	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->TransitionToLevel(SelectedLevel);
	}
}
