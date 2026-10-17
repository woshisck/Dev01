#include "RenderStyle/YogRenderStyleSettings.h"

#include "Components/SceneComponent.h"
#include "Engine/Level.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "StylizedLightingSettings.h"

namespace
{
	TArray<TWeakObjectPtr<UYogRenderStyleComponent>> RegisteredStyles;

	bool SupportsWorld(const UWorld* World)
	{
		return World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Game);
	}

	float SafeValue(float Value, float Min, float Max)
	{
		return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : 1.0f;
	}

	class FYogNativeRenderStyleExtension final : public FSceneViewExtensionBase
	{
	public:
		explicit FYogNativeRenderStyleExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {}

		virtual void BeginRenderViewFamily(FSceneViewFamily& Family) override
		{
			// Retired global/level display grading. Preserve the camera, volumes,
			// native Film curve and exposure exactly; only lighting layers run.
		}
	};
}

FYogMaterialLightingStyle FYogMaterialLightingStyle::Sanitized() const
{
	FYogMaterialLightingStyle Result = *this;
	auto Clamp = [](float Value, float Min, float Max, float Default) { return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : Default; };
	Result.DiffuseContrast = Clamp(DiffuseContrast, .5f, 2.f, 1.f);
	Result.RampBands = Clamp(RampBands, 2.f, 8.f, 5.f);
	Result.RampAmount = Clamp(RampAmount, 0.f, 1.f, 0.f);
	Result.RampSoftness = Clamp(RampSoftness, .05f, .5f, .35f);
	Result.ShadowContrast = Clamp(ShadowContrast, .5f, 2.f, 1.f);
	Result.IndirectContrast = Clamp(IndirectContrast, .5f, 2.f, 1.f);
	Result.IndirectPivot = Clamp(IndirectPivot, .01f, 16.f, .5f);
	Result.IndirectAmount = Clamp(IndirectAmount, 0.f, 1.f, 0.f);
	Result.HighlightWidth = Clamp(HighlightWidth, .25f, 4.f, 1.f);
	Result.HighlightFalloff = Clamp(HighlightFalloff, 1.f, 4.f, 1.f);
	Result.ReflectionContrast = Clamp(ReflectionContrast, .5f, 2.f, 1.f);
	Result.ReflectionAmount = Clamp(ReflectionAmount, 0.f, 1.f, 0.f);
	Result.ShadowTint = FLinearColor(Clamp(ShadowTint.R, .1f, 2.f, 1.f), Clamp(ShadowTint.G, .1f, 2.f, 1.f), Clamp(ShadowTint.B, .1f, 2.f, 1.f), 1.f);
	Result.ShadowTintAmount = Clamp(ShadowTintAmount, 0.f, 1.f, 0.f);
	return Result;
}

void FYogMaterialLightingStyle::ApplyToRenderer(bool bGlobalEnabled) const
{
	const FYogMaterialLightingStyle Safe = Sanitized();
	auto Set = [](const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name)) Variable->Set(Value, ECVF_SetByCode);
	};
	// Keep serialized presets loadable, but retire their diffuse/GGX/shadow
	// and GI response modifications. Only the separate lighting layers run.
	Set(TEXT("r.Yog.MaterialLighting.Enable"), 0.f);
	Set(TEXT("r.Yog.MaterialLighting.DefaultLit"), Safe.bAffectDefaultLit ? 1.f : 0.f);
	Set(TEXT("r.Yog.MaterialLighting.Character"), Safe.bAffectCharacters ? 1.f : 0.f);
	Set(TEXT("r.Yog.MaterialLighting.DiffuseContrast"), Safe.DiffuseContrast);
	Set(TEXT("r.Yog.MaterialLighting.Bands"), Safe.RampBands);
	Set(TEXT("r.Yog.MaterialLighting.RampAmount"), Safe.RampAmount);
	Set(TEXT("r.Yog.MaterialLighting.RampSoftness"), Safe.RampSoftness);
	Set(TEXT("r.Yog.MaterialLighting.ShadowContrast"), Safe.ShadowContrast);
	Set(TEXT("r.Yog.MaterialLighting.IndirectContrast"), Safe.IndirectContrast);
	Set(TEXT("r.Yog.MaterialLighting.IndirectPivot"), Safe.IndirectPivot);
	Set(TEXT("r.Yog.MaterialLighting.IndirectAmount"), Safe.IndirectAmount);
	Set(TEXT("r.Yog.MaterialLighting.HighlightWidth"), Safe.HighlightWidth);
	Set(TEXT("r.Yog.MaterialLighting.HighlightFalloff"), Safe.HighlightFalloff);
	Set(TEXT("r.Yog.MaterialLighting.ReflectionContrast"), Safe.ReflectionContrast);
	Set(TEXT("r.Yog.MaterialLighting.ReflectionAmount"), Safe.ReflectionAmount);
	Set(TEXT("r.Yog.MaterialLighting.ShadowTintR"), Safe.ShadowTint.R);
	Set(TEXT("r.Yog.MaterialLighting.ShadowTintG"), Safe.ShadowTint.G);
	Set(TEXT("r.Yog.MaterialLighting.ShadowTintB"), Safe.ShadowTint.B);
	Set(TEXT("r.Yog.MaterialLighting.ShadowTintAmount"), Safe.ShadowTintAmount);
}

FYogNativeRenderStyleSettings FYogNativeRenderStyleSettings::Sanitized() const
{
	FYogNativeRenderStyleSettings Result = *this;
	Result.GlobalContrast = SafeValue(GlobalContrast, .5f, 1.5f);
	Result.GlobalSaturation = SafeValue(GlobalSaturation, 0.f, 2.f);
	Result.ShadowsContrast = SafeValue(ShadowsContrast, .5f, 1.5f);
	Result.MidtonesContrast = SafeValue(MidtonesContrast, .5f, 1.5f);
	Result.ShadowsGamma = SafeValue(ShadowsGamma, .8f, 1.2f);
	return Result;
}

bool FYogNativeRenderStyleSettings::IsNeutral() const
{
	return !bEnabled || !bEnablePostProcessGrading || (GlobalContrast == 1.f && GlobalSaturation == 1.f && ShadowsContrast == 1.f
		&& MidtonesContrast == 1.f && ShadowsGamma == 1.f);
}

void FYogNativeRenderStyleSettings::ApplyToPostProcess(FPostProcessSettings& Settings) const
{
	const FYogNativeRenderStyleSettings Safe = Sanitized();
	if (Safe.IsNeutral()) return;
	// W is the native all-channel multiplier. RGB is kept intact for authored level color palettes.
	Settings.ColorContrast.W *= Safe.GlobalContrast;
	Settings.ColorSaturation.W *= Safe.GlobalSaturation;
	Settings.ColorContrastShadows.W *= Safe.ShadowsContrast;
	// Native smooth shadows/midtones/highlights weights keep transitions continuous.
	// No luminance bands, EV ranges, hard black floor or indirect-light multiplier.
	Settings.ColorContrastMidtones.W *= Safe.MidtonesContrast;
	Settings.ColorGammaShadows.W *= Safe.ShadowsGamma;
}

void UYogRenderStyleSettings::ApplyToRenderer()
{
	Style = Style.Sanitized();
	Lighting = Lighting.Sanitized();
	Lighting.ApplyToRenderer(Style.bEnabled);
	GetMutableDefault<UStylizedLightingSettings>()->ApplyToConsoleVariables();
}

void UYogRenderStyleSettings::ResetToNativeDefaults()
{
	Style = FYogNativeRenderStyleSettings();
	Style.bEnabled = false;
	Lighting = FYogMaterialLightingStyle();
	Lighting.bEnabled = false;
	ApplyToRenderer();
}

UYogRenderStyleComponent::UYogRenderStyleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UYogRenderStyleComponent::OnRegister()
{
	Super::OnRegister();
	if (!IsTemplate() && SupportsWorld(GetWorld())) RegisteredStyles.AddUnique(this);
}

void UYogRenderStyleComponent::OnUnregister()
{
	RegisteredStyles.Remove(this);
	Super::OnUnregister();
}

AYogRenderStyleActor::AYogRenderStyleActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	StyleComponent = CreateDefaultSubobject<UYogRenderStyleComponent>(TEXT("RenderStyle"));
	PrimaryActorTick.bCanEverTick = false;
#if WITH_EDITOR
	SetIsSpatiallyLoaded(false);
#endif
}

FYogNativeRenderStyleSettings ResolveYogNativeRenderStyle(const UWorld* World, FString* OutSource)
{
	check(IsInGameThread());
	const FYogNativeRenderStyleSettings Global = GetDefault<UYogRenderStyleSettings>()->Style.Sanitized();
	if (OutSource) *OutSource = TEXT("项目全局默认 → 原生相机 / Post Process 调色 × 全局倍率");
	if (!SupportsWorld(World) || !Global.bEnabled)
	{
		FYogNativeRenderStyleSettings Native;
		Native.bEnabled = false;
		if (OutSource) *OutSource = TEXT("UE 原生（总开关关闭或非游戏 / 关卡视口）");
		return Native;
	}
	const UYogRenderStyleComponent* Best = nullptr;
	RegisteredStyles.RemoveAll([](const auto& Entry) { return !Entry.IsValid(); });
	for (const auto& Entry : RegisteredStyles)
	{
		const UYogRenderStyleComponent* Component = Entry.Get();
		const AActor* Owner = Component ? Component->GetOwner() : nullptr;
		if (!Component || !Component->bOverrideGlobalStyle || !Owner || Owner->GetWorld() != World
			|| Owner->IsActorBeingDestroyed() || !Owner->GetLevel() || !Owner->GetLevel()->bIsVisible) continue;
		if (!Best || Component->Priority > Best->Priority
			|| (Component->Priority == Best->Priority && Component->GetPathName() < Best->GetPathName())) Best = Component;
	}
	if (Best)
	{
		if (OutSource) *OutSource = FString::Printf(TEXT("关卡覆盖：%s（优先级 %d）"), *Best->GetOwner()->GetName(), Best->Priority);
		return Best->Style.Sanitized();
	}
	return Global;
}

TSharedRef<FSceneViewExtensionBase, ESPMode::ThreadSafe> CreateYogNativeRenderStyleExtension()
{
	return FSceneViewExtensions::NewExtension<FYogNativeRenderStyleExtension>();
}
