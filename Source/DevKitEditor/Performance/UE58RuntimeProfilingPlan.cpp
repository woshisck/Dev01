#include "Performance/UE58RuntimeProfilingPlan.h"

namespace
{
TArray<FString> BuildCommonCaptureCommands()
{
	return {
		TEXT("stat unit"),
		TEXT("stat rhi"),
		TEXT("stat scenerendering"),
		TEXT("stat gpu"),
		TEXT("r.MeshDrawCommands.LogDynamicInstancingStats 1"),
		TEXT("profilegpu")
	};
}

FUE58RuntimeProfilingScenario MakeScenario(
	const TCHAR* Name,
	const TCHAR* Tier,
	const TCHAR* Description,
	bool bRequiresBatchProxy,
	const TArray<FString>& CVars)
{
	FUE58RuntimeProfilingScenario Scenario;
	Scenario.Name = Name;
	Scenario.Tier = Tier;
	Scenario.Description = Description;
	Scenario.bRequiresBatchProxy = bRequiresBatchProxy;
	Scenario.CVars = CVars;
	Scenario.CaptureCommands = BuildCommonCaptureCommands();
	return Scenario;
}
}

TArray<FUE58RuntimeProfilingScenario> FUE58RuntimeProfilingPlanBuilder::BuildDefaultScenarios()
{
	return {
		MakeScenario(
			TEXT("Baseline_LumenOff_NoBatch"),
			TEXT("Medium"),
			TEXT("Current project renderer baseline: Lumen off, no generated batch proxy."),
			false,
			{
				TEXT("r.SetRes 1280x720"),
				TEXT("sg.GlobalIlluminationQuality 0"),
				TEXT("sg.ReflectionQuality 1"),
				TEXT("sg.ShadowQuality 1"),
				TEXT("r.ScreenPercentage 70"),
				TEXT("r.Lumen.DiffuseIndirect.Allow 0"),
				TEXT("t.MaxFPS 60")
			}),
		MakeScenario(
			TEXT("LumenLite_NoBatch"),
			TEXT("Handheld15W"),
			TEXT("Handheld 15W candidate with Lumen Lite enabled before proxy batching."),
			false,
			{
				TEXT("r.SetRes 1280x720"),
				TEXT("sg.GlobalIlluminationQuality 1"),
				TEXT("sg.ReflectionQuality 1"),
				TEXT("sg.ShadowQuality 1"),
				TEXT("r.ScreenPercentage 70"),
				TEXT("r.Lumen.DiffuseIndirect.Allow 1"),
				TEXT("r.Lumen.FinalGatherMethod 0"),
				TEXT("r.Lumen.TraceMeshSDFs.Allow 0"),
				TEXT("r.Lumen.HardwareRayTracing.HitLighting.Allowed 0"),
				TEXT("t.MaxFPS 60")
			}),
		MakeScenario(
			TEXT("BatchProxy_LumenOff"),
			TEXT("Medium"),
			TEXT("Generated geometry-merge proxy path with Lumen off."),
			true,
			{
				TEXT("r.SetRes 1280x720"),
				TEXT("sg.GlobalIlluminationQuality 0"),
				TEXT("sg.ReflectionQuality 1"),
				TEXT("sg.ShadowQuality 1"),
				TEXT("r.ScreenPercentage 70"),
				TEXT("r.Lumen.DiffuseIndirect.Allow 0"),
				TEXT("t.MaxFPS 60")
			}),
		MakeScenario(
			TEXT("BatchProxy_LumenLite"),
			TEXT("Handheld15W"),
			TEXT("Combined handheld candidate: generated proxy plus Lumen Lite."),
			true,
			{
				TEXT("r.SetRes 1280x720"),
				TEXT("sg.GlobalIlluminationQuality 1"),
				TEXT("sg.ReflectionQuality 1"),
				TEXT("sg.ShadowQuality 1"),
				TEXT("r.ScreenPercentage 70"),
				TEXT("r.Lumen.DiffuseIndirect.Allow 1"),
				TEXT("r.Lumen.FinalGatherMethod 0"),
				TEXT("r.Lumen.TraceMeshSDFs.Allow 0"),
				TEXT("r.Lumen.HardwareRayTracing.HitLighting.Allowed 0"),
				TEXT("t.MaxFPS 60")
			}),
		MakeScenario(
			TEXT("Handheld5W_LumenOff_Aggressive"),
			TEXT("Handheld5W"),
			TEXT("Low-power safety profile; Lumen remains off and the batch/proxy path should be preferred."),
			true,
			{
				TEXT("r.SetRes 1280x720"),
				TEXT("sg.ViewDistanceQuality 0"),
				TEXT("sg.ShadowQuality 0"),
				TEXT("sg.GlobalIlluminationQuality 0"),
				TEXT("sg.ReflectionQuality 0"),
				TEXT("sg.PostProcessQuality 0"),
				TEXT("sg.TextureQuality 1"),
				TEXT("sg.EffectsQuality 0"),
				TEXT("sg.FoliageQuality 0"),
				TEXT("r.ScreenPercentage 55"),
				TEXT("r.Lumen.DiffuseIndirect.Allow 0"),
				TEXT("t.MaxFPS 30")
			}),
		MakeScenario(
			TEXT("PCUltra_LumenHigh"),
			TEXT("PCUltra"),
			TEXT("PC upper-bound quality profile for comparison against handheld cuts."),
			false,
			{
				TEXT("r.SetRes 1920x1080"),
				TEXT("sg.ViewDistanceQuality 3"),
				TEXT("sg.ShadowQuality 3"),
				TEXT("sg.GlobalIlluminationQuality 3"),
				TEXT("sg.ReflectionQuality 3"),
				TEXT("sg.PostProcessQuality 3"),
				TEXT("sg.TextureQuality 3"),
				TEXT("sg.EffectsQuality 3"),
				TEXT("sg.FoliageQuality 3"),
				TEXT("r.ScreenPercentage 100"),
				TEXT("r.Lumen.DiffuseIndirect.Allow 1"),
				TEXT("t.MaxFPS 0")
			})
	};
}

TArray<FString> FUE58RuntimeProfilingPlanBuilder::BuildMarkdownReport(const FUE58RuntimeProfilingPlanOptions& Options)
{
	const TArray<FUE58RuntimeProfilingScenario> Scenarios = BuildDefaultScenarios();

	TArray<FString> Lines;
	Lines.Add(TEXT("# UE5.8 Runtime Profiling Plan"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Map: `%s`"), Options.MapPath.IsEmpty() ? TEXT("(not set)") : *Options.MapPath));
	Lines.Add(FString::Printf(TEXT("- Cluster: `%s`"), Options.ClusterName.IsEmpty() ? TEXT("(not set)") : *Options.ClusterName));
	Lines.Add(FString::Printf(TEXT("- Camera label: `%s`"), Options.CameraLabel.IsEmpty() ? TEXT("(not set)") : *Options.CameraLabel));
	Lines.Add(TEXT("- Evidence status: NotMeasured"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Rules"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Run every scenario from the same camera position and scene state."));
	Lines.Add(TEXT("- Record at least a five-second stable sample for `stat unit`, `stat rhi`, and `stat scenerendering`."));
	Lines.Add(TEXT("- Run `profilegpu` once per scenario and store the generated CSV path."));
	Lines.Add(TEXT("- Batch proxy scenarios require the generated proxy to be visible and matching source actors to be hidden or isolated in a test layer."));
	Lines.Add(TEXT("- This report is a measurement checklist; it does not contain measured GPU values."));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Scenario Matrix"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Scenario | Tier | Requires Batch Proxy | Description |"));
	Lines.Add(TEXT("| --- | --- | --- | --- |"));
	for (const FUE58RuntimeProfilingScenario& Scenario : Scenarios)
	{
		Lines.Add(FString::Printf(
			TEXT("| `%s` | %s | %s | %s |"),
			*Scenario.Name,
			*Scenario.Tier,
			Scenario.bRequiresBatchProxy ? TEXT("Yes") : TEXT("No"),
			*Scenario.Description));
	}
	Lines.Add(TEXT(""));

	for (const FUE58RuntimeProfilingScenario& Scenario : Scenarios)
	{
		Lines.Add(FString::Printf(TEXT("## Scenario: %s"), *Scenario.Name));
		Lines.Add(TEXT(""));
		Lines.Add(FString::Printf(TEXT("- Tier: `%s`"), *Scenario.Tier));
		Lines.Add(FString::Printf(TEXT("- Requires batch proxy: %s"), Scenario.bRequiresBatchProxy ? TEXT("Yes") : TEXT("No")));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("### Setup CVars"));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("```text"));
		for (const FString& CVar : Scenario.CVars)
		{
			Lines.Add(CVar);
		}
		Lines.Add(TEXT("```"));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("### Capture Commands"));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("```text"));
		for (const FString& Command : Scenario.CaptureCommands)
		{
			Lines.Add(Command);
		}
		Lines.Add(TEXT("```"));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("### Result Template"));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("| Metric | Value |"));
		Lines.Add(TEXT("| --- | --- |"));
		Lines.Add(TEXT("| FPS | NotMeasured |"));
		Lines.Add(TEXT("| Game ms | NotMeasured |"));
		Lines.Add(TEXT("| Draw ms | NotMeasured |"));
		Lines.Add(TEXT("| GPU ms | NotMeasured |"));
		Lines.Add(TEXT("| RHI ms | NotMeasured |"));
		Lines.Add(TEXT("| Mesh draw calls | NotMeasured |"));
		Lines.Add(TEXT("| Total draw calls | NotMeasured |"));
		Lines.Add(TEXT("| Visible dynamic primitives | NotMeasured |"));
		Lines.Add(TEXT("| Highest GPU pass | NotMeasured |"));
		Lines.Add(TEXT("| profilegpu CSV | NotMeasured |"));
		Lines.Add(TEXT("| Visual notes | NotMeasured |"));
		Lines.Add(TEXT(""));
	}

	Lines.Add(TEXT("## Acceptance Checks"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Handheld15W passes only if `BatchProxy_LumenLite` is stable at the target frame budget or has a documented fallback to `BatchProxy_LumenOff`."));
	Lines.Add(TEXT("- Handheld5W passes only if `Handheld5W_LumenOff_Aggressive` is stable at 30 FPS with acceptable visual loss."));
	Lines.Add(TEXT("- Batch proxy is considered useful only if mesh draw calls drop enough to offset any extra material shader cost."));
	Lines.Add(TEXT("- Lumen Lite is optional for handheld if Lumen passes consume more frame time than the visual gain justifies."));

	return Lines;
}
