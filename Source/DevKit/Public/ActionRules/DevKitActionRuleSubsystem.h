#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActionRules/DevKitActionRuleSet.h"
#include "DevKitActionRuleSubsystem.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Action Rules"))
class DEVKIT_API UDevKitActionRuleSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/** Opt in after validating a profile. Existing gameplay stays unchanged until enabled. */
	UPROPERTY(Config, EditAnywhere, Category="Rule source") bool bEnabled = false;
	/** Relative to Content/ActionRules, for example MyCombat/rules.json. */
	UPROPERTY(Config, EditAnywhere, Category="Rule source") FString Profile = TEXT("Examples/player_combat.example.json");
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};

/** Per-world snapshot: no global PIE leakage, no timers and no persistent block counters. */
UCLASS()
class DEVKIT_API UDevKitActionRuleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** Development-only atomic reload. Invalid candidates keep the last valid snapshot. */
	UFUNCTION(BlueprintCallable, Category="Actions|Rules") bool ReloadRules(FString& Error);
	UFUNCTION(BlueprintPure, Category="Actions|Rules") FString GetStatus() const;
	FDevKitActionRuleDecision Evaluate(const FGameplayTagContainer& RequestTags,
		const FGameplayTagContainer& OwnedTags, const FString& Subject, const TMap<FString, double>& Values) const;
	static bool ResolveProfilePath(const FString& RelativePath, FString& FullPath, FString& Error);
	TSharedPtr<const FDevKitActionRuleSet> GetSnapshot() const { return Snapshot; }
private:
	friend struct FDevKitActionRuleTestAccess;
	bool LoadConfigured(FString& Error);
	bool bEnabled = false;
	FString LastError;
	TSharedPtr<const FDevKitActionRuleSet> Snapshot;
};
