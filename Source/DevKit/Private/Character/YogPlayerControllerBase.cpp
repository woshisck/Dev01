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
#include "Map/AltarActor.h"
#include "Map/ShopActor.h"
#include "Map/Portal.h"
#include "World/HubFacilityActor.h"
#include "Item/Weapon/WeaponSpawner.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "GameModes/YogGameMode.h"
#include "UI/YogInputKeyUtils.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"

#include "Component/BufferComponent.h"
#include "Component/CombatDeckComponent.h"
#include "Component/CombatItemComponent.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Component/PlayerSpecialAttackComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "AbilitySystemComponent.h"

#if !UE_BUILD_SHIPPING || DEVKIT_ENABLE_SHIPPING_CHEATS
#include "Cheater/Cheater.h"
#endif



AYogPlayerControllerBase::AYogPlayerControllerBase()
{
	PlayerCameraManagerClass = AYogPlayerCameraManager::StaticClass();

#if !UE_BUILD_SHIPPING || DEVKIT_ENABLE_SHIPPING_CHEATS
	CheatClass = UYogCheatManager::StaticClass();
#endif
}

void AYogPlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	SetBlockGameInput(false);

	if (APlayerCharacterBase* PossessedPlayer = Cast<APlayerCharacterBase>(InPawn))
	{
		TWeakObjectPtr<APlayerCharacterBase> WeakPlayer(PossessedPlayer);
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakPlayer]()
		{
			if (APlayerCharacterBase* DeferredPlayer = WeakPlayer.Get())
			{
				DeferredPlayer->RestoreRunStateFromGI();
				DeferredPlayer->GrantCraftedStarterRunesAsync();
			}
		}));
	}

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

#if !UE_BUILD_SHIPPING || DEVKIT_ENABLE_SHIPPING_CHEATS
	if (IsLocalController() && CheatManager == nullptr)
	{
		AddCheats(/*bForce=*/true);
	}
#endif

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// UI widget creation is owned by AYogHUD and UYogUIManagerSubsystem.
	
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
		UInputAction* NormalAttackAction = Input_NormalAttack ? Input_NormalAttack.Get() : Input_LightAttack.Get();
		if (NormalAttackAction)
		{
			const FEnhancedInputActionEventBinding& normalAttackBinding = EnhancedInputComp->BindAction(NormalAttackAction, ETriggerEvent::Started, this, &AYogPlayerControllerBase::NormalAttack);
			NormalAttackInputHandle = normalAttackBinding.GetHandle();
		}
		UInputAction* SpecialAttackAction = Input_SpecialAttack ? Input_SpecialAttack.Get() : Input_HeavyAttack.Get();
		if (SpecialAttackAction)
		{
			const FEnhancedInputActionEventBinding& specialAttackBinding = EnhancedInputComp->BindAction(SpecialAttackAction, ETriggerEvent::Started, this, &AYogPlayerControllerBase::SpecialAttack);
			SpecialAttackInputHandle = specialAttackBinding.GetHandle();
			// Completed fires when the key is released — sends GameplayEvent so WaitGameplayEvent in GA can fire
			EnhancedInputComp->BindAction(SpecialAttackAction, ETriggerEvent::Completed, this, &AYogPlayerControllerBase::SpecialAttackReleased);
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
		if (Input_UseActiveSkill)
		{
			const FEnhancedInputActionEventBinding& activeSkillBinding = EnhancedInputComp->BindAction(Input_UseActiveSkill, ETriggerEvent::Started, this, &AYogPlayerControllerBase::UseActiveSkill);
			UseActiveSkillInputHandle = activeSkillBinding.GetHandle();
		}
		if (Input_SwitchActiveSkill)
		{
			const FEnhancedInputActionEventBinding& switchActiveSkillBinding = EnhancedInputComp->BindAction(Input_SwitchActiveSkill, ETriggerEvent::Started, this, &AYogPlayerControllerBase::SwitchActiveSkill);
			SwitchActiveSkillInputHandle = switchActiveSkillBinding.GetHandle();
		}
		if (Input_SwitchWeapon)
		{
			const FEnhancedInputActionEventBinding& switchWeaponBinding = EnhancedInputComp->BindAction(Input_SwitchWeapon, ETriggerEvent::Started, this, &AYogPlayerControllerBase::SwitchWeapon);
			SwitchWeaponInputHandle = switchWeaponBinding.GetHandle();
		}
		UInputAction* WeaponSkillAction = Input_WeaponSkill ? Input_WeaponSkill.Get() : Input_Dash.Get();
		if (WeaponSkillAction)
		{
			const FEnhancedInputActionEventBinding& weaponSkillBinding = EnhancedInputComp->BindAction(WeaponSkillAction, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::WeaponSkill);
			WeaponSkillInputHandle = weaponSkillBinding.GetHandle();
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
		if (Input_PauseAction)
		{
			const FEnhancedInputActionEventBinding& pauseBinding = EnhancedInputComp->BindAction(Input_PauseAction, ETriggerEvent::Started, this, &AYogPlayerControllerBase::HandlePauseInput);
			PauseInputHandle = pauseBinding.GetHandle();
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

	auto BindPausedMenuKey = [this](const FKey& Key)
	{
		FInputKeyBinding Binding(FInputChord(Key), IE_Pressed);
		Binding.bConsumeInput = false;
		Binding.bExecuteWhenPaused = true;
		Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([this, Key]()
		{
			HandleMenuBackInput(Key);
		});
		InputComponent->KeyBindings.Add(Binding);
	};
	BindPausedMenuKey(EKeys::Escape);
	BindPausedMenuKey(EKeys::Virtual_Back);
	BindPausedMenuKey(EKeys::Gamepad_Special_Right);
	BindPausedMenuKey(EKeys::Gamepad_FaceButton_Right);

}

bool AYogPlayerControllerBase::InputKey(const FInputKeyParams& Params)
{
	if (Params.Event == IE_Pressed && HandleMenuBackInput(Params.Key))
	{
		return true;
	}

	if (!IsGameplayInputBlocked())
	{
		float WeaponFloatScrollDirection = 0.f;
		if (Params.Key == EKeys::MouseWheelAxis && !FMath::IsNearlyZero(Params.Delta.X))
		{
			WeaponFloatScrollDirection = -FMath::Sign(Params.Delta.X);
		}
		else if (Params.Event == IE_Pressed && Params.Key == EKeys::MouseScrollUp)
		{
			WeaponFloatScrollDirection = -1.f;
		}
		else if (Params.Event == IE_Pressed && Params.Key == EKeys::MouseScrollDown)
		{
			WeaponFloatScrollDirection = 1.f;
		}

		if (!FMath::IsNearlyZero(WeaponFloatScrollDirection))
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(GetHUD()))
			{
				if (HUD->ScrollWeaponFloatCards(WeaponFloatScrollDirection))
				{
					return true;
				}
			}
		}
	}

	if ((Params.Event == IE_Pressed || Params.Event == IE_Repeat) && !IsGameplayInputBlocked())
	{
		if (Params.Key == EKeys::Gamepad_DPad_Up || Params.Key == EKeys::Gamepad_LeftStick_Up)
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(GetHUD()))
			{
				if (HUD->ScrollWeaponFloatCards(-1.f))
				{
					return true;
				}
			}
		}
		else if (Params.Key == EKeys::Gamepad_DPad_Down || Params.Key == EKeys::Gamepad_LeftStick_Down)
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(GetHUD()))
			{
				if (HUD->ScrollWeaponFloatCards(1.f))
				{
					return true;
				}
			}
		}
	}

	return Super::InputKey(Params);
}

bool AYogPlayerControllerBase::HandleMenuBackInput(const FKey& Key)
{
	const bool bBackKey = YogInputKeys::IsBackKey(Key);
	const bool bMenuKey = YogInputKeys::IsMenuKey(Key);
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

bool AYogPlayerControllerBase::IsGameplayInputBlocked() const
{
	if (bBlockGameInput)
	{
		return true;
	}

	const APlayerCharacterBase* PlayerPawn = Cast<APlayerCharacterBase>(GetPawn());
	if (PlayerPawn && (PlayerPawn->bIsDead || PlayerPawn->IsWaitingForDeathReviveChoice()))
	{
		return true;
	}

	if (const AYogGameMode* GM = GetWorld() ? Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		return GM->IsPlayerDeathPending();
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
		// Always use GameAndUI for in-game UI. UIOnly broke D-pad navigation by killing
		// CommonUI's focus routing. Attacks are blocked by widget-side DisableInput, not by mode.
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
	}
}

void AYogPlayerControllerBase::NormalAttack(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		if (UBufferComponent* Buffer = player->GetInputBufferComponent())
		{
			Buffer->RecordNormalAttack();
		}

		static const FGameplayTag SpecialAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack"), false);
		if (SpecialAttackTag.IsValid())
		{
			if (UAbilitySystemComponent* ASC = player->GetASC())
			{
				if (ASC->HasMatchingGameplayTag(SpecialAttackTag))
				{
					const FGameplayTag CanComboTag =
						FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
					if (CanComboTag.IsValid() && ASC->GetTagCount(CanComboTag) > 0)
					{
						const bool bActivated = player->ComboRuntimeComponent
							&& player->ComboRuntimeComponent->HasComboSource()
							&& player->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Light, player);
						if (bActivated)
						{
							if (UBufferComponent* Buffer = player->GetInputBufferComponent())
							{
								Buffer->ClearBuffer();
							}

							FGameplayTagContainer SpecialAttackTags;
							SpecialAttackTags.AddTag(SpecialAttackTag);
							SpecialAttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Player.SpecialAttack"), false));
							ASC->CancelAbilities(&SpecialAttackTags);
						}
					}
					return;
				}
			}
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

void AYogPlayerControllerBase::LightAtack(const FInputActionValue& Value)
{
	NormalAttack(Value);
}

void AYogPlayerControllerBase::SpecialAttack(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		if (player->SpecialAttackComponent && player->SpecialAttackComponent->HasSpecialAttack())
		{
			player->SpecialAttackComponent->UseSpecialAttack();
			return;
		}

		if (UBufferComponent* Buffer = player->GetInputBufferComponent())
		{
			Buffer->RecordSpecialAttack();
		}

		// Fallback for characters without an equipped special attack.
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

void AYogPlayerControllerBase::HeavyAtack(const FInputActionValue& Value)
{
	SpecialAttack(Value);
}


void AYogPlayerControllerBase::SpecialAttackReleased(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
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

void AYogPlayerControllerBase::HeavyAttackReleased(const FInputActionValue& Value)
{
	SpecialAttackReleased(Value);
}

void AYogPlayerControllerBase::MusketReload(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.Reload")));
		player->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
	}
}

void AYogPlayerControllerBase::UseCombatItem(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
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
	if (IsGameplayInputBlocked()) return;
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
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
	{
		if (PlayerCharacter->CombatItemComponent)
		{
			PlayerCharacter->CombatItemComponent->SelectPreviousItem();
		}
	}
}

void AYogPlayerControllerBase::UseActiveSkill(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
	{
		if (PlayerCharacter->ActiveSkillComponent)
		{
			PlayerCharacter->ActiveSkillComponent->UseActiveSkill();
		}
	}
}

void AYogPlayerControllerBase::SwitchActiveSkill(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
	{
		if (PlayerCharacter->ActiveSkillComponent)
		{
			PlayerCharacter->ActiveSkillComponent->SelectNextSkill();
		}
	}
}

void AYogPlayerControllerBase::SwitchWeapon(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(GetPawn()))
	{
		PlayerCharacter->SwitchWeapon();
	}
}

void AYogPlayerControllerBase::HandlePauseInput(const FInputActionValue& Value)
{
	HandleMenuBackInput(EKeys::Gamepad_Special_Right);
}

void AYogPlayerControllerBase::WeaponSkill(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		// 冲刺前先将角色朝向对齐最后一次移动输入方向
		// 放在 TryActivateAbilitiesByTag 之前，确保 GA 激活时朝向已经正确
		if (!player->LastInputDirection.IsNearlyZero())
		{
			const FRotator DashFacing(0.f, player->LastInputDirection.Rotation().Yaw, 0.f);
			player->SetActorRotation(DashFacing);
		}

		if (player->ComboRuntimeComponent && player->ComboRuntimeComponent->TryActivateWeaponSkill(player))
		{
			if (UBufferComponent* Buffer = player->GetInputBufferComponent())
			{
				Buffer->RecordWeaponSkill();
			}
			return;
		}
	}
	//UE_LOG(LogTemp, Log, TEXT("Dash"));
}

void AYogPlayerControllerBase::Dash(const FInputActionValue& Value)
{
	WeaponSkill(Value);
}

void AYogPlayerControllerBase::Move(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
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
	if (IsGameplayInputBlocked()) return;
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
		// 范围内有主城设施 → 按 E 触发交互（打开对应 UI）
		else if (player->PendingFacility)
		{
			player->PendingFacility->Interact(player);
		}
	}
}



void AYogPlayerControllerBase::ToggleBackpack(const FInputActionValue& Value)
{
	if (AYogHUD* HUD = Cast<AYogHUD>(GetHUD()))
	{
		if (HUD->CloseTopMostOverlay())
		{
			return;
		}

		if (IsGameplayInputBlocked())
		{
			return;
		}

		const APlayerCharacterBase* PC = Cast<APlayerCharacterBase>(GetPawn());
		if (!PC || !PC->EquippedWeaponInstance)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[YogPlayerController] Backpack input ignored: no weapon equipped."));
			return;
		}

		HUD->OpenBackpack();
	}
}

void AYogPlayerControllerBase::OpenBackpack()
{
	const APlayerCharacterBase* PC = Cast<APlayerCharacterBase>(GetPawn());
	if (!PC || !PC->EquippedWeaponInstance)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[YogPlayerController] OpenBackpack ignored: no weapon equipped."));
		return;
	}

	if (AYogHUD* HUD = Cast<AYogHUD>(GetHUD()))
	{
		HUD->OpenBackpack();
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
	if (IsGameplayInputBlocked()) return;

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

