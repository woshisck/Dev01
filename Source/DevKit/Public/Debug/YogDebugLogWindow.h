#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Misc/OutputDevice.h"
#include "YogDebugLogWindow.generated.h"

/**
 * In-game ImGui window that captures UE_LOG Warning/Error/Display output and lets you
 * filter by tag prefix ([ComboRuntime], [CDC], GAS, Damage, etc.).
 *
 * Auto-created by the GameInstance subsystem system — no manual setup required.
 * Toggle ImGui input with the plugin's default shortcut (Ctrl+Alt+F1 or ImGui.InputEnabled 1).
 */
UCLASS()
class DEVKIT_API UYogDebugLogWindow : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override
	{
		return !IsRunningCommandlet();
	}

private:
	void DrawImGui();

	// ── Log capture ───────────────────────────────────────────────

	struct FLogEntry
	{
		float              Time      = 0.f;
		ELogVerbosity::Type Verbosity = ELogVerbosity::Log;
		FString            Message;
	};

	class FLogOutputDevice final : public FOutputDevice
	{
	public:
		static constexpr int32 MaxEntries = 512;

		TArray<FLogEntry>  Entries;
		mutable FCriticalSection Mutex;

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;
		virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	};

	TUniquePtr<FLogOutputDevice> LogDevice;
	FDelegateHandle              ImGuiHandle;
	double                       InitTime = 0.0;

	// ── UI state ──────────────────────────────────────────────────

	bool bAutoScroll   = true;
	bool bShowError    = true;
	bool bShowWarning  = true;
	bool bShowDisplay  = false;
	char FilterBuf[256] = {};
};
