// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Weapon/WeaponSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "YogBlueprintFunctionLibrary.h"
#include "Component/CharacterDataComponent.h"
#include "Component/BackpackGridComponent.h"
#include "Tutorial/TutorialManager.h"
#include "Character/YogPlayerControllerBase.h"
#include "UI/WeaponFloatWidget.h"

// Sets default values
AWeaponSpawner::AWeaponSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CollisionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionVolume->SetGenerateOverlapEvents(true);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &AWeaponSpawner::OnOverlapBegin);
	CollisionVolume->OnComponentEndOverlap.AddDynamic(this, &AWeaponSpawner::OnOverlapEnd);

	//WeaponMesh
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	if (WeaponDefinition != nullptr)
	{
		WeaponMesh->SetStaticMesh(WeaponDefinition->DisplayMesh);

	}

	WeaponMeshRotationSpeed = 40.0f;

	WeaponInfoWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WeaponInfoWidgetComp"));
	WeaponInfoWidgetComp->SetupAttachment(RootComponent);
	WeaponInfoWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	WeaponInfoWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WeaponInfoWidgetComp->SetVisibility(false);
}

// Called when the game starts or when spawned
void AWeaponSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 保存 WeaponMesh 原始材质，供换武器时恢复
	OriginalMeshMaterials.Empty();
	for (int32 i = 0; i < WeaponMesh->GetNumMaterials(); i++)
	{
		OriginalMeshMaterials.Add(WeaponMesh->GetMaterial(i));
	}

	// 初始化浮窗 Widget
	if (WeaponFloatWidgetClass && WeaponInfoWidgetComp && WeaponDefinition)
	{
		WeaponInfoWidgetComp->SetWidgetClass(WeaponFloatWidgetClass);
		WeaponInfoWidgetComp->InitWidget();
		if (UWeaponFloatWidget* FloatWidget = Cast<UWeaponFloatWidget>(WeaponInfoWidgetComp->GetWidget()))
		{
			FloatWidget->SetWeaponDefinition(WeaponDefinition);
		}
	}
}

void AWeaponSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

// Called every frame
void AWeaponSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	WeaponMesh->AddRelativeRotation(FRotator(0.0f, World->GetDeltaSeconds() * WeaponMeshRotationSpeed, 0.0f));

	// 朝向判断：玩家在范围内且面朝武器时显示浮窗（±105°以内）
	if (bPlayerInRange && NearbyPlayer.IsValid() && WeaponInfoWidgetComp)
	{
		FVector ToWeapon = GetActorLocation() - NearbyPlayer->GetActorLocation();
		ToWeapon.Z = 0.f;
		ToWeapon.Normalize();
		FVector Forward = NearbyPlayer->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();
		WeaponInfoWidgetComp->SetVisibility(FVector::DotProduct(Forward, ToWeapon) > -0.3f);
	}
}

void AWeaponSpawner::OnConstruction(const FTransform& Transform)
{	
	if (WeaponDefinition != nullptr && WeaponDefinition->DisplayMesh != nullptr)
	{
		WeaponMesh->SetStaticMesh(WeaponDefinition->DisplayMesh);
		WeaponMesh->SetRelativeLocation(WeaponDefinition->WeaponMeshOffset);
		WeaponMesh->SetRelativeScale3D(WeaponDefinition->WeaponMeshScale);
	}
}




void AWeaponSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player || !WeaponDefinition) return;

	// 进入范围时登记，等待玩家主动按 E 拾取
	Player->PendingWeaponSpawner = this;
	NearbyPlayer = Player;
	bPlayerInRange = true;
	UE_LOG(LogTemp, Log, TEXT("WeaponSpawner: 玩家进入范围，按 E 拾取武器"));
}

void AWeaponSpawner::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player) return;

	if (Player->PendingWeaponSpawner == this)
	{
		Player->PendingWeaponSpawner = nullptr;
		UE_LOG(LogTemp, Log, TEXT("WeaponSpawner: 玩家离开范围"));
	}
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer = nullptr;
		bPlayerInRange = false;
		if (WeaponInfoWidgetComp) WeaponInfoWidgetComp->SetVisibility(false);
	}
}

void AWeaponSpawner::TryPickupWeapon(APlayerCharacterBase* Player)
{
	if (!Player || !WeaponDefinition) return;

	// 拾取时立即隐藏浮窗
	bPlayerInRange = false;
	NearbyPlayer = nullptr;
	if (WeaponInfoWidgetComp) WeaponInfoWidgetComp->SetVisibility(false);

	// ── 1. 处理旧武器 ────────────────────────────────────────────────
	if (Player->EquippedFromSpawner && Player->EquippedFromSpawner != this)
	{
		Player->EquippedFromSpawner->RestoreSpawnerMesh();
	}
	if (Player->EquippedWeaponInstance)
	{
		// 解绑热度委托，再销毁
		Player->OnHeatPhaseChanged.RemoveDynamic(Player->EquippedWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		Player->EquippedWeaponInstance->Destroy();
		Player->EquippedWeaponInstance = nullptr;
	}

	// ── 2. 生成并装备新武器 ──────────────────────────────────────────
	AWeaponInstance* NewWeapon = nullptr;
	for (const FWeaponSpawnData& WeaponSpawnData : WeaponDefinition->ActorsToSpawn)
	{
		FWeaponSpawnData SpawnData;
		SpawnData.ActorToSpawn   = WeaponSpawnData.ActorToSpawn;
		SpawnData.AttachSocket   = WeaponSpawnData.AttachSocket;
		SpawnData.AttachTransform = WeaponSpawnData.AttachTransform;
		SpawnData.WeaponLayer    = WeaponSpawnData.WeaponLayer;
		SpawnData.WeaponAbilities = WeaponSpawnData.WeaponAbilities;
		SpawnData.bShouldSaveToGame = true;

		NewWeapon = UYogBlueprintFunctionLibrary::SpawnWeaponOnCharacter(Player, Player->GetTransform(), SpawnData);

		UCharacterData* CD = Player->GetCharacterDataComponent()->GetCharacterData();
		UE_LOG(LogTemp, Warning, TEXT("[WeaponSetup][WeaponSpawner] Owner=%s | CD=%s | NewAbilityData=%s"),
			*Player->GetName(),
			CD ? *CD->GetName() : TEXT("null"),
			WeaponDefinition->WeaponAbilityData ? *WeaponDefinition->WeaponAbilityData->GetName() : TEXT("null"));
		CD->AbilityData = WeaponDefinition->WeaponAbilityData;
	}

	// ── 3. 传入热度材质 + 绑定热度委托 ──────────────────────────────
	if (NewWeapon)
	{
		// 从 DA 把 Overlay 材质传给武器实例，无需在 BP 里手动赋值
		NewWeapon->HeatOverlayMaterial = WeaponDefinition->HeatOverlayMaterial;

		Player->OnHeatPhaseChanged.AddDynamic(NewWeapon, &AWeaponInstance::OnHeatPhaseChanged);
		Player->EquippedWeaponInstance = NewWeapon;

		// 查当前实际热度阶段，追赶同步（防止升阶早于武器拾取）
		int32 CurrentPhase = 0;
		if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
		{
			if      (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.3")))) CurrentPhase = 3;
			else if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.2")))) CurrentPhase = 2;
			else if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.1")))) CurrentPhase = 1;
		}
		Player->OnHeatPhaseChanged.Broadcast(CurrentPhase);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				TEXT("[WeaponSpawner] 武器生成失败，NewWeapon 为空"));
		}
	}

	// ── 4. 注入背包配置 ───────────────────────────────────────────────
	if (UBackpackGridComponent* BG = Player->BackpackGridComponent)
	{
		BG->ApplyBackpackConfig(
			WeaponDefinition->BackpackConfig.GridWidth,
			WeaponDefinition->BackpackConfig.GridHeight,
			WeaponDefinition->BackpackConfig.ActivationZoneConfig);
	}

	// ── 5. 记录状态 ──────────────────────────────────────────────────
	Player->EquippedWeaponDef    = WeaponDefinition;
	Player->EquippedFromSpawner  = this;
	Player->PendingWeaponSpawner = nullptr;

	// ── 5. 本 Spawner 展示网格变黑 ───────────────────────────────────
	if (BlackedOutMaterial)
	{
		for (int32 i = 0; i < WeaponMesh->GetNumMaterials(); i++)
		{
			WeaponMesh->SetMaterial(i, BlackedOutMaterial);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("WeaponSpawner: 武器已拾取 [%s]"), *WeaponDefinition->GetName());

	// 武器教程：首次拾取时延迟打开背包并弹出引导弹窗
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
	{
		if (AYogPlayerControllerBase* PC = Cast<AYogPlayerControllerBase>(Player->GetController()))
		{
			TM->TryWeaponTutorial(PC);
		}
	}
}

void AWeaponSpawner::RestoreSpawnerMesh()
{
	for (int32 i = 0; i < OriginalMeshMaterials.Num(); i++)
	{
		WeaponMesh->SetMaterial(i, OriginalMeshMaterials[i]);
	}
}

AWeaponInstance* AWeaponSpawner::SpawnWeaponDeferred(UWorld* World, const FTransform& SpawnTransform, const FWeaponSpawnData& SpawnData)
{

	AWeaponInstance* WeaponActor = GetWorld()->SpawnActorDeferred<AWeaponInstance>(
		SpawnData.ActorToSpawn,
		SpawnTransform,
		this,  // Owner
		nullptr,                // Instigator
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (WeaponActor)
	{
		ApplySpawnDataToWeapon(WeaponActor, SpawnData);
	}

	WeaponActor->FinishSpawning(SpawnTransform);
		
	return WeaponActor;
}

void AWeaponSpawner::ApplySpawnDataToWeapon(AWeaponInstance* Weapon, const FWeaponSpawnData& Data)
{
	Weapon->AttachSocket = Data.AttachSocket;
	Weapon->AttachTransform = Data.AttachTransform;
	Weapon->WeaponLayer = Data.WeaponLayer;
	Weapon->WeaponAbilities = Data.WeaponAbilities;
}



