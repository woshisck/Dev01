#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Character/EnemyCharacterBase.h"
#include "Component/EnemyHealthDisplayComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#if WITH_EDITORONLY_DATA
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#endif
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraParameterStore.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"

#if WITH_EDITORONLY_DATA
namespace
{
	bool FindEnemyHealthRapidParameter(
		const UNiagaraScript* Script,
		const FString& NameContains,
		const FNiagaraVariableWithOffset*& OutParameter)
	{
		if (!Script)
		{
			return false;
		}

		for (const FNiagaraVariableWithOffset& Parameter : Script->RapidIterationParameters.ReadParameterVariables())
		{
			if (Parameter.GetName().ToString().Contains(NameContains))
			{
				OutParameter = &Parameter;
				return true;
			}
		}

		return false;
	}
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayDefaultConfigTest,
	"DevKit.Enemy.HealthDisplay.DefaultConfigMatchesNiagaraAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayDefaultConfigTest::RunTest(const FString& Parameters)
{
	const UEnemyHealthDisplayComponent* ComponentDefaults = GetDefault<UEnemyHealthDisplayComponent>();
	TestNotNull(TEXT("Enemy health display component defaults exist"), ComponentDefaults);
	if (!ComponentDefaults)
	{
		return false;
	}

	TestEqual(TEXT("Health percent Niagara parameter matches existing material"), ComponentDefaults->HealthPercentParameterName, FName(TEXT("new")));
	TestEqual(TEXT("Old health percent Niagara user parameter matches existing Niagara system"), ComponentDefaults->OldHealthPercentParameterName, FName(TEXT("temp")));
	TestEqual(TEXT("Legacy old health percent Niagara parameter is still written for compatibility"), ComponentDefaults->LegacyOldHealthPercentParameterName, FName(TEXT("old")));
	TestEqual(TEXT("Damage value Niagara parameter matches existing damage value system"), ComponentDefaults->DamageValueParameterName, FName(TEXT("num")));
	TestEqual(TEXT("Damage marker bool Niagara parameter preserves existing name"), ComponentDefaults->DamageMarkerBoolParameterName, FName(TEXT("\u5E03\u5C14")));
	TestEqual(TEXT("Damage value color Niagara parameter is explicit"), ComponentDefaults->DamageValueColorParameterName, FName(TEXT("color")));
	TestEqual(TEXT("Armor tint Niagara parameter is explicit"), ComponentDefaults->ArmorParameterName, FName(TEXT("armor")));
	TestEqual(TEXT("Health damage values are red by default"), ComponentDefaults->HealthDamageValueColor, FLinearColor(1.0f, 0.05f, 0.02f, 1.0f));
	TestEqual(TEXT("Armor damage values are white by default"), ComponentDefaults->ArmorDamageValueColor, FLinearColor::White);
	TestEqual(TEXT("Direct material health percent parameter is explicit"),
		ComponentDefaults->MaterialHealthPercentParameterName,
		FName(TEXT("HealthPercent")));
	TestEqual(TEXT("Direct material old health percent parameter is explicit"),
		ComponentDefaults->MaterialOldHealthPercentParameterName,
		FName(TEXT("OldHealthPercent")));
	TestEqual(TEXT("Direct material uses C++ scalar parameters by default"),
		ComponentDefaults->MaterialUseDynamicParametersParameterName,
		FName(TEXT("UseDynamicParameters")));
	TestTrue(TEXT("Direct material prefers Niagara Dynamic Parameters by default"),
		ComponentDefaults->bUseNiagaraDynamicMaterialParameters);
	TestFalse(TEXT("Verbose health display debug logging is disabled by default"),
		ComponentDefaults->bLogHealthDisplayDebug);
	TestFalse(TEXT("Duplicate health bar diagnostics are disabled by default"),
		ComponentDefaults->bLogDuplicateHealthBarDiagnostics);
	TestFalse(TEXT("Niagara runtime diagnostics are disabled by default"),
		ComponentDefaults->bLogNiagaraRuntimeDiagnostics);
	TestTrue(TEXT("Health bars are hidden until combat starts by default"),
		ComponentDefaults->bHideHealthBarOnBeginPlay);
	TestTrue(TEXT("Existing blueprint overrides still hide health bars until damage by default"),
		ComponentDefaults->bOnlyShowHealthBarAfterDamage);
	TestTrue(TEXT("Armor HP drives the visible bar percent while armor exists"),
		ComponentDefaults->bDisplayArmorAsHealthPercent);
	TestEqual(TEXT("Direct material armor parameter is explicit"),
		ComponentDefaults->MaterialArmorParameterName,
		FName(TEXT("Armor")));
	TestEqual(TEXT("Health bar stays visible for the default combat readability window"), ComponentDefaults->HideDelay, 10.0f);
	TestTrue(TEXT("Enemy health display hides inherited character widget components by default"),
		ComponentDefaults->bHideLegacyCharacterWidgetComponent);
	TestTrue(TEXT("Armor tint is enabled by default"), ComponentDefaults->bUseArmorTint);
	TestTrue(TEXT("Direct material parameter updates are enabled by default"),
		ComponentDefaults->bUpdateMaterialParameters);
	TestTrue(TEXT("Missing blueprint health bars are created from the canonical Niagara system by default"),
		ComponentDefaults->bCreateMissingHealthBarComponent);
	TestEqual(TEXT("Created health bar uses the existing blueprint variable name"),
		ComponentDefaults->HealthBarComponentName,
		FName(TEXT("NiagaraSystem_HealthBar")));
	TestEqual(TEXT("Health bar Niagara default uses a soft path"),
		ComponentDefaults->DefaultHealthBarSystem.ToSoftObjectPath().ToString(),
		FString(TEXT("/Game/UI/Health_NiagaraUI/NS_EnemyHealth.NS_EnemyHealth")));
	TestEqual(TEXT("Damage value Niagara default uses a soft path"),
		ComponentDefaults->DefaultDamageValueSystem.ToSoftObjectPath().ToString(),
		FString(TEXT("/Game/UI/Health_NiagaraUI/NS_EnemyDamageValue.NS_EnemyDamageValue")));
	TestEqual(TEXT("Direct health bar material default uses a soft path"),
		ComponentDefaults->DefaultHealthBarMaterial.ToSoftObjectPath().ToString(),
		FString(TEXT("/Game/UI/Health_NiagaraUI/M_NiagaraUI_Health_Direct.M_NiagaraUI_Health_Direct")));

	return true;
}

#if WITH_EDITORONLY_DATA
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayMaterialGraphTest,
	"DevKit.Enemy.HealthDisplay.DirectMaterialKeepsDynamicNewOldInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayMaterialGraphTest::RunTest(const FString& Parameters)
{
	const UMaterial* Material = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/UI/Health_NiagaraUI/M_NiagaraUI_Health_Direct.M_NiagaraUI_Health_Direct"));
	TestNotNull(TEXT("Direct health bar material asset exists"), Material);
	if (!Material || !Material->GetEditorOnlyData())
	{
		return false;
	}

	bool bFoundExpectedDynamicParameter = false;
	bool bFoundUseDynamicParameters = false;
	for (const TObjectPtr<UMaterialExpression>& Expression : Material->GetEditorOnlyData()->ExpressionCollection.Expressions)
	{
		const UMaterialExpressionDynamicParameter* DynamicParameter =
			Cast<UMaterialExpressionDynamicParameter>(Expression.Get());
		if (DynamicParameter && DynamicParameter->ParamNames.Num() >= 3)
		{
			bFoundExpectedDynamicParameter =
				DynamicParameter->ParamNames[0] == TEXT("new")
				&& DynamicParameter->ParamNames[1] == TEXT("old")
				&& DynamicParameter->ParamNames[2] == TEXT("armor");
		}

		const UMaterialExpressionScalarParameter* ScalarParameter =
			Cast<UMaterialExpressionScalarParameter>(Expression.Get());
		if (ScalarParameter && ScalarParameter->ParameterName == TEXT("UseDynamicParameters"))
		{
			bFoundUseDynamicParameters = FMath::IsNearlyEqual(ScalarParameter->DefaultValue, 1.0f);
		}
	}

	TestTrue(TEXT("Direct material keeps Dynamic Parameter channels new/old/armor"),
		bFoundExpectedDynamicParameter);
	TestTrue(TEXT("Direct material reads Niagara Dynamic Parameters by default"),
		bFoundUseDynamicParameters);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayNiagaraAssetLayoutTest,
	"DevKit.Enemy.HealthDisplay.NiagaraAssetUsesSingleCenteredSprite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayNiagaraAssetLayoutTest::RunTest(const FString& Parameters)
{
	const UNiagaraSystem* System = LoadObject<UNiagaraSystem>(
		nullptr,
		TEXT("/Game/UI/Health_NiagaraUI/NS_EnemyHealth.NS_EnemyHealth"));
	TestNotNull(TEXT("Enemy health Niagara system exists"), System);
	if (!System)
	{
		return false;
	}

	const TArray<FNiagaraEmitterHandle>& EmitterHandles = System->GetEmitterHandles();
	TestEqual(TEXT("Enemy health Niagara uses one emitter"), EmitterHandles.Num(), 1);
	if (EmitterHandles.Num() <= 0)
	{
		return false;
	}

	const FVersionedNiagaraEmitterData* EmitterData = EmitterHandles[0].GetEmitterData();
	TestNotNull(TEXT("Enemy health emitter data exists"), EmitterData);
	if (!EmitterData)
	{
		return false;
	}

	const FNiagaraVariableWithOffset* SpawnCountParameter = nullptr;
	TestTrue(TEXT("Enemy health Niagara has a spawn count rapid parameter"),
		FindEnemyHealthRapidParameter(
			EmitterData->EmitterUpdateScriptProps.Script,
			TEXT("SpawnBurst_Instantaneous.Spawn Count"),
			SpawnCountParameter));
	if (SpawnCountParameter)
	{
		const FNiagaraParameterStore& Store =
			EmitterData->EmitterUpdateScriptProps.Script->RapidIterationParameters;
		if (SpawnCountParameter->GetType() == FNiagaraTypeDefinition::GetIntDef())
		{
			TestEqual(TEXT("Enemy health Niagara spawns one sprite"),
				Store.GetParameterValue<int32>(*SpawnCountParameter),
				1);
		}
		else if (SpawnCountParameter->GetType() == FNiagaraTypeDefinition::GetFloatDef())
		{
			TestEqual(TEXT("Enemy health Niagara spawns one sprite"),
				Store.GetParameterValue<float>(*SpawnCountParameter),
				1.0f);
		}
		else
		{
			AddError(FString::Printf(
				TEXT("Spawn count rapid parameter has unsupported type %s"),
				*SpawnCountParameter->GetType().GetName()));
		}
	}

	const FNiagaraVariableWithOffset* PositionOffsetParameter = nullptr;
	TestTrue(TEXT("Enemy health Niagara has a position offset rapid parameter"),
		FindEnemyHealthRapidParameter(
			EmitterData->SpawnScriptProps.Script,
			TEXT("InitializeParticle.Position Offset"),
			PositionOffsetParameter));
	if (PositionOffsetParameter)
	{
		const FNiagaraParameterStore& Store =
			EmitterData->SpawnScriptProps.Script->RapidIterationParameters;
		if (PositionOffsetParameter->GetType() == FNiagaraTypeDefinition::GetVec3Def()
			|| PositionOffsetParameter->GetType() == FNiagaraTypeDefinition::GetPositionDef())
		{
			const FVector3f PositionOffset =
				Store.GetParameterValue<FVector3f>(*PositionOffsetParameter);
			TestEqual(TEXT("Enemy health Niagara keeps internal X offset centered"),
				PositionOffset.X,
				0.0f);
			TestEqual(TEXT("Enemy health Niagara keeps internal Y offset centered"),
				PositionOffset.Y,
				0.0f);
			TestEqual(TEXT("Enemy health Niagara keeps internal Z offset centered"),
				PositionOffset.Z,
				0.0f);
		}
		else
		{
			AddError(FString::Printf(
				TEXT("Position offset rapid parameter has unsupported type %s"),
				*PositionOffsetParameter->GetType().GetName()));
		}
	}

	return true;
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayMathTest,
	"DevKit.Enemy.HealthDisplay.MathClampsHealthAndArmorState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayMathTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Health percent uses health divided by max health"),
		UEnemyHealthDisplayComponent::CalculateHealthPercent(25.0f, 100.0f),
		0.25f);
	TestEqual(TEXT("Health percent clamps above one"),
		UEnemyHealthDisplayComponent::CalculateHealthPercent(125.0f, 100.0f),
		1.0f);
	TestEqual(TEXT("Health percent clamps below zero"),
		UEnemyHealthDisplayComponent::CalculateHealthPercent(-10.0f, 100.0f),
		0.0f);
	TestEqual(TEXT("Health percent handles invalid max health"),
		UEnemyHealthDisplayComponent::CalculateHealthPercent(25.0f, 0.0f),
		0.0f);

	TestTrue(TEXT("Positive armor enables armor tint when feature is enabled"),
		UEnemyHealthDisplayComponent::ShouldUseArmorTint(1.0f, true));
	TestFalse(TEXT("Zero armor disables armor tint"),
		UEnemyHealthDisplayComponent::ShouldUseArmorTint(0.0f, true));
	TestFalse(TEXT("Disabled armor tint ignores armor value"),
		UEnemyHealthDisplayComponent::ShouldUseArmorTint(50.0f, false));

	TestEqual(TEXT("Display percent uses armor percent while armor exists"),
		UEnemyHealthDisplayComponent::CalculateDisplayHealthPercent(0.9f, 25.0f, 100.0f, true),
		0.25f);
	TestEqual(TEXT("Display percent falls back to health percent without armor"),
		UEnemyHealthDisplayComponent::CalculateDisplayHealthPercent(0.9f, 0.0f, 100.0f, true),
		0.9f);
	TestEqual(TEXT("Display percent falls back to health percent without max armor"),
		UEnemyHealthDisplayComponent::CalculateDisplayHealthPercent(0.9f, 25.0f, 0.0f, true),
		0.9f);
	TestEqual(TEXT("Display percent can ignore armor when disabled"),
		UEnemyHealthDisplayComponent::CalculateDisplayHealthPercent(0.9f, 25.0f, 100.0f, false),
		0.9f);

	TestTrue(TEXT("Health bar component finder accepts exact blueprint variable name"),
		UEnemyHealthDisplayComponent::IsLikelyHealthBarComponentName(
			TEXT("NiagaraSystem_HealthBar"),
			FName(TEXT("NiagaraSystem_HealthBar"))));
	TestTrue(TEXT("Health bar component finder accepts generated blueprint component names"),
		UEnemyHealthDisplayComponent::IsLikelyHealthBarComponentName(
			TEXT("NiagaraSystem_HealthBar_GEN_VARIABLE"),
			FName(TEXT("NiagaraSystem_HealthBar"))));
	TestTrue(TEXT("Health bar system finder accepts enemy health Niagara systems"),
		UEnemyHealthDisplayComponent::IsLikelyHealthBarSystemName(TEXT("NS_EnemyHealth")));
	TestFalse(TEXT("Health bar system finder rejects unrelated slash effects"),
		UEnemyHealthDisplayComponent::IsLikelyHealthBarSystemName(TEXT("NS_SlashTrail_Burn")));
	TestFalse(TEXT("Health bar component finder rejects unrelated splash components"),
		UEnemyHealthDisplayComponent::IsLikelyHealthBarComponentName(
			TEXT("Splash"),
			FName(TEXT("NiagaraSystem_HealthBar"))));
	TestFalse(TEXT("Health bar system finder rejects empty systems"),
		UEnemyHealthDisplayComponent::IsLikelyHealthBarSystemName(TEXT("")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayRejectsOnlySplashCandidateTest,
	"DevKit.Enemy.HealthDisplay.DoesNotBindUnrelatedOnlyNiagaraCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayRejectsOnlySplashCandidateTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AActor* Owner = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Owner actor spawned"), Owner);
	if (!Owner)
	{
		return false;
	}

	UNiagaraComponent* SplashComponent = NewObject<UNiagaraComponent>(Owner, TEXT("Splash"));
	TestNotNull(TEXT("Unrelated splash Niagara component created"), SplashComponent);
	if (!SplashComponent)
	{
		Owner->Destroy();
		return false;
	}

	Owner->AddInstanceComponent(SplashComponent);
	SplashComponent->RegisterComponent();

	UEnemyHealthDisplayComponent* DisplayComponent =
		NewObject<UEnemyHealthDisplayComponent>(Owner, TEXT("EnemyHealthDisplay"));
	TestNotNull(TEXT("Health display component created"), DisplayComponent);
	if (!DisplayComponent)
	{
		Owner->Destroy();
		return false;
	}

	Owner->AddInstanceComponent(DisplayComponent);
	DisplayComponent->bCreateMissingHealthBarComponent = false;
	DisplayComponent->HealthBarSystem = nullptr;
	DisplayComponent->RegisterComponent();

	DisplayComponent->SetHealthBarVisible(true);

	TestNull(TEXT("Unrelated only Niagara candidate is not cached as health bar"),
		DisplayComponent->HealthBarComponent.Get());

	Owner->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayCreatesMissingHealthBarTest,
	"DevKit.Enemy.HealthDisplay.CreatesMissingHealthBarInsteadOfUsingSplash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayCreatesMissingHealthBarTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AActor* Owner = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Owner actor spawned"), Owner);
	if (!Owner)
	{
		return false;
	}

	UNiagaraComponent* SplashComponent = NewObject<UNiagaraComponent>(Owner, TEXT("Splash"));
	TestNotNull(TEXT("Unrelated splash Niagara component created"), SplashComponent);
	if (!SplashComponent)
	{
		Owner->Destroy();
		return false;
	}

	Owner->AddInstanceComponent(SplashComponent);
	SplashComponent->RegisterComponent();

	UEnemyHealthDisplayComponent* DisplayComponent =
		NewObject<UEnemyHealthDisplayComponent>(Owner, TEXT("EnemyHealthDisplay"));
	TestNotNull(TEXT("Health display component created"), DisplayComponent);
	if (!DisplayComponent)
	{
		Owner->Destroy();
		return false;
	}

	Owner->AddInstanceComponent(DisplayComponent);
	DisplayComponent->RegisterComponent();

	DisplayComponent->SetHealthBarVisible(true);

	UNiagaraComponent* CreatedHealthBar = DisplayComponent->HealthBarComponent.Get();
	TestNotNull(TEXT("Missing health bar component is created"), CreatedHealthBar);
	if (CreatedHealthBar)
	{
		TestNotEqual(TEXT("Created health bar is not the unrelated splash Niagara"),
			CreatedHealthBar,
			SplashComponent);
		TestEqual(TEXT("Created health bar keeps the expected component name"),
			CreatedHealthBar->GetFName(),
			FName(TEXT("NiagaraSystem_HealthBar")));
		TestNotNull(TEXT("Created health bar has the canonical Niagara system"),
			CreatedHealthBar->GetAsset());
		if (CreatedHealthBar->GetAsset())
		{
			TestEqual(TEXT("Created health bar uses NS_EnemyHealth"),
				CreatedHealthBar->GetAsset()->GetPathName(),
				FString(TEXT("/Game/UI/Health_NiagaraUI/NS_EnemyHealth.NS_EnemyHealth")));
		}
		TestFalse(TEXT("Inactive splash slot is hidden when the health bar is shown"),
			SplashComponent->IsVisible());
		TestTrue(TEXT("Inactive splash slot is hidden in game when the health bar is shown"),
			SplashComponent->bHiddenInGame);
	}

	Owner->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayUsesAttachedSpawnedHealthBarTest,
	"DevKit.Enemy.HealthDisplay.UsesAttachedSpawnedHealthBarBeforeCreatingOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayUsesAttachedSpawnedHealthBarTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AActor* Owner = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Owner actor spawned"), Owner);
	if (!Owner)
	{
		return false;
	}

	USceneComponent* RootComponent = NewObject<USceneComponent>(Owner, TEXT("Root"));
	TestNotNull(TEXT("Root component created"), RootComponent);
	if (!RootComponent)
	{
		Owner->Destroy();
		return false;
	}

	Owner->SetRootComponent(RootComponent);
	Owner->AddInstanceComponent(RootComponent);
	RootComponent->RegisterComponent();

	const UEnemyHealthDisplayComponent* ComponentDefaults = GetDefault<UEnemyHealthDisplayComponent>();
	UNiagaraComponent* SpawnedHealthBar = NewObject<UNiagaraComponent>(World, TEXT("SpawnedHealthBar"));
	TestNotNull(TEXT("Blueprint-spawned attached health bar created"), SpawnedHealthBar);
	if (!SpawnedHealthBar)
	{
		Owner->Destroy();
		return false;
	}

	SpawnedHealthBar->SetAsset(ComponentDefaults->HealthBarSystem);
	SpawnedHealthBar->SetupAttachment(RootComponent);
	SpawnedHealthBar->RegisterComponentWithWorld(World);

	UEnemyHealthDisplayComponent* DisplayComponent =
		NewObject<UEnemyHealthDisplayComponent>(Owner, TEXT("EnemyHealthDisplay"));
	TestNotNull(TEXT("Health display component created"), DisplayComponent);
	if (!DisplayComponent)
	{
		SpawnedHealthBar->DestroyComponent();
		Owner->Destroy();
		return false;
	}

	Owner->AddInstanceComponent(DisplayComponent);
	DisplayComponent->RegisterComponent();

	DisplayComponent->SetHealthBarVisible(true);

	TestEqual(TEXT("Attached spawned health bar is reused instead of creating a duplicate"),
		DisplayComponent->HealthBarComponent.Get(),
		SpawnedHealthBar);

	TArray<UNiagaraComponent*> OwnedNiagaraComponents;
	Owner->GetComponents<UNiagaraComponent>(OwnedNiagaraComponents);
	TestEqual(TEXT("No extra owned Niagara component is created"),
		OwnedNiagaraComponents.Num(),
		0);

	SpawnedHealthBar->DestroyComponent();
	Owner->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayStaysVisibleUntilOwnerDestroyAfterDeathTest,
	"DevKit.Enemy.HealthDisplay.StaysVisibleUntilOwnerDestroyAfterDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayStaysVisibleUntilOwnerDestroyAfterDeathTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AEnemyCharacterBase* Enemy = World->SpawnActor<AEnemyCharacterBase>();
	TestNotNull(TEXT("Enemy spawned"), Enemy);
	if (!Enemy)
	{
		return false;
	}

	UEnemyHealthDisplayComponent* DisplayComponent = Enemy->FindComponentByClass<UEnemyHealthDisplayComponent>();
	TestNotNull(TEXT("Enemy health display component exists"), DisplayComponent);
	if (!DisplayComponent)
	{
		Enemy->Destroy();
		return false;
	}

	UNiagaraComponent* HealthBar = NewObject<UNiagaraComponent>(Enemy, TEXT("NiagaraSystem_HealthBar"));
	TestNotNull(TEXT("Health bar component created"), HealthBar);
	if (!HealthBar)
	{
		Enemy->Destroy();
		return false;
	}

	Enemy->AddInstanceComponent(HealthBar);
	HealthBar->RegisterComponent();
	DisplayComponent->HealthBarComponent = HealthBar;
	DisplayComponent->SetHealthBarVisible(true);
	TestTrue(TEXT("Health bar starts visible"), HealthBar->IsVisible());

	UFunction* DeathStartedFunction = DisplayComponent->FindFunction(TEXT("HandleOwnerDeathStarted"));
	TestNotNull(TEXT("Health display exposes owner death handler"), DeathStartedFunction);
	if (!DeathStartedFunction)
	{
		Enemy->Destroy();
		return false;
	}

	struct FDeathStartedParams
	{
		AYogCharacterBase* Character;
	};
	FDeathStartedParams DeathParams{ Enemy };
	DisplayComponent->ProcessEvent(DeathStartedFunction, &DeathParams);

	TestTrue(TEXT("Health bar remains visible when owner death starts"), HealthBar->IsVisible());

	UFunction* HealthUpdateFunction = DisplayComponent->FindFunction(TEXT("HandleCharacterHealthUpdate"));
	TestNotNull(TEXT("Health display still has health update handler"), HealthUpdateFunction);
	if (HealthUpdateFunction)
	{
		struct FHealthUpdateParams
		{
			float HealthPercent;
			float DamageTaken;
		};
		FHealthUpdateParams HealthParams{ 0.0f, 100.0f };
		DisplayComponent->ProcessEvent(HealthUpdateFunction, &HealthParams);
	}

	TestTrue(TEXT("Death health update keeps the health bar visible until the owner is destroyed"), HealthBar->IsVisible());

	Enemy->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayAttachedToEnemyTest,
	"DevKit.Enemy.HealthDisplay.EnemyBaseCreatesDisplayComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayAttachedToEnemyTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AEnemyCharacterBase* Enemy = World->SpawnActor<AEnemyCharacterBase>();
	TestNotNull(TEXT("Enemy spawned"), Enemy);
	if (!Enemy)
	{
		return false;
	}

	UEnemyHealthDisplayComponent* DisplayComponent = Enemy->FindComponentByClass<UEnemyHealthDisplayComponent>();
	TestNotNull(TEXT("Enemy base creates enemy health display component"), DisplayComponent);

	Enemy->Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyHealthDisplayHidesLegacyWidgetComponentTest,
	"DevKit.Enemy.HealthDisplay.HidesLegacyCharacterWidgetComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthDisplayHidesLegacyWidgetComponentTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AEnemyCharacterBase* Enemy = World->SpawnActor<AEnemyCharacterBase>();
	TestNotNull(TEXT("Enemy spawned"), Enemy);
	if (!Enemy)
	{
		return false;
	}

	UEnemyHealthDisplayComponent* DisplayComponent = Enemy->FindComponentByClass<UEnemyHealthDisplayComponent>();
	UWidgetComponent* LegacyWidgetComponent = Enemy->GetWidgetcomponent();
	TestNotNull(TEXT("Enemy health display component exists"), DisplayComponent);
	TestNotNull(TEXT("Enemy inherited widget component exists"), LegacyWidgetComponent);
	if (!DisplayComponent || !LegacyWidgetComponent)
	{
		Enemy->Destroy();
		return false;
	}

	LegacyWidgetComponent->SetHiddenInGame(false);
	LegacyWidgetComponent->SetVisibility(true);
	TestTrue(TEXT("Legacy widget component starts visible"), LegacyWidgetComponent->IsVisible());

	DisplayComponent->SetHealthBarVisible(true);

	TestFalse(TEXT("Enemy health display hides inherited legacy widget component"), LegacyWidgetComponent->IsVisible());
	TestTrue(TEXT("Enemy health display marks inherited legacy widget hidden in game"), LegacyWidgetComponent->bHiddenInGame);

	Enemy->Destroy();
	return true;
}

#endif
