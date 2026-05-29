#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "EnemyHealthDisplayComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UYogAbilitySystemComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UEnemyHealthDisplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyHealthDisplayComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bEnableHealthDisplay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bUseArmorTint = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bDisplayArmorAsHealthPercent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bAutoFindHealthBarComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bUpdateMaterialParameters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bUseNiagaraDynamicMaterialParameters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bHideHealthBarOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bOnlyShowHealthBarAfterDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display")
	bool bHideLegacyCharacterWidgetComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Debug")
	bool bLogHealthDisplayDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Debug")
	bool bLogDuplicateHealthBarDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Debug")
	bool bLogNiagaraRuntimeDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Debug")
	bool bBreakOnNiagaraRuntimeDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	bool bCreateMissingHealthBarComponent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float HideDelay = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	TObjectPtr<UNiagaraComponent> HealthBarComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName HealthBarComponentName = TEXT("NiagaraSystem_HealthBar");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	TObjectPtr<UNiagaraSystem> HealthBarSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FVector HealthBarRelativeLocation = FVector(0.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	TObjectPtr<UNiagaraSystem> DamageValueSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName HealthPercentParameterName = TEXT("new");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName OldHealthPercentParameterName = TEXT("temp");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName LegacyOldHealthPercentParameterName = TEXT("old");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName DamageValueParameterName = TEXT("num");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName DamageMarkerBoolParameterName = TEXT("\u5E03\u5C14");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName DamageValueColorParameterName = TEXT("color");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Niagara")
	FName ArmorParameterName = TEXT("armor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Material")
	FName MaterialHealthPercentParameterName = TEXT("HealthPercent");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Material")
	FName MaterialOldHealthPercentParameterName = TEXT("OldHealthPercent");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Material")
	FName MaterialUseDynamicParametersParameterName = TEXT("UseDynamicParameters");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Material")
	TObjectPtr<UMaterialInterface> HealthBarMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Material")
	FName MaterialArmorParameterName = TEXT("Armor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Damage Value")
	FVector DamageValueLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Damage Value")
	FRotator DamageValueRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Damage Value")
	FVector DamageValueScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Damage Value", meta = (ClampMin = "0.0"))
	float MinDamageValueToDisplay = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Damage Value")
	FLinearColor HealthDamageValueColor = FLinearColor(1.0f, 0.05f, 0.02f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health Display|Damage Value")
	FLinearColor ArmorDamageValueColor = FLinearColor::White;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Health Display")
	void RefreshDisplayFromAttributes();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Health Display")
	void SetHealthBarVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Enemy|Health Display")
	static float CalculateHealthPercent(float Health, float MaxHealth);

	UFUNCTION(BlueprintPure, Category = "Enemy|Health Display")
	static bool ShouldUseArmorTint(float ArmorHP, bool bEnableArmorTint);

	UFUNCTION(BlueprintPure, Category = "Enemy|Health Display")
	static float CalculateDisplayHealthPercent(
		float HealthPercent,
		float ArmorHP,
		float MaxArmorHP,
		bool bShouldDisplayArmorAsHealthPercent);

	static bool IsLikelyHealthBarComponentName(const FString& ComponentName, FName DesiredName);
	static bool IsLikelyHealthBarSystemName(const FString& SystemName);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleCharacterHealthUpdate(float HealthPercent, float DamageTaken);

	UFUNCTION()
	void HandleCharacterDamageValue(float DamageAmount, EYogDamageValueType DamageValueType);

	UFUNCTION()
	void HandleOwnerDeathStarted(AYogCharacterBase* Character);

	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
	void HandleArmorChanged(const FOnAttributeChangeData& Data);
	void BindAttributeDelegates();
	void UnbindAttributeDelegates();
	void UpdateHealthBar(float HealthPercent, float OldHealthPercent);
	void UpdateArmorParameter();
	void SpawnDamageValue(float DamageAmount, EYogDamageValueType DamageValueType);
	static void SetNiagaraFloatParameter(UNiagaraComponent& NiagaraComponent, FName ParameterName, float Value);
	static void SetNiagaraColorParameter(UNiagaraComponent& NiagaraComponent, FName ParameterName, const FLinearColor& Value);
	void LogHealthDisplayState(
		const TCHAR* Source,
		float InputHealthPercent,
		float OutputHealthPercent,
		float OldHealthPercent,
		float DamageTaken) const;
	void InitializeHealthDisplayAfterBeginPlay();
	void ShowHealthBarTemporarily();
	void HideLegacyCharacterWidgetComponent();
	void HideInactiveSplashNiagaraComponents(const UNiagaraComponent* HealthBarToKeep);
	void LogDuplicateHealthBarDiagnostics(
		AActor& Owner,
		const TArray<UNiagaraComponent*>& NiagaraComponents,
		const UNiagaraComponent* SelectedHealthBar,
		const TCHAR* Source,
		bool bWillCreateMissingHealthBar);
	void LogNiagaraRuntimeDiagnostics(
		UNiagaraComponent& HealthBar,
		const TCHAR* Source,
		float NewHealthPercent,
		float OldHealthPercent);
	UNiagaraComponent* CreateHealthBarComponent(AActor& Owner);
	UNiagaraComponent* ResolveHealthBarComponent();
	UNiagaraSystem* ResolveDamageValueSystem() const;
	UMaterialInstanceDynamic* ResolveHealthBarMaterialInstance();
	float GetCurrentArmorHP() const;
	float GetCurrentMaxArmorHP() const;
	float GetCurrentMaxHealth() const;
	float GetCurrentHealthPercent() const;
	float GetCurrentDisplayHealthPercent(float FallbackHealthPercent) const;
	float GetHealthBarVisibleDuration() const;
	void HideHealthBar();
	void HideHealthBarImmediately();
	void ShowHealthBarUntilOwnerDestroyed();

	UPROPERTY(Transient)
	TObjectPtr<UYogAbilitySystemComponent> CachedAbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HealthBarMaterialInstance;

	FTimerHandle HideHealthBarTimerHandle;
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
	FDelegateHandle ArmorChangedDelegateHandle;
	float LastHealthPercent = 1.0f;
	bool bHasLastHealthPercent = false;
	bool bInitialHealthDisplayRefreshPending = false;
	bool bOwnerDeathStarted = false;
	bool bLoggedDuplicateHealthBarDiagnostics = false;
	bool bLoggedLegacyWidgetDiagnostics = false;
	bool bLoggedNiagaraRuntimeStartupDiagnostics = false;
	bool bLoggedNiagaraRuntimeDamageDiagnostics = false;
};
