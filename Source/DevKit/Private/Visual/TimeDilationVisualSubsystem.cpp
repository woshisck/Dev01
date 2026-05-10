#include "Visual/TimeDilationVisualSubsystem.h"

#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Misc/App.h"

void UTimeDilationVisualSubsystem::BeginTimeDilationVisual(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		if (UTimeDilationVisualSubsystem* Subsystem = World->GetSubsystem<UTimeDilationVisualSubsystem>())
		{
			Subsystem->BeginVisual();
		}
	}
}

void UTimeDilationVisualSubsystem::EndTimeDilationVisual(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		if (UTimeDilationVisualSubsystem* Subsystem = World->GetSubsystem<UTimeDilationVisualSubsystem>())
		{
			Subsystem->EndVisual();
		}
	}
}

void UTimeDilationVisualSubsystem::ForceClearTimeDilationVisual(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		if (UTimeDilationVisualSubsystem* Subsystem = World->GetSubsystem<UTimeDilationVisualSubsystem>())
		{
			Subsystem->ForceClearVisual();
		}
	}
}

void UTimeDilationVisualSubsystem::BeginVisual()
{
	EnsureVolume();
	++ActiveRequestCount;
	VisualAlpha = FMath::Max(VisualAlpha, MinimumStartAlpha);
	ApplyVisualState();
}

void UTimeDilationVisualSubsystem::EndVisual()
{
	ActiveRequestCount = FMath::Max(0, ActiveRequestCount - 1);
}

void UTimeDilationVisualSubsystem::ForceClearVisual()
{
	ActiveRequestCount = 0;
	VisualAlpha = 0.f;
	ApplyVisualState();
}

void UTimeDilationVisualSubsystem::Tick(float DeltaTime)
{
	const float TargetAlpha = ActiveRequestCount > 0 ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(VisualAlpha, TargetAlpha, 0.001f))
	{
		VisualAlpha = TargetAlpha;
		ApplyVisualState();
		return;
	}

	const float RealDelta = FMath::Max(FApp::GetDeltaTime(), DeltaTime);
	const float Duration = TargetAlpha > VisualAlpha ? FadeInDuration : FadeOutDuration;
	const float Step = Duration > 0.f ? RealDelta / Duration : 1.f;
	VisualAlpha = FMath::Clamp(VisualAlpha + (TargetAlpha > VisualAlpha ? Step : -Step), 0.f, 1.f);
	ApplyVisualState();
}

TStatId UTimeDilationVisualSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTimeDilationVisualSubsystem, STATGROUP_Tickables);
}

void UTimeDilationVisualSubsystem::Deinitialize()
{
	if (DesaturationVolume)
	{
		DesaturationVolume->Destroy();
		DesaturationVolume = nullptr;
	}

	ActiveRequestCount = 0;
	VisualAlpha = 0.f;

	Super::Deinitialize();
}

void UTimeDilationVisualSubsystem::EnsureVolume()
{
	if (DesaturationVolume || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = TEXT("TimeDilationDesaturationPPV");
	SpawnParams.ObjectFlags |= RF_Transient;

	DesaturationVolume = GetWorld()->SpawnActor<APostProcessVolume>(
		APostProcessVolume::StaticClass(),
		FTransform::Identity,
		SpawnParams);

	if (DesaturationVolume)
	{
		DesaturationVolume->bUnbound = true;
		DesaturationVolume->BlendWeight = 0.f;
		DesaturationVolume->Settings.bOverride_ColorSaturation = true;
		DesaturationVolume->Settings.ColorSaturation = FVector4(1.f, 1.f, 1.f, 1.f);
	}
}

void UTimeDilationVisualSubsystem::ApplyVisualState()
{
	EnsureVolume();
	if (!DesaturationVolume)
	{
		return;
	}

	const float Saturation = FMath::Lerp(1.f, TargetSaturation, VisualAlpha);
	DesaturationVolume->Settings.ColorSaturation = FVector4(1.f, 1.f, 1.f, Saturation);
	DesaturationVolume->BlendWeight = VisualAlpha > 0.001f ? 1.f : 0.f;
}
