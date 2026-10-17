#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Tools/Workflow/SDevKitGraphicsQualityWorkbenchWidget.h"
#include "Widgets/Input/SSpinBox.h"

namespace
{
	constexpr int32 TierCount = 4;
	const TCHAR* QualityCVars[] =
	{
		TEXT("r.Lumen.ScreenProbeGather.RadianceCache.NumProbesToTraceBudget"),
		TEXT("r.Shadow.Virtual.MaxPhysicalPages"),
		TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"),
		TEXT("r.Shadow.DistanceScale"), TEXT("r.ViewDistanceScale"),
		TEXT("r.StaticMeshLODDistanceScale"), TEXT("r.SkeletalMeshLODBias"),
		TEXT("r.Streaming.MipBias"), TEXT("r.MaxAnisotropy"), TEXT("r.MaterialQualityLevel"),
		TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectionalMoving"),
		TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"), TEXT("r.Shadow.Virtual.ResolutionLodBiasLocalMoving"),
		TEXT("r.Shadow.Virtual.Clipmap.FirstLevel"), TEXT("r.Shadow.Virtual.Clipmap.LastLevel"),
		TEXT("r.Shadow.Virtual.SMRT.RayCountDirectional"), TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayDirectional"),
		TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"), TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayLocal"),
		TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"), TEXT("r.Lumen.ScreenProbeGather.TracingOctahedronResolution"),
		TEXT("r.Lumen.Reflections.DownsampleFactor"),
		TEXT("r.Shadow.Virtual.Character.PreserveCasters"), TEXT("r.Shadow.Virtual.NonNanite.UseRadiusThreshold"),
		TEXT("r.Shadow.RadiusThreshold"), TEXT("r.Shadow.Virtual.CoarsePagePixelThresholdDynamic"), TEXT("r.Shadow.Virtual.Character.ReceiverOffsetCm")
	};
	constexpr int32 QualityParameterCount = static_cast<int32>(UE_ARRAY_COUNT(QualityCVars));
	const TCHAR* QualityGroups[] =
	{
		TEXT("GlobalIlluminationQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"),
		TEXT("ShadowQuality"), TEXT("ViewDistanceQuality"), TEXT("ViewDistanceQuality"),
		TEXT("ViewDistanceQuality"), TEXT("TextureQuality"), TEXT("TextureQuality"), TEXT("EffectsQuality"),
		TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"),
		TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"),
		TEXT("GlobalIlluminationQuality"), TEXT("GlobalIlluminationQuality"), TEXT("ReflectionQuality"),
		TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality"), TEXT("ShadowQuality")
	};
	static_assert(UE_ARRAY_COUNT(QualityGroups) == UE_ARRAY_COUNT(QualityCVars));

	/** Preserve all CVars affected by the test's group refresh, not only the ten editor rows. */
	struct FScopedQualityState
	{
		Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
		TMap<IConsoleVariable*, FDevKitQualityCVarSnapshot> CVars;

		FScopedQualityState()
		{
			auto Capture = [this](const TCHAR* Name)
			{
				if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
				{
					FDevKitQualityCVarSnapshot& Snapshot = CVars.FindOrAdd(Variable);
					Snapshot.Name = Name;
					Snapshot.Value = Variable->GetString();
					Snapshot.Flags = Variable->GetFlags();
				}
			};
			for (const TCHAR* Name : QualityCVars) Capture(Name);
			IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(
				FConsoleObjectVisitor::CreateLambda([&Capture](const TCHAR* Name, IConsoleObject* Object)
				{
					if (Object->AsVariable()) Capture(Name);
				}), TEXT("sg."));
			TArray<FString> Sections;
			if (GConfig) GConfig->GetSectionNames(GScalabilityIni, Sections);
			for (const FString& Section : Sections)
			{
				if (!Section.Contains(TEXT("Quality@"))) continue;
				TArray<FString> Entries;
				GConfig->GetSection(*Section, Entries, GScalabilityIni);
				for (const FString& Entry : Entries)
				{
					FString Name, Value;
					if (Entry.Split(TEXT("="), &Name, &Value)) Capture(*Name.TrimStartAndEnd());
				}
			}
		}

		~FScopedQualityState()
		{
			Scalability::SetQualityLevels(Levels, true);
			for (const auto& Pair : CVars)
			{
				Pair.Key->ClearFlags(ECVF_SetByMask);
				Pair.Key->Set(*Pair.Value.Value, static_cast<EConsoleVariableFlags>(Pair.Value.Flags & ECVF_SetByMask));
				Pair.Key->SetFlags(Pair.Value.Flags);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitGraphicsQualityCloudContractTest,
	"DevKit.Workflow.GraphicsQualityCloudContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitGraphicsQualityCloudContractTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Merged scalability config is available"), GConfig)) return false;
	FScopedQualityState Restore;
	Scalability::FQualityLevels InitialLevels = Restore.Levels;
	InitialLevels.SetFromSingleQualityLevel(2);
	Scalability::SetQualityLevels(InitialLevels, true);
	for (const TCHAR* Name : QualityCVars)
	{
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		if (!TestNotNull(FString::Printf(TEXT("Quality CVar is registered: %s"), Name), Variable)) return false;
		Variable->ClearFlags(ECVF_SetByMask);
		Variable->Set(Variable->GetFloat(), ECVF_SetByScalability);
	}
	IConsoleVariable* StaticLOD = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StaticMeshLODDistanceScale"));
	IConsoleVariable* ProbeBudget = IConsoleManager::Get().FindConsoleVariable(QualityCVars[0]);
	StaticLOD->Set(2.25f, ECVF_SetByScalability);
	ProbeBudget->Set(577, ECVF_SetByScalability);
	const Scalability::FQualityLevels BeforeOpen = Scalability::GetQualityLevels();
	const TSharedRef<SDevKitGraphicsQualityWorkbenchWidget> Widget = SNew(SDevKitGraphicsQualityWorkbenchWidget);
	TestTrue(TEXT("Construct does not apply a scalability preset"), BeforeOpen == Scalability::GetQualityLevels());
	TestEqual(TEXT("Construct does not replace a current custom LOD scale"), StaticLOD->GetFloat(), 2.25f);
	TestEqual(TEXT("Construct does not replace the current custom probe budget"), ProbeBudget->GetInt(), 577);
	TestEqual(TEXT("Opening snapshot excludes only retired CSM distance"), Widget->OpeningCVars.Num(), QualityParameterCount - 1);
	// Migrate the old ten-row format without reviving its ignored shadow values.
	TArray<FString> PreviousPresets;
	const bool bHadPresets = GConfig->GetArray(TEXT("DevKit.GraphicsQualityWorkbench"), TEXT("Preset"), PreviousPresets, GEditorPerProjectIni) > 0;
	FString Legacy = TEXT("AutomationLegacy|");
	for (int32 Index = 0; Index < 40; ++Index)
	{ if (Index) Legacy += TEXT(","); Legacy += Index == 5 * TierCount + 2 ? TEXT("1.75") : TEXT("1"); }
	TArray<FString> TemporaryPresets = {Legacy};
	GConfig->SetArray(TEXT("DevKit.GraphicsQualityWorkbench"), TEXT("Preset"), TemporaryPresets, GEditorPerProjectIni);
	Widget->LoadPresets();
	Widget->LoadPreset(TEXT("AutomationLegacy"));
	TestEqual(TEXT("Legacy matrix expands with native defaults"), Widget->CurrentValues.Num(), QualityParameterCount * TierCount);
	TestEqual(TEXT("Legacy active LOD tuning is preserved"), Widget->CurrentValues[5 * TierCount + 2], 1.75f);
	const auto* DefaultPreset = Widget->Presets.FindByPredicate([](const FDevKitQualityPreset& Item) { return Item.Name == TEXT("Default"); });
	if (TestNotNull(TEXT("Native Default remains present"), DefaultPreset))
	{
		TestEqual(TEXT("Legacy ignored VSM page slot is not reactivated"), Widget->CurrentValues[1 * TierCount + 2], DefaultPreset->Values[1 * TierCount + 2]);
		TestEqual(TEXT("New Lumen density uses native default"), Widget->CurrentValues[19 * TierCount + 2], DefaultPreset->Values[19 * TierCount + 2]);
	}
	if (bHadPresets) GConfig->SetArray(TEXT("DevKit.GraphicsQualityWorkbench"), TEXT("Preset"), PreviousPresets, GEditorPerProjectIni);
	else GConfig->RemoveKey(TEXT("DevKit.GraphicsQualityWorkbench"), TEXT("Preset"), GEditorPerProjectIni);
	Widget->LoadPresets();

	Widget->RestoreDefaults();
	for (int32 Row = 0; Row < QualityParameterCount; ++Row)
	{
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(QualityCVars[Row]);
		for (int32 Tier = 0; Tier < TierCount; ++Tier)
		{
			float Expected = FCString::Atof(*Variable->GetDefaultValue());
			const FString Section = FString::Printf(TEXT("%s@%d"), QualityGroups[Row], Tier);
			float ConfigValue = 0.f;
			if (GConfig->GetFloat(*Section, QualityCVars[Row], ConfigValue, GScalabilityIni)) Expected = ConfigValue;
			TestEqual(FString::Printf(TEXT("Default row %d tier %d matches merged Scalability / constructor"), Row, Tier),
				Widget->CurrentValues[Row * TierCount + Tier], Expected);
		}
	}
	TestEqual(TEXT("Static LOD default is the engine constructor, not the live override"), Widget->CurrentValues[5 * TierCount + 2], 1.f);
	TestEqual(TEXT("Published Epic probe budget is 150, not the former workbench 256"), Widget->CurrentValues[3], 150.f);
	TestEqual(TEXT("Default applies the selected column immediately"), StaticLOD->GetFloat(), Widget->CurrentValues[5 * TierCount + Widget->SelectedTier]);
	TestEqual(TEXT("Default applies selected probes"), ProbeBudget->GetInt(), FMath::RoundToInt(Widget->CurrentValues[Widget->SelectedTier]));
	TestTrue(TEXT("Default leaves active quality unchanged"), BeforeOpen == Scalability::GetQualityLevels());

	FDevKitQualityPreset& DraftPreset = Widget->Presets.AddDefaulted_GetRef();
	DraftPreset.Name = TEXT("AutomationDraftOnly");
	DraftPreset.Values = Widget->CurrentValues;
	DraftPreset.Values[5 * TierCount + 2] = 1.75f;
	DraftPreset.Values[2] = 222.f;
	// New presets include native VSM budgets, while CSM remains inactive.
	DraftPreset.Values[1 * TierCount + 2] = 6144.f;
	DraftPreset.Values[2 * TierCount + 2] = -2.25f;
	DraftPreset.Values[3 * TierCount + 2] = 1.75f;
	DraftPreset.Values[19 * TierCount + 2] = 12.f;
	Widget->LoadPreset(DraftPreset.Name);
	Widget->SelectTier(2);
	TestEqual(TEXT("Selecting a preset edits the draft"), Widget->CurrentValues[5 * TierCount + 2], 1.75f);
	TestEqual(TEXT("Selecting a tier previews the selected preset immediately"), StaticLOD->GetFloat(), 1.75f);
	TestTrue(TEXT("Selected tier is active"), BeforeOpen == Scalability::GetQualityLevels());
	Widget->ApplySelectedTier();
	TestEqual(TEXT("Apply uses the edited matrix even at the existing tier"), StaticLOD->GetFloat(), 1.75f);
	TestEqual(TEXT("Apply uses the custom probe budget"), ProbeBudget->GetInt(), 222);
	for (int32 Row = 3; Row <= 3; ++Row)
	{
		float NativeValue = FCString::Atof(*IConsoleManager::Get().FindConsoleVariable(QualityCVars[Row])->GetDefaultValue());
		GConfig->GetFloat(TEXT("ShadowQuality@2"), QualityCVars[Row], NativeValue, GScalabilityIni);
		TestEqual(TEXT("Retired shadow preset slot never overrides the native budget"),
			IConsoleManager::Get().FindConsoleVariable(QualityCVars[Row])->GetFloat(), NativeValue);
	}
	TestEqual(TEXT("VSM page capacity is applied"), IConsoleManager::Get().FindConsoleVariable(QualityCVars[1])->GetInt(), 6144);
	TestEqual(TEXT("VSM camera resolution bias is applied"), IConsoleManager::Get().FindConsoleVariable(QualityCVars[2])->GetFloat(), -2.25f);
	TestEqual(TEXT("Lumen probe density is applied"), IConsoleManager::Get().FindConsoleVariable(QualityCVars[19])->GetInt(), 12);
	TestEqual(TEXT("Apply reads back every row"), Widget->LastAppliedValues.Num(), QualityParameterCount);
	TestEqual(TEXT("Apply readback uses the actual CVar value"), Widget->LastAppliedValues[5], StaticLOD->GetFloat());
	int32 ConfigShadowMethod = INDEX_NONE;
	if (GConfig->GetInt(TEXT("ShadowQuality@2"), TEXT("r.Shadow.Virtual.Enable"), ConfigShadowMethod, GScalabilityIni))
		TestEqual(TEXT("Apply does not force VSM over the published shadow method"),
			IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Enable"))->GetInt(), ConfigShadowMethod);

	// Reproduce the project-ini priority bug without clearing the SetBy flags.
	StaticLOD->ClearFlags(ECVF_SetByMask);
	StaticLOD->Set(3.25f, ECVF_SetBySystemSettingsIni);
	TArray<TSharedRef<SSpinBox<float>>> Controls;
	TFunction<void(TSharedRef<SWidget>)> Collect;
	Collect = [&Controls, &Collect](TSharedRef<SWidget> Node)
	{
		if (Node->GetType() == FName(TEXT("SSpinBox<float>"))) Controls.Add(StaticCastSharedRef<SSpinBox<float>>(Node));
		FChildren* Children = Node->GetChildren();
		for (int32 Index = 0; Children && Index < Children->Num(); ++Index) Collect(Children->GetChildAt(Index));
	};
	Collect(Widget->MakeParameterRow(5));
	if (!TestEqual(TEXT("Each matrix row exposes four actual controls"), Controls.Num(), TierCount)) return false;
	Controls[2]->SetValue(1.5f, false);
	TestEqual(TEXT("Actual spinbox callback previews over project ini priority"), StaticLOD->GetFloat(), 1.5f);
	Controls[1]->SetValue(1.8f, false);
	TestEqual(TEXT("Non-selected column edits its draft, not the active preview"), StaticLOD->GetFloat(), 1.5f);
	TestEqual(TEXT("Non-selected draft is retained"), Widget->CurrentValues[5 * TierCount + 1], 1.8f);
	// A higher-priority console author override must remain visible in readback.
	StaticLOD->Set(3.25f, ECVF_SetByConsole);
	Widget->CurrentValues[5 * TierCount + 2] = 1.25f;
	Widget->ApplySelectedTier();
	TestEqual(TEXT("Apply respects a higher-priority override"), Widget->LastAppliedValues[5], 3.25f);
	TestTrue(TEXT("Apply reports unapplied draft rows"), Widget->LastAction.Contains(TEXT("1 项未按草稿生效")));
	StaticLOD->ClearFlags(ECVF_SetByMask);
	StaticLOD->Set(3.f, ECVF_SetByCommandline);
	Widget->ApplySelectedTier();
	TestEqual(TEXT("Explicit command-line override is protected even though Code priority is higher"), StaticLOD->GetFloat(), 3.f);
	TestEqual(TEXT("Command-line priority is not replaced"), StaticLOD->GetFlags() & ECVF_SetByMask, ECVF_SetByCommandline);
	TestTrue(TEXT("Same-tier restore fixture still uses the opening quality levels"), BeforeOpen == Scalability::GetQualityLevels());
	Widget->RestoreProjectDefault();
	TestEqual(TEXT("Restore restores a same-tier custom static LOD value"), StaticLOD->GetFloat(), 2.25f);
	TestEqual(TEXT("Restore restores a same-tier custom probe value"), ProbeBudget->GetInt(), 577);
	for (const FDevKitQualityCVarSnapshot& Snapshot : Widget->OpeningCVars)
	{
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Snapshot.Name);
		TestEqual(FString::Printf(TEXT("Restore returns actual %s"), *Snapshot.Name), Variable->GetString(), Snapshot.Value);
		TestEqual(FString::Printf(TEXT("Restore returns SetBy priority of %s"), *Snapshot.Name),
			Variable->GetFlags() & ECVF_SetByMask, Snapshot.Flags & ECVF_SetByMask);
	}
	TestTrue(TEXT("Restore preserves draft edits rather than resetting the table"), FMath::IsNearlyEqual(Widget->CurrentValues[5 * TierCount + 2], 1.25f));

	StaticLOD->Set(2.5f, ECVF_SetBySystemSettingsIni);
	const EConsoleVariableFlags HigherPriorityFlags = StaticLOD->GetFlags();
	const TSharedRef<SDevKitGraphicsQualityWorkbenchWidget> SecondWidget = SNew(SDevKitGraphicsQualityWorkbenchWidget);
	StaticLOD->Set(3.5f, ECVF_SetByConsole);
	SecondWidget->RestoreProjectDefault();
	TestEqual(TEXT("Second restore returns the captured higher-priority custom value"), StaticLOD->GetFloat(), 2.5f);
	TestEqual(TEXT("Second restore returns the captured priority"), StaticLOD->GetFlags() & ECVF_SetByMask, HigherPriorityFlags & ECVF_SetByMask);
	Widget->SelectTier(0);
	Widget->ApplySelectedTier();
	for (const auto& GroupAndSwitch : {
		TPair<const TCHAR*, const TCHAR*>(TEXT("GlobalIlluminationQuality@0"), TEXT("r.Lumen.DiffuseIndirect.Allow")),
		TPair<const TCHAR*, const TCHAR*>(TEXT("ReflectionQuality@0"), TEXT("r.Lumen.Reflections.Allow")),
		TPair<const TCHAR*, const TCHAR*>(TEXT("ShadowQuality@0"), TEXT("r.Shadow.Virtual.Enable"))})
	{
		int32 Expected = INDEX_NONE;
		TestTrue(TEXT("Published Low tier defines the rendering switch"),
			GConfig->GetInt(GroupAndSwitch.Key, GroupAndSwitch.Value, Expected, GScalabilityIni));
		TestEqual(FString::Printf(TEXT("Low tier preserves authored switch %s rather than forcing Lumen / VSM"), GroupAndSwitch.Value),
			IConsoleManager::Get().FindConsoleVariable(GroupAndSwitch.Value)->GetInt(), Expected);
	}
	Widget->RestoreProjectDefault();
	const Scalability::FQualityLevels BeforeInvalidRange = Scalability::GetQualityLevels();
	Widget->CurrentValues[13 * TierCount] = 16.f;
	Widget->CurrentValues[14 * TierCount] = 6.f;
	Widget->SelectTier(0);
	Widget->ApplySelectedTier();
	TestTrue(TEXT("Invalid clipmap range does not apply any quality"), BeforeInvalidRange == Scalability::GetQualityLevels());
	TestTrue(TEXT("Invalid range is explained"), Widget->LastAction.Contains(TEXT("未应用")));
	IConsoleVariable* ClipmapFirst = IConsoleManager::Get().FindConsoleVariable(QualityCVars[13]);
	IConsoleVariable* ClipmapLast = IConsoleManager::Get().FindConsoleVariable(QualityCVars[14]);
	Widget->EditParameter(14, 0, 18.f);
	TestEqual(TEXT("Repairing the last endpoint applies the previously invalid first draft"), ClipmapFirst->GetInt(), 16);
	TestEqual(TEXT("Repairing the last endpoint applies the new last draft"), ClipmapLast->GetInt(), 18);
	Widget->EditParameter(14, 0, 6.f);
	TestEqual(TEXT("An invalid live edit preserves the first endpoint"), ClipmapFirst->GetInt(), 16);
	TestEqual(TEXT("An invalid live edit preserves the last endpoint"), ClipmapLast->GetInt(), 18);
	Widget->EditParameter(13, 0, 4.f);
	TestEqual(TEXT("Repairing the first endpoint applies its new draft"), ClipmapFirst->GetInt(), 4);
	TestEqual(TEXT("Repairing the first endpoint applies the previously invalid last draft"), ClipmapLast->GetInt(), 6);
	Widget->CurrentValues[13 * TierCount] = 16.f;
	Widget->CurrentValues[14 * TierCount] = 18.f;
	Widget->ApplySelectedTier();
	TestEqual(TEXT("Full Apply expands the last endpoint before advancing the first"), ClipmapFirst->GetInt(), 16);
	TestEqual(TEXT("Full Apply retains the expanded last endpoint"), ClipmapLast->GetInt(), 18);
	Widget->EditParameter(13, 0, 4.f);
	Widget->EditParameter(14, 0, 6.f);
	ClipmapLast->Set(6, ECVF_SetByConsole);
	Widget->CurrentValues[13 * TierCount] = 16.f;
	Widget->CurrentValues[14 * TierCount] = 18.f;
	TestFalse(TEXT("Live preview rejects a first level beyond the console-locked last level"), Widget->PreviewParameter(13));
	Widget->ApplySelectedTier();
	TestEqual(TEXT("Full Apply does not create an invalid live range when last is locked"), ClipmapFirst->GetInt(), 4);
	TestEqual(TEXT("Full Apply respects the console-locked last endpoint"), ClipmapLast->GetInt(), 6);
	TestEqual(TEXT("Full Apply readback reports rejected first endpoint"), Widget->LastAppliedValues[13], 4.f);
	TestEqual(TEXT("Full Apply readback reports locked last endpoint"), Widget->LastAppliedValues[14], 6.f);
	return true;
}

#endif
