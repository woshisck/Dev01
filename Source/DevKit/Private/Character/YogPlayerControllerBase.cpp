// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/YogPlayerControllerBase.h"
#include <AbilitySystemGlobals.h>
#include "Character/YogCharacterBase.h"
#include "UI/BackpackScreenWidget.h"
#include "UI/LootSelectionWidget.h"
#include "UI/YogHUD.h"
#include "Blueprint/UserWidget.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/YogCameraPawn.h"
#include "Camera/YogPlayerCameraManager.h"
#include "Character/PlayerCharacterBase.h"

#include "Character/YogCharacterBase.h"
#include <EnhancedInputSubsystems.h>
#include "Item/ItemSpawner.h"
#include "Map/RewardPickup.h"
#include "Map/SacrificeGracePickup.h"
#include "Map/AltarActor.h"
#include "Map/ShopActor.h"
#include "Map/Portal.h"
#include "Item/Weapon/WeaponSpawner.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"

#include "Component/BufferComponent.h"
#include "Component/CombatDeckComponent.h"
#include "Component/CombatItemComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "AbilitySystemComponent.h"



AYogPlayerControllerBase::AYogPlayerControllerBase()
{
	PlayerCameraManagerClass = AYogPlayerCameraManager::StaticClass();
}

void AYogPlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

	//UGameInstance* GameInstancePtr = Cast<UGameInstance>(GetWorld()->GetGameInstance());
	//UYogSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UYogSaveSubsystem>();

	//if (SaveSubsystem->CurrentSaveGame)
	//{
	//	SaveSubsystem->LoadSaveGame(SaveSubsystem->CurrentSaveGame);
	//}

}

void AYogPlayerControllerBase::OnUnPossess()
{
	Super::OnUnPossess();
}

UYogAbilitySystemComponent* AYogPlayerControllerBase::GetYogAbilitySystemComponent() const
{
	APawn* PossessdPawn = GetPawn();
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PossessdPawn);
	return Cast<UYogAbilitySystemComponent>(ASC) ? Cast<UYogAbilitySystemComponent>(ASC) : nullptr;


}

void AYogPlayerControllerBase::SetEnableRotationRate(FRotator RotationRate, bool isEnable)
{
	if (isEnable)
	{
		
		AYogCharacterBase* OwnedCharacter =Cast<AYogCharacterBase>(this->GetPawn());
		OwnedCharacter->GetCharacterMovement()->RotationRate = RotationRate;
	}
	else
	{

	}
}


void AYogPlayerControllerBase::SpawnCameraPawn(AYogCharacterBase* TargetCharacter) {
	//Get possessed character and spawn camera pawn attached on it
	if (IsValid(TargetCharacter))
	{
		FVector Location = TargetCharacter->GetActorLocation();
		FRotator Rotation = FRotator::ZeroRotator;
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AYogCameraPawn* CameraActorPawn = GetWorld()->SpawnActor<AYogCameraPawn>(CameraPawnClass, Location, Rotation, SpawnParams);
		CameraActorPawn->SetOwner(TargetCharacter);
		if (APlayerCharacterBase* PlayerChar = Cast<APlayerCharacterBase>(TargetCharacter))
		{
			PlayerChar->SetOwnCamera(CameraActorPawn);
		}
		this->SetViewTargetWithBlend(CameraActorPawn, 0.0f, EViewTargetBlendFunction::VTBlend_Linear, 0.0f, false);
	}


}

void AYogPlayerControllerBase::SetPlayerState(EYogCharacterState newState)
{

}

void AYogPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// 创建战斗 HUD（最底层，z=0）
	if (CombatHUDClass && IsLocalController())
	{
		CombatHUDWidget = CreateWidget<UUserWidget>(this, CombatHUDClass);
		if (CombatHUDWidget)
			CombatHUDWidget->AddToViewport(0);
	}

	// 创建三选一 UI（在 GameMode 广播前必须存在，NativeConstruct 里完成事件绑定）
	if (LootSelectionWidgetClass && IsLocalController())
	{
		LootSelectionWidget = CreateWidget<ULootSelectionWidget>(this, LootSelectionWidgetClass);
		if (LootSelectionWidget)
		{
			LootSelectionWidget->AddToViewport(10);
			// CommonUI 控制显隐，无需手动 SetVisibility
			LootSelectionWidget->OnActivated().AddUObject(this, &AYogPlayerControllerBase::OnMenuWidgetActivated);
			LootSelectionWidget->OnDeactivated().AddUObject(this, &AYogPlayerControllerBase::OnMenuWidgetDeactivated);
		}
	}

	// 创建背包 UI
	if (BackpackWidgetClass && IsLocalController())
	{
		BackpackWidget = CreateWidget<UBackpackScreenWidget>(this, BackpackWidgetClass);
		if (BackpackWidget)
		{
			BackpackWidget->AddToViewport(10);
			// CommonUI 控制显隐，无需手动 SetVisibility
			BackpackWidget->OnActivated().AddUObject(this, &AYogPlayerControllerBase::OnMenuWidgetActivated);
			BackpackWidget->OnDeactivated().AddUObject(this, &AYogPlayerControllerBase::OnMenuWidgetDeactivated);
		}
	}


	
	//AYogCharacterBase* TargetCharacter = Cast<AYogCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	
	// 相机已切换为 AYogPlayerCameraManager + SpringArm + CameraComponent 方案
	// SpawnCameraPawn 已废弃，PlayerCameraManagerClass 在构造函数中设置

}

void AYogPlayerControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (Input_MoveAction)
		{
			const FEnhancedInputActionEventBinding& moveBinding = EnhancedInputComp->BindAction(Input_MoveAction, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::Move);
			MoveInputHandle = moveBinding.GetHandle();
		}
		if (Input_LightAttack)
		{
			const FEnhancedInputActionEventBinding& lightAttackBinding = EnhancedInputComp->BindAction(Input_LightAttack, ETriggerEvent::Started, this, &AYogPlayerControllerBase::LightAtack);
			LightAttackInputHandle = lightAttackBinding.GetHandle();
		}
		if (Input_HeavyAttack)
		{
			const FEnhancedInputActionEventBinding& heavyAttackBinding = EnhancedInputComp->BindAction(Input_HeavyAttack, ETriggerEvent::Started, this, &AYogPlayerControllerBase::HeavyAtack);
			HeavyAttackInputHandle = heavyAttackBinding.GetHandle();
			// Completed fires when the key is released — sends GameplayEvent so WaitGameplayEvent in GA can fire
			EnhancedInputComp->BindAction(Input_HeavyAttack, ETriggerEvent::Completed, this, &AYogPlayerControllerBase::HeavyAttackReleased);
		}
		if (Input_Reload)
		{
			const FEnhancedInputActionEventBinding& reloadBinding = EnhancedInputComp->BindAction(Input_Reload, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::MusketReload);
			ReloadInputHandle = reloadBinding.GetHandle();
		}
		if (Input_UseCombatItem)
		{
			const FEnhancedInputActionEventBinding& itemBinding = EnhancedInputComp->BindAction(Input_UseCombatItem, ETriggerEvent::Started, this, &AYogPlayerControllerBase::UseCombatItem);
			UseCombatItemInputHandle = itemBinding.GetHandle();
		}
		if (Input_SwitchCombatItem)
		{
			const FEnhancedInputActionEventBinding& switchItemBinding = EnhancedInputComp->BindAction(Input_SwitchCombatItem, ETriggerEvent::Started, this, &AYogPlayerControllerBase::SwitchCombatItem);
			SwitchCombatItemInputHandle = switchItemBinding.GetHandle();
		}
		if (Input_SwitchCombatItemPrevious)
		{
			const FEnhancedInputActionEventBinding& switchPrevItemBinding = EnhancedInputComp->BindAction(Input_SwitchCombatItemPrevious, ETriggerEvent::Started, this, &AYogPlayerControllerBase::SwitchCombatItemPrevious);
			SwitchCombatItemPreviousInputHandle = switchPrevItemBinding.GetHandle();
		}
		if (Input_Dash)
		{
			const FEnhancedInputActionEventBinding& dashBinding = EnhancedInputComp->BindAction(Input_Dash, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::Dash);
			DashInputHandle = dashBinding.GetHandle();
		}
		if (Input_Interact)
		{
			const FEnhancedInputActionEventBinding& interactBinding = EnhancedInputComp->BindAction(Input_Interact, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::Interact);
			InteractInputHandle = interactBinding.GetHandle();
		}
		if (Input_OpenBackpack)
		{
			const FEnhancedInputActionEventBinding& backpackBinding = EnhancedInputComp->BindAction(Input_OpenBackpack, ETriggerEvent::Started, this, &AYogPlayerControllerBase::ToggleBackpack);
			OpenBackpackInputHandle = backpackBinding.GetHandle();
		}
		if (Input_CameraLook)
		{
			// Triggered: 右摇杆拨动时持续更新轴值
			const FEnhancedInputActionEventBinding& lookBinding = EnhancedInputComp->BindAction(Input_CameraLook, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::CameraLook);
			CameraLookInputHandle = lookBinding.GetHandle();
			// Completed: 右摇杆回中时将轴值重置为零
			EnhancedInputComp->BindAction(Input_CameraLook, ETriggerEvent::Completed, this, &AYogPlayerControllerBase::CameraLookReleased);
		}

	}

}

bool AYogPlayerControllerBase::InputKey(const FInputKeyParams& Params)
{
	if (Params.Event == IE_Pressed && HandleMenuBackInput(Params.Key))
	{
		return true;
	}

	if (Params.Event == IE_Pressed && !bBlockGameInput)
	{
		if (Params.Key == EKeys::F
			|| Params.Key == EKeys::Gamepad_FaceButton_Top
			|| Params.Key == EKeys::Gamepad_LeftShoulder
			|| Params.Key == EKeys::Gamepad_RightShoulder
			|| Params.Key == EKeys::Gamepad_LeftTrigger
			|| Params.Key == EKeys::Gamepad_RightTrigger)
		{
			if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
			{
				if (PlayerCharacter->CombatItemComponent && PlayerCharacter->CombatItemComponent->UseActiveItem())
				{
					return true;
				}
			}
		}
		else if (Params.Key == EKeys::Q || Params.Key == EKeys::Gamepad_DPad_Right)
		{
			if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
			{
				if (PlayerCharacter->CombatItemComponent)
				{
					PlayerCharacter->CombatItemComponent->SelectNextItem();
					return true;
				}
			}
		}
		else if (Params.Key == EKeys::Gamepad_DPad_Left)
		{
			if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
			{
				if (PlayerCharacter->CombatItemComponent)
				{
					PlayerCharacter->CombatItemComponent->SelectPreviousItem();
					return true;
				}
			}
		}
	}

	return Super::InputKey(Params);
}

bool AYogPlayerControllerBase::HandleMenuBackInput(const FKey& Key)
{
	const bool bBackKey = Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
	const bool bMenuKey = Key == EKeys::Gamepad_Special_Right;
	if (!bBackKey && !bMenuKey)
	{
		return false;
	}

	if (AYogHUD* HUD = Cast<AYogHUD>(GetHUD()))
	{
		if (HUD->CloseTopMostOverlay())
		{
			return true;
		}
	}

	if (BackpackWidget && BackpackWidget->IsActivated())
	{
		BackpackWidget->DeactivateWidget();
		return true;
	}

	if (bBackKey && LootSelectionWidget && LootSelectionWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		LootSelectionWidget->SkipSelection();
		return true;
	}

	if (Key == EKeys::Escape || bMenuKey)
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(GetHUD()))
		{
			HUD->OpenPauseMenu();
			return true;
		}
	}

	return false;
}

AYogCharacterBase* AYogPlayerControllerBase::GetControlledCharacter()
{
	AYogCharacterBase* MyCharacter = Cast<AYogCharacterBase>(GetPawn());
	return MyCharacter;
}

void AYogPlayerControllerBase::OnInteractTriggered(const AItemSpawner* item)
{
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn());

	UE_LOG(LogTemp, Warning, TEXT("OnInteractTriggered::item is triggered"));
}

//--------------------------------------------
// Controller Input
//--------------------------------------------

void AYogPlayerControllerBase::SetBlockGameInput(bool bBlock, bool bUIOnly)
{
	bBlockGameInput = bBlock;
	if (bBlock)
	{
		SetShowMouseCursor(true);
		if (bUIOnly)
		{
			// 三选一：UIOnly 模式，LMB 完全给 Slate，不会被攻击 Enhanced Input 消耗
			FInputModeUIOnly InputMode;
			SetInputMode(InputMode);
		}
		else
		{
			// 背包：GameAndUI 模式，Tab 键仍可触发 Enhanced Input 关闭背包
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
		}
	}
	else
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
	}
}

void AYogPlayerControllerBase::LightAtack(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		if (UBufferComponent* Buffer = player->GetInputBufferComponent())
		{
			Buffer->RecordLightAttack();
		}

		if (player->ComboRuntimeComponent && player->ComboRuntimeComponent->HasComboSource())
		{
			player->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Light, player);
			return;
		}

		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.LightAtk")));
		player->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
	}
	//UE_LOG(LogTemp, Log, TEXT("LightAtack"));
}
void AYogPlayerControllerBase::HeavyAtack(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		if (UBufferComponent* Buffer = player->GetInputBufferComponent())
		{
			Buffer->RecordHeavyAttack();
		}

		if (player->ComboRuntimeComponent && player->ComboRuntimeComponent->HasComboSource())
		{
			player->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Heavy, player);
			return;
		}

		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.HeavyAtk")));
		player->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
	}
	//UE_LOG(LogTemp, Log, TEXT("HeavyAtack"));
}


void AYogPlayerControllerBase::HeavyAttackReleased(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		UAbilitySystemComponent* ASC = player->GetASC();
		if (!ASC) return;
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(
			FGameplayTag::RequestGameplayTag(FName("GameplayEvent.Musket.HeavyRelease")),
			&EventData);
	}
}

void AYogPlayerControllerBase::MusketReload(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.Reload")));
		player->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
	}
}

void AYogPlayerControllerBase::UseCombatItem(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
	{
		if (PlayerCharacter->CombatItemComponent)
		{
			PlayerCharacter->CombatItemComponent->UseActiveItem();
		}
	}
}

void AYogPlayerControllerBase::SwitchCombatItem(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
	{
		if (PlayerCharacter->CombatItemComponent)
		{
			PlayerCharacter->CombatItemComponent->SelectNextItem();
		}
	}
}

void AYogPlayerControllerBase::SwitchCombatItemPrevious(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
	{
		if (PlayerCharacter->CombatItemComponent)
		{
			PlayerCharacter->CombatItemComponent->SelectPreviousItem();
		}
	}
}

void AYogPlayerControllerBase::Dash(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		// 冲刺前先将角色朝向对齐最后一次移动输入方向
		// 放在 TryActivateAbilitiesByTag 之前，确保 GA 激活时朝向已经正确
		if (!player->LastInputDirection.IsNearlyZero())
		{
			const FRotator DashFacing(0.f, player->LastInputDirection.Rotation().Yaw, 0.f);
			player->SetActorRotation(DashFacing);
		}

		bool bActivated = false;
		const bool bHasGraphDash = player->ComboRuntimeComponent && player->ComboRuntimeComponent->HasDashInputNode();
		if (bHasGraphDash)
		{
			bActivated = player->ComboRuntimeComponent->TryActivateDash(player);
		}
		else
		{
			bool bSavedDashCombo = false;
			if (player->ComboRuntimeComponent)
			{
				bSavedDashCombo = player->ComboRuntimeComponent->SaveCurrentNodeForDash();
			}
			if (bSavedDashCombo && player->CombatDeckComponent)
			{
				player->CombatDeckComponent->SavePendingLinkContextForDash();
			}

			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.Dash")));
			bActivated = player->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
			if (!bActivated)
			{
				if (player->ComboRuntimeComponent)
				{
					player->ComboRuntimeComponent->ClearSavedDashNode();
				}
				if (player->CombatDeckComponent)
				{
					player->CombatDeckComponent->ClearDashSavedLinkContext();
				}
			}
		}

		player->GetInputBufferComponent()->RecordDash();

		if (bActivated)
		{
			player->GetASC()->OnDashExecuted.Broadcast();
		}
	}
	//UE_LOG(LogTemp, Log, TEXT("Dash"));
}

void AYogPlayerControllerBase::Move(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;
	if (const AYogCharacterBase* YogPawn = Cast<AYogCharacterBase>(GetPawn()))
	{
		if (!YogPawn->bMovable)
		{
			return;
		}
	}
	// Get the movement vector (2D: X = forward/back, Y = right/left)
	FVector2D Input = Value.Get<FVector2D>();
	FVector2D Rotated = Input.GetRotated(-45.0f);


	FVector MoveDir(Rotated.X, Rotated.Y, 0.0f);

	if (!MoveDir.IsNearlyZero())
	{
		// Get yaw-only rotation from the vector
		FRotator DesiredRotation = MoveDir.Rotation();

		if (APawn* ControlledPawn = GetPawn())
		{
			// Option A: snap instantly
			//ControlledPawn->SetActorRotation(DesiredRotation);

			 //Option B: smooth interpolation
			 //FRotator Current = ControlledPawn->GetActorRotation();
			 //FRotator NewRotation = FMath::RInterpTo(Current, DesiredRotation, GetWorld()->GetDeltaSeconds(), 10.0f);
			 //ControlledPawn->SetActorRotation(NewRotation);

			// Apply movement
			ControlledPawn->AddMovementInput(MoveDir.GetSafeNormal(), 1.0f);
		}
	}

	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		player->GetInputBufferComponent()->RecordMove(Input);

		// 记录最后一次非零输入方向，供冲刺朝向使用
		if (!MoveDir.IsNearlyZero())
		{
			player->LastInputDirection = MoveDir.GetSafeNormal();
		}
	}

	//const FRotator playerTowards = UKismetMathLibrary::Conv_VectorToRotator(FVector(Rotated, 0));

	//SetControlRotation(playerTowards);

	////FRotator UKismetMathLibrary::Conv_VectorToRotator(FVector InVec)
	//if (APawn* ControlledPawn = GetPawn())
	//{
	//	
	//	//void AController::SetControlRotation(const FRotator & NewRotation)
	//	// Add movement input (forward/right)
	//	ControlledPawn->AddMovementInput(ControlledPawn->GetActorForwardVector(), Rotated.X);
	//	ControlledPawn->AddMovementInput(ControlledPawn->GetActorRightVector(), Rotated.Y);
	//}
	//UE_LOG(LogTemp, Log, TEXT("Move"));
}

void AYogPlayerControllerBase::Interact(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Log, TEXT("Interact"));

	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		if (player->OverlappingSpawner)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player is overlapping with spawner: %s"), *player->OverlappingSpawner->GetName());
		}

		// 范围内有武器 Spawner → 按 E 触发武器拾取
		if (player->PendingWeaponSpawner)
		{
			player->PendingWeaponSpawner->TryPickupWeapon(player);
		}
		// 范围内有奖励拾取物 → 按 E 触发拾取
		else if (player->PendingPickup)
		{
			player->PendingPickup->TryPickup(player);
		}
		// 范围内有献祭恩赐拾取物 → 按 E 触发获取
		else if (player->PendingSacrificePickup)
		{
			player->PendingSacrificePickup->TryPickup(player);
		}
		else if (player->PendingAltar)
		{
			player->PendingAltar->TryInteract(player);
		}
		else if (player->PendingShop)
		{
			player->PendingShop->TryInteract(player);
		}
		// 范围内有可进入的传送门 → 按 E 触发进入（v3 替代 Overlap 自动入门）
		// 设计约束：门与拾取物不会同范围，所以放在拾取物之后即可
		else if (player->PendingPortal)
		{
			player->PendingPortal->TryEnter(player);
		}
	}
}



void AYogPlayerControllerBase::ToggleBackpack(const FInputActionValue& Value)
{
	if (!BackpackWidget) return;

	// CommonUI：IsActivated() 判断是否当前激活
	if (BackpackWidget->IsActivated())
	{
		// NativeOnDeactivated 处理：SetPause(false) + 恢复输入 + 清除手柄状态
		BackpackWidget->DeactivateWidget();
	}
	else
	{
		// NativeOnActivated 处理：SetPause(true) + GameAndUI 输入 + 刷新网格
		BackpackWidget->ActivateWidget();
	}
}

void AYogPlayerControllerBase::OpenBackpack()
{
	if (BackpackWidget && !BackpackWidget->IsActivated())
	{
		BackpackWidget->ActivateWidget();
	}
}

void AYogPlayerControllerBase::OnMenuWidgetActivated()
{
	ActiveMenuCount++;
	bBlockGameInput = true;
	if (CombatHUDWidget && ActiveMenuCount == 1)
		CombatHUDWidget->SetVisibility(ESlateVisibility::Hidden);
}

void AYogPlayerControllerBase::OnMenuWidgetDeactivated()
{
	ActiveMenuCount = FMath::Max(0, ActiveMenuCount - 1);
	if (ActiveMenuCount == 0)
	{
		bBlockGameInput = false;
		if (CombatHUDWidget)
			CombatHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void AYogPlayerControllerBase::CameraLook(const FInputActionValue& Value)
{
	if (bBlockGameInput) return;

	if (AYogPlayerCameraManager* CM = Cast<AYogPlayerCameraManager>(PlayerCameraManager))
	{
		CM->SetCameraInputAxis(Value.Get<FVector2D>());
	}
}

void AYogPlayerControllerBase::CameraLookReleased(const FInputActionValue& Value)
{
	if (AYogPlayerCameraManager* CM = Cast<AYogPlayerCameraManager>(PlayerCameraManager))
	{
		CM->SetCameraInputAxis(FVector2D::ZeroVector);
	}
}

void AYogPlayerControllerBase::ToggleInput(bool bEnable)
{
	if (bEnable)
	{
		// Enable input
		EnableInput(this); // Re-enables input if disabled
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
	}
	else
	{
		// Disable input
		DisableInput(this); // Disables all input
		// (Optional) Explicitly ignore move/look input
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}

}

