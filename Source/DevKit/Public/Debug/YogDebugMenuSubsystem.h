#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "YogDebugMenuSubsystem.generated.h"

/**
 * Central registry for project debug ImGui windows. Each window registers a Label
 * and a pointer to its own bVisible flag; the menu shows a checkbox per entry.
 *
 * Hidden by default. Toggle the menu itself with the console command `Yog.DebugMenu`.
 * Once visible, click any checkbox to show/hide individual debug windows.
 */
UCLASS()
class DEVKIT_API UYogDebugMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override
	{
		return !IsRunningCommandlet();
	}

	/** Register a debug window. Visible must point to a bool that outlives the
	 *  window's participation here — call UnregisterWindow from the window's
	 *  Deinitialize. Re-registering the same Id replaces the previous entry. */
	void RegisterWindow(FName Id, FString Label, bool* Visible);
	void UnregisterWindow(FName Id);

private:
	void DrawImGui();
	void ToggleMenu();

	struct FEntry
	{
		FName   Id;
		FString Label;
		bool*   Visible = nullptr;
	};

	TArray<FEntry>                  Entries;
	FDelegateHandle                 ImGuiHandle;
	TUniquePtr<FAutoConsoleCommand> ToggleCommand;
	bool                            bMenuVisible = false;
};
