#include "System/YogRuntimeGMSubsystem.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/EnemyData.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/YogGameMode.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "System/YogRuntimeGMSettings.h"
#include "UI/YogRuntimeGMWidget.h"
#include "UI/YogUIManagerSubsystem.h"

#define LOCTEXT_NAMESPACE "YogRuntimeGMSubsystem"

void UYogRuntimeGMSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UYogRuntimeGMSettings* Settings = GetDefault<UYogRuntimeGMSettings>();
	RuntimeSpawnCount = Settings ? FMath::Max(1, Settings->SpawnCount) : 1;
	RuntimeSpawnRadius = Settings ? FMath::Max(100.f, Settings->SpawnRadius) : 1200.f;
	LastStatus = LOCTEXT("Ready", "Runtime GM 就绪。");
}

bool UYogRuntimeGMSubsystem::ToggleGMPanel(APlayerController* PlayerController)
{
	const UYogRuntimeGMSettings* Settings = GetDefault<UYogRuntimeGMSettings>();
	if (!Settings || !Settings->bEnableRuntimeGM)
	{
		SetStatus(LOCTEXT("Disabled", "Runtime GM 已在 Project Settings 中关闭。"));
		return false;
	}

	APlayerController* PC = ResolvePlayerController(PlayerController);
	if (!PC)
	{
		SetStatus(LOCTEXT("NoPlayerController", "Runtime GM 打开失败：找不到 PlayerController。"));
		return false;
	}

	if (IsGMPanelOpen())
	{
		CloseGMPanel(PC);
		return true;
	}

	TSubclassOf<UYogRuntimeGMWidget> WidgetClass = Settings->RuntimeGMWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		WidgetClass = UYogRuntimeGMWidget::StaticClass();
	}

	bool bOpenedViaUIManager = false;
	if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
		{
			FYogUIScreenInputPolicy Policy;
			Policy.bShowMouseCursor = true;
			Policy.bPauseGame = false;
			Policy.bDisablePawnInput = false;
			Policy.bAffectsMajorUI = false;

			UIManager->SetWidgetClassOverride(EYogUIScreenId::RuntimeGM, WidgetClass);
			UIManager->SetInputPolicyOverride(EYogUIScreenId::RuntimeGM, Policy);
			ActiveWidget = UIManager->PushTypedScreen<UYogRuntimeGMWidget>(EYogUIScreenId::RuntimeGM);
			bOpenedViaUIManager = ActiveWidget != nullptr;
			bActiveWidgetManagedByUIManager = bOpenedViaUIManager;
		}
	}

	if (!ActiveWidget)
	{
		ActiveWidget = CreateWidget<UYogRuntimeGMWidget>(PC, WidgetClass);
		bActiveWidgetManagedByUIManager = false;
	}
	if (!ActiveWidget)
	{
		SetStatus(LOCTEXT("WidgetCreateFailed", "Runtime GM 打开失败：Widget 创建失败。"));
		return false;
	}

	ActiveWidget->InitializeRuntimeGM(this);
	ActiveWidget->SetVisibility(ESlateVisibility::Visible);
	ActiveWidget->SetIsEnabled(true);
	ActiveWidget->SetRenderOpacity(1.f);
	ActiveWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 0.f, 0.f));
	ActiveWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
	ActiveWidget->SetPositionInViewport(FVector2D(24.f, 96.f), false);
	ActiveWidget->SetDesiredSizeInViewport(FVector2D(560.f, 460.f));

	constexpr int32 RuntimeGMZOrder = 100000;
	bool bAddedToViewportFallback = false;
	if (!bOpenedViaUIManager && !ActiveWidget->IsInViewport())
	{
		ActiveWidget->AddToViewport(RuntimeGMZOrder);
		bAddedToViewportFallback = true;
	}
	if (!ActiveWidget->IsActivated())
	{
		ActiveWidget->ActivateWidget();
	}
	ActiveWidget->ForceLayoutPrepass();

	if (!bOpenedViaUIManager)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[RuntimeGM] Panel opened. Widget=%s Class=%s Player=%s World=%s Route=%s AddedToViewportFallback=%d InViewport=%d Activated=%d"),
		*GetNameSafe(ActiveWidget),
		*GetNameSafe(ActiveWidget->GetClass()),
		*GetNameSafe(PC),
		*GetNameSafe(PC->GetWorld()),
		bOpenedViaUIManager ? TEXT("UIManager") : TEXT("ViewportFallback"),
		bAddedToViewportFallback ? 1 : 0,
		ActiveWidget->IsInViewport() ? 1 : 0,
		ActiveWidget->IsActivated() ? 1 : 0);

	SetStatus(LOCTEXT("PanelOpened", "Runtime GM 面板已打开。"));
	return true;
}

void UYogRuntimeGMSubsystem::CloseGMPanel(APlayerController* PlayerController)
{
	const bool bWasManagedByUIManager = bActiveWidgetManagedByUIManager;
	if (ActiveWidget)
	{
		bool bClosedByUIManager = false;
		if (bWasManagedByUIManager)
		{
			if (APlayerController* PC = ResolvePlayerController(PlayerController))
			{
				if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
				{
					if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
					{
						UIManager->PopScreen(EYogUIScreenId::RuntimeGM);
						UIManager->ClearInputPolicyOverride(EYogUIScreenId::RuntimeGM);
						bClosedByUIManager = true;
					}
				}
			}
		}

		if (!bClosedByUIManager)
		{
			ActiveWidget->DeactivateWidget();
			ActiveWidget->RemoveFromParent();
		}
		ActiveWidget = nullptr;
	}
	bActiveWidgetManagedByUIManager = false;

	if (!bWasManagedByUIManager)
	{
		if (APlayerController* PC = ResolvePlayerController(PlayerController))
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
	}

	SetStatus(LOCTEXT("PanelClosed", "Runtime GM 面板已关闭。"));
}

bool UYogRuntimeGMSubsystem::GiveConfiguredWeapon(APlayerController* PlayerController)
{
	APlayerCharacterBase* Player = ResolvePlayerCharacter(PlayerController);
	if (!Player)
	{
		SetStatus(LOCTEXT("GiveWeaponNoPlayer", "给武器失败：找不到玩家角色。"));
		return false;
	}

	UWeaponDefinition* Weapon = LoadConfiguredWeapon();
	if (!Weapon)
	{
		SetStatus(LOCTEXT("GiveWeaponNoWeapon", "给武器失败：Project Settings -> Game -> Yog Runtime GM 未配置 WeaponDefinition。"));
		return false;
	}

	Weapon->SetupWeaponToCharacter(Player->GetMesh(), Player);
	SetStatus(FText::Format(
		LOCTEXT("GiveWeaponSuccess", "已给予玩家武器：{0}。"),
		FText::FromString(GetNameSafe(Weapon))));
	return true;
}

int32 UYogRuntimeGMSubsystem::SpawnConfiguredEnemies(APlayerController* PlayerController, int32 OverrideCount)
{
	APlayerController* PC = ResolvePlayerController(PlayerController);
	APlayerCharacterBase* Player = ResolvePlayerCharacter(PC);
	if (!PC || !Player)
	{
		SetStatus(LOCTEXT("SpawnNoPlayer", "刷敌失败：找不到玩家角色。"));
		return 0;
	}

	UEnemyData* EnemyData = LoadConfiguredEnemyData();
	if (!EnemyData || !EnemyData->EnemyClass)
	{
		SetStatus(LOCTEXT("SpawnNoEnemyData", "刷敌失败：Project Settings -> Game -> Yog Runtime GM 未配置有效 EnemyData。"));
		return 0;
	}

	AYogGameMode* GameMode = PC->GetWorld() ? PC->GetWorld()->GetAuthGameMode<AYogGameMode>() : nullptr;
	if (!GameMode)
	{
		SetStatus(LOCTEXT("SpawnNoGameMode", "刷敌失败：当前 World 不是 YogGameMode。"));
		return 0;
	}

	const UYogRuntimeGMSettings* Settings = GetDefault<UYogRuntimeGMSettings>();
	const int32 Count = FMath::Max(1, OverrideCount > 0 ? OverrideCount : RuntimeSpawnCount);
	const float MinDistance = Settings ? Settings->SpawnMinDistance : 300.f;
	const float ZOffset = Settings ? Settings->SpawnZOffset : 96.f;
	const bool bCountForLevelClear = Settings && Settings->bSpawnedEnemiesCountForLevelClear;
	const bool bApplyRoomBuffs = Settings && Settings->bApplyRoomBuffsToSpawnedEnemies;

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		AEnemyCharacterBase* Enemy = GameMode->SpawnGMEnemyNearPlayer(
			EnemyData,
			Player->GetActorLocation(),
			RuntimeSpawnRadius,
			MinDistance,
			ZOffset,
			bCountForLevelClear,
			bApplyRoomBuffs);
		SpawnedCount += Enemy ? 1 : 0;
	}

	SetStatus(FText::Format(
		LOCTEXT("SpawnSuccess", "已刷出 {0}/{1} 个敌人：{2}。"),
		FText::AsNumber(SpawnedCount),
		FText::AsNumber(Count),
		FText::FromString(GetNameSafe(EnemyData))));
	return SpawnedCount;
}

int32 UYogRuntimeGMSubsystem::ResetPlayerAndEnemies(APlayerController* PlayerController)
{
	APlayerController* PC = ResolvePlayerController(PlayerController);
	UWorld* World = PC ? PC->GetWorld() : GetWorld();
	if (!World)
	{
		SetStatus(LOCTEXT("ResetNoWorld", "重置失败：找不到 World。"));
		return 0;
	}

	int32 ResetCount = 0;
	if (APlayerCharacterBase* Player = ResolvePlayerCharacter(PC))
	{
		ResetCharacterForGM(Player);
		++ResetCount;
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacterBase::StaticClass(), Enemies);
	for (AActor* Actor : Enemies)
	{
		if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Actor))
		{
			if (Enemy->IsAlive())
			{
				ResetCharacterForGM(Enemy);
				++ResetCount;
			}
		}
	}

	SetStatus(FText::Format(
		LOCTEXT("ResetSuccess", "已重置玩家/存活敌人的生命、护甲、护盾和当前动作状态：{0} 个角色。"),
		FText::AsNumber(ResetCount)));
	return ResetCount;
}

void UYogRuntimeGMSubsystem::SetRuntimeSpawnCount(int32 InSpawnCount)
{
	RuntimeSpawnCount = FMath::Clamp(InSpawnCount, 1, 50);
}

void UYogRuntimeGMSubsystem::SetRuntimeSpawnRadius(float InSpawnRadius)
{
	RuntimeSpawnRadius = FMath::Max(100.f, InSpawnRadius);
}

bool UYogRuntimeGMSubsystem::IsGMPanelOpen() const
{
	return ActiveWidget && ActiveWidget->IsInViewport() && ActiveWidget->IsActivated();
}

UWeaponDefinition* UYogRuntimeGMSubsystem::LoadConfiguredWeapon() const
{
	const UYogRuntimeGMSettings* Settings = GetDefault<UYogRuntimeGMSettings>();
	return Settings ? Settings->WeaponDefinition.LoadSynchronous() : nullptr;
}

UEnemyData* UYogRuntimeGMSubsystem::LoadConfiguredEnemyData() const
{
	const UYogRuntimeGMSettings* Settings = GetDefault<UYogRuntimeGMSettings>();
	return Settings ? Settings->EnemyData.LoadSynchronous() : nullptr;
}

APlayerController* UYogRuntimeGMSubsystem::ResolvePlayerController(APlayerController* PlayerController) const
{
	if (PlayerController)
	{
		return PlayerController;
	}

	UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

APlayerCharacterBase* UYogRuntimeGMSubsystem::ResolvePlayerCharacter(APlayerController* PlayerController) const
{
	APlayerController* PC = ResolvePlayerController(PlayerController);
	return PC ? Cast<APlayerCharacterBase>(PC->GetPawn()) : nullptr;
}

void UYogRuntimeGMSubsystem::SetStatus(const FText& Status)
{
	LastStatus = Status;
	if (ActiveWidget)
	{
		ActiveWidget->RefreshFromSubsystem();
	}
}

void UYogRuntimeGMSubsystem::ResetCharacterForGM(AYogCharacterBase* Character) const
{
	if (!Character)
	{
		return;
	}

	Character->CustomTimeDilation = 1.f;
	Character->ResetRuntimeGMDeathState();
	Character->UpdateCharacterState(EYogCharacterState::Idle);
	Character->EnableMovement();
	Character->UnblockMovementControl();

	if (UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		ASC->CancelAllAbilities();
		const float MaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth > 0.f)
		{
			ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), MaxHealth);
		}
		const float MaxArmorHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxArmorHPAttribute());
		if (MaxArmorHP > 0.f)
		{
			ASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), MaxArmorHP);
		}
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetShieldAttribute(), 0.f);
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), 0.f);
	}

	if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		Mesh->bPauseAnims = false;
		Mesh->SetHiddenInGame(false);
	}
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}
}

#undef LOCTEXT_NAMESPACE
