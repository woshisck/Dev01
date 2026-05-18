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
#include "Data/LevelInfoPopupDA.h"

namespace
{
	constexpr bool bDisableLegacyHeatBackpackRuneForCardTestSpawner = true;

	AYogHUD* GetYogHUDForPlayer(APlayerCharacterBase* Player)
	{
		if (!Player)
		{
			return nullptr;
		}

		APlayerController* PC = Player->GetController<APlayerController>();
		return PC ? Cast<AYogHUD>(PC->GetHUD()) : nullptr;
	}
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
		if (WeaponInfoWidgetComp)
			WeaponInfoWidgetComp->SetVisibility(false);
		if (!bCollapsingForPickup)
			if (AYogHUD* HUD = GetYogHUDForPlayer(NearbyPlayer.Get()))
				HUD->HideWeaponFloatInfo(WeaponDefinition);
		return;
	}
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
	{
		if (TM->IsPopupShowing())
		{
			if (WeaponInfoWidgetComp) WeaponInfoWidgetComp->SetVisibility(false);
			if (AYogHUD* HUD = GetYogHUDForPlayer(NearbyPlayer.Get()))
				HUD->HideWeaponFloatInfo(WeaponDefinition);
			return;
		}
	}

	// 朝向判断：武器信息由 HUD 固定显示在关卡信息区，避免世界投影浮窗越界。
	if (bPlayerInRange && NearbyPlayer.IsValid())
	{
		FVector ToWeapon = GetActorLocation() - NearbyPlayer->GetActorLocation();
		ToWeapon.Z = 0.f;
		ToWeapon.Normalize();
		FVector Forward = NearbyPlayer->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();

		const bool bShouldShow = FVector::DotProduct(Forward, ToWeapon) > -0.3f;
		if (WeaponInfoWidgetComp)
		{
			WeaponInfoWidgetComp->SetVisibility(false);
		}

		if (AYogHUD* HUD = GetYogHUDForPlayer(NearbyPlayer.Get()))
		{
			if (bShouldShow)
				HUD->ShowWeaponFloatInfoAtLocation(
					WeaponDefinition,
					GetActorLocation() + FVector(0.f, 0.f, WidgetZOffset));
			else
				HUD->HideWeaponFloatInfo(WeaponDefinition);
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
		if (!bCollapsingForPickup)
		{
			if (AYogHUD* HUD = GetYogHUDForPlayer(Player))
			{
				HUD->HideWeaponFloatInfo(WeaponDefinition);
			}
		}
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
	APlayerCharacterBase* PreviousNearbyPlayer = NearbyPlayer.Get();

	bPickedUp            = false;
	bCollapsingForPickup = false;
	bPlayerInRange       = false;
	NearbyPlayer         = nullptr;

	if (WeaponInfoWidgetComp)
		WeaponInfoWidgetComp->SetVisibility(false);

	if (AYogHUD* HUD = GetYogHUDForPlayer(PreviousNearbyPlayer))
	{
		HUD->HideWeaponFloatInfo(WeaponDefinition);
	}

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
	if (NewWeapon && !bDisableLegacyHeatBackpackRuneForCardTestSpawner)
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
	if (!bDisableLegacyHeatBackpackRuneForCardTestSpawner && Player->BackpackGridComponent)
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
		if (WeaponDefinition->GameplayAbilityComboGraph)
		{
			ComboRuntime->LoadComboGraph(WeaponDefinition->GameplayAbilityComboGraph);
		}
		else
		{
			ComboRuntime->LoadComboConfig(WeaponDefinition->WeaponComboConfig);
		}
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

	// HUD 信息区内折叠浮窗 → 缩略图飞向左下角武器图标
	if (AYogHUD* HUD = GetYogHUDForPlayer(Player))
	{
		bCollapsingForPickup = true;
		const bool bStartedHudCollapse = HUD->StartWeaponFloatPickup(
			WeaponDefinition,
			GetActorLocation() + FVector(0.f, 0.f, 80.f),
			PickupCollapseDuration,
			FSimpleDelegate::CreateWeakLambda(this, [this]()
			{
				bCollapsingForPickup = false;
				if (WeaponInfoWidgetComp)
				{
					WeaponInfoWidgetComp->SetVisibility(false);
				}
			}));

		if (!bStartedHudCollapse)
		{
			bCollapsingForPickup = false;
			UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] HUD weapon float collapse fallback used."));
		}
	}
	else
	{
		bCollapsingForPickup = false;
		UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] HUD=NULL — 动画不会触发"));
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



