// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Subsystems/GSCConsoleManagerSubsystem.h"

#include "GSCLog.h"
#include "Components/GSCAbilityQueueComponent.h"
#include "Components/GSCComboManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "UI/GSCUWDebugAbilityQueue.h"
#include "UI/GSCUWDebugComboWidget.h"

void UGSCConsoleManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem: Initializing GAS Companion console manager subsystem."))
	Super::Initialize(Collection);

	AbilityQueueWidgetClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/UI/WB_DebugAbilityQueue.WB_DebugAbilityQueue_C"));
	if (!AbilityQueueWidgetClass)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem:Initialize() Failed to load AbilityQueueWidgetClass. If it was moved, please update the reference location in C++."));
	}

	ComboWidgetClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/UI/WB_Debug_Combo.WB_Debug_Combo_C"));
	if (!AbilityQueueWidgetClass)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem:Initialize() Failed to load ComboWidgetClass. If it was moved, please update the reference location in C++."));
	}

	ResetRegisteredCommands();
	
	// Register Console Commands
	RegisterConsoleCommand(
		*FString::Printf(TEXT("GASCompanion.Debug.Combo%s"), *GetConsoleCommandSuffix()),
		TEXT("Toogles display of Combo Widget"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateUObject(this, &UGSCConsoleManagerSubsystem::ExecToggleComboWidget)
	);
	
	RegisterConsoleCommand(
		*FString::Printf(TEXT("GASCompanion.Debug.AbilityQueue%s"), *GetConsoleCommandSuffix()),
		TEXT("Toogles display of AbilityQueue Widget"),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateUObject(this, &UGSCConsoleManagerSubsystem::ExecToggleAbilityQueueWidget)
	);
}

void UGSCConsoleManagerSubsystem::Deinitialize()
{
	GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem: Shutting down GAS Companion console manager subsystem"));

	ResetRegisteredCommands();
	Super::Deinitialize();
}

APawn* UGSCConsoleManagerSubsystem::GetPawnFromLocalPlayerController() const
{
	const APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem:GetPawnFromLocalPlayerController() Cannot get PlayerController."))
		return nullptr;
	}

	return PC->GetPawn();
}

APlayerController* UGSCConsoleManagerSubsystem::GetPlayerController() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem:GetPlayerController() Cannot get LocalPlayer."))
		return nullptr;
	}
	
	return LocalPlayer->GetPlayerController(LocalPlayer->GetWorld());
}

void UGSCConsoleManagerSubsystem::ResetRegisteredCommands()
{
	for (IConsoleCommand* RegisteredCommand : RegisteredCommands)
	{
		if (RegisteredCommand)
		{
			IConsoleManager::Get().UnregisterConsoleObject(RegisteredCommand);
		}
	}

	RegisteredCommands.Reset();
}

FString UGSCConsoleManagerSubsystem::GetConsoleCommandSuffix()
{
	if (GPlayInEditorID == 0)
	{
		return TEXT("");
	}

	return FString::Printf(TEXT(".PIE_%d"), GPlayInEditorID);
}

void UGSCConsoleManagerSubsystem::RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldArgsAndOutputDeviceDelegate& Command, uint32 Flags)
{
	if (!IConsoleManager::Get().IsNameRegistered(Name))
	{
		UWorld* World = GetWorld();
		GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem: RegisterConsoleCommand [%s]"), Name)
		RegisteredCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags));
	}
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UGSCConsoleManagerSubsystem::ExecToggleComboWidget(const TArray<FString>& Args, UWorld* InWorld, FOutputDevice& Ar)
{
	ToggleComboDebugWidgetWithPC(GetPlayerController());
}

void UGSCConsoleManagerSubsystem::ToggleComboDebugWidgetWithPC(APlayerController* PC)
{
	if (!PC)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem:ToggleComboDebugWidgetWithPC() Passed in Player Controller invalid"))
		return;
	}

	if (const APawn* Pawn = PC->GetPawn())
	{
		const UGSCComboManagerComponent* Component = Pawn->FindComponentByClass<UGSCComboManagerComponent>();
		if (!Component)
		{
			GSC_SLOG(Error, TEXT("UGSCConsoleManagerSubsystem:ToggleComboDebugWidget() Pawn %s doesn't have an ComboManagerComponent.\nMake sure to add it in Blueprint to your Pawn."), *Pawn->GetName())
			return;
		}

		if (IsComboWidgetVisible())
		{
			GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem:ToggleComboDebugWidget() Hide Combo Widget"))
			HideComboWidget();
		}
		else
		{
			GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem:ToggleComboDebugWidget() Show Combo Widget"))
			ShowComboWidget(PC);
		}
	}
}

void UGSCConsoleManagerSubsystem::ToggleComboDebugWidget()
{
	APlayerController* PlayerController = GetPlayerController();
	GSC_LOG(Log, TEXT("console manager subsystem: ToggleComboDebugWidget %s - Outer: %s"), *GetNameSafe(PlayerController), *GetNameSafe(GetOuter()));

	ToggleComboDebugWidgetWithPC(PlayerController);
}

void UGSCConsoleManagerSubsystem::ExecToggleAbilityQueueWidget(const TArray<FString>& Args, UWorld* InWorld, FOutputDevice& Ar)
{
	ToggleAbilityQueueWidgetWithPC(GetPlayerController());
}

void UGSCConsoleManagerSubsystem::ToggleAbilityQueueWidgetWithPC(APlayerController* PC)
{
	if (!PC)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem:ToggleAbilityQueueWidgetWithPC() Passed in Player Controller invalid"))
		return;
	}

	if (const APawn* Pawn = PC->GetPawn())
	{
		const UGSCAbilityQueueComponent* Component = Pawn->FindComponentByClass<UGSCAbilityQueueComponent>();
		if (!Component)
		{
			GSC_SLOG(Error, TEXT("UGSCConsoleManagerSubsystem:ToggleAbilityQueueDebugWidget() Pawn %s doesn't have an AbilityQueueComponent.\nMake sure to add it in Blueprint to your Pawn."), *Pawn->GetName())
			return;
		}

		if (IsAbilityQueueWidgetVisible())
		{
			GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem:ToggleAbilityQueueDebugWidget() Hide AbilityQueue Widget"))
			HideAbilityQueueWidget();
		}
		else
		{
			GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem:ToggleAbilityQueueDebugWidget() Show AbilityQueue Widget"))
			ShowAbilityQueueWidget(PC);
		}
	}
}

void UGSCConsoleManagerSubsystem::ToggleAbilityQueueDebugWidget()
{
	APlayerController* PlayerController = GetPlayerController();
	GSC_LOG(Log, TEXT("console manager subsystem: ToggleComboDebugWidget %s - Outer: %s"), *GetNameSafe(PlayerController), *GetNameSafe(GetOuter()));

	ToggleAbilityQueueWidgetWithPC(PlayerController);
}

UUserWidget* UGSCConsoleManagerSubsystem::CreateAbilityQueueWidget(APlayerController* PC) const
{
	GSC_LOG(Log, TEXT("UGSCConsoleManagerSubsystem::CreateAbilityQueueWidget()"))

	if (!PC)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem::CreateAbilityQueueWidget() No Player Controller"))
		return nullptr;
	}

	if (!AbilityQueueWidgetClass)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem::CreateAbilityQueueWidget() Widget Class for ability queue widget is null."))
		return nullptr;
	}

	UGSCUWDebugAbilityQueue* Widget = Cast<UGSCUWDebugAbilityQueue>(CreateWidget(PC, AbilityQueueWidgetClass));
	if (!Widget)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem::CreateAbilityQueueWidget() Couldn't create widget."))
		return nullptr;
	}

	Widget->SetOwnerActor(PC->GetPawn());
	return Widget;
}

void UGSCConsoleManagerSubsystem::ShowAbilityQueueWidget(APlayerController* PC)
{
	if (!AbilityQueueDebugWidget)
	{
		AbilityQueueDebugWidget = CreateAbilityQueueWidget(PC);
	}

	if (AbilityQueueDebugWidget)
	{
		AbilityQueueDebugWidget->AddToViewport();
	}
}

void UGSCConsoleManagerSubsystem::HideAbilityQueueWidget() const
{
	if (AbilityQueueDebugWidget)
	{
		AbilityQueueDebugWidget->RemoveFromParent();
	}
}

bool UGSCConsoleManagerSubsystem::IsAbilityQueueWidgetVisible() const
{
	return AbilityQueueDebugWidget && AbilityQueueDebugWidget->IsVisible();
}

UUserWidget* UGSCConsoleManagerSubsystem::CreateComboWidget(APlayerController* PC) const
{
	if (!PC)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem::CreateComboWidget() No Player Controller"))
		return nullptr;
	}

	if (!ComboWidgetClass)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem::CreateComboWidget() Widget Class for combo widget is null."))
		return nullptr;
	}

	UGSCUWDebugComboWidget* Widget = Cast<UGSCUWDebugComboWidget>(CreateWidget(PC, ComboWidgetClass));
	if (!Widget)
	{
		GSC_LOG(Error, TEXT("UGSCConsoleManagerSubsystem::CreateComboWidget() Couldn't create widget."))
		return nullptr;
	}

	Widget->SetOwnerActor(PC->GetPawn());
	return Widget;
}

void UGSCConsoleManagerSubsystem::ShowComboWidget(APlayerController* PC)
{
	if (!ComboDebugWidget)
	{
		ComboDebugWidget = CreateComboWidget(PC);
	}

	if (ComboDebugWidget)
	{
		ComboDebugWidget->AddToViewport();
	}
}

void UGSCConsoleManagerSubsystem::HideComboWidget() const
{
	if (ComboDebugWidget)
	{
		ComboDebugWidget->RemoveFromParent();
	}
}

bool UGSCConsoleManagerSubsystem::IsComboWidgetVisible() const
{
	return ComboDebugWidget && ComboDebugWidget->IsVisible();
}
