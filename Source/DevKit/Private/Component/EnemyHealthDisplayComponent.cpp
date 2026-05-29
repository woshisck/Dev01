#include "Component/EnemyHealthDisplayComponent.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraParameterStore.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyHealthDisplay, Log, All);

namespace
{
	constexpr TCHAR DefaultDamageValueSystemPath[] =
		TEXT("/Game/UI/Health_NiagaraUI/NS_EnemyDamageValue.NS_EnemyDamageValue");

	void CollectAttachedNiagaraComponents(USceneComponent* Parent, TArray<UNiagaraComponent*>& OutComponents)
	{
		if (!Parent)
		{
			return;
		}

		for (USceneComponent* Child : Parent->GetAttachChildren())
		{
			if (UNiagaraComponent* NiagaraChild = Cast<UNiagaraComponent>(Child))
			{
				OutComponents.AddUnique(NiagaraChild);
			}

			CollectAttachedNiagaraComponents(Child, OutComponents);
		}
	}

	bool IsHealthBarDiagnosticCandidate(const UNiagaraComponent* NiagaraComponent, FName DesiredName)
	{
		if (!NiagaraComponent)
		{
			return false;
		}

		const FString ComponentName = NiagaraComponent->GetName();
		const FString SystemName = GetNameSafe(NiagaraComponent->GetAsset());
		if ((!DesiredName.IsNone() && NiagaraComponent->GetFName() == DesiredName)
			|| UEnemyHealthDisplayComponent::IsLikelyHealthBarComponentName(ComponentName, DesiredName)
			|| UEnemyHealthDisplayComponent::IsLikelyHealthBarSystemName(SystemName))
		{
			return true;
		}

		const UMaterialInterface* Material = NiagaraComponent->GetMaterial(0);
		const FString MaterialName = GetNameSafe(Material);
		return MaterialName.Contains(TEXT("Health"), ESearchCase::IgnoreCase)
			|| MaterialName.Contains(TEXT("NiagaraUI"), ESearchCase::IgnoreCase);
	}

	FString NiagaraExecutionStateToString(ENiagaraExecutionState State)
	{
		if (const UEnum* ExecutionStateEnum = StaticEnum<ENiagaraExecutionState>())
		{
			return ExecutionStateEnum->GetNameStringByValue(static_cast<int64>(State));
		}

		return FString::FromInt(static_cast<int32>(State));
	}

	bool ShouldLogNiagaraParameter(const FString& ParameterName)
	{
		return ParameterName.Contains(TEXT("SpawnBurst_Instantaneous.Spawn Count"), ESearchCase::IgnoreCase)
			|| ParameterName.Contains(TEXT("DynamicMaterial"), ESearchCase::IgnoreCase)
			|| ParameterName.Contains(TEXT("Sprite Size"), ESearchCase::IgnoreCase)
			|| ParameterName.Contains(TEXT("Position Offset"), ESearchCase::IgnoreCase)
			|| ParameterName.Contains(TEXT("User.new"), ESearchCase::IgnoreCase)
			|| ParameterName.Contains(TEXT("User.old"), ESearchCase::IgnoreCase)
			|| ParameterName.Contains(TEXT("User.temp"), ESearchCase::IgnoreCase)
			|| ParameterName.Contains(TEXT("User.armor"), ESearchCase::IgnoreCase);
	}

	FString DescribeNiagaraParameterValue(
		const FNiagaraParameterStore& Parameters,
		const FNiagaraVariableWithOffset& Parameter)
	{
		const uint8* Data = Parameters.GetParameterData(Parameter.Offset);
		if (!Data)
		{
			return TEXT("<null>");
		}

		const FNiagaraTypeDefinition& Type = Parameter.GetType();
		if (Type == FNiagaraTypeDefinition::GetFloatDef())
		{
			return FString::Printf(TEXT("%.6f"), *reinterpret_cast<const float*>(Data));
		}

		if (Type == FNiagaraTypeDefinition::GetIntDef())
		{
			return FString::Printf(TEXT("%d"), *reinterpret_cast<const int32*>(Data));
		}

		if (Type == FNiagaraTypeDefinition::GetBoolDef())
		{
			const FNiagaraBool& Value = *reinterpret_cast<const FNiagaraBool*>(Data);
			return FString::Printf(TEXT("%d raw=%d"), Value.GetValue() ? 1 : 0, Value.GetRawValue());
		}

		if (Type == FNiagaraTypeDefinition::GetVec2Def())
		{
			const FVector2f& Value = *reinterpret_cast<const FVector2f*>(Data);
			return FString::Printf(TEXT("(X=%.6f,Y=%.6f)"), Value.X, Value.Y);
		}

		if (Type == FNiagaraTypeDefinition::GetVec3Def()
			|| Type == FNiagaraTypeDefinition::GetPositionDef())
		{
			const FVector3f& Value = *reinterpret_cast<const FVector3f*>(Data);
			return FString::Printf(TEXT("(X=%.6f,Y=%.6f,Z=%.6f)"), Value.X, Value.Y, Value.Z);
		}

		if (Type == FNiagaraTypeDefinition::GetVec4Def())
		{
			const FVector4f& Value = *reinterpret_cast<const FVector4f*>(Data);
			return FString::Printf(
				TEXT("(X=%.6f,Y=%.6f,Z=%.6f,W=%.6f)"),
				Value.X,
				Value.Y,
				Value.Z,
				Value.W);
		}

		if (Type == FNiagaraTypeDefinition::GetColorDef())
		{
			const FLinearColor& Value = *reinterpret_cast<const FLinearColor*>(Data);
			return FString::Printf(
				TEXT("(R=%.6f,G=%.6f,B=%.6f,A=%.6f)"),
				Value.R,
				Value.G,
				Value.B,
				Value.A);
		}

		return FString::Printf(
			TEXT("<%s bytes=%d>"),
			*Type.GetName(),
			Parameter.GetSizeInBytes());
	}

	void LogNiagaraParameterStoreValues(
		const TCHAR* OwnerName,
		const TCHAR* StoreLabel,
		const FNiagaraParameterStore& Parameters)
	{
		int32 LoggedParameters = 0;
		for (const FNiagaraVariableWithOffset& Parameter : Parameters.ReadParameterVariables())
		{
			const FString ParameterName = Parameter.GetName().ToString();
			if (!ShouldLogNiagaraParameter(ParameterName))
			{
				continue;
			}

			++LoggedParameters;
			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][NiagaraRuntime][Param] Owner=%s Store=%s Param=%s Type=%s Value=%s"),
				OwnerName,
				StoreLabel,
				*ParameterName,
				*Parameter.GetType().GetName(),
				*DescribeNiagaraParameterValue(Parameters, Parameter));
		}

		if (LoggedParameters == 0)
		{
			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][NiagaraRuntime][Param] Owner=%s Store=%s Params=%d InterestingParams=0"),
				OwnerName,
				StoreLabel,
				Parameters.Num());
		}
	}

	void LogNiagaraParameterStore(
		const TCHAR* OwnerName,
		const TCHAR* ScriptLabel,
		const UNiagaraScript* Script)
	{
		if (!Script)
		{
			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][NiagaraRuntime][Script] Owner=%s Script=%s NiagaraScript=None"),
				OwnerName,
				ScriptLabel);
			return;
		}

		LogNiagaraParameterStoreValues(OwnerName, ScriptLabel, Script->RapidIterationParameters);
	}
}

UEnemyHealthDisplayComponent::UEnemyHealthDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultHealthBarMaterial(
		TEXT("/Game/UI/Health_NiagaraUI/M_NiagaraUI_Health_Direct.M_NiagaraUI_Health_Direct"));
	if (DefaultHealthBarMaterial.Succeeded())
	{
		HealthBarMaterial = DefaultHealthBarMaterial.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultHealthBarSystem(
		TEXT("/Game/UI/Health_NiagaraUI/NS_EnemyHealth.NS_EnemyHealth"));
	if (DefaultHealthBarSystem.Succeeded())
	{
		HealthBarSystem = DefaultHealthBarSystem.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultDamageValueSystem(
		DefaultDamageValueSystemPath);
	if (DefaultDamageValueSystem.Succeeded())
	{
		DamageValueSystem = DefaultDamageValueSystem.Object;
	}
}

void UEnemyHealthDisplayComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AYogCharacterBase* OwnerCharacter = Cast<AYogCharacterBase>(GetOwner()))
	{
		OwnerCharacter->OnCharacterHealthUpdate.AddUniqueDynamic(
			this,
			&UEnemyHealthDisplayComponent::HandleCharacterHealthUpdate);
		OwnerCharacter->OnCharacterDamageValue.AddUniqueDynamic(
			this,
			&UEnemyHealthDisplayComponent::HandleCharacterDamageValue);
		OwnerCharacter->OnCharacterDeathStarted.AddUniqueDynamic(
			this,
			&UEnemyHealthDisplayComponent::HandleOwnerDeathStarted);
		CachedAbilitySystemComponent = OwnerCharacter->GetASC();
		bOwnerDeathStarted = OwnerCharacter->bIsDead;
	}

	bInitialHealthDisplayRefreshPending = true;
	BindAttributeDelegates();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UEnemyHealthDisplayComponent::InitializeHealthDisplayAfterBeginPlay));
	}
	else
	{
		InitializeHealthDisplayAfterBeginPlay();
	}
}

void UEnemyHealthDisplayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideHealthBarTimerHandle);
	}

	if (AYogCharacterBase* OwnerCharacter = Cast<AYogCharacterBase>(GetOwner()))
	{
		OwnerCharacter->OnCharacterHealthUpdate.RemoveDynamic(
			this,
			&UEnemyHealthDisplayComponent::HandleCharacterHealthUpdate);
		OwnerCharacter->OnCharacterDamageValue.RemoveDynamic(
			this,
			&UEnemyHealthDisplayComponent::HandleCharacterDamageValue);
		OwnerCharacter->OnCharacterDeathStarted.RemoveDynamic(
			this,
			&UEnemyHealthDisplayComponent::HandleOwnerDeathStarted);
	}

	UnbindAttributeDelegates();

	Super::EndPlay(EndPlayReason);
}

void UEnemyHealthDisplayComponent::RefreshDisplayFromAttributes()
{
	if (bOwnerDeathStarted)
	{
		UpdateHealthBar(0.0f, bHasLastHealthPercent ? LastHealthPercent : 0.0f);
		LastHealthPercent = 0.0f;
		bHasLastHealthPercent = true;
		UpdateArmorParameter();
		ShowHealthBarUntilOwnerDestroyed();
		return;
	}

	const float CurrentDisplayHealthPercent = GetCurrentDisplayHealthPercent(GetCurrentHealthPercent());
	LogHealthDisplayState(
		TEXT("RefreshDisplayFromAttributes"),
		GetCurrentHealthPercent(),
		CurrentDisplayHealthPercent,
		CurrentDisplayHealthPercent,
		0.0f);
	UpdateHealthBar(CurrentDisplayHealthPercent, CurrentDisplayHealthPercent);
	LastHealthPercent = CurrentDisplayHealthPercent;
	bHasLastHealthPercent = true;
	UpdateArmorParameter();
}

void UEnemyHealthDisplayComponent::SetHealthBarVisible(bool bVisible)
{
	if (bVisible && bOwnerDeathStarted)
	{
		return;
	}

	if (bVisible)
	{
		HideLegacyCharacterWidgetComponent();
	}

	if (UNiagaraComponent* HealthBar = ResolveHealthBarComponent())
	{
		if (bVisible)
		{
			HideInactiveSplashNiagaraComponents(HealthBar);
		}

		HealthBar->SetVisibility(bVisible);
		HealthBar->SetHiddenInGame(!bVisible);
		if (bVisible && !HealthBar->IsActive())
		{
			HealthBar->Activate(true);
		}
	}
}

float UEnemyHealthDisplayComponent::CalculateHealthPercent(float Health, float MaxHealth)
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
}

bool UEnemyHealthDisplayComponent::ShouldUseArmorTint(float ArmorHP, bool bEnableArmorTint)
{
	return bEnableArmorTint && ArmorHP > 0.0f;
}

float UEnemyHealthDisplayComponent::CalculateDisplayHealthPercent(
	float HealthPercent,
	float ArmorHP,
	float MaxArmorHP,
	bool bShouldDisplayArmorAsHealthPercent)
{
	const float ClampedHealthPercent = FMath::Clamp(HealthPercent, 0.0f, 1.0f);
	if (bShouldDisplayArmorAsHealthPercent && ArmorHP > 0.0f && MaxArmorHP > 0.0f)
	{
		return CalculateHealthPercent(ArmorHP, MaxArmorHP);
	}

	return ClampedHealthPercent;
}

bool UEnemyHealthDisplayComponent::IsLikelyHealthBarComponentName(const FString& ComponentName, FName DesiredName)
{
	const FString DesiredString = DesiredName.ToString();
	if (!DesiredString.IsEmpty() && ComponentName.Contains(DesiredString, ESearchCase::IgnoreCase))
	{
		return true;
	}

	return ComponentName.Contains(TEXT("HealthBar"), ESearchCase::IgnoreCase)
		|| ComponentName.Contains(TEXT("Health_Bar"), ESearchCase::IgnoreCase)
		|| ComponentName.Contains(TEXT("EnemyHealth"), ESearchCase::IgnoreCase)
		|| ComponentName.Contains(TEXT("Enemy_Health"), ESearchCase::IgnoreCase);
}

bool UEnemyHealthDisplayComponent::IsLikelyHealthBarSystemName(const FString& SystemName)
{
	return SystemName.Contains(TEXT("Health"), ESearchCase::IgnoreCase)
		&& !SystemName.Contains(TEXT("Damage"), ESearchCase::IgnoreCase)
		&& !SystemName.Contains(TEXT("Slash"), ESearchCase::IgnoreCase)
		&& !SystemName.Contains(TEXT("Trail"), ESearchCase::IgnoreCase);
}

void UEnemyHealthDisplayComponent::HandleCharacterHealthUpdate(float HealthPercent, float DamageTaken)
{
	if (!bEnableHealthDisplay)
	{
		return;
	}

	const float ClampedHealthPercent = GetCurrentDisplayHealthPercent(HealthPercent);
	const float OldHealthPercent = bHasLastHealthPercent
		? LastHealthPercent
		: ClampedHealthPercent;

	const bool bDeadFromHealthUpdate = bOwnerDeathStarted || (HealthPercent <= 0.0f && GetCurrentMaxHealth() > 0.0f);
	if (bDeadFromHealthUpdate)
	{
		bOwnerDeathStarted = true;
		LogHealthDisplayState(
			TEXT("CharacterHealthUpdateDead"),
			HealthPercent,
			0.0f,
			OldHealthPercent,
			DamageTaken);
		UpdateHealthBar(0.0f, OldHealthPercent);
		LastHealthPercent = 0.0f;
		bHasLastHealthPercent = true;
		UpdateArmorParameter();
		ShowHealthBarUntilOwnerDestroyed();
		return;
	}

	ShowHealthBarTemporarily();
	LogHealthDisplayState(
		TEXT("CharacterHealthUpdate"),
		HealthPercent,
		ClampedHealthPercent,
		OldHealthPercent,
		DamageTaken);
	UpdateHealthBar(ClampedHealthPercent, OldHealthPercent);
	LastHealthPercent = ClampedHealthPercent;
	bHasLastHealthPercent = true;
	UpdateArmorParameter();
}

void UEnemyHealthDisplayComponent::HandleCharacterDamageValue(float DamageAmount, EYogDamageValueType DamageValueType)
{
	if (!bEnableHealthDisplay)
	{
		return;
	}

	SpawnDamageValue(DamageAmount, DamageValueType);
}

void UEnemyHealthDisplayComponent::HandleOwnerDeathStarted(AYogCharacterBase* Character)
{
	if (Character && Character != GetOwner())
	{
		return;
	}

	const float OldHealthPercent = bHasLastHealthPercent ? LastHealthPercent : GetCurrentDisplayHealthPercent(0.0f);
	bOwnerDeathStarted = true;
	LogHealthDisplayState(TEXT("DeathStarted"), 0.0f, 0.0f, OldHealthPercent, 0.0f);
	UpdateHealthBar(0.0f, OldHealthPercent);
	LastHealthPercent = 0.0f;
	bHasLastHealthPercent = true;
	UpdateArmorParameter();
	ShowHealthBarUntilOwnerDestroyed();
}

void UEnemyHealthDisplayComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!bEnableHealthDisplay || FMath::IsNearlyEqual(Data.NewValue, Data.OldValue))
	{
		return;
	}

	const float MaxHealth = GetCurrentMaxHealth();
	const float NewHealthPercent = CalculateHealthPercent(Data.NewValue, MaxHealth);
	const float OldHealthPercent = CalculateHealthPercent(Data.OldValue, MaxHealth);
	const float NewDisplayHealthPercent = CalculateDisplayHealthPercent(
		NewHealthPercent,
		GetCurrentArmorHP(),
		GetCurrentMaxArmorHP(),
		bDisplayArmorAsHealthPercent);
	const float OldDisplayHealthPercent = bHasLastHealthPercent
		? LastHealthPercent
		: CalculateDisplayHealthPercent(
			OldHealthPercent,
			GetCurrentArmorHP(),
			GetCurrentMaxArmorHP(),
			bDisplayArmorAsHealthPercent);

	const bool bDeadFromAttribute = bOwnerDeathStarted || (Data.NewValue <= 0.0f && MaxHealth > 0.0f);
	if (bDeadFromAttribute)
	{
		bOwnerDeathStarted = true;
		LogHealthDisplayState(
			TEXT("ASCHealthChangedDead"),
			NewHealthPercent,
			0.0f,
			OldDisplayHealthPercent,
			FMath::Max(0.0f, Data.OldValue - Data.NewValue));
		UpdateHealthBar(0.0f, OldDisplayHealthPercent);
		LastHealthPercent = 0.0f;
		bHasLastHealthPercent = true;
		UpdateArmorParameter();
		ShowHealthBarUntilOwnerDestroyed();
		return;
	}

	ShowHealthBarTemporarily();
	LogHealthDisplayState(
		TEXT("ASCHealthChanged"),
		NewHealthPercent,
		NewDisplayHealthPercent,
		OldDisplayHealthPercent,
		FMath::Max(0.0f, Data.OldValue - Data.NewValue));
	UpdateHealthBar(NewDisplayHealthPercent, OldDisplayHealthPercent);
	LastHealthPercent = NewDisplayHealthPercent;
	bHasLastHealthPercent = true;
	UpdateArmorParameter();
}

void UEnemyHealthDisplayComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	if (FMath::IsNearlyEqual(Data.NewValue, Data.OldValue))
	{
		return;
	}

	RefreshDisplayFromAttributes();
}

void UEnemyHealthDisplayComponent::HandleArmorChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue == Data.OldValue)
	{
		return;
	}

	if (bEnableHealthDisplay && bDisplayArmorAsHealthPercent)
	{
		const float CurrentDisplayHealthPercent = GetCurrentDisplayHealthPercent(GetCurrentHealthPercent());
		const float OldHealthPercent = bHasLastHealthPercent
			? LastHealthPercent
			: CalculateDisplayHealthPercent(
				GetCurrentHealthPercent(),
				Data.OldValue,
				GetCurrentMaxArmorHP(),
				bDisplayArmorAsHealthPercent);

		if (bOwnerDeathStarted)
		{
			UpdateHealthBar(0.0f, OldHealthPercent);
			LastHealthPercent = 0.0f;
			bHasLastHealthPercent = true;
			UpdateArmorParameter();
			ShowHealthBarUntilOwnerDestroyed();
			return;
		}

		ShowHealthBarTemporarily();
		LogHealthDisplayState(
			TEXT("ASCArmorChanged"),
			GetCurrentHealthPercent(),
			CurrentDisplayHealthPercent,
			OldHealthPercent,
			FMath::Max(0.0f, Data.OldValue - Data.NewValue));
		UpdateHealthBar(CurrentDisplayHealthPercent, OldHealthPercent);
		LastHealthPercent = CurrentDisplayHealthPercent;
		bHasLastHealthPercent = true;
	}

	UpdateArmorParameter();
}

void UEnemyHealthDisplayComponent::BindAttributeDelegates()
{
	UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute()))
	{
		HealthChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetHealthAttribute()).AddUObject(
				this,
				&UEnemyHealthDisplayComponent::HandleHealthChanged);
	}

	if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute()))
	{
		MaxHealthChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetMaxHealthAttribute()).AddUObject(
				this,
				&UEnemyHealthDisplayComponent::HandleMaxHealthChanged);
	}

	if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetArmorHPAttribute()))
	{
		ArmorChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetArmorHPAttribute()).AddUObject(
				this,
				&UEnemyHealthDisplayComponent::HandleArmorChanged);
	}
}

void UEnemyHealthDisplayComponent::UnbindAttributeDelegates()
{
	UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (ASC && HealthChangedDelegateHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetHealthAttribute()).Remove(HealthChangedDelegateHandle);
	}

	if (ASC && MaxHealthChangedDelegateHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedDelegateHandle);
	}

	if (ASC && ArmorChangedDelegateHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UBaseAttributeSet::GetArmorHPAttribute()).Remove(ArmorChangedDelegateHandle);
	}

	HealthChangedDelegateHandle.Reset();
	MaxHealthChangedDelegateHandle.Reset();
	ArmorChangedDelegateHandle.Reset();
}

void UEnemyHealthDisplayComponent::UpdateHealthBar(float HealthPercent, float OldHealthPercent)
{
	const float ClampedHealthPercent = FMath::Clamp(HealthPercent, 0.0f, 1.0f);
	const float ClampedOldHealthPercent = FMath::Clamp(OldHealthPercent, 0.0f, 1.0f);
	if (UNiagaraComponent* HealthBar = ResolveHealthBarComponent())
	{
		if (!HealthPercentParameterName.IsNone())
		{
			SetNiagaraFloatParameter(*HealthBar, HealthPercentParameterName, ClampedHealthPercent);
		}

		if (!OldHealthPercentParameterName.IsNone())
		{
			SetNiagaraFloatParameter(*HealthBar, OldHealthPercentParameterName, ClampedOldHealthPercent);
		}

		if (!LegacyOldHealthPercentParameterName.IsNone()
			&& LegacyOldHealthPercentParameterName != OldHealthPercentParameterName)
		{
			SetNiagaraFloatParameter(*HealthBar, LegacyOldHealthPercentParameterName, ClampedOldHealthPercent);
		}

		if (bLogNiagaraRuntimeDiagnostics)
		{
			const bool bHealthMovedDown = ClampedHealthPercent < ClampedOldHealthPercent - KINDA_SMALL_NUMBER;
			const bool bShouldLogStartup = !bLoggedNiagaraRuntimeStartupDiagnostics && ClampedHealthPercent >= 0.999f;
			const bool bShouldLogDamage = !bLoggedNiagaraRuntimeDamageDiagnostics
				&& (bHealthMovedDown || ClampedHealthPercent < 0.999f);
			if (bShouldLogStartup || bShouldLogDamage)
			{
				bLoggedNiagaraRuntimeStartupDiagnostics |= bShouldLogStartup;
				bLoggedNiagaraRuntimeDamageDiagnostics |= bShouldLogDamage;
				LogNiagaraRuntimeDiagnostics(
					*HealthBar,
					bShouldLogDamage ? TEXT("UpdateHealthBarDamage") : TEXT("UpdateHealthBarStartup"),
					ClampedHealthPercent,
					ClampedOldHealthPercent);
			}
		}
	}

	LogHealthDisplayState(
		TEXT("UpdateHealthBar"),
		GetCurrentHealthPercent(),
		ClampedHealthPercent,
		ClampedOldHealthPercent,
		0.0f);

	if (bUpdateMaterialParameters)
	{
		if (UMaterialInstanceDynamic* MID = ResolveHealthBarMaterialInstance())
		{
			if (!MaterialUseDynamicParametersParameterName.IsNone())
			{
				MID->SetScalarParameterValue(
					MaterialUseDynamicParametersParameterName,
					bUseNiagaraDynamicMaterialParameters ? 1.0f : 0.0f);
			}

			if (!MaterialHealthPercentParameterName.IsNone())
			{
				MID->SetScalarParameterValue(MaterialHealthPercentParameterName, ClampedHealthPercent);
			}

			if (!MaterialOldHealthPercentParameterName.IsNone())
			{
				MID->SetScalarParameterValue(MaterialOldHealthPercentParameterName, ClampedOldHealthPercent);
			}
		}
	}
}

void UEnemyHealthDisplayComponent::UpdateArmorParameter()
{
	if (ArmorParameterName.IsNone())
	{
		return;
	}

	const float ArmorValue = ShouldUseArmorTint(GetCurrentArmorHP(), bUseArmorTint) ? 1.0f : 0.0f;
	if (UNiagaraComponent* HealthBar = ResolveHealthBarComponent())
	{
		SetNiagaraFloatParameter(*HealthBar, ArmorParameterName, ArmorValue);
	}

	if (bUpdateMaterialParameters && !MaterialArmorParameterName.IsNone())
	{
		if (UMaterialInstanceDynamic* MID = ResolveHealthBarMaterialInstance())
		{
			MID->SetScalarParameterValue(MaterialArmorParameterName, ArmorValue);
		}
	}

	if (bLogHealthDisplayDebug)
	{
		UE_LOG(LogEnemyHealthDisplay, Verbose,
			TEXT("[EnemyHealthDisplay][Armor] Owner=%s ArmorValue=%.3f RawArmor=%.3f MaxArmor=%.3f UseArmorTint=%d NiagaraParam=%s MaterialParam=%s"),
			*GetNameSafe(GetOwner()),
			ArmorValue,
			GetCurrentArmorHP(),
			GetCurrentMaxArmorHP(),
			bUseArmorTint ? 1 : 0,
			*ArmorParameterName.ToString(),
			*MaterialArmorParameterName.ToString());
	}
}

void UEnemyHealthDisplayComponent::SpawnDamageValue(float DamageAmount, EYogDamageValueType DamageValueType)
{
	UNiagaraSystem* ResolvedDamageValueSystem = ResolveDamageValueSystem();
	if (!ResolvedDamageValueSystem || DamageAmount < MinDamageValueToDisplay)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	UNiagaraComponent* DamageComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		ResolvedDamageValueSystem,
		Owner->GetActorLocation() + DamageValueLocationOffset,
		DamageValueRotation,
		DamageValueScale,
		true,
		false,
		ENCPoolMethod::None,
		true);

	if (!DamageComponent)
	{
		return;
	}

	if (!DamageValueParameterName.IsNone())
	{
		DamageComponent->SetVariableFloat(DamageValueParameterName, DamageAmount);
	}

	if (!DamageMarkerBoolParameterName.IsNone())
	{
		DamageComponent->SetVariableBool(DamageMarkerBoolParameterName, true);
	}

	const bool bArmorDamageValue = DamageValueType == EYogDamageValueType::Armor;
	if (!ArmorParameterName.IsNone())
	{
		SetNiagaraFloatParameter(*DamageComponent, ArmorParameterName, bArmorDamageValue ? 1.0f : 0.0f);
	}

	if (!DamageValueColorParameterName.IsNone())
	{
		SetNiagaraColorParameter(
			*DamageComponent,
			DamageValueColorParameterName,
			bArmorDamageValue ? ArmorDamageValueColor : HealthDamageValueColor);
	}

	DamageComponent->Activate(true);
}

void UEnemyHealthDisplayComponent::SetNiagaraFloatParameter(
	UNiagaraComponent& NiagaraComponent,
	FName ParameterName,
	float Value)
{
	if (ParameterName.IsNone())
	{
		return;
	}

	NiagaraComponent.SetVariableFloat(ParameterName, Value);

	const FString ParameterString = ParameterName.ToString();
	if (!ParameterString.StartsWith(TEXT("User.")))
	{
		NiagaraComponent.SetVariableFloat(
			FName(*FString::Printf(TEXT("User.%s"), *ParameterString)),
			Value);
	}
}

void UEnemyHealthDisplayComponent::LogHealthDisplayState(
	const TCHAR* Source,
	float InputHealthPercent,
	float OutputHealthPercent,
	float OldHealthPercent,
	float DamageTaken) const
{
	if (!bLogHealthDisplayDebug)
	{
		return;
	}

	const UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	float RawHealth = -1.0f;
	float RawMaxHealth = -1.0f;
	float RawArmor = -1.0f;
	float RawMaxArmor = -1.0f;
	if (ASC)
	{
		if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute()))
		{
			RawHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		}

		if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute()))
		{
			RawMaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		}

		if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetArmorHPAttribute()))
		{
			RawArmor = ASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute());
		}

		if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxArmorHPAttribute()))
		{
			RawMaxArmor = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxArmorHPAttribute());
		}
	}

	const UNiagaraComponent* HealthBar = HealthBarComponent.Get();
	UE_LOG(LogEnemyHealthDisplay, Verbose,
		TEXT("[EnemyHealthDisplay][%s] Owner=%s ASC=%s Health=%.2f MaxHealth=%.2f AttrPct=%.3f InputPct=%.3f WriteNew=%.3f WriteOld=%.3f Last=%.3f Damage=%.2f Armor=%.2f MaxArmor=%.2f DisplayArmorAsHealth=%d HealthBar=%s HealthBarVisible=%d Material=%s MID=%s UseDynamic=%d UpdateMID=%d NiagaraParams(new=%s oldUser=%s oldLegacy=%s armor=%s)"),
		Source,
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ASC),
		RawHealth,
		RawMaxHealth,
		CalculateHealthPercent(RawHealth, RawMaxHealth),
		InputHealthPercent,
		OutputHealthPercent,
		OldHealthPercent,
		LastHealthPercent,
		DamageTaken,
		RawArmor,
		RawMaxArmor,
		bDisplayArmorAsHealthPercent ? 1 : 0,
		*GetNameSafe(HealthBar),
		HealthBar ? (HealthBar->IsVisible() ? 1 : 0) : 0,
		*GetNameSafe(HealthBarMaterial.Get()),
		*GetNameSafe(HealthBarMaterialInstance.Get()),
		bUseNiagaraDynamicMaterialParameters ? 1 : 0,
		bUpdateMaterialParameters ? 1 : 0,
		*HealthPercentParameterName.ToString(),
		*OldHealthPercentParameterName.ToString(),
		*LegacyOldHealthPercentParameterName.ToString(),
		*ArmorParameterName.ToString());
}

void UEnemyHealthDisplayComponent::InitializeHealthDisplayAfterBeginPlay()
{
	const bool bDelayMissingHealthBarCreation = bOnlyShowHealthBarAfterDamage || bHideHealthBarOnBeginPlay;
	if (!bDelayMissingHealthBarCreation)
	{
		bInitialHealthDisplayRefreshPending = false;
	}

	if (const AYogCharacterBase* OwnerCharacter = Cast<AYogCharacterBase>(GetOwner()))
	{
		bOwnerDeathStarted = OwnerCharacter->bIsDead;
	}

	HideLegacyCharacterWidgetComponent();
	HideInactiveSplashNiagaraComponents(HealthBarComponent.Get());

	if (bAutoFindHealthBarComponent)
	{
		ResolveHealthBarComponent();
	}

	RefreshDisplayFromAttributes();
	LogHealthDisplayState(TEXT("BeginPlay"), GetCurrentHealthPercent(), LastHealthPercent, LastHealthPercent, 0.0f);

	if (bDelayMissingHealthBarCreation)
	{
		SetHealthBarVisible(false);
	}

	bInitialHealthDisplayRefreshPending = false;
}

void UEnemyHealthDisplayComponent::ShowHealthBarTemporarily()
{
	if (bOwnerDeathStarted)
	{
		return;
	}

	SetHealthBarVisible(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideHealthBarTimerHandle);
		const float VisibleDuration = GetHealthBarVisibleDuration();
		if (VisibleDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				HideHealthBarTimerHandle,
				this,
				&UEnemyHealthDisplayComponent::HideHealthBar,
				VisibleDuration,
				false);
		}
		else
		{
			HideHealthBar();
		}
	}
}

void UEnemyHealthDisplayComponent::HideLegacyCharacterWidgetComponent()
{
	if (!bHideLegacyCharacterWidgetComponent)
	{
		return;
	}

	AYogCharacterBase* OwnerCharacter = Cast<AYogCharacterBase>(GetOwner());
	UWidgetComponent* LegacyWidgetComponent = OwnerCharacter ? OwnerCharacter->GetWidgetcomponent() : nullptr;
	if (!LegacyWidgetComponent)
	{
		return;
	}

	const bool bCouldRenderLegacyWidget = LegacyWidgetComponent->IsVisible() && !LegacyWidgetComponent->bHiddenInGame;
	if (bCouldRenderLegacyWidget
		&& bLogDuplicateHealthBarDiagnostics
		&& !bLoggedLegacyWidgetDiagnostics)
	{
		bLoggedLegacyWidgetDiagnostics = true;
		UE_LOG(LogEnemyHealthDisplay, Warning,
			TEXT("[EnemyHealthDisplay][DuplicateHealthBars][LegacyWidget] Owner=%s Component=%s WidgetClass=%s Visible=%d HiddenInGame=%d DrawSize=%s WorldLocation=%s"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(LegacyWidgetComponent),
			*GetNameSafe(LegacyWidgetComponent->GetWidgetClass()),
			LegacyWidgetComponent->IsVisible() ? 1 : 0,
			LegacyWidgetComponent->bHiddenInGame ? 1 : 0,
			*LegacyWidgetComponent->GetDrawSize().ToString(),
			*LegacyWidgetComponent->GetComponentLocation().ToCompactString());
	}

	LegacyWidgetComponent->SetVisibility(false);
	LegacyWidgetComponent->SetHiddenInGame(true);
}

void UEnemyHealthDisplayComponent::HideInactiveSplashNiagaraComponents(const UNiagaraComponent* HealthBarToKeep)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	Owner->GetComponents<UNiagaraComponent>(NiagaraComponents);
	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		CollectAttachedNiagaraComponents(SceneComponent, NiagaraComponents);
	}

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (!NiagaraComponent || NiagaraComponent == HealthBarToKeep)
		{
			continue;
		}

		const FString ComponentName = NiagaraComponent->GetName();
		const bool bLooksLikeOldSplashSlot = ComponentName.Contains(TEXT("Splash"), ESearchCase::IgnoreCase);
		const bool bHasRenderableSetup = NiagaraComponent->GetAsset() || NiagaraComponent->GetMaterial(0);
		const bool bInactiveOrComplete = !NiagaraComponent->IsActive()
			|| NiagaraComponent->GetExecutionState() == ENiagaraExecutionState::Complete
			|| NiagaraComponent->GetExecutionState() == ENiagaraExecutionState::Inactive
			|| NiagaraComponent->GetExecutionState() == ENiagaraExecutionState::InactiveClear;
		if (!bLooksLikeOldSplashSlot || bHasRenderableSetup || !bInactiveOrComplete)
		{
			continue;
		}

		NiagaraComponent->DeactivateImmediate();
		NiagaraComponent->SetVisibility(false);
		NiagaraComponent->SetHiddenInGame(true);
	}
}

void UEnemyHealthDisplayComponent::LogDuplicateHealthBarDiagnostics(
	AActor& Owner,
	const TArray<UNiagaraComponent*>& NiagaraComponents,
	const UNiagaraComponent* SelectedHealthBar,
	const TCHAR* Source,
	bool bWillCreateMissingHealthBar)
{
	if (!bLogDuplicateHealthBarDiagnostics || bLoggedDuplicateHealthBarDiagnostics)
	{
		return;
	}

	TArray<UNiagaraComponent*> PotentialHealthBars;
	TArray<UNiagaraComponent*> VisibleNiagaraComponents;
	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (!NiagaraComponent)
		{
			continue;
		}

		if (IsHealthBarDiagnosticCandidate(NiagaraComponent, HealthBarComponentName))
		{
			PotentialHealthBars.AddUnique(NiagaraComponent);
		}

		if (NiagaraComponent->IsVisible()
			&& (NiagaraComponent->GetAsset() || NiagaraComponent->GetMaterial(0) || NiagaraComponent->IsActive()))
		{
			VisibleNiagaraComponents.AddUnique(NiagaraComponent);
		}
	}

	const bool bHasDuplicatePotentialHealthBars = PotentialHealthBars.Num() > 1;
	const bool bCreatingNextToVisibleNiagara = bWillCreateMissingHealthBar && VisibleNiagaraComponents.Num() > 0;
	if (!bHasDuplicatePotentialHealthBars && !bCreatingNextToVisibleNiagara)
	{
		return;
	}

	bLoggedDuplicateHealthBarDiagnostics = true;
	UE_LOG(LogEnemyHealthDisplay, Warning,
		TEXT("[EnemyHealthDisplay][DuplicateHealthBars] Owner=%s Source=%s Selected=%s SelectedSystem=%s WillCreateMissing=%d PotentialHealthBarCount=%d VisibleNiagaraCount=%d TotalNiagaraCount=%d DesiredComponentName=%s DefaultHealthBarSystem=%s"),
		*GetNameSafe(&Owner),
		Source,
		*GetNameSafe(SelectedHealthBar),
		SelectedHealthBar ? *GetNameSafe(SelectedHealthBar->GetAsset()) : TEXT("None"),
		bWillCreateMissingHealthBar ? 1 : 0,
		PotentialHealthBars.Num(),
		VisibleNiagaraComponents.Num(),
		NiagaraComponents.Num(),
		*HealthBarComponentName.ToString(),
		*GetNameSafe(HealthBarSystem.Get()));

	for (UNiagaraComponent* PotentialHealthBar : PotentialHealthBars)
	{
		const UMaterialInterface* Material = PotentialHealthBar ? PotentialHealthBar->GetMaterial(0) : nullptr;
		UE_LOG(LogEnemyHealthDisplay, Warning,
			TEXT("[EnemyHealthDisplay][DuplicateHealthBars][Potential] Owner=%s Component=%s FName=%s System=%s SystemPath=%s Material0=%s AttachParent=%s Visible=%d HiddenInGame=%d Active=%d WorldLocation=%s"),
			*GetNameSafe(&Owner),
			*GetNameSafe(PotentialHealthBar),
			PotentialHealthBar ? *PotentialHealthBar->GetFName().ToString() : TEXT("None"),
			PotentialHealthBar ? *GetNameSafe(PotentialHealthBar->GetAsset()) : TEXT("None"),
			(PotentialHealthBar && PotentialHealthBar->GetAsset()) ? *PotentialHealthBar->GetAsset()->GetPathName() : TEXT("None"),
			*GetNameSafe(Material),
			(PotentialHealthBar && PotentialHealthBar->GetAttachParent()) ? *GetNameSafe(PotentialHealthBar->GetAttachParent()) : TEXT("None"),
			(PotentialHealthBar && PotentialHealthBar->IsVisible()) ? 1 : 0,
			(PotentialHealthBar && PotentialHealthBar->bHiddenInGame) ? 1 : 0,
			(PotentialHealthBar && PotentialHealthBar->IsActive()) ? 1 : 0,
			PotentialHealthBar ? *PotentialHealthBar->GetComponentLocation().ToCompactString() : TEXT("None"));
	}

	if (bCreatingNextToVisibleNiagara)
	{
		for (UNiagaraComponent* VisibleNiagara : VisibleNiagaraComponents)
		{
			const UMaterialInterface* Material = VisibleNiagara ? VisibleNiagara->GetMaterial(0) : nullptr;
			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][DuplicateHealthBars][Visible] Owner=%s Component=%s FName=%s System=%s SystemPath=%s Material0=%s AttachParent=%s Active=%d WorldLocation=%s"),
				*GetNameSafe(&Owner),
				*GetNameSafe(VisibleNiagara),
				VisibleNiagara ? *VisibleNiagara->GetFName().ToString() : TEXT("None"),
				VisibleNiagara ? *GetNameSafe(VisibleNiagara->GetAsset()) : TEXT("None"),
				(VisibleNiagara && VisibleNiagara->GetAsset()) ? *VisibleNiagara->GetAsset()->GetPathName() : TEXT("None"),
				*GetNameSafe(Material),
				(VisibleNiagara && VisibleNiagara->GetAttachParent()) ? *GetNameSafe(VisibleNiagara->GetAttachParent()) : TEXT("None"),
				(VisibleNiagara && VisibleNiagara->IsActive()) ? 1 : 0,
				VisibleNiagara ? *VisibleNiagara->GetComponentLocation().ToCompactString() : TEXT("None"));
		}
	}
}

void UEnemyHealthDisplayComponent::LogNiagaraRuntimeDiagnostics(
	UNiagaraComponent& HealthBar,
	const TCHAR* Source,
	float NewHealthPercent,
	float OldHealthPercent)
{
	AActor* Owner = GetOwner();
	const FString OwnerName = GetNameSafe(Owner);
	UNiagaraSystem* System = HealthBar.GetAsset();
	const UMaterialInterface* ComponentMaterial = HealthBar.GetMaterial(0);

	UE_LOG(LogEnemyHealthDisplay, Warning,
		TEXT("[EnemyHealthDisplay][NiagaraRuntime] Owner=%s Source=%s Component=%s ComponentPtr=%p Asset=%s AssetPath=%s AssetPtr=%p Material0=%s Visible=%d HiddenInGame=%d Active=%d ExecState=%s New=%.6f Old=%.6f WorldLocation=%s"),
		*OwnerName,
		Source,
		*GetNameSafe(&HealthBar),
		&HealthBar,
		*GetNameSafe(System),
		System ? *System->GetPathName() : TEXT("None"),
		System,
		*GetNameSafe(ComponentMaterial),
		HealthBar.IsVisible() ? 1 : 0,
		HealthBar.bHiddenInGame ? 1 : 0,
		HealthBar.IsActive() ? 1 : 0,
		*NiagaraExecutionStateToString(HealthBar.GetExecutionState()),
		NewHealthPercent,
		OldHealthPercent,
		*HealthBar.GetComponentLocation().ToCompactString());

	LogNiagaraParameterStoreValues(
		*OwnerName,
		TEXT("ComponentOverrideParameters"),
		HealthBar.GetOverrideParameters());

	if (Owner)
	{
		TArray<UNiagaraComponent*> NiagaraComponents;
		Owner->GetComponents<UNiagaraComponent>(NiagaraComponents);
		TArray<USceneComponent*> SceneComponents;
		Owner->GetComponents<USceneComponent>(SceneComponents);
		for (USceneComponent* SceneComponent : SceneComponents)
		{
			CollectAttachedNiagaraComponents(SceneComponent, NiagaraComponents);
		}

		UE_LOG(LogEnemyHealthDisplay, Warning,
			TEXT("[EnemyHealthDisplay][NiagaraRuntime][OwnerInventory] Owner=%s NiagaraCount=%d"),
			*OwnerName,
			NiagaraComponents.Num());

		for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		{
			if (!NiagaraComponent)
			{
				continue;
			}

			const UMaterialInterface* Material = NiagaraComponent->GetMaterial(0);
			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][NiagaraRuntime][OwnerNiagara] Owner=%s Component=%s Ptr=%p FName=%s Asset=%s AssetPath=%s Material0=%s Visible=%d HiddenInGame=%d Active=%d ExecState=%s AttachParent=%s WorldLocation=%s"),
				*OwnerName,
				*GetNameSafe(NiagaraComponent),
				NiagaraComponent,
				*NiagaraComponent->GetFName().ToString(),
				*GetNameSafe(NiagaraComponent->GetAsset()),
				NiagaraComponent->GetAsset() ? *NiagaraComponent->GetAsset()->GetPathName() : TEXT("None"),
				*GetNameSafe(Material),
				NiagaraComponent->IsVisible() ? 1 : 0,
				NiagaraComponent->bHiddenInGame ? 1 : 0,
				NiagaraComponent->IsActive() ? 1 : 0,
				*NiagaraExecutionStateToString(NiagaraComponent->GetExecutionState()),
				NiagaraComponent->GetAttachParent() ? *GetNameSafe(NiagaraComponent->GetAttachParent()) : TEXT("None"),
				*NiagaraComponent->GetComponentLocation().ToCompactString());
		}

		TArray<UWidgetComponent*> WidgetComponents;
		Owner->GetComponents<UWidgetComponent>(WidgetComponents);
		for (UWidgetComponent* WidgetComponent : WidgetComponents)
		{
			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][NiagaraRuntime][OwnerWidget] Owner=%s Component=%s Ptr=%p WidgetClass=%s Visible=%d HiddenInGame=%d DrawSize=%s WorldLocation=%s"),
				*OwnerName,
				*GetNameSafe(WidgetComponent),
				WidgetComponent,
				*GetNameSafe(WidgetComponent ? WidgetComponent->GetWidgetClass() : nullptr),
				(WidgetComponent && WidgetComponent->IsVisible()) ? 1 : 0,
				(WidgetComponent && WidgetComponent->bHiddenInGame) ? 1 : 0,
				WidgetComponent ? *WidgetComponent->GetDrawSize().ToString() : TEXT("None"),
				WidgetComponent ? *WidgetComponent->GetComponentLocation().ToCompactString() : TEXT("None"));
		}
	}

	if (System)
	{
		UE_LOG(LogEnemyHealthDisplay, Warning,
			TEXT("[EnemyHealthDisplay][NiagaraRuntime][System] Owner=%s System=%s Path=%s EmitterHandles=%d"),
			*OwnerName,
			*GetNameSafe(System),
			*System->GetPathName(),
			System->GetEmitterHandles().Num());

		for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
			const FString EmitterModeName = StaticEnum<ENiagaraEmitterMode>()
				? StaticEnum<ENiagaraEmitterMode>()->GetNameStringByValue(static_cast<int64>(EmitterHandle.GetEmitterMode()))
				: FString::FromInt(static_cast<int32>(EmitterHandle.GetEmitterMode()));
			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][NiagaraRuntime][EmitterAsset] Owner=%s Handle=%s Unique=%s Enabled=%d Mode=%s Emitter=%s EmitterData=%p Renderers=%d SimTarget=%d LocalSpace=%d"),
				*OwnerName,
				*EmitterHandle.GetName().ToString(),
				*EmitterHandle.GetUniqueInstanceName(),
				EmitterHandle.GetIsEnabled() ? 1 : 0,
				*EmitterModeName,
				*GetNameSafe(EmitterHandle.GetInstance().Emitter),
				EmitterData,
				EmitterData ? EmitterData->GetRenderers().Num() : 0,
				EmitterData ? static_cast<int32>(EmitterData->SimTarget) : -1,
				EmitterData && EmitterData->bLocalSpace ? 1 : 0);

			if (!EmitterData)
			{
				continue;
			}

			int32 RendererIndex = 0;
			for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
			{
				UE_LOG(LogEnemyHealthDisplay, Warning,
					TEXT("[EnemyHealthDisplay][NiagaraRuntime][RendererAsset] Owner=%s Handle=%s Index=%d Renderer=%s Class=%s Enabled=%d"),
					*OwnerName,
					*EmitterHandle.GetName().ToString(),
					RendererIndex++,
					*GetNameSafe(Renderer),
					Renderer ? *Renderer->GetClass()->GetName() : TEXT("None"),
					(Renderer && Renderer->GetIsEnabled()) ? 1 : 0);
			}

			const FString ParticleSpawnLabel = FString::Printf(
				TEXT("%s.ParticleSpawn"),
				*EmitterHandle.GetUniqueInstanceName());
			const FString ParticleUpdateLabel = FString::Printf(
				TEXT("%s.ParticleUpdate"),
				*EmitterHandle.GetUniqueInstanceName());
			LogNiagaraParameterStore(*OwnerName, *ParticleSpawnLabel, EmitterData->SpawnScriptProps.Script);
			LogNiagaraParameterStore(*OwnerName, *ParticleUpdateLabel, EmitterData->UpdateScriptProps.Script);

#if WITH_EDITORONLY_DATA
			const FString EmitterSpawnLabel = FString::Printf(
				TEXT("%s.EmitterSpawn"),
				*EmitterHandle.GetUniqueInstanceName());
			const FString EmitterUpdateLabel = FString::Printf(
				TEXT("%s.EmitterUpdate"),
				*EmitterHandle.GetUniqueInstanceName());
			LogNiagaraParameterStore(*OwnerName, *EmitterSpawnLabel, EmitterData->EmitterSpawnScriptProps.Script);
			LogNiagaraParameterStore(*OwnerName, *EmitterUpdateLabel, EmitterData->EmitterUpdateScriptProps.Script);
#endif
		}
	}

	FNiagaraSystemInstanceControllerPtr SystemInstanceController = HealthBar.GetSystemInstanceController();
	FNiagaraSystemInstance* SystemInstance = SystemInstanceController.IsValid()
		? SystemInstanceController->GetSystemInstance_Unsafe()
		: nullptr;
	if (!SystemInstance)
	{
		UE_LOG(LogEnemyHealthDisplay, Warning,
			TEXT("[EnemyHealthDisplay][NiagaraRuntime][LiveInstance] Owner=%s SystemInstance=None"),
			*OwnerName);
	}
	else
	{
		UE_LOG(LogEnemyHealthDisplay, Warning,
			TEXT("[EnemyHealthDisplay][NiagaraRuntime][LiveInstance] Owner=%s SystemInstance=%p ActualState=%s RequestedState=%s Emitters=%d LocalBounds=%s"),
			*OwnerName,
			SystemInstance,
			*NiagaraExecutionStateToString(SystemInstance->GetActualExecutionState()),
			*NiagaraExecutionStateToString(SystemInstance->GetRequestedExecutionState()),
			SystemInstance->GetEmitters().Num(),
			*SystemInstance->GetLocalBounds().ToString());

		for (const FNiagaraEmitterInstanceRef& EmitterRef : SystemInstance->GetEmitters())
		{
			const FNiagaraEmitterInstance& EmitterInstance = EmitterRef.Get();
			const FNiagaraEmitterHandle& EmitterHandle = EmitterInstance.GetEmitterHandle();
			int32 EnabledRendererCount = 0;
			EmitterInstance.ForEachEnabledRenderer(
				[&EnabledRendererCount, &OwnerName, &EmitterHandle](const UNiagaraRendererProperties* Renderer)
				{
					UE_LOG(LogEnemyHealthDisplay, Warning,
						TEXT("[EnemyHealthDisplay][NiagaraRuntime][LiveRenderer] Owner=%s Emitter=%s Renderer=%s Class=%s"),
						*OwnerName,
						*EmitterHandle.GetName().ToString(),
						*GetNameSafe(Renderer),
						Renderer ? *Renderer->GetClass()->GetName() : TEXT("None"));
					++EnabledRendererCount;
				});

			UE_LOG(LogEnemyHealthDisplay, Warning,
				TEXT("[EnemyHealthDisplay][NiagaraRuntime][LiveEmitter] Owner=%s Emitter=%s Unique=%s Enabled=%d State=%s Particles=%d TotalSpawned=%d SimTarget=%d LocalSpace=%d EnabledRenderers=%d Bounds=%s"),
				*OwnerName,
				*EmitterHandle.GetName().ToString(),
				*EmitterHandle.GetUniqueInstanceName(),
				EmitterHandle.GetIsEnabled() ? 1 : 0,
				*NiagaraExecutionStateToString(EmitterInstance.GetExecutionState()),
				EmitterInstance.GetNumParticles(),
				EmitterInstance.GetTotalSpawnedParticles(),
				static_cast<int32>(EmitterInstance.GetSimTarget()),
				EmitterInstance.IsLocalSpace() ? 1 : 0,
				EnabledRendererCount,
				*EmitterInstance.GetBounds().ToString());
		}
	}

	if (bBreakOnNiagaraRuntimeDiagnostics)
	{
		UE_DEBUG_BREAK();
	}
}

UNiagaraComponent* UEnemyHealthDisplayComponent::CreateHealthBarComponent(AActor& Owner)
{
	if (!bCreateMissingHealthBarComponent || !HealthBarSystem)
	{
		return nullptr;
	}

	const FName CreatedComponentName = HealthBarComponentName.IsNone()
		? FName(TEXT("NiagaraSystem_HealthBar"))
		: HealthBarComponentName;
	UNiagaraComponent* CreatedComponent = NewObject<UNiagaraComponent>(&Owner, CreatedComponentName);
	if (!CreatedComponent)
	{
		return nullptr;
	}

	const bool bShouldStartVisible = !bOwnerDeathStarted
		&& !bOnlyShowHealthBarAfterDamage
		&& !bHideHealthBarOnBeginPlay;
	CreatedComponent->SetAsset(HealthBarSystem);
	CreatedComponent->SetAutoActivate(bShouldStartVisible);
	if (USceneComponent* RootComponent = Owner.GetRootComponent())
	{
		CreatedComponent->SetupAttachment(RootComponent);
	}
	CreatedComponent->SetRelativeLocation(HealthBarRelativeLocation);
	CreatedComponent->SetVisibility(bShouldStartVisible);
	CreatedComponent->SetHiddenInGame(!bShouldStartVisible);
	if (HealthBarMaterial)
	{
		CreatedComponent->SetMaterial(0, HealthBarMaterial);
	}

	Owner.AddInstanceComponent(CreatedComponent);
	CreatedComponent->RegisterComponent();
	if (bShouldStartVisible)
	{
		CreatedComponent->Activate(true);
	}
	else
	{
		CreatedComponent->DeactivateImmediate();
	}

	HealthBarComponent = CreatedComponent;
	if (bLogDuplicateHealthBarDiagnostics)
	{
		UE_LOG(LogEnemyHealthDisplay, Verbose,
			TEXT("[EnemyHealthDisplay][HealthBarBinding] Owner=%s CreatedDefault=1 Component=%s System=%s SystemPath=%s RelativeLocation=%s WorldLocation=%s"),
			*GetNameSafe(&Owner),
			*GetNameSafe(CreatedComponent),
			*GetNameSafe(HealthBarSystem.Get()),
			HealthBarSystem ? *HealthBarSystem->GetPathName() : TEXT("None"),
			*HealthBarRelativeLocation.ToCompactString(),
			*CreatedComponent->GetComponentLocation().ToCompactString());
	}

	return CreatedComponent;
}

UNiagaraComponent* UEnemyHealthDisplayComponent::ResolveHealthBarComponent()
{
	if (IsValid(HealthBarComponent))
	{
		return HealthBarComponent;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !bAutoFindHealthBarComponent)
	{
		return nullptr;
	}

	for (TFieldIterator<FObjectPropertyBase> PropertyIt(Owner->GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		const FObjectPropertyBase* ObjectProperty = *PropertyIt;
		if (!ObjectProperty
			|| !ObjectProperty->PropertyClass
			|| !ObjectProperty->PropertyClass->IsChildOf(UNiagaraComponent::StaticClass()))
		{
			continue;
		}

		if (!IsLikelyHealthBarComponentName(ObjectProperty->GetName(), HealthBarComponentName))
		{
			continue;
		}

		if (UNiagaraComponent* ReflectedComponent =
			Cast<UNiagaraComponent>(ObjectProperty->GetObjectPropertyValue_InContainer(Owner)))
		{
			HealthBarComponent = ReflectedComponent;
			if (bLogHealthDisplayDebug)
			{
				UE_LOG(LogEnemyHealthDisplay, Verbose,
					TEXT("[EnemyHealthDisplay][ResolveHealthBar] Owner=%s found reflected Niagara property %s -> %s System=%s"),
					*GetNameSafe(Owner),
					*ObjectProperty->GetName(),
					*GetNameSafe(ReflectedComponent),
					*GetNameSafe(ReflectedComponent->GetAsset()));
			}
			return HealthBarComponent;
		}
	}

	TArray<UNiagaraComponent*> NiagaraComponents;
	Owner->GetComponents<UNiagaraComponent>(NiagaraComponents);
	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		CollectAttachedNiagaraComponents(SceneComponent, NiagaraComponents);
	}

	UNiagaraComponent* ExactNameCandidate = nullptr;
	UNiagaraComponent* NameCandidate = nullptr;
	UNiagaraComponent* SystemCandidate = nullptr;
	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (!NiagaraComponent)
		{
			continue;
		}

		if (HealthBarComponentName.IsNone() || NiagaraComponent->GetFName() == HealthBarComponentName)
		{
			if (!ExactNameCandidate)
			{
				ExactNameCandidate = NiagaraComponent;
			}
			continue;
		}

		const FString ComponentName = NiagaraComponent->GetName();
		const FString SystemName = GetNameSafe(NiagaraComponent->GetAsset());
		if (IsLikelyHealthBarComponentName(ComponentName, HealthBarComponentName))
		{
			NameCandidate = NiagaraComponent;
		}

		if (IsLikelyHealthBarSystemName(SystemName))
		{
			SystemCandidate = NiagaraComponent;
		}
	}

	HealthBarComponent = ExactNameCandidate ? ExactNameCandidate : (NameCandidate ? NameCandidate : SystemCandidate);
	if (IsValid(HealthBarComponent))
	{
		LogDuplicateHealthBarDiagnostics(*Owner, NiagaraComponents, HealthBarComponent.Get(), TEXT("ResolveMatched"), false);
		if (bLogHealthDisplayDebug)
		{
			UE_LOG(LogEnemyHealthDisplay, Verbose,
				TEXT("[EnemyHealthDisplay][ResolveHealthBar] Owner=%s found matched Niagara %s System=%s CandidateCount=%d"),
				*GetNameSafe(Owner),
				*GetNameSafe(HealthBarComponent.Get()),
				*GetNameSafe(HealthBarComponent->GetAsset()),
				NiagaraComponents.Num());
		}
		return HealthBarComponent;
	}

	if (!bOwnerDeathStarted
		&& !bInitialHealthDisplayRefreshPending
		&& bCreateMissingHealthBarComponent
		&& HealthBarSystem)
	{
		LogDuplicateHealthBarDiagnostics(*Owner, NiagaraComponents, nullptr, TEXT("CreateMissing"), true);
		if (UNiagaraComponent* CreatedComponent = CreateHealthBarComponent(*Owner))
		{
			return CreatedComponent;
		}
	}

	LogDuplicateHealthBarDiagnostics(*Owner, NiagaraComponents, nullptr, TEXT("ResolveNone"), false);
	if (bLogHealthDisplayDebug)
	{
		UE_LOG(LogEnemyHealthDisplay, Verbose,
			TEXT("[EnemyHealthDisplay][ResolveHealthBar] Owner=%s no Niagara health bar found. Desired=%s CandidateCount=%d"),
			*GetNameSafe(Owner),
			*HealthBarComponentName.ToString(),
			NiagaraComponents.Num());

		for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		{
			UE_LOG(LogEnemyHealthDisplay, Verbose,
				TEXT("[EnemyHealthDisplay][ResolveHealthBar] Candidate Owner=%s Component=%s FName=%s System=%s Visible=%d"),
				*GetNameSafe(Owner),
				*GetNameSafe(NiagaraComponent),
				NiagaraComponent ? *NiagaraComponent->GetFName().ToString() : TEXT("None"),
				NiagaraComponent ? *GetNameSafe(NiagaraComponent->GetAsset()) : TEXT("None"),
				NiagaraComponent && NiagaraComponent->IsVisible() ? 1 : 0);
		}
	}

	return nullptr;
}

UMaterialInstanceDynamic* UEnemyHealthDisplayComponent::ResolveHealthBarMaterialInstance()
{
	if (IsValid(HealthBarMaterialInstance))
	{
		return HealthBarMaterialInstance;
	}

	UNiagaraComponent* HealthBar = ResolveHealthBarComponent();
	if (!HealthBar)
	{
		return nullptr;
	}

	if (HealthBarMaterial)
	{
		HealthBar->SetMaterial(0, HealthBarMaterial);
	}

	if (HealthBar->GetNumMaterials() <= 0)
	{
		return nullptr;
	}

	HealthBarMaterialInstance = HealthBar->CreateDynamicMaterialInstance(0, HealthBarMaterial);
	if (IsValid(HealthBarMaterialInstance) && !MaterialUseDynamicParametersParameterName.IsNone())
	{
		HealthBarMaterialInstance->SetScalarParameterValue(
			MaterialUseDynamicParametersParameterName,
			bUseNiagaraDynamicMaterialParameters ? 1.0f : 0.0f);
	}

	return HealthBarMaterialInstance;
}

UNiagaraSystem* UEnemyHealthDisplayComponent::ResolveDamageValueSystem() const
{
	UNiagaraSystem* ConfiguredSystem = DamageValueSystem.Get();
	const bool bConfiguredSystemLooksLikeHealthBar = ConfiguredSystem
		&& (ConfiguredSystem == HealthBarSystem
			|| IsLikelyHealthBarSystemName(ConfiguredSystem->GetName()));
	if (ConfiguredSystem && !bConfiguredSystemLooksLikeHealthBar)
	{
		return ConfiguredSystem;
	}

	UNiagaraSystem* FallbackSystem = LoadObject<UNiagaraSystem>(nullptr, DefaultDamageValueSystemPath);
	return FallbackSystem;
}

float UEnemyHealthDisplayComponent::GetCurrentArmorHP() const
{
	const UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!ASC || !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetArmorHPAttribute()))
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, ASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute()));
}

float UEnemyHealthDisplayComponent::GetCurrentMaxArmorHP() const
{
	const UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!ASC || !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxArmorHPAttribute()))
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxArmorHPAttribute()));
}

float UEnemyHealthDisplayComponent::GetCurrentMaxHealth() const
{
	const UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!ASC || !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute()))
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute()));
}

float UEnemyHealthDisplayComponent::GetCurrentHealthPercent() const
{
	const UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!ASC
		|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute())
		|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute()))
	{
		return 0.0f;
	}

	const float Health = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
	return CalculateHealthPercent(Health, GetCurrentMaxHealth());
}

float UEnemyHealthDisplayComponent::GetCurrentDisplayHealthPercent(float FallbackHealthPercent) const
{
	const UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	const bool bHasHealthAttributes = ASC
		&& ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute())
		&& ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	const float HealthPercent = bHasHealthAttributes
		? GetCurrentHealthPercent()
		: FMath::Clamp(FallbackHealthPercent, 0.0f, 1.0f);

	return CalculateDisplayHealthPercent(
		HealthPercent,
		GetCurrentArmorHP(),
		GetCurrentMaxArmorHP(),
		bDisplayArmorAsHealthPercent);
}

float UEnemyHealthDisplayComponent::GetHealthBarVisibleDuration() const
{
	const float FallbackDuration = FMath::Max(HideDelay, 0.0f);
	const UYogAbilitySystemComponent* ASC = CachedAbilitySystemComponent.Get();
	if (!ASC)
	{
		return FallbackDuration;
	}

	const float RecentlyDamagedRemaining = ASC->GetRecentlyDamagedStateRemainingTime();
	if (RecentlyDamagedRemaining > 0.0f)
	{
		return FMath::Max(RecentlyDamagedRemaining, FallbackDuration);
	}

	const float RecentlyDamagedDuration = FMath::Max(ASC->RecentlyDamagedStateDuration, 0.0f);
	return FMath::Max(RecentlyDamagedDuration, FallbackDuration);
}

void UEnemyHealthDisplayComponent::HideHealthBar()
{
	SetHealthBarVisible(false);
}

void UEnemyHealthDisplayComponent::HideHealthBarImmediately()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideHealthBarTimerHandle);
	}

	SetHealthBarVisible(false);
}

void UEnemyHealthDisplayComponent::ShowHealthBarUntilOwnerDestroyed()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideHealthBarTimerHandle);
	}

	if (UNiagaraComponent* HealthBar = ResolveHealthBarComponent())
	{
		HideInactiveSplashNiagaraComponents(HealthBar);
		HealthBar->SetVisibility(true);
		HealthBar->SetHiddenInGame(false);
		if (!HealthBar->IsActive())
		{
			HealthBar->Activate(true);
		}
	}
}

void UEnemyHealthDisplayComponent::SetNiagaraColorParameter(
	UNiagaraComponent& NiagaraComponent,
	FName ParameterName,
	const FLinearColor& Value)
{
	if (ParameterName.IsNone())
	{
		return;
	}

	NiagaraComponent.SetVariableLinearColor(ParameterName, Value);

	const FString ParameterString = ParameterName.ToString();
	if (!ParameterString.StartsWith(TEXT("User.")))
	{
		NiagaraComponent.SetVariableLinearColor(
			FName(*FString::Printf(TEXT("User.%s"), *ParameterString)),
			Value);
	}
}
