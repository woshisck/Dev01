// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Templates/SubclassOf.h"
#include "GSCConsoleManagerSubsystem.generated.h"

class UUserWidget;

/**
 * Local Player Subsystem to handle cheat commands, available in game. Namely to open
 * various debug UMG widgets.
 */
UCLASS(DisplayName = "GSC Console Manager")
class GASCOMPANION_API UGSCConsoleManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	//~UEngineSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of UEngineSubsystem interface

protected:
	
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> AbilityQueueDebugWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ComboDebugWidget;

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> AbilityQueueWidgetClass;

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> ComboWidgetClass;

	virtual APawn* GetPawnFromLocalPlayerController() const;
	virtual APlayerController* GetPlayerController() const;

private:

	TArray<IConsoleCommand*> RegisteredCommands;

	void ResetRegisteredCommands();
	FString GetConsoleCommandSuffix();
	void RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldArgsAndOutputDeviceDelegate& Command, uint32 Flags = ECVF_Default);

	void ExecToggleComboWidget(const TArray<FString>& Args, UWorld* InWorld, FOutputDevice& Ar);
	void ToggleComboDebugWidgetWithPC(APlayerController* PC);

	/** Create and display, or hide if visible, debug widget for GAS Companion combo component and ability. */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion | Debug")
	void ToggleComboDebugWidget();
	
	void ExecToggleAbilityQueueWidget(const TArray<FString>& Args, UWorld* InWorld, FOutputDevice& Ar);
	void ToggleAbilityQueueWidgetWithPC(APlayerController* PC);
	
	/** Create and display, or hide if visible, debug widget for GAS Companion ability queue system. */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion | Debug")
	void ToggleAbilityQueueDebugWidget();

	UUserWidget* CreateAbilityQueueWidget(APlayerController* PC) const;
	void ShowAbilityQueueWidget(APlayerController* PC);
	void HideAbilityQueueWidget() const;
	bool IsAbilityQueueWidgetVisible() const;

	UUserWidget* CreateComboWidget(APlayerController* PC) const;
	void ShowComboWidget(APlayerController* PC);
	void HideComboWidget() const;
	bool IsComboWidgetVisible() const;
};
