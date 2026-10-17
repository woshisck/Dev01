#include "LookDev/YogLookDevLightingRig.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UnrealType.h"

namespace
{
template <typename TComponent>
TComponent* FindRigComponent(const AActor* Actor, const FName Name)
{
	TInlineComponentArray<TComponent*> Components;
	Actor->GetComponents(Components);
	for (TComponent* Component : Components)
	{
		if (Component && Component->GetFName() == Name)
		{
			return Component;
		}
	}
	return nullptr;
}

FName DirectionalComponentName(const EYogLookDevLightingScenario Scenario)
{
	switch (Scenario)
	{
	case EYogLookDevLightingScenario::LowContrast: return TEXT("DirectionalLight_Low");
	case EYogLookDevLightingScenario::HighContrast: return TEXT("DirectionalLight_High");
	default: return TEXT("DirectionalLight_Mid");
	}
}
}

AYogLookDevLightingRig::AYogLookDevLightingRig()
{
	bIsEditorOnlyActor = true;
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AYogLookDevLightingRig::ResetScenarioCalibration()
{
	switch (LightingScenario)
	{
	case EYogLookDevLightingScenario::LowContrast:
		DirectionalSettings.Intensity = 0.8f;
		SkySettings.Intensity = 2.5f;
		DirectionalSettings.Rotation = FRotator(-71.685211, -49.120483, 32.971264);
		break;
	case EYogLookDevLightingScenario::HighContrast:
		DirectionalSettings.Intensity = 7.f;
		SkySettings.Intensity = 0.42f;
		DirectionalSettings.Rotation = FRotator(-60.0, -50.500160, 12.080514);
		break;
	default:
		DirectionalSettings.Intensity = 2.3f;
		SkySettings.Intensity = 1.2f;
		DirectionalSettings.Rotation = FRotator(-45.999348, -49.999786, 0.000088);
		break;
	}
}

void AYogLookDevLightingRig::SetLightingScenario(const EYogLookDevLightingScenario NewScenario)
{
	if (LightingScenario != NewScenario)
	{
		LightingScenario = NewScenario;
		ResetScenarioCalibration();
	}
	ApplyLighting();
}

void AYogLookDevLightingRig::RefreshLighting()
{
	ApplyLighting();
}

UDirectionalLightComponent* AYogLookDevLightingRig::GetActiveDirectionalLight() const
{
	return FindRigComponent<UDirectionalLightComponent>(this, DirectionalComponentName(LightingScenario));
}

USkyLightComponent* AYogLookDevLightingRig::GetSkyLight() const
{
	return FindRigComponent<USkyLightComponent>(this, TEXT("Skylight"));
}

void AYogLookDevLightingRig::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLighting();
}

void AYogLookDevLightingRig::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	// Loading a saved level need not run its construction script. Wait until
	// the serialized SCS components are registered before restoring the rig.
	ApplyLighting();
}

#if WITH_EDITOR
void AYogLookDevLightingRig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(AYogLookDevLightingRig, LightingScenario))
	{
		ResetScenarioCalibration();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyLighting();
}

void AYogLookDevLightingRig::PostEditUndo()
{
	Super::PostEditUndo();
	ApplyLighting();
}
#endif

void AYogLookDevLightingRig::ApplyLighting()
{
	UDirectionalLightComponent* ActiveDirectional = GetActiveDirectionalLight();
	for (const FName Name : { FName(TEXT("DirectionalLight_Low")), FName(TEXT("DirectionalLight_Mid")), FName(TEXT("DirectionalLight_High")) })
	{
		if (UDirectionalLightComponent* Light = FindRigComponent<UDirectionalLightComponent>(this, Name))
		{
			Light->SetVisibility(Light == ActiveDirectional);
		}
	}
	if (ActiveDirectional)
	{
		ActiveDirectional->SetIntensity(DirectionalSettings.Intensity);
		ActiveDirectional->SetLightColor(DirectionalSettings.LightColor);
		ActiveDirectional->SetIndirectLightingIntensity(DirectionalSettings.IndirectLightingIntensity);
		ActiveDirectional->SetVolumetricScatteringIntensity(DirectionalSettings.VolumetricScatteringIntensity);
		ActiveDirectional->SetRelativeRotation(DirectionalSettings.Rotation);
	}

	if (USkyLightComponent* Sky = GetSkyLight())
	{
		Sky->SetIntensity(SkySettings.Intensity);
		Sky->SetLightColor(SkySettings.LightColor);
		Sky->SetOcclusionExponent(SkySettings.OcclusionExponent);
		Sky->SetMinOcclusion(SkySettings.MinOcclusion);
		Sky->SetOcclusionTint(SkySettings.OcclusionTint);
		if (Sky->IndirectLightingIntensity != SkySettings.IndirectLightingIntensity
			|| Sky->VolumetricScatteringIntensity != SkySettings.VolumetricScatteringIntensity
			|| Sky->OcclusionMaxDistance != SkySettings.OcclusionMaxDistance)
		{
			Sky->IndirectLightingIntensity = SkySettings.IndirectLightingIntensity;
			Sky->VolumetricScatteringIntensity = SkySettings.VolumetricScatteringIntensity;
			Sky->OcclusionMaxDistance = SkySettings.OcclusionMaxDistance;
			Sky->MarkRenderStateDirty();
		}
	}

	if (UPostProcessComponent* Post = FindRigComponent<UPostProcessComponent>(this, TEXT("PostProcess")))
	{
		Post->Settings.bOverride_AutoExposureMethod = true;
		Post->Settings.AutoExposureMethod = AEM_Manual;
		Post->Settings.bOverride_AutoExposureBias = true;
		Post->Settings.AutoExposureBias = ExposureCompensation;
	}
	if (UPostProcessComponent* Checker = FindRigComponent<UPostProcessComponent>(this, TEXT("PostProcess_PBRChecker")))
	{
		Checker->bEnabled = false;
		Checker->SetVisibility(false);
	}

	if (UStaticMeshComponent* Skydome = FindRigComponent<UStaticMeshComponent>(this, TEXT("Skydome")))
	{
		if (Skydome->GetMaterial(0))
		{
			if (UMaterialInstanceDynamic* Backdrop = Skydome->CreateDynamicMaterialInstance(0))
			{
				Backdrop->SetScalarParameterValue(TEXT("Mask Radius"), bGrounded ? 10000.f : 0.1f);
				Backdrop->SetScalarParameterValue(TEXT("PDO"), bGrounded ? 0.f : 3000.f);
				const float BaseBackground = LightingScenario == EYogLookDevLightingScenario::HighContrast ? 0.1f : 0.18f;
				Backdrop->SetScalarParameterValue(TEXT("Far Value"), BaseBackground * FMath::Pow(2.f, BackgroundCompensation));
			}
		}
	}

	// Preserve the saved calibration layout instead of re-running the old trace
	// that moved the group whenever the Blueprint construction script executed.
	for (const FName Name : { FName(TEXT("SpawnLocation")), FName(TEXT("Character_STM")) })
	{
		if (USceneComponent* Calibration = FindRigComponent<USceneComponent>(this, Name))
		{
			Calibration->SetVisibility(bCalibrator, true);
		}
	}
}
