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
#include "Engine/GameInstance.h"
#include "UI/WeaponFloatWidget.h"
#include "UI/YogHUD.h"

// Sets default values
AWeaponSpawner::AWeaponSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = PlayerInteractVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("PlayerInteractVolume"));
	PlayerInteractVolume->InitCapsuleSize(80.f, 80.f);
	PlayerInteractVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PlayerInteractVolume->SetGenerateOverlapEvents(true);
	PlayerInteractVolume->OnComponentBeginOverlap.AddDynamic(this, &AWeaponSpawner::OnOverlapBegin);
	PlayerInteractVolume->OnComponentEndOverlap.AddDynamic(this, &AWeaponSpawner::OnOverlapEnd);

	BlockVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BlockVolume"));
	BlockVolume->InitCapsuleSize(60.f, 60.f);
	BlockVolume->SetupAttachment(RootComponent);
	BlockVolume->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	BlockVolume->SetGenerateOverlapEvents(false);

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

	// BP 里未赋值时自动按路径兜底，防止 merge 丢失引用
	if (!WeaponFloatWidgetClass)
	{
		WeaponFloatWidgetClass = LoadClass<UWeaponFloatWidget>(
			nullptr,
			TEXT("/Game/UI/Playtest_UI/WeaponInfo/WBP_WeaponFloat.WBP_WeaponFloat_C"));
		if (!WeaponFloatWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WeaponSpawner] WBP_WeaponFloat 未找到，请检查资产路径"));
		}
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

	// Tutorial 弹窗显示期间、或武器已被拾取后隐藏浮窗
	if (bPickedUp)
	{
		// 折叠动画进行中时保持可见，动画结束后由回调隐藏
		if (!bCollapsingForPickup && WeaponInfoWidgetComp)
			WeaponInfoWidgetComp->SetVisibility(false);
		return;
	}
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
	{
		if (TM->IsPopupShowing())
		{
			if (WeaponInfoWidgetComp) WeaponInfoWidgetComp->SetVisibility(false);
			return;
		}
	}

	// 朝向判断 + 动态偏移：浮窗始终在武器右侧，不遮挡玩家和武器
	if (bPlayerInRange && NearbyPlayer.IsValid() && WeaponInfoWidgetComp)
	{
		FVector ToWeapon = GetActorLocation() - NearbyPlayer->GetActorLocation();
		ToWeapon.Z = 0.f;
		ToWeapon.Normalize();
		FVector Forward = NearbyPlayer->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();

		const bool bShouldShow = FVector::DotProduct(Forward, ToWeapon) > -0.3f;
		WeaponInfoWidgetComp->SetVisibility(bShouldShow);

		if (bShouldShow)
		{
			// 45°斜视角：用摄像机 Right 向量投影到水平面，确保偏移与屏幕对齐
			FVector Right = FVector(0.f, 1.f, 0.f); // 兜底：世界 Y
			if (APlayerController* PC = NearbyPlayer->GetController<APlayerController>())
			{
				if (PC->PlayerCameraManager)
				{
					FVector CamRight = FRotationMatrix(PC->PlayerCameraManager->GetCameraRotation())
						.GetScaledAxis(EAxis::Y);
					CamRight.Z = 0.f;
					if (!CamRight.IsNearlyZero())
						Right = CamRight.GetSafeNormal();
				}

				// 根据武器在屏幕上的位置决定偏移方向：屏幕右半则向左偏，左半则向右偏
				FVector2D WeaponScreenPos;
				if (PC->ProjectWorldLocationToScreen(GetActorLocation(), WeaponScreenPos, false))
				{
					FVector2D ViewportSize;
					if (GEngine && GEngine->GameViewport)
						GEngine->GameViewport->GetViewportSize(ViewportSize);
					if (WeaponScreenPos.X > ViewportSize.X * 0.5f)
						Right = -Right;
				}
			}
			WeaponInfoWidgetComp->SetRelativeLocation(
				Right * WidgetSideOffset + FVector(0.f, 0.f, WidgetZOffset));
		}
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
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerBeginOverlap(Player);
	}
}

void AWeaponSpawner::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerEndOverlap(Player);
	}
}

void AWeaponSpawner::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	if (!Player || !WeaponDefinition) return;

	Player->PendingWeaponSpawner = this;
	NearbyPlayer = Player;
	bPlayerInRange = true;

	if (UWidgetComponent* WC = Player->GetWidgetcomponent())
	{
		WC->SetVisibility(true);
	}

	// 武器教程由关卡中的 LevelEventTrigger + Flow 管理，不在此触发
}

void AWeaponSpawner::OnPlayerEndOverlap(APlayerCharacterBase* Player)
{
	if (!Player) return;

	if (Player->PendingWeaponSpawner == this)
	{
		Player->PendingWeaponSpawner = nullptr;
	}
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer = nullptr;
		bPlayerInRange = false;
		if (WeaponInfoWidgetComp) WeaponInfoWidgetComp->SetVisibility(false);
	}

	if (UWidgetComponent* WC = Player->GetWidgetcomponent())
	{
		WC->SetVisibility(false);
	}
}

void AWeaponSpawner::TryPickupWeapon(APlayerCharacterBase* Player)
{
	if (!Player || !WeaponDefinition) return;

	// 拾取后浮窗永久隐藏
	bPickedUp = true;

	// 若教程弹窗仍在显示（玩家在弹窗期间按 E 拾取），强制关闭
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
		if (TM->IsPopupShowing()) TM->ForceClosePopup();

	// ── 1. 处理旧武器 ────────────────────────────────────────────────
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

	UE_LOG(LogTemp, Log, TEXT("WeaponSpawner: 武器已拾取 [%s]"), *WeaponDefinition->GetName());

	// 浮窗折叠动画 → 完成后以缩略图屏幕坐标触发飞行
	if (APlayerController* PC = Player->GetController<APlayerController>())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			UWeaponFloatWidget* FloatWidget = Cast<UWeaponFloatWidget>(WeaponInfoWidgetComp->GetWidget());
			if (FloatWidget)
			{
				bCollapsingForPickup = true;

				TWeakObjectPtr<AWeaponSpawner>     WeakThis(this);
				TWeakObjectPtr<APlayerController>  WeakPC(PC);
				TWeakObjectPtr<AYogHUD>            WeakHUD(HUD);
				UWeaponDefinition*                 CapturedDef = WeaponDefinition;

				FloatWidget->OnCollapseComplete.BindLambda(
					[WeakThis, WeakPC, WeakHUD, CapturedDef](FVector2D ThumbnailPos)
				{
					if (WeakThis.IsValid())
					{
						WeakThis->bCollapsingForPickup = false;
						if (WeakThis->WeaponInfoWidgetComp)
							WeakThis->WeaponInfoWidgetComp->SetVisibility(false);
					}
					if (WeakHUD.IsValid() && CapturedDef)
					{
						UE_LOG(LogTemp, Warning,
							TEXT("[WeaponPickup] 折叠完成 → TriggerWeaponPickup Pos=(%.0f,%.0f)"),
							ThumbnailPos.X, ThumbnailPos.Y);
						WeakHUD->TriggerWeaponPickup(CapturedDef, ThumbnailPos);
					}
				});

				FloatWidget->StartCollapse(PickupCollapseDuration);
			}
			else
			{
				// 浮窗未就绪，直接用 Spawner 世界坐标投影兜底
				FVector2D FallbackPos(0.f, 0.f);
				PC->ProjectWorldLocationToScreen(
					GetActorLocation() + FVector(0.f, 0.f, 80.f), FallbackPos, false);
				UE_LOG(LogTemp, Warning,
					TEXT("[WeaponPickup] FloatWidget=NULL，使用兜底坐标 (%.0f,%.0f)"),
					FallbackPos.X, FallbackPos.Y);
				HUD->TriggerWeaponPickup(WeaponDefinition, FallbackPos);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] HUD=NULL — 动画不会触发"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] PC=NULL — 动画不会触发"));
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



