#include "UI/EnemyHealthNiagaraFixCommandlet.h"

#include "FileHelpers.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraParameterStore.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "UObject/Package.h"

namespace
{
	constexpr const TCHAR* EnemyHealthSystemObjectPath =
		TEXT("/Game/UI/Health_NiagaraUI/NS_EnemyHealth.NS_EnemyHealth");

	bool SetSpawnCountToOne(UNiagaraScript* Script, const FString& ScriptLabel)
	{
		if (!Script)
		{
			return false;
		}

		bool bChanged = false;
		FNiagaraParameterStore& Parameters = Script->RapidIterationParameters;
		for (const FNiagaraVariableWithOffset& Parameter : Parameters.ReadParameterVariables())
		{
			const FString ParameterName = Parameter.GetName().ToString();
			if (!ParameterName.Contains(TEXT("SpawnBurst_Instantaneous.Spawn Count")))
			{
				continue;
			}

			const FNiagaraVariable Variable(Parameter.GetType(), Parameter.GetName());
			if (Parameter.GetType() == FNiagaraTypeDefinition::GetFloatDef())
			{
				const float NewValue = 1.0f;
				bChanged |= Parameters.SetParameterValue(NewValue, Variable);
				UE_LOG(LogTemp, Display,
					TEXT("[EnemyHealthNiagaraFix] %s %s -> %.1f"),
					*ScriptLabel,
					*ParameterName,
					NewValue);
			}
			else if (Parameter.GetType() == FNiagaraTypeDefinition::GetIntDef())
			{
				const int32 NewValue = 1;
				bChanged |= Parameters.SetParameterValue(NewValue, Variable);
				UE_LOG(LogTemp, Display,
					TEXT("[EnemyHealthNiagaraFix] %s %s -> %d"),
					*ScriptLabel,
					*ParameterName,
					NewValue);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[EnemyHealthNiagaraFix] %s %s has unsupported type %s"),
					*ScriptLabel,
					*ParameterName,
					*Parameter.GetType().GetName());
			}
		}

		if (bChanged)
		{
			Script->Modify();
			Script->MarkPackageDirty();
		}
		return bChanged;
	}

	bool SetPositionOffsetToZero(UNiagaraScript* Script, const FString& ScriptLabel)
	{
		if (!Script)
		{
			return false;
		}

		bool bChanged = false;
		FNiagaraParameterStore& Parameters = Script->RapidIterationParameters;
		for (const FNiagaraVariableWithOffset& Parameter : Parameters.ReadParameterVariables())
		{
			const FString ParameterName = Parameter.GetName().ToString();
			if (!ParameterName.Contains(TEXT("InitializeParticle.Position Offset")))
			{
				continue;
			}

			const FNiagaraVariable Variable(Parameter.GetType(), Parameter.GetName());
			if (Parameter.GetType() == FNiagaraTypeDefinition::GetVec3Def()
				|| Parameter.GetType() == FNiagaraTypeDefinition::GetPositionDef())
			{
				const FVector3f NewValue(0.0f, 0.0f, 0.0f);
				bChanged |= Parameters.SetParameterValue(NewValue, Variable);
				UE_LOG(LogTemp, Display,
					TEXT("[EnemyHealthNiagaraFix] %s %s -> %s"),
					*ScriptLabel,
					*ParameterName,
					*NewValue.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[EnemyHealthNiagaraFix] %s %s has unsupported type %s"),
					*ScriptLabel,
					*ParameterName,
					*Parameter.GetType().GetName());
			}
		}

		if (bChanged)
		{
			Script->Modify();
			Script->MarkPackageDirty();
		}
		return bChanged;
	}

	bool FixEmitterSpawnCount(FNiagaraEmitterHandle& EmitterHandle)
	{
		FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			return false;
		}

		const FString EmitterLabel = EmitterHandle.GetName().ToString();
		bool bChanged = false;
		bChanged |= SetSpawnCountToOne(EmitterData->EmitterSpawnScriptProps.Script, EmitterLabel + TEXT(".EmitterSpawn"));
		bChanged |= SetSpawnCountToOne(EmitterData->EmitterUpdateScriptProps.Script, EmitterLabel + TEXT(".EmitterUpdate"));
		bChanged |= SetSpawnCountToOne(EmitterData->SpawnScriptProps.Script, EmitterLabel + TEXT(".ParticleSpawn"));
		bChanged |= SetSpawnCountToOne(EmitterData->UpdateScriptProps.Script, EmitterLabel + TEXT(".ParticleUpdate"));
		bChanged |= SetPositionOffsetToZero(EmitterData->SpawnScriptProps.Script, EmitterLabel + TEXT(".ParticleSpawn"));

		if (bChanged)
		{
			EmitterHandle.GetInstance().Emitter->Modify();
		}
		return bChanged;
	}
}

UEnemyHealthNiagaraFixCommandlet::UEnemyHealthNiagaraFixCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UEnemyHealthNiagaraFixCommandlet::Main(const FString& Params)
{
	UNiagaraSystem* System = Cast<UNiagaraSystem>(
		StaticLoadObject(UNiagaraSystem::StaticClass(), nullptr, EnemyHealthSystemObjectPath));
	if (!System)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemyHealthNiagaraFix] Failed to load %s"),
			EnemyHealthSystemObjectPath);
		return 1;
	}

	System->Modify();
	bool bChanged = false;
	for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
	{
		bChanged |= FixEmitterSpawnCount(EmitterHandle);
	}

	if (!bChanged)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[EnemyHealthNiagaraFix] No Niagara health bar parameters were changed in %s"),
			EnemyHealthSystemObjectPath);
		return 2;
	}

	System->PostEditChange();
	System->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(System->GetPackage());
	UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);

	UE_LOG(LogTemp, Display,
		TEXT("[EnemyHealthNiagaraFix] Fixed and saved %s"),
		EnemyHealthSystemObjectPath);
	return 0;
}
