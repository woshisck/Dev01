#include "Debug/YogDebugLogWindow.h"
#include "ImGuiDelegates.h"
#include "imgui.h"
#include "Misc/OutputDeviceRedirector.h"
#include "HAL/PlatformTime.h"

// ── FLogOutputDevice ─────────────────────────────────────────────────────────

void UYogDebugLogWindow::FLogOutputDevice::Serialize(
	const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
	// Only capture messages meaningful for gameplay debugging
	if (Verbosity > ELogVerbosity::Display)
		return;

	FScopeLock Lock(&Mutex);
	if (Entries.Num() >= MaxEntries)
		Entries.RemoveAt(0, 1, /*bAllowShrinking*/false);

	FLogEntry& E  = Entries.AddDefaulted_GetRef();
	E.Time        = (float)FPlatformTime::Seconds(); // subtracted by InitTime in draw
	E.Verbosity   = Verbosity;
	E.Message     = V;
}

// ── Subsystem lifecycle ───────────────────────────────────────────────────────

void UYogDebugLogWindow::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	InitTime  = FPlatformTime::Seconds();
	LogDevice = MakeUnique<FLogOutputDevice>();
	GLog->AddOutputDevice(LogDevice.Get());

	ImGuiHandle = FImGuiDelegates::OnMultiContextDebug().AddUObject(
		this, &UYogDebugLogWindow::DrawImGui);
}

void UYogDebugLogWindow::Deinitialize()
{
	FImGuiDelegates::OnMultiContextDebug().Remove(ImGuiHandle);

	if (LogDevice)
	{
		GLog->RemoveOutputDevice(LogDevice.Get());
		LogDevice.Reset();
	}

	Super::Deinitialize();
}

// ── ImGui draw ────────────────────────────────────────────────────────────────

void UYogDebugLogWindow::DrawImGui()
{
	ImGui::SetNextWindowSize(ImVec2(980.f, 440.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(20.f, 20.f),    ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(0.88f);

	if (!ImGui::Begin("Yog Debug Log"))
	{
		ImGui::End();
		return;
	}

	// ── Top controls ─────────────────────────────────────────────
	if (ImGui::Button("Clear"))
	{
		FScopeLock Lock(&LogDevice->Mutex);
		LogDevice->Entries.Empty();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &bAutoScroll);

	ImGui::SameLine(); ImGui::TextDisabled(" | ");
	ImGui::SameLine(); ImGui::Checkbox("Error",   &bShowError);
	ImGui::SameLine(); ImGui::Checkbox("Warning", &bShowWarning);
	ImGui::SameLine(); ImGui::Checkbox("Display", &bShowDisplay);

	// Entry count
	{
		FScopeLock Lock(&LogDevice->Mutex);
		ImGui::SameLine();
		ImGui::TextDisabled(" (%d/%d)", LogDevice->Entries.Num(), FLogOutputDevice::MaxEntries);
	}

	// ── Filter bar ───────────────────────────────────────────────
	ImGui::Spacing();
	ImGui::SetNextItemWidth(300.f);
	ImGui::InputText("##filter", FilterBuf, IM_ARRAYSIZE(FilterBuf));
	ImGui::SameLine(); ImGui::TextDisabled("Filter");

	// Preset buttons — set filter text on click
	auto Preset = [&](const char* Label, const char* Value)
	{
		ImGui::SameLine();
		const bool bActive = FCStringAnsi::Strcmp(FilterBuf, Value) == 0;
		if (bActive)
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		if (ImGui::SmallButton(Label))
			FCStringAnsi::Strncpy(FilterBuf, Value, IM_ARRAYSIZE(FilterBuf));
		if (bActive)
			ImGui::PopStyleColor();
	};

	Preset("All",         "");
	Preset("ComboRuntime","[ComboRuntime]");
	Preset("CDC",         "[CDC]");
	Preset("GAS",         "GAS");
	Preset("Damage",      "Damage");
	Preset("AI",          "[AI]");
	Preset("Wave",        "[Wave]");

	ImGui::Separator();

	// ── Scrollable log list ───────────────────────────────────────
	ImGui::BeginChild("##log_region", ImVec2(0.f, 0.f), false,
		ImGuiWindowFlags_HorizontalScrollbar);

	{
		FScopeLock Lock(&LogDevice->Mutex);
		const FString Filter = FString(UTF8_TO_TCHAR(FilterBuf));

		for (const FLogEntry& Entry : LogDevice->Entries)
		{
			// Verbosity filter
			if (Entry.Verbosity <= ELogVerbosity::Error   && !bShowError)   continue;
			if (Entry.Verbosity == ELogVerbosity::Warning  && !bShowWarning) continue;
			if (Entry.Verbosity == ELogVerbosity::Display  && !bShowDisplay) continue;

			// Text filter
			if (!Filter.IsEmpty() &&
				!Entry.Message.Contains(Filter, ESearchCase::IgnoreCase))
				continue;

			ImVec4      Color;
			const char* Tag;

			if (Entry.Verbosity <= ELogVerbosity::Error)
			{
				Color = { 1.00f, 0.30f, 0.30f, 1.f };
				Tag   = "[ERR]";
			}
			else if (Entry.Verbosity == ELogVerbosity::Warning)
			{
				Color = { 1.00f, 0.85f, 0.20f, 1.f };
				Tag   = "[WRN]";
			}
			else
			{
				Color = { 0.80f, 0.80f, 0.80f, 1.f };
				Tag   = "[DSP]";
			}

			const float RelTime = Entry.Time - (float)InitTime;
			ImGui::TextColored(Color, "%-5s %7.2fs  %s",
				Tag, RelTime, TCHAR_TO_UTF8(*Entry.Message));
		}
	}

	if (bAutoScroll)
		ImGui::SetScrollHereY(1.f);

	ImGui::EndChild();
	ImGui::End();
}
