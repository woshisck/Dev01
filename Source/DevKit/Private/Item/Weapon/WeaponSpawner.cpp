// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Weapon/WeaponSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
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
#include "Component/CombatDeckComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Tutorial/TutorialManager.h"
#include "Character/YogPlayerControllerBase.h"
#include "Engine/GameInstance.h"
#include "UI/WeaponFloatWidget.h"
#include "UI/YogHUD.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Data/LevelInfoPopupDA.h"

namespace
{
	constexpr bool bDisableLegacyHeatBackpackRuneForCardTest = true;
}

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

	// 缓存网格原始材质（BP Construction Script 之后运行，拿到的是最终设置值）
	if (WeaponMesh)
	{
		CachedMeshMaterials.Reset();
		for (int32 i = 0; i < WeaponMesh->GetNumMaterials(); ++i)
			CachedMeshMaterials.Add(WeaponMesh->GetMaterial(i));
	}
}

void AWeaponSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

// Called every frame
void AWeaponSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 旋转
	WeaponMesh->AddRelativeRotation(FRotator(
		RotationRate.Pitch * DeltaTime,
		RotationRate.Yaw   * DeltaTime,
		RotationRate.Roll  * DeltaTime));

	// 浮动偏移
	if (BobAmplitude > 0.f)
	{
		BobTimer += DeltaTime;
		const float BobOffset = FMath::Sin(BobTimer * BobFrequency * 2.f * PI) * BobAmplitude;
		WeaponMesh->SetRelativeLocation(BaseMeshOffset + BobAxis.GetSafeNormal() * BobOffset);
	}

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
		BaseMeshOffset = WeaponDefinition->WeaponMeshOffset;
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

void AWeaponSpawner::OnPlayerEnterRange(APlayerCharacterBase* Player)
{
	if (!Player || !WeaponDefinition) return;

	Player->PendingWeaponSpawner = this;
	NearbyPlayer = Player;
	bPlayerInRange = true;
	// 浮窗可见性由 Tick 根据朝向动态控制
}

void AWeaponSpawner::OnPlayerLeaveRange(APlayerCharacterBase* Player)
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
}

void AWeaponSpawner::TryPickup(APlayerCharacterBase* Player)
{
	TryPickupWeapon(Player);
}

void AWeaponSpawner::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	OnPlayerEnterRange(Player);
}

void AWeaponSpawner::OnPlayerEndOverlap(APlayerCharacterBase* Player)
{
	OnPlayerLeaveRange(Player);
}

void AWeaponSpawner::ResetToAvailable()
{
	bPickedUp            = false;
	bCollapsingForPickup = false;
	bPlayerInRange       = false;
	NearbyPlayer         = nullptr;

	if (WeaponInfoWidgetComp)
		WeaponInfoWidgetComp->SetVisibility(false);

	// 还原浮窗内部状态：折叠动画把 InfoContainer 透明度降到 0、Visibility=Collapsed，
	// 必须重新调 SetWeaponDefinition 把 RenderOpacity/Visibility/RenderTransform 全部还原
	if (WeaponInfoWidgetComp && WeaponDefinition)
	{
		if (UWeaponFloatWidget* FloatWidget = Cast<UWeaponFloatWidget>(WeaponInfoWidgetComp->GetWidget()))
			FloatWidget->SetWeaponDefinition(WeaponDefinition);
	}

	// 还原网格材质
	if (WeaponMesh)
	{
		for (int32 i = 0; i < CachedMeshMaterials.Num(); ++i)
			WeaponMesh->SetMaterial(i, CachedMeshMaterials[i]);
	}

	OnResetToAvailable();
}

void AWeaponSpawner::TryPickupWeapon(APlayerCharacterBase* Player)
{
	if (!Player || !WeaponDefinition) return;

	// ── 预览模式：仅弹 LevelInfoPopup，不执行拾取 ──────────────────
	if (WeaponDefinition->bPreviewOnly)
	{
		if (WeaponDefinition->PreviewPopup)
		{
			if (APlayerController* PC = Player->GetController<APlayerController>())
				if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
					HUD->ShowInfoPopup(WeaponDefinition->PreviewPopup);
		}
		return;
	}

	// 拾取后浮窗永久隐藏
	bPickedUp = true;

	// 网格变黑
	if (WeaponMesh && PickedUpMaterial)
	{
		for (int32 i = 0; i < WeaponMesh->GetNumMaterials(); ++i)
			WeaponMesh->SetMaterial(i, PickedUpMaterial);
	}

	// 若教程弹窗仍在显示（玩家在弹窗期间按 E 拾取），强制关闭
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
		if (TM->IsPopupShowing()) TM->ForceClosePopup();

	// ── 1. 处理旧武器 ────────────────────────────────────────────────
	if (Player->EquippedWeaponInstance)
	{
		// 恢复旧武器来源的 Spawner（让它重新可交互、材质恢复）
		if (Player->EquippedFromSpawner && Player->EquippedFromSpawner != this)
			Player->EquippedFromSpawner->ResetToAvailable();

		// 解绑热度委托，再销毁
		Player->OnHeatPhaseChanged.RemoveDynamic(Player->EquippedWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		Player->EquippedWeaponInstance->Destroy();
		Player->EquippedWeaponInstance = nullptr;
	}

	// ── 1.5 清旧武器类型 Tag（避免装备替换时残留导致两类 GA 都通过）────
	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(Player->GetAbilitySystemComponent()))
	{
		YogASC->ClearWeaponTypeTags();
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
	if (NewWeapon && !bDisableLegacyHeatBackpackRuneForCardTest)
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
	else if (NewWeapon)
	{
		Player->EquippedWeaponInstance = NewWeapon;
		UE_LOG(LogTemp, Warning, TEXT("[WeaponSpawner] Legacy heat weapon overlay disabled for combat card test"));
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
	if (!bDisableLegacyHeatBackpackRuneForCardTest && Player->BackpackGridComponent)
	{
		UBackpackGridComponent* BG = Player->BackpackGridComponent;
		BG->ApplyBackpackConfig(
			WeaponDefinition->BackpackConfig.GridWidth,
			WeaponDefinition->BackpackConfig.GridHeight,
			WeaponDefinition->BackpackConfig.ActivationZoneConfig);

		// ── 4b. 放置武器初始符文到热度一激活区 ──────────────────────────
		if (WeaponDefinition->InitialRunes.Num() > 0)
		{
			TArray<FIntPoint> Phase1Cells = BG->GetActivationZoneCellsForPhase(0);
			for (URuneDataAsset* RuneDA : WeaponDefinition->InitialRunes)
			{
				if (!RuneDA) continue;
				FRuneInstance RuneInst = RuneDA->CreateInstance();
				for (const FIntPoint& Cell : Phase1Cells)
				{
					if (BG->TryPlaceRune(RuneInst, Cell))
						break;
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponSpawner] Legacy backpack/rune config disabled for combat card test"));
	}

	// ── 5. 记录状态 ──────────────────────────────────────────────────
	Player->EquippedWeaponDef    = WeaponDefinition;
	Player->EquippedFromSpawner  = this;
	Player->PendingWeaponSpawner = nullptr;

	// ── 5.25 注入战斗卡组与连招配置 ─────────────────────────────────
	// TryPickupWeapon 自己实现了装备流程，不走 WeaponDefinition::SetupWeaponToCharacter。
	// 因此这里需要同步加载武器 DA 上的 InitialCombatDeck / InitialRunes 和 WeaponComboConfig。
	if (UCombatDeckComponent* CombatDeck = Player->CombatDeckComponent.Get())
	{
		CombatDeck->LoadDeckFromWeapon(WeaponDefinition);
	}

	if (UComboRuntimeComponent* ComboRuntime = Player->ComboRuntimeComponent.Get())
	{
		ComboRuntime->LoadComboConfig(WeaponDefinition->WeaponComboConfig);
	}

	// ── 5.5 武器类型 Tag 守卫：挂当前 WeaponType LooseTag ─────────────
	// 让玩家专属攻击 GA 的 ActivationRequiredTags 能匹配通过；
	// 注意：TryPickupWeapon 自己实现了装备流程未调 SetupWeaponToCharacter，
	// 故此处必须手动 Apply（与 SetupWeaponToCharacter 装备完成段对应）
	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(Player->GetAbilitySystemComponent()))
	{
		YogASC->ApplyWeaponTypeTag(WeaponDefinition->WeaponType);
	}

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
					[WeakThis, WeakPC, WeakHUD, CapturedDef](FVector2D /*ThumbnailPos*/)
				{
					if (WeakThis.IsValid())
					{
						WeakThis->bCollapsingForPickup = false;
						if (WeakThis->WeaponInfoWidgetComp)
							WeakThis->WeaponInfoWidgetComp->SetVisibility(false);
					}
					if (WeakHUD.IsValid() && CapturedDef && WeakPC.IsValid() && WeakThis.IsValid())
					{
						// ProjectWorldLocationToScreen 返回像素坐标，除以 DPI 转为 Slate/UMG 单位
						FVector2D ScreenPos(0.f, 0.f);
						WeakPC->ProjectWorldLocationToScreen(
							WeakThis->GetActorLocation() + FVector(0.f, 0.f, 80.f),
							ScreenPos, false);
						const float DPI = UWidgetLayoutLibrary::GetViewportScale(WeakThis.Get());
						if (DPI > 0.f) ScreenPos /= DPI;

						UE_LOG(LogTemp, Warning,
							TEXT("[WeaponPickup] 折叠完成 → TriggerWeaponPickup Pos=(%.0f,%.0f)"),
							ScreenPos.X, ScreenPos.Y);
						WeakHUD->TriggerWeaponPickup(CapturedDef, ScreenPos);
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
				const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
				if (DPI > 0.f) FallbackPos /= DPI;
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



