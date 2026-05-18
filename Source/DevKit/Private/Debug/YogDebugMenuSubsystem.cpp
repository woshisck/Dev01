#include "Debug/YogDebugMenuSubsystem.h"
#include "ImGuiDelegates.h"
#include "imgui.h"

void UYogDebugMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ImGuiHandle = FImGuiDelegates::OnMultiContextDebug().AddUObject(
		this, &UYogDebugMenuSubsystem::DrawImGui);

	ToggleCommand = MakeUnique<FAutoConsoleCommand>(
		TEXT("Yog.DebugMenu"),
		TEXT("Toggle the Yog Debug Menu (lists all registered debug windows)."),
		FConsoleCommandDelegate::CreateUObject(this, &UYogDebugMenuSubsystem::ToggleMenu));
}

void UYogDebugMenuSubsystem::Deinitialize()
{
	ToggleCommand.Reset();
	FImGuiDelegates::OnMultiContextDebug().Remove(ImGuiHandle);
	Entries.Reset();

	Super::Deinitialize();
}

void UYogDebugMenuSubsystem::RegisterWindow(FName Id, FString Label, bool* Visible)
{
	if (Id.IsNone() || !Visible)
	{
		return;
	}

	Entries.RemoveAll([Id](const FEntry& E) { return E.Id == Id; });
	Entries.Add({ Id, MoveTemp(Label), Visible });
	Entries.Sort([](const FEntry& A, const FEntry& B) { return A.Label < B.Label; });
}

void UYogDebugMenuSubsystem::UnregisterWindow(FName Id)
{
	Entries.RemoveAll([Id](const FEntry& E) { return E.Id == Id; });
}

void UYogDebugMenuSubsystem::ToggleMenu()
{
	bMenuVisible = !bMenuVisible;
}

void UYogDebugMenuSubsystem::DrawImGui()
{
	if (!bMenuVisible)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(280.f, 0.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(0.9f);

	if (!ImGui::Begin("Yog Debug Menu", &bMenuVisible))
	{
		ImGui::End();
		return;
	}

	if (Entries.IsEmpty())
	{
		ImGui::TextDisabled("No debug windows registered.");
	}
	else
	{
		for (const FEntry& Entry : Entries)
		{
			if (Entry.Visible)
			{
				ImGui::Checkbox(TCHAR_TO_UTF8(*Entry.Label), Entry.Visible);
			}
		}
	}

	ImGui::End();
}
