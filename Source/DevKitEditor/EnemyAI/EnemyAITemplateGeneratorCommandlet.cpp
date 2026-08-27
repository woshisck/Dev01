#include "DevKitEditor/EnemyAI/EnemyAITemplateGeneratorCommandlet.h"

#include "AbilitySystem/Abilities/GA_EnemyMeleeAttacks.h"
#include "AbilitySystem/Abilities/GA_EnemyWeaponSkills.h"
#include "Commandlets/CommandletReportUtils.h"
#include "AI/StateTree/YogStateTreeConditions.h"
#include "AI/StateTree/YogStateTreeEvaluators.h"
#include "AI/StateTree/YogStateTreeTask_EnemyAttackByProfile.h"
#include "AI/StateTree/YogStateTreeTasks.h"
#include "Tasks/StateTreeMoveToTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Character/EnemyCharacterBase.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "Data/GasTemplate.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorModule.h"
#include "StateTreeEditorSchema.h"
#include "StateTreeFactory.h"
#include "StateTreeState.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace EnemyAITemplateGenerator
{
	const FString BlackboardPath = TEXT("/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee");
	const FString StateTreePath = TEXT("/Game/Code/Enemy/AI/StateTree/ST_Enemy_DefaultMelee");
	const FString EnemyGASTemplatePath = TEXT("/Game/Docs/Data/Enemy/DA_Enemy_GASTemplate");
	const FString RatDataPath = TEXT("/Game/Docs/Data/Enemy/Rat/DA_Rat");
	const FString RottenGuardDataPath = TEXT("/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard");
	const FString AlarmBellJailerDataPath = TEXT("/Game/Docs/Data/Enemy/AlarmBellJailer/DA_AlarmBellJailer");
	const FString AlarmBellJailerAbilityDataPath = TEXT("/Game/Docs/Data/Enemy/AlarmBellJailer/DA_AbilityMontage_AlarmBellJailer_01");
	const FString GuardCaptainDataPath = TEXT("/Game/Docs/Data/Enemy/GuardCaptain/DA_GuardCaptain");
	const FString GuardCaptainAbilityDataPath = TEXT("/Game/Docs/Data/Enemy/GuardCaptain/DA_AbilityMontage_GuardCaptain_01");
	const FString BossStateTreePath = TEXT("/Game/Code/Enemy/AI/StateTree/ST_Boss");
	const FString BossDataPath = TEXT("/Game/Docs/Data/Enemy/Boss/DA_Boss");
	const FString DebuggerStateTreePath = TEXT("/Game/Code/Enemy/AI/StateTree/ST_Debugger");
	// Phase 2 stat buff. Authored as a GE asset rather than typed into ST_Boss because
	// RebuildBossStateTree wipes anything set on the StateTree itself.
	const FString BossPhase2EffectPath = TEXT("/Game/Code/Enemy/AI/Phase/GE_BossPhase2");

	// Boss flips to phase 2 at or below this fraction of max HP.
	constexpr float BossPhase2HealthPercent = 0.5f;

	enum class EDefaultEnemyProfile : uint8
	{
		Rat,
		RottenGuard,
		AlarmBellJailer,
		GuardCaptain,
		Boss,
	};

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString ExistingPackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &ExistingPackageFile);
	}

	template <typename T>
	T* LoadAssetByPackagePath(const FString& PackagePath, uint32 LoadFlags = LOAD_None)
	{
		if (T* Existing = FindObject<T>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}

		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}

		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath), nullptr, LoadFlags));
	}

	template <typename T>
	T* CreateOrLoadAsset(const FString& PackagePath, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (T* Existing = LoadAssetByPackagePath<T>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		const FName AssetName(*FPackageName::GetLongPackageAssetName(PackagePath));
		T* Asset = NewObject<T>(Package, AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		return Asset;
	}

	template <typename T>
	T* EnsureBlackboardKey(UBlackboardData& Blackboard, FName KeyName)
	{
		if (T* NewKey = Blackboard.UpdatePersistentKey<T>(KeyName))
		{
			return NewKey;
		}

		const FBlackboard::FKey KeyID = Blackboard.GetKeyID(KeyName);
		const FBlackboardEntry* Entry = Blackboard.GetKey(KeyID);
		return Entry ? Cast<T>(Entry->KeyType) : nullptr;
	}

	void ConfigureBlackboard(UBlackboardData& Blackboard)
	{
		if (UBlackboardKeyType_Enum* StateKey = EnsureBlackboardKey<UBlackboardKeyType_Enum>(Blackboard, TEXT("EnemyAIState")))
		{
			StateKey->EnumType = StaticEnum<EEnemyAIState>();
			StateKey->EnumName = TEXT("/Script/DevKit.EEnemyAIState");
		}

		if (UBlackboardKeyType_Object* TargetKey = EnsureBlackboardKey<UBlackboardKeyType_Object>(Blackboard, TEXT("TargetActor")))
		{
			TargetKey->BaseClass = AActor::StaticClass();
		}

		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("LastKnownTargetLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("PatrolOriginLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("PatrolTargetLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("MoveTargetLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("DistanceToTarget"));
		EnsureBlackboardKey<UBlackboardKeyType_Bool>(Blackboard, TEXT("bInAttackRange"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("AcceptanceRadius"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("AlertExpireTime"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("LastSeenTargetTime"));
		EnsureBlackboardKey<UBlackboardKeyType_Bool>(Blackboard, TEXT("bLastAttackWhiffed"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("LastWhiffTime"));
		EnsureBlackboardKey<UBlackboardKeyType_Bool>(Blackboard, TEXT("bPostAttackReposition"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("LastRepositionRequestTime"));

		Blackboard.UpdateKeyIDs();
		Blackboard.MarkPackageDirty();
	}

	FEnemyAIAttackOption MakeAttackOption(
		const TCHAR* AttackName,
		const TCHAR* TagName,
		float MinRange,
		float MaxRange,
		float Weight,
		float Cooldown,
		EEnemyAIAttackRole AttackRole = EEnemyAIAttackRole::CloseMelee,
		EEnemyAIAttackMovementMode MovementMode = EEnemyAIAttackMovementMode::None,
		float LungeStartRange = 0.0f,
		float LungeDistance = 0.0f,
		float LungeDuration = 0.35f,
		float LungeStopDistance = 0.0f,
		float MovementAttackRangeMultiplier = 2.5f,
		float MovementAttackCooldown = 10.0f,
		float MinHealthPercent = 0.0f,
		float MaxHealthPercent = 1.0f,
		bool bRequestRepositionOnResolve = false,
		float RepositionAngleMin = 60.0f,
		float RepositionAngleMax = 120.0f,
		bool bPreAttackFlash = true)
	{
		FEnemyAIAttackOption Option;
		Option.AttackName = FName(AttackName);
		Option.MinRange = MinRange;
		Option.MaxRange = MaxRange;
		Option.Weight = Weight;
		Option.Cooldown = Cooldown;
		Option.bPreAttackFlash = bPreAttackFlash;
		Option.MinHealthPercent = MinHealthPercent;
		Option.MaxHealthPercent = MaxHealthPercent;
		Option.AttackRole = AttackRole;
		Option.AttackMovementMode = MovementMode;
		Option.LungeStartRange = LungeStartRange;
		Option.LungeDistance = LungeDistance;
		Option.LungeDuration = LungeDuration;
		Option.LungeStopDistance = LungeStopDistance;
		Option.MovementAttackRangeMultiplier = MovementAttackRangeMultiplier;
		Option.MovementAttackCooldown = MovementAttackCooldown;
		Option.bRequestRepositionOnResolve = bRequestRepositionOnResolve;
		Option.RepositionAngleMin = RepositionAngleMin;
		Option.RepositionAngleMax = RepositionAngleMax;

		const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		if (AbilityTag.IsValid())
		{
			Option.AbilityTags.AddTag(AbilityTag);
		}

		return Option;
	}

	void UpsertAttackOption(FEnemyAIAttackProfile& AttackProfile, const FEnemyAIAttackOption& Option)
	{
		for (FEnemyAIAttackOption& Existing : AttackProfile.Attacks)
		{
			if (Existing.AttackName == Option.AttackName)
			{
				Existing = Option;
				return;
			}
		}

		AttackProfile.Attacks.Add(Option);
	}

	bool AttackOptionHasAbilityTag(const FEnemyAIAttackOption& Option, const FGameplayTag& AbilityTag)
	{
		return AbilityTag.IsValid() && Option.AbilityTags.HasTagExact(AbilityTag);
	}

	bool AttackProfileHasAbilityTag(const FEnemyAIAttackProfile& AttackProfile, const FGameplayTag& AbilityTag)
	{
		for (const FEnemyAIAttackOption& Attack : AttackProfile.Attacks)
		{
			if (AttackOptionHasAbilityTag(Attack, AbilityTag))
			{
				return true;
			}
		}
		return false;
	}

	void UpsertAttackOptionByTag(FEnemyAIAttackProfile& AttackProfile, const FEnemyAIAttackOption& Option)
	{
		for (const FGameplayTag& AbilityTag : Option.AbilityTags)
		{
			if (!AbilityTag.IsValid())
			{
				continue;
			}

			for (FEnemyAIAttackOption& Existing : AttackProfile.Attacks)
			{
				if (AttackOptionHasAbilityTag(Existing, AbilityTag))
				{
					Existing = Option;
					return;
				}
			}
		}

		UpsertAttackOption(AttackProfile, Option);
	}

	void EnsureGASTemplateAbility(UGASTemplate& GASTemplate, TSubclassOf<UYogGameplayAbility> AbilityClass)
	{
		if (!AbilityClass)
		{
			return;
		}

		for (const TSubclassOf<UYogGameplayAbility>& Existing : GASTemplate.AbilityMap)
		{
			if (Existing == AbilityClass)
			{
				return;
			}
		}

		GASTemplate.AbilityMap.Add(AbilityClass);
	}

	void ConfigureSharedEnemyGASTemplate(UGASTemplate& GASTemplate)
	{
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_LAtk1::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_LAtk2::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_LAtk3::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_LAtk4::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_HAtk1::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_HAtk2::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_HAtk3::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_HAtk4::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_LAtk1::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_LAtk2::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_LAtk3::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_LAtk4::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_HAtk1::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_HAtk2::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_HAtk3::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Range_HAtk4::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Skill1::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Skill2::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Skill3::StaticClass());
		EnsureGASTemplateAbility(GASTemplate, UGA_Enemy_Skill4::StaticClass());
	}

	FString GetAttackNameFromTag(const FGameplayTag& AbilityTag)
	{
		const FString TagString = AbilityTag.ToString();
		int32 LastDotIndex = INDEX_NONE;
		return TagString.FindLastChar(TEXT('.'), LastDotIndex) ? TagString.RightChop(LastDotIndex + 1) : TagString;
	}

	bool IsGeneratedAttackTag(const FString& TagString)
	{
		return TagString.StartsWith(TEXT("Enemy.Melee."))
			|| TagString.StartsWith(TEXT("Enemy.Range."))
			|| TagString.StartsWith(TEXT("Enemy.Skill."));
	}

	FEnemyAIAttackOption MakeGeneratedAttackOptionFromAbilityTag(const UEnemyData& EnemyData, const FGameplayTag& AbilityTag)
	{
		const FString TagString = AbilityTag.ToString();
		const FString AttackName = GetAttackNameFromTag(AbilityTag);
		const float CloseAttackRange = FMath::Max(EnemyData.MovementTuning.AttackRange, 1.0f);

		if (TagString.StartsWith(TEXT("Enemy.Skill.")))
		{
			const float SkillRange = FMath::Max(EnemyData.AwarenessTuning.CombatEnterRadius, CloseAttackRange);
			return MakeAttackOption(
				*AttackName,
				*TagString,
				0.0f,
				SkillRange,
				1.0f,
				15.0f,
				EEnemyAIAttackRole::Skill);
		}

		if (TagString.StartsWith(TEXT("Enemy.Range.")))
		{
			const float RangedMax = FMath::Max(EnemyData.AwarenessTuning.CombatEnterRadius, CloseAttackRange);
			const float RangedMin = TagString.Contains(TEXT(".HAtk")) ? CloseAttackRange : 0.0f;
			return MakeAttackOption(
				*AttackName,
				*TagString,
				RangedMin,
				RangedMax,
				TagString.Contains(TEXT(".HAtk")) ? 1.2f : 2.0f,
				TagString.Contains(TEXT(".HAtk")) ? 4.0f : 2.2f,
				EEnemyAIAttackRole::Skill);
		}

		if (TagString.Contains(TEXT(".LAtk")))
		{
			return MakeAttackOption(
				*AttackName,
				*TagString,
				0.0f,
				CloseAttackRange,
				2.5f,
				0.9f,
				EEnemyAIAttackRole::CloseMelee);
		}

		const float HeavyCloseRange = FMath::Max(CloseAttackRange + EnemyData.MovementTuning.AttackRangeExitBuffer, CloseAttackRange);
		return MakeAttackOption(
			*AttackName,
			*TagString,
			0.0f,
			HeavyCloseRange,
			1.25f,
			1.4f,
			EEnemyAIAttackRole::CloseMelee);
	}

	void SyncAttackProfileFromAbilityData(UEnemyData& EnemyData)
	{
		const UAbilityData* AbilityData = EnemyData.AbilityData;
		if (!AbilityData)
		{
			return;
		}

		TArray<FGameplayTag> ValidAttackTags;
		auto AddValidAttackTag = [&ValidAttackTags, AbilityData](const FGameplayTag& AbilityTag)
		{
			const FString TagString = AbilityTag.ToString();
			if (!AbilityTag.IsValid() || !IsGeneratedAttackTag(TagString) || !AbilityData->HasAbility(AbilityTag))
			{
				return;
			}
			ValidAttackTags.AddUnique(AbilityTag);
		};

		for (const TPair<FGameplayTag, TObjectPtr<UAnimMontage>>& MontageEntry : AbilityData->MontageMap)
		{
			AddValidAttackTag(MontageEntry.Key);
		}
		for (const TPair<FGameplayTag, FAbilityMontageConfigList>& ConfigEntry : AbilityData->MontageConfigMap)
		{
			AddValidAttackTag(ConfigEntry.Key);
		}

		ValidAttackTags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
		{
			return A.ToString() < B.ToString();
		});

		for (const FGameplayTag& AbilityTag : ValidAttackTags)
		{
			if (AttackProfileHasAbilityTag(EnemyData.AttackProfile, AbilityTag))
			{
				continue;
			}

			EnemyData.AttackProfile.Attacks.Add(MakeGeneratedAttackOptionFromAbilityTag(EnemyData, AbilityTag));
		}
	}

	void ConfigureDefaultEnemyData(UEnemyData& EnemyData, EDefaultEnemyProfile Profile)
	{
		FEnemyAIAwarenessTuning Awareness;
		Awareness.DetectionRadius = 900.f;
		Awareness.CombatEnterRadius = 650.f;
		Awareness.CombatExitRadius = 1200.f;
		Awareness.LoseTargetDelay = 2.0f;
		Awareness.AlertDuration = 4.0f;
		Awareness.AlertBroadcastRadius = 1200.f;
		Awareness.PatrolRadius = 600.f;
		Awareness.PatrolWaitMin = 0.6f;
		Awareness.PatrolWaitMax = 1.5f;
		EnemyData.AwarenessTuning = Awareness;
		EnemyData.AttackProfile.RecentAttackMemoryDuration = 2.0f;
		EnemyData.AttackProfile.RepeatAttackWeightMultiplier = 0.25f;

		switch (Profile)
		{
		case EDefaultEnemyProfile::Rat:
			EnemyData.MovementTuning.ApproachStyle = EEnemyAIApproachStyle::SwarmFlank;
			EnemyData.MovementTuning.PreferredRange = 180.f;
			EnemyData.MovementTuning.AttackRange = 150.f;
			EnemyData.MovementTuning.AcceptanceRadius = 60.f;
			EnemyData.MovementTuning.RepathInterval = 0.2f;
			EnemyData.MovementTuning.FlankDistance = 160.f;
			EnemyData.MovementTuning.StrafeChance = 0.45f;
			EnemyData.MovementTuning.CrowdSeparationWeight = 2.4f;
			EnemyData.MovementTuning.bUseForwardSteering = true;
			EnemyData.MovementTuning.ForwardTurnLeadDistance = 170.f;
			EnemyData.MovementTuning.MaxTurnYawSpeed = 520.f;
			EnemyData.MovementTuning.MoveTargetSmoothingSpeed = 8.0f;
			EnemyData.MovementTuning.SharpTurnAngle = 125.f;
			EnemyData.MovementTuning.MaxWalkSpeedOverride = 0.f;
			EnemyData.MovementTuning.CombatSlotLockDuration = 1.2f;
			EnemyData.MovementTuning.AttackRangeExitBuffer = 40.f;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("QuickBite"), TEXT("Enemy.Melee.LAtk1"), 0.f, 170.f, 2.0f, 0.8f));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("Bite"), TEXT("Enemy.Melee.LAtk2"), 0.f, 180.f, 1.0f, 1.1f));
			break;

		case EDefaultEnemyProfile::RottenGuard:
			EnemyData.MovementTuning.ApproachStyle = EEnemyAIApproachStyle::BruiserHold;
			EnemyData.MovementTuning.PreferredRange = 320.f;
			EnemyData.MovementTuning.AttackRange = 260.f;
			EnemyData.MovementTuning.AcceptanceRadius = 110.f;
			EnemyData.MovementTuning.RepathInterval = 0.35f;
			EnemyData.MovementTuning.FlankDistance = 120.f;
			EnemyData.MovementTuning.StrafeChance = 0.15f;
			EnemyData.MovementTuning.CrowdSeparationWeight = 3.0f;
			EnemyData.MovementTuning.bUseForwardSteering = true;
			EnemyData.MovementTuning.ForwardTurnLeadDistance = 240.f;
			EnemyData.MovementTuning.MaxTurnYawSpeed = 260.f;
			EnemyData.MovementTuning.MoveTargetSmoothingSpeed = 5.0f;
			EnemyData.MovementTuning.SharpTurnAngle = 105.f;
			EnemyData.MovementTuning.MaxWalkSpeedOverride = 420.f;
			EnemyData.MovementTuning.CombatSlotLockDuration = 0.4f;
			EnemyData.MovementTuning.AttackRangeExitBuffer = 40.f;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("Sweep"), TEXT("Enemy.Melee.HAtk1"), 0.f, 290.f, 2.0f, 1.6f));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(
				TEXT("Heavy"),
				TEXT("Enemy.Melee.HAtk2"),
				160.f,
				650.f,
				1.0f,
				2.2f,
				EEnemyAIAttackRole::SpecialMovement,
				EEnemyAIAttackMovementMode::RadialLunge,
				300.f,
				280.f,
				0.35f,
				170.f,
				2.5f,
				10.0f));
			break;

		case EDefaultEnemyProfile::AlarmBellJailer:
		{
			EnemyData.DifficultyScore = 4;
			EnemyData.AwarenessTuning.DetectionRadius = 950.f;
			EnemyData.AwarenessTuning.CombatEnterRadius = 760.f;
			EnemyData.AwarenessTuning.CombatExitRadius = 1250.f;
			EnemyData.AwarenessTuning.AlertDuration = 5.0f;
			EnemyData.AwarenessTuning.AlertBroadcastRadius = 1400.f;
			EnemyData.AwarenessTuning.PatrolRadius = 550.f;
			EnemyData.MovementTuning.ApproachStyle = EEnemyAIApproachStyle::BruiserHold;
			EnemyData.MovementTuning.PreferredRange = 520.f;
			EnemyData.MovementTuning.AttackRange = 680.f;
			EnemyData.MovementTuning.AcceptanceRadius = 95.f;
			EnemyData.MovementTuning.RepathInterval = 0.28f;
			EnemyData.MovementTuning.FlankDistance = 120.f;
			EnemyData.MovementTuning.StrafeChance = 0.25f;
			EnemyData.MovementTuning.CrowdSeparationWeight = 2.2f;
			EnemyData.MovementTuning.bUseForwardSteering = true;
			EnemyData.MovementTuning.ForwardTurnLeadDistance = 220.f;
			EnemyData.MovementTuning.MaxTurnYawSpeed = 300.f;
			EnemyData.MovementTuning.MoveTargetSmoothingSpeed = 5.5f;
			EnemyData.MovementTuning.SharpTurnAngle = 110.f;
			EnemyData.MovementTuning.MaxWalkSpeedOverride = 360.f;
			EnemyData.MovementTuning.CombatSlotLockDuration = 0.4f;
			EnemyData.MovementTuning.AttackRangeExitBuffer = 70.f;
			EnemyData.AttackProfile.WhiffRepositionChance = 0.75f;

			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("BellAlarm"), TEXT("Enemy.Skill.Skill1"), 250.f, 900.f, 2.2f, 14.0f, EEnemyAIAttackRole::Skill));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("BellShock"), TEXT("Enemy.Skill.Skill2"), 0.f, 520.f, 1.0f, 9.0f, EEnemyAIAttackRole::Skill));

			FEnemyAIAttackOption BatonShove = MakeAttackOption(TEXT("BatonShove"), TEXT("Enemy.Melee.LAtk1"), 0.f, 220.f, 1.0f, 2.4f);
			BatonShove.bRequestRepositionOnResolve = true;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, BatonShove);

			FEnemyAIAttackOption PanicRetreat = MakeAttackOption(TEXT("PanicRetreat"), TEXT("Enemy.Melee.LAtk2"), 0.f, 700.f, 1.0f, 4.0f, EEnemyAIAttackRole::Reposition);
			PanicRetreat.bPreAttackFlash = false;
			PanicRetreat.LungeDistance = 360.f;
			PanicRetreat.LungeDuration = 0.4f;
			PanicRetreat.RepositionAngleMin = 0.f;
			PanicRetreat.RepositionAngleMax = 25.f;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, PanicRetreat);
			break;
		}

		case EDefaultEnemyProfile::GuardCaptain:
		{
			EnemyData.DifficultyScore = 10;
			EnemyData.SuperArmorThreshold = 4;
			EnemyData.SuperArmorDuration = 2.2f;
			EnemyData.RecentlyDamagedStateDuration = 3.5f;
			EnemyData.AwarenessTuning.DetectionRadius = 1200.f;
			EnemyData.AwarenessTuning.CombatEnterRadius = 950.f;
			EnemyData.AwarenessTuning.CombatExitRadius = 1600.f;
			EnemyData.AwarenessTuning.AlertDuration = 5.0f;
			EnemyData.AwarenessTuning.AlertBroadcastRadius = 1600.f;
			EnemyData.AwarenessTuning.PatrolRadius = 700.f;
			EnemyData.MovementTuning.ApproachStyle = EEnemyAIApproachStyle::BruiserHold;
			EnemyData.MovementTuning.PreferredRange = 650.f;
			EnemyData.MovementTuning.AttackRange = 900.f;
			EnemyData.MovementTuning.AcceptanceRadius = 110.f;
			EnemyData.MovementTuning.RepathInterval = 0.3f;
			EnemyData.MovementTuning.FlankDistance = 140.f;
			EnemyData.MovementTuning.StrafeChance = 0.2f;
			EnemyData.MovementTuning.CrowdSeparationWeight = 3.6f;
			EnemyData.MovementTuning.bUseForwardSteering = true;
			EnemyData.MovementTuning.ForwardTurnLeadDistance = 260.f;
			EnemyData.MovementTuning.MaxTurnYawSpeed = 220.f;
			EnemyData.MovementTuning.MoveTargetSmoothingSpeed = 4.5f;
			EnemyData.MovementTuning.SharpTurnAngle = 100.f;
			EnemyData.MovementTuning.MaxWalkSpeedOverride = 360.f;
			EnemyData.MovementTuning.CombatSlotLockDuration = 0.3f;
			EnemyData.MovementTuning.AttackRangeExitBuffer = 80.f;
			EnemyData.AttackProfile.RecentAttackMemoryDuration = 3.0f;
			EnemyData.AttackProfile.RepeatAttackWeightMultiplier = 0.2f;
			EnemyData.AttackProfile.WhiffRepositionChance = 0.6f;

			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("ExecutionShot"), TEXT("Enemy.Range.LAtk1"), 320.f, 950.f, 2.0f, 3.2f, EEnemyAIAttackRole::Skill));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("SuppressiveShot"), TEXT("Enemy.Range.HAtk1"), 450.f, 1050.f, 1.1f, 8.0f, EEnemyAIAttackRole::Skill));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("SummonJailer"), TEXT("Enemy.Skill.Skill1"), 450.f, 1100.f, 1.1f, 18.0f, EEnemyAIAttackRole::Skill));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("CommandBuff"), TEXT("Enemy.Skill.Skill2"), 0.f, 950.f, 1.2f, 16.0f, EEnemyAIAttackRole::Skill));

			FEnemyAIAttackOption CaptainPush = MakeAttackOption(TEXT("CaptainPush"), TEXT("Enemy.Melee.HAtk1"), 0.f, 300.f, 2.4f, 5.0f);
			CaptainPush.bRequestRepositionOnResolve = true;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, CaptainPush);

			FEnemyAIAttackOption TacticalRetreat = MakeAttackOption(TEXT("TacticalRetreat"), TEXT("Enemy.Melee.HAtk2"), 0.f, 1000.f, 1.0f, 3.0f, EEnemyAIAttackRole::Reposition);
			TacticalRetreat.bPreAttackFlash = false;
			TacticalRetreat.LungeDistance = 520.f;
			TacticalRetreat.LungeDuration = 0.45f;
			TacticalRetreat.RepositionAngleMin = 0.f;
			TacticalRetreat.RepositionAngleMax = 25.f;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, TacticalRetreat);

			FEnemyAIAttackOption FlameIgnition = MakeAttackOption(TEXT("FlameIgnition"), TEXT("Enemy.Skill.Skill4"), 0.f, 900.f, 3.0f, 60.0f, EEnemyAIAttackRole::Skill);
			FlameIgnition.MaxHealthPercent = 0.35f;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, FlameIgnition);
			break;
		}

		case EDefaultEnemyProfile::Boss:
		{
			EnemyData.DifficultyScore = 12;
			EnemyData.AwarenessTuning.DetectionRadius = 1400.f;
			EnemyData.AwarenessTuning.CombatEnterRadius = 1000.f;
			EnemyData.AwarenessTuning.CombatExitRadius = 2500.f;
			EnemyData.AwarenessTuning.AlertDuration = 3.0f;
			EnemyData.AwarenessTuning.AlertBroadcastRadius = 1800.f;
			EnemyData.AwarenessTuning.PatrolRadius = 500.f;
			EnemyData.AwarenessTuning.PatrolWaitMin = 1.0f;
			EnemyData.AwarenessTuning.PatrolWaitMax = 2.5f;
			EnemyData.MovementTuning.ApproachStyle = EEnemyAIApproachStyle::BruiserHold;
			EnemyData.MovementTuning.PreferredRange = 260.f;
			EnemyData.MovementTuning.AttackRange = 250.f;
			EnemyData.MovementTuning.AcceptanceRadius = 120.f;
			EnemyData.MovementTuning.RepathInterval = 0.3f;
			EnemyData.MovementTuning.FlankDistance = 120.f;
			EnemyData.MovementTuning.StrafeChance = 0.f;
			EnemyData.MovementTuning.CrowdSeparationWeight = 3.6f;
			EnemyData.MovementTuning.bUseForwardSteering = true;
			EnemyData.MovementTuning.ForwardTurnLeadDistance = 260.f;
			EnemyData.MovementTuning.MaxTurnYawSpeed = 200.f;
			EnemyData.MovementTuning.MoveTargetSmoothingSpeed = 4.0f;
			EnemyData.MovementTuning.SharpTurnAngle = 100.f;
			EnemyData.MovementTuning.MaxWalkSpeedOverride = 380.f;
			EnemyData.MovementTuning.CombatSlotLockDuration = 0.3f;
			EnemyData.MovementTuning.AttackRangeExitBuffer = 60.f;
			EnemyData.AttackProfile.RecentAttackMemoryDuration = 0.f;
			EnemyData.AttackProfile.RepeatAttackWeightMultiplier = 1.0f;
			EnemyData.AttackProfile.WhiffRepositionChance = 0.f;

			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("HeavySlam"), TEXT("Enemy.Melee.HAtk1"), 0.f, 250.f, 1.0f, 1.8f));
			break;
		}
		}

		SyncAttackProfileFromAbilityData(EnemyData);
	}

	UStateTree* CreateOrLoadStateTree(const FString& PackagePath, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UStateTree* Existing = LoadAssetByPackagePath<UStateTree>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		const FName AssetName(*FPackageName::GetLongPackageAssetName(PackagePath));

		UStateTreeFactory* Factory = NewObject<UStateTreeFactory>();
		Factory->SetSchemaClass(UStateTreeAIComponentSchema::StaticClass());
		UStateTree* StateTree = Cast<UStateTree>(Factory->FactoryCreateNew(
			UStateTree::StaticClass(),
			Package,
			AssetName,
			RF_Public | RF_Standalone | RF_Transactional,
			nullptr,
			GWarn));

		if (!StateTree)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create `%s`."), *PackagePath));
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(StateTree);
		StateTree->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		return StateTree;
	}

	template <typename TaskType>
	TStateTreeEditorNode<TaskType>& AddNamedTask(UStateTreeState& State, const TCHAR* TaskName)
	{
		TStateTreeEditorNode<TaskType>& Task = State.AddTask<TaskType>();
		Task.SetNodeName(FName(TaskName));
		return Task;
	}

	UStateTreeState& AddAttackState(UStateTreeState& Parent, const TCHAR* StateName, EEnemyAIAttackRole Role)
	{
		UStateTreeState& State = Parent.AddChildState(FName(StateName));
		State.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeTask_EnemyAttackByProfile>& Task = AddNamedTask<FStateTreeTask_EnemyAttackByProfile>(State, StateName);
		Task.GetInstanceData().RequiredAttackRole = Role;
		State.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Parent);
		State.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::NextSelectableState);
		return State;
	}

	UStateTreeState& AddMoveState(
		UStateTreeState& Parent,
		const TCHAR* StateName,
		const TCHAR* DestinationKey,
		float AcceptanceRadius)
	{
		UStateTreeState& State = Parent.AddChildState(FName(StateName));
		State.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeTask_MoveToControllerTarget>& Task =
			AddNamedTask<FStateTreeTask_MoveToControllerTarget>(State, StateName);
		Task.GetInstanceData().DestinationKey = FName(DestinationKey);
		Task.GetInstanceData().AcceptanceRadius = AcceptanceRadius;
		return State;
	}

	// Chase state: repaths toward the combat slot as the player moves and succeeds
	// once in attack range. Distinct from AddMoveState, which fires a single path
	// request and waits on it.
	UStateTreeState& AddChaseState(UStateTreeState& Parent, const TCHAR* StateName)
	{
		UStateTreeState& State = Parent.AddChildState(FName(StateName));
		State.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		AddNamedTask<FStateTreeTask_EnemyCombatMove>(State, StateName);
		return State;
	}

	// OnTick escape hatch so a state hands off the moment the awareness evaluator
	// flips EnemyAIState, instead of waiting for its own tasks to finish.
	void AddStateGateTransition(
		UStateTreeState& From,
		UStateTreeState& To,
		EEnemyAIState RequiredState,
		const TCHAR* ConditionName)
	{
		FStateTreeTransition& Transition = From.AddTransition(
			EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &To);
		TStateTreeEditorNode<FStateTreeCondition_EnemyAIState>& Condition =
			Transition.AddConditionWithOuter<FStateTreeCondition_EnemyAIState>(&From);
		Condition.SetNodeName(FName(ConditionName));
		Condition.GetInstanceData().RequiredState = RequiredState;
	}

	// A boss phase is a container whose children are tried in order. Phases are added
	// low-HP-threshold first, because the selector takes the first child whose enter
	// conditions pass and the base phase carries no condition at all.
	UStateTreeState& AddBossPhaseState(UStateTreeState& Parent, const TCHAR* StateName)
	{
		UStateTreeState& State = Parent.AddChildState(FName(StateName));
		State.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
		return State;
	}

	// Close-and-repeat melee loop shared by every boss phase, so phases only have to
	// declare what they add on top of it.
	void AddBossCombatBody(UStateTreeState& Phase)
	{
		// The attack task itself fails when nothing in the profile is in range or off
		// cooldown, which is the signal to fall through to the chase state.
		AddAttackState(Phase, TEXT("Heavy Attack"), EEnemyAIAttackRole::CloseMelee);

		// Chase exits to the recheck wait rather than straight back to the phase: the task
		// succeeds the instant the pawn is in range, so a direct loop would spin every
		// frame while the heavy attack is on cooldown.
		UStateTreeState& ChaseTarget = AddChaseState(Phase, TEXT("Chase Target"));
		ChaseTarget.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextSelectableState);

		UStateTreeState& CombatRecheck = Phase.AddChildState(TEXT("Combat Recheck"));
		CombatRecheck.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		AddNamedTask<FStateTreeTask_EnemyPatrolWait>(CombatRecheck, TEXT("Combat Recheck Wait"));
		CombatRecheck.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Phase);
	}

	void RebuildBossStateTree(UStateTree& StateTree, TArray<FString>& ReportLines)
	{
		UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree.EditorData);
		if (!EditorData)
		{
			ReportLines.Add(TEXT("- Boss StateTree missing editor data; rebuild skipped."));
			return;
		}

		EditorData->Modify();
		EditorData->Evaluators.Reset();
		EditorData->GlobalTasks.Reset();
		EditorData->SubTrees.Reset();

		TStateTreeEditorNode<FStateTreeEvaluator_EnemyAwareness>& Awareness =
			EditorData->AddEvaluator<FStateTreeEvaluator_EnemyAwareness>();
		Awareness.SetNodeName(TEXT("Update Enemy Awareness"));

		TStateTreeEditorNode<FStateTreeEvaluator_EnemyCombatMove>& CombatMove =
			EditorData->AddEvaluator<FStateTreeEvaluator_EnemyCombatMove>();
		CombatMove.SetNodeName(TEXT("Update Enemy Combat Move"));

		UStateTreeState& Root = EditorData->AddRootState();
		Root.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
		Root.Description = TEXT("Heavy boss root: holds position until aware, alert approach, then a two-phase fight. Phase 1 is close-and-repeat heavy attack; phase 2 gates in at or below 50% HP and adds Skill and Special Movement attacks plus the phase effect. Distances live on the boss UEnemyData.");

		UStateTreeState& Dead = Root.AddChildState(TEXT("Dead"));
		Dead.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		Dead.AddEnterCondition<FStateTreeCondition_IsDead>().SetNodeName(TEXT("Enemy Is Dead"));
		AddNamedTask<FStateTreeTask_PlayDead>(Dead, TEXT("Play Dead"));

		UStateTreeState& Combat = Root.AddChildState(TEXT("Combat"));
		Combat.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
		TStateTreeEditorNode<FStateTreeCondition_EnemyAIState>& CombatCondition = Combat.AddEnterCondition<FStateTreeCondition_EnemyAIState>();
		CombatCondition.SetNodeName(TEXT("Enemy AI State Is Combat"));
		CombatCondition.GetInstanceData().RequiredState = EEnemyAIState::Combat;

		// Phase 2 is declared before phase 1 so the low-HP gate gets first refusal.
		UStateTreeState& Phase2 = AddBossPhaseState(Combat, TEXT("Phase 2"));
		TStateTreeEditorNode<FStateTreeCondition_SelfHealthPercentBelow>& Phase2Gate =
			Phase2.AddEnterCondition<FStateTreeCondition_SelfHealthPercentBelow>();
		Phase2Gate.SetNodeName(TEXT("Self Health % Below Phase 2"));
		Phase2Gate.GetInstanceData().Threshold = BossPhase2HealthPercent;

		// Kept as a leaf: the task succeeds on EnterState, and an immediately-completing
		// task on a container state would complete the container and unwind the branch.
		UStateTreeState& Phase2Enter = Phase2.AddChildState(TEXT("Phase 2 Enter"));
		Phase2Enter.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeTask_EnterBossPhase>& Phase2EnterTask =
			AddNamedTask<FStateTreeTask_EnterBossPhase>(Phase2Enter, TEXT("Enter Boss Phase 2"));
		if (const UBlueprint* PhaseEffectBlueprint = LoadAssetByPackagePath<UBlueprint>(BossPhase2EffectPath))
		{
			Phase2EnterTask.GetInstanceData().PhaseEffect = PhaseEffectBlueprint->GeneratedClass;
			ReportLines.Add(FString::Printf(TEXT("- Boss phase 2 effect bound to `%s`."), *BossPhase2EffectPath));
		}
		else
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Boss phase 2 effect `%s` not found; phase 2 changes moveset only. Author it as an Infinite, non-stacking GE."),
				*BossPhase2EffectPath));
		}
		Phase2Enter.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextSelectableState);

		// Extra roles unlocked by phase 2. Harmless before they are authored on DA_Boss:
		// the attack task fails and AddAttackState already falls through to the next state.
		AddAttackState(Phase2, TEXT("Phase 2 Skill"), EEnemyAIAttackRole::Skill);
		AddAttackState(Phase2, TEXT("Phase 2 Special Movement"), EEnemyAIAttackRole::SpecialMovement);
		AddBossCombatBody(Phase2);

		// No enter condition: phase 1 is the always-selectable fallback, so a boss at full
		// health starts here.
		UStateTreeState& Phase1 = AddBossPhaseState(Combat, TEXT("Phase 1"));
		AddBossCombatBody(Phase1);

		UStateTreeState& Alert = Root.AddChildState(TEXT("Alert"));
		Alert.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
		TStateTreeEditorNode<FStateTreeCondition_EnemyAIState>& AlertCondition = Alert.AddEnterCondition<FStateTreeCondition_EnemyAIState>();
		AlertCondition.SetNodeName(TEXT("Enemy AI State Is Alert"));
		AlertCondition.GetInstanceData().RequiredState = EEnemyAIState::Alert;

		UStateTreeState& AlertApproach = AddMoveState(Alert, TEXT("Alert Approach"), TEXT("LastKnownTargetLocation"), 100.f);
		AlertApproach.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextState);

		UStateTreeState& AlertWait = Alert.AddChildState(TEXT("Alert Wait"));
		AlertWait.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		AddNamedTask<FStateTreeTask_EnemyPatrolWait>(AlertWait, TEXT("Alert Wait"));
		AlertWait.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Alert);

		// A boss holds its arena instead of patrolling, but EEnemyAIState still defaults to
		// Patrol, so a Patrol-gated state has to exist or state selection finds no match at
		// all while the boss is unaware of the player.
		UStateTreeState& Idle = Root.AddChildState(TEXT("Idle"));
		Idle.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeCondition_EnemyAIState>& IdleCondition = Idle.AddEnterCondition<FStateTreeCondition_EnemyAIState>();
		IdleCondition.SetNodeName(TEXT("Enemy AI State Is Patrol"));
		IdleCondition.GetInstanceData().RequiredState = EEnemyAIState::Patrol;
		AddNamedTask<FStateTreeTask_EnemyPatrolWait>(Idle, TEXT("Idle Wait"));
		Idle.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Idle);

		EditorData->ReparentStates();
		EditorData->FixDuplicateIDs();
		EditorData->UpdateBindings();
		UStateTreeEditingSubsystem::MarkAsPubliclyModified(&StateTree);

		FStateTreeCompilerLog Log;
		const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(&StateTree, Log);
		if (!bCompiled)
		{
			Log.DumpToLog(&StateTree, LogTemp);
			ReportLines.Add(TEXT("- Boss StateTree compile failed; see editor log for details."));
		}
		else
		{
			ReportLines.Add(TEXT("- Rebuilt and compiled Boss StateTree template."));
		}

		StateTree.MarkPackageDirty();
	}

	void RebuildStateTree(UStateTree& StateTree, TArray<FString>& ReportLines)
	{
		UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree.EditorData);
		if (!EditorData)
		{
			ReportLines.Add(TEXT("- StateTree missing editor data; rebuild skipped."));
			return;
		}

		EditorData->Modify();
		EditorData->Evaluators.Reset();
		EditorData->GlobalTasks.Reset();
		EditorData->SubTrees.Reset();

		TStateTreeEditorNode<FStateTreeEvaluator_EnemyAwareness>& Awareness =
			EditorData->AddEvaluator<FStateTreeEvaluator_EnemyAwareness>();
		Awareness.SetNodeName(TEXT("Update Enemy Awareness"));

		TStateTreeEditorNode<FStateTreeEvaluator_EnemyCombatMove>& CombatMove =
			EditorData->AddEvaluator<FStateTreeEvaluator_EnemyCombatMove>();
		CombatMove.SetNodeName(TEXT("Update Enemy Combat Move"));

		UStateTreeState& Root = EditorData->AddRootState();
		Root.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
		Root.Description = TEXT("Generated enemy AI root. State order mirrors the default melee behavior tree.");

		UStateTreeState& Dead = Root.AddChildState(TEXT("Dead"));
		Dead.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		Dead.AddEnterCondition<FStateTreeCondition_IsDead>().SetNodeName(TEXT("Enemy Is Dead"));
		AddNamedTask<FStateTreeTask_PlayDead>(Dead, TEXT("Play Dead"));

		UStateTreeState& Combat = Root.AddChildState(TEXT("Combat"));
		Combat.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;
		TStateTreeEditorNode<FStateTreeCondition_EnemyAIState>& CombatCondition = Combat.AddEnterCondition<FStateTreeCondition_EnemyAIState>();
		CombatCondition.SetNodeName(TEXT("Enemy AI State Is Combat"));
		CombatCondition.GetInstanceData().RequiredState = EEnemyAIState::Combat;

		// Reposition is gated on the whiff flag the controller sets in
		// NotifyAttackResolved; without it this branch is attempted on every
		// combat re-selection, which reads as reposition spam.
		UStateTreeState& Reposition = AddAttackState(Combat, TEXT("Post Attack Reposition"), EEnemyAIAttackRole::Reposition);
		Reposition.AddEnterCondition<FStateTreeCondition_EnemyPostAttackReposition>()
			.SetNodeName(TEXT("Enemy Post Attack Reposition"));

		AddAttackState(Combat, TEXT("Skill"), EEnemyAIAttackRole::Skill);
		AddAttackState(Combat, TEXT("Special Movement"), EEnemyAIAttackRole::SpecialMovement);
		AddAttackState(Combat, TEXT("Close Melee"), EEnemyAIAttackRole::CloseMelee);

		// Every attack task fails when nothing in the profile is in range or off
		// cooldown, which is the signal to close distance. Chase exits to the recheck
		// wait rather than straight back to Combat: the task succeeds the instant the
		// pawn is in range, so a direct loop would spin every frame while attacks are
		// on cooldown.
		UStateTreeState& ChaseTarget = AddChaseState(Combat, TEXT("Chase Target"));
		ChaseTarget.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextSelectableState);

		UStateTreeState& CombatRecheck = Combat.AddChildState(TEXT("Combat Recheck"));
		CombatRecheck.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		AddNamedTask<FStateTreeTask_EnemyPatrolWait>(CombatRecheck, TEXT("Combat Recheck Wait"));
		CombatRecheck.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Combat);

		// Alert and Patrol run their wait and move tasks in parallel on a single
		// state: FStateTreeTask_MoveToControllerTarget stays Running until its path
		// request settles, so it coexists with the wait timer.
		UStateTreeState& Alert = Root.AddChildState(TEXT("Alert"));
		Alert.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeCondition_EnemyAIState>& AlertCondition = Alert.AddEnterCondition<FStateTreeCondition_EnemyAIState>();
		AlertCondition.SetNodeName(TEXT("Enemy AI State Is Alert"));
		AlertCondition.GetInstanceData().RequiredState = EEnemyAIState::Alert;
		AddNamedTask<FStateTreeTask_EnemyPatrolWait>(Alert, TEXT("Alert Wait"));
		TStateTreeEditorNode<FStateTreeTask_MoveToControllerTarget>& AlertMove =
			AddNamedTask<FStateTreeTask_MoveToControllerTarget>(Alert, TEXT("Alert Approach"));
		AlertMove.GetInstanceData().DestinationKey = TEXT("LastKnownTargetLocation");
		AlertMove.GetInstanceData().AcceptanceRadius = 100.f;
		Alert.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Alert);

		UStateTreeState& Patrol = Root.AddChildState(TEXT("Patrol"));
		Patrol.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeCondition_EnemyAIState>& PatrolCondition = Patrol.AddEnterCondition<FStateTreeCondition_EnemyAIState>();
		PatrolCondition.SetNodeName(TEXT("Enemy AI State Is Patrol"));
		PatrolCondition.GetInstanceData().RequiredState = EEnemyAIState::Patrol;
		AddNamedTask<FStateTreeTask_UpdateEnemyPatrolTarget>(Patrol, TEXT("Update Patrol Target"));
		AddNamedTask<FStateTreeTask_EnemyPatrolWait>(Patrol, TEXT("Patrol Wait"));
		TStateTreeEditorNode<FStateTreeTask_MoveToControllerTarget>& PatrolMove =
			AddNamedTask<FStateTreeTask_MoveToControllerTarget>(Patrol, TEXT("Patrol Move"));
		PatrolMove.GetInstanceData().DestinationKey = TEXT("PatrolTargetLocation");
		PatrolMove.GetInstanceData().AcceptanceRadius = 50.f;
		Patrol.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Patrol);

		// Wired after all three states exist so each can name the others as targets.
		AddStateGateTransition(Combat, Alert, EEnemyAIState::Alert, TEXT("Enemy AI State Is Alert"));
		AddStateGateTransition(Combat, Patrol, EEnemyAIState::Patrol, TEXT("Enemy AI State Is Patrol"));
		AddStateGateTransition(Alert, Combat, EEnemyAIState::Combat, TEXT("Enemy AI State Is Combat"));
		AddStateGateTransition(Alert, Patrol, EEnemyAIState::Patrol, TEXT("Enemy AI State Is Patrol"));
		AddStateGateTransition(Patrol, Combat, EEnemyAIState::Combat, TEXT("Enemy AI State Is Combat"));
		AddStateGateTransition(Patrol, Alert, EEnemyAIState::Alert, TEXT("Enemy AI State Is Alert"));

		EditorData->ReparentStates();
		EditorData->FixDuplicateIDs();
		EditorData->UpdateBindings();
		UStateTreeEditingSubsystem::MarkAsPubliclyModified(&StateTree);

		FStateTreeCompilerLog Log;
		const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(&StateTree, Log);
		if (!bCompiled)
		{
			Log.DumpToLog(&StateTree, LogTemp);
			ReportLines.Add(TEXT("- StateTree compile failed; see editor log for details."));
		}
		else
		{
			ReportLines.Add(TEXT("- Rebuilt and compiled StateTree template."));
		}

		StateTree.MarkPackageDirty();
	}

	void RebuildDebuggerStateTree(UStateTree& StateTree, TArray<FString>& ReportLines)
	{
		UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree.EditorData);
		if (!EditorData)
		{
			ReportLines.Add(TEXT("- ST_Debugger missing editor data; rebuild skipped."));
			return;
		}

		EditorData->Modify();
		EditorData->Evaluators.Reset();
		EditorData->GlobalTasks.Reset();
		EditorData->SubTrees.Reset();

		TStateTreeEditorNode<FStateTreeEvaluator_PlayerReference>& PlayerReference =
			EditorData->AddEvaluator<FStateTreeEvaluator_PlayerReference>();
		PlayerReference.SetNodeName(TEXT("Player Reference"));

		UStateTreeState& Root = EditorData->AddRootState();
		Root.Name = TEXT("Chase Debug Root");
		Root.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;

		UStateTreeState& InRange = Root.AddChildState(TEXT("In Range"));
		InRange.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeCondition_TargetWithin2DDistance>& InRangeCondition =
			InRange.AddEnterCondition<FStateTreeCondition_TargetWithin2DDistance>();
		InRangeCondition.GetInstanceData().Distance = 100.0f;

		UStateTreeState& Chase = Root.AddChildState(TEXT("Chase Player"));
		Chase.SelectionBehavior = EStateTreeStateSelectionBehavior::TryEnterState;
		TStateTreeEditorNode<FStateTreeTask_ChasePlayerUntilDistance>& ChaseTask =
			Chase.AddTask<FStateTreeTask_ChasePlayerUntilDistance>();
		ChaseTask.SetNodeName(TEXT("Chase Player Until < 100"));
		ChaseTask.GetInstanceData().StopDistance = 100.0f;
		ChaseTask.GetInstanceData().AcceptanceRadius = 35.0f;
		ChaseTask.GetInstanceData().RepathInterval = 0.2f;
		Chase.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &InRange);

		FStateTreeTransition& LeaveRange = InRange.AddTransition(
			EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &Chase);
		TStateTreeEditorNode<FStateTreeCondition_TargetBeyond2DDistance>& BeyondCondition =
			LeaveRange.AddConditionWithOuter<FStateTreeCondition_TargetBeyond2DDistance>(&InRange);
		BeyondCondition.GetInstanceData().Distance = 100.0f;

		EditorData->AddPropertyBinding(
			FPropertyBindingPath(PlayerReference.ID, GET_MEMBER_NAME_CHECKED(FStateTreeEvaluator_PlayerReferenceInstanceData, PlayerPawn)),
			FPropertyBindingPath(InRangeCondition.ID, GET_MEMBER_NAME_CHECKED(FStateTreeCondition_TargetWithin2DDistanceInstanceData, Target)));
		EditorData->AddPropertyBinding(
			FPropertyBindingPath(PlayerReference.ID, GET_MEMBER_NAME_CHECKED(FStateTreeEvaluator_PlayerReferenceInstanceData, PlayerPawn)),
			FPropertyBindingPath(BeyondCondition.ID, GET_MEMBER_NAME_CHECKED(FStateTreeCondition_TargetBeyond2DDistanceInstanceData, Target)));

		EditorData->ReparentStates();
		EditorData->FixDuplicateIDs();
		EditorData->UpdateBindings();
		UStateTreeEditingSubsystem::MarkAsPubliclyModified(&StateTree);
		FStateTreeCompilerLog Log;
		if (!UStateTreeEditingSubsystem::CompileStateTree(&StateTree, Log))
		{
			Log.DumpToLog(&StateTree, LogTemp);
			ReportLines.Add(TEXT("- ST_Debugger compile failed; see editor log."));
		}
		else
		{
			ReportLines.Add(TEXT("- Rebuilt ST_Debugger as standalone chase-until-100 test tree."));
		}
		StateTree.MarkPackageDirty();
	}

	void AssignAIAssetsToEnemyData(
		const FString& EnemyDataPath,
		UStateTree* StateTree,
		UBlackboardData* Blackboard,
		EDefaultEnemyProfile Profile,
		UAbilityData* AbilityData,
		UGASTemplate* GASTemplate,
		bool bCreateIfMissing,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const FString StateTreeLabel = StateTree ? StateTree->GetPathName() : TEXT("<none>");

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- %s `%s`.StateTree -> `%s`."),
				PackageExists(EnemyDataPath) ? TEXT("Would set") : bCreateIfMissing ? TEXT("Would create enemy DA and set") : TEXT("Missing enemy DA"),
				*EnemyDataPath,
				*StateTreeLabel));
			return;
		}

		const uint32 EnemyDataLoadFlags = LOAD_NoWarn | LOAD_NoVerify | LOAD_DisableDependencyPreloading | LOAD_DisableCompileOnLoad;
		UEnemyData* EnemyData = bCreateIfMissing
			? CreateOrLoadAsset<UEnemyData>(EnemyDataPath, bDryRun, ReportLines, DirtyPackages)
			: LoadAssetByPackagePath<UEnemyData>(EnemyDataPath, EnemyDataLoadFlags);
		if (!EnemyData)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing enemy DA `%s`."), *EnemyDataPath));
			return;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`.StateTree -> `%s`."),
			bDryRun ? TEXT("Would set") : TEXT("Set"),
			*EnemyDataPath,
			*StateTreeLabel));

		if (!StateTree)
		{
			return;
		}

		EnemyData->Modify();
		EnemyData->StateTree = StateTree;
		if (Blackboard)
		{
			EnemyData->StateTreeBlackboard = Blackboard;
		}
		if (AbilityData)
		{
			EnemyData->AbilityData = AbilityData;
		}
		if (GASTemplate)
		{
			EnemyData->GasTemplate = GASTemplate;
		}
		ConfigureDefaultEnemyData(*EnemyData, Profile);
		EnemyData->MarkPackageDirty();
		DirtyPackages.AddUnique(EnemyData->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Ensured `%s` default movement, awareness and attack profile."), *EnemyDataPath));
		if (bCreateIfMissing && !EnemyData->EnemyClass)
		{
			ReportLines.Add(FString::Printf(TEXT("- `%s`.EnemyClass is intentionally empty; assign the final BP class when the model/animation BP is ready."), *EnemyDataPath));
		}
	}
}

UEnemyAITemplateGeneratorCommandlet::UEnemyAITemplateGeneratorCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UEnemyAITemplateGeneratorCommandlet::Main(const FString& Params)
{
	using namespace EnemyAITemplateGenerator;

	const bool bDryRun = Params.Contains(TEXT("DryRun"), ESearchCase::IgnoreCase);
	const bool bPresetBoss = Params.Contains(TEXT("Preset=Boss"), ESearchCase::IgnoreCase);
	const bool bPresetDefaultMelee = Params.Contains(TEXT("Preset=DefaultMelee"), ESearchCase::IgnoreCase)
		|| !Params.Contains(TEXT("Preset="), ESearchCase::IgnoreCase);
	const bool bPresetDebugger = Params.Contains(TEXT("Preset=Debugger"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Enemy AI Template Generator Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Preset: %s"),
		bPresetBoss ? TEXT("Boss") : bPresetDebugger ? TEXT("Debugger") : bPresetDefaultMelee ? TEXT("DefaultMelee") : TEXT("Unsupported")));
	ReportLines.Add(TEXT(""));

	if (bPresetDebugger)
	{
		ReportLines.Add(TEXT("## ST_Debugger"));
		UStateTree* DebuggerStateTree = CreateOrLoadStateTree(DebuggerStateTreePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && DebuggerStateTree)
		{
			RebuildDebuggerStateTree(*DebuggerStateTree, ReportLines);
			DirtyPackages.AddUnique(DebuggerStateTree->GetPackage());
		}
	}
	else if (bPresetBoss)
	{
		ReportLines.Add(TEXT("## Blackboard"));
		UBlackboardData* Blackboard = CreateOrLoadAsset<UBlackboardData>(BlackboardPath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && Blackboard)
		{
			ConfigureBlackboard(*Blackboard);
			DirtyPackages.AddUnique(Blackboard->GetPackage());
			ReportLines.Add(TEXT("- Ensured shared enemy blackboard keys."));
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## StateTree"));
		ReportLines.Add(TEXT("- State order: Dead -> Combat (Phase 2 [HP <= 50%: Enter Phase -> Skill -> Special Movement -> Heavy Attack -> Chase -> Recheck] -> Phase 1 [Heavy Attack -> Chase -> Recheck]) -> Alert (Approach -> Wait) -> Idle (holds position)."));
		UStateTree* BossStateTree = CreateOrLoadStateTree(BossStateTreePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && BossStateTree)
		{
			RebuildBossStateTree(*BossStateTree, ReportLines);
			DirtyPackages.AddUnique(BossStateTree->GetPackage());
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Enemy Data"));
		AssignAIAssetsToEnemyData(BossDataPath, BossStateTree, Blackboard, EDefaultEnemyProfile::Boss, nullptr, nullptr, true, bDryRun, ReportLines, DirtyPackages);
	}
	else if (!bPresetDefaultMelee)
	{
		ReportLines.Add(TEXT("- Unsupported preset. Use `-Preset=DefaultMelee` or `-Preset=Boss`."));
	}
	else
	{
		ReportLines.Add(TEXT("## Blackboard"));
		UBlackboardData* Blackboard = CreateOrLoadAsset<UBlackboardData>(BlackboardPath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && Blackboard)
		{
			ConfigureBlackboard(*Blackboard);
			DirtyPackages.AddUnique(Blackboard->GetPackage());
			ReportLines.Add(TEXT("- Ensured default melee blackboard keys."));
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## StateTree"));
		ReportLines.Add(TEXT("- State order: Dead -> Combat (Post Attack Reposition -> Skill -> Special Movement -> Close Melee -> Chase Target -> Combat Recheck) -> Alert (wait + approach) -> Patrol (pick + wait + move)."));
		UStateTree* StateTree = CreateOrLoadStateTree(StateTreePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && StateTree)
		{
			RebuildStateTree(*StateTree, ReportLines);
			DirtyPackages.AddUnique(StateTree->GetPackage());
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## GAS Template"));
		UGASTemplate* EnemyGASTemplate = CreateOrLoadAsset<UGASTemplate>(EnemyGASTemplatePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && EnemyGASTemplate)
		{
			ConfigureSharedEnemyGASTemplate(*EnemyGASTemplate);
			EnemyGASTemplate->MarkPackageDirty();
			DirtyPackages.AddUnique(EnemyGASTemplate->GetPackage());
			ReportLines.Add(TEXT("- Ensured shared enemy GAS grants melee, ranged, and Skill1-4 attack slots."));
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Enemy Ability Data"));
		UEnemyAbilityMontageData* AlarmBellAbilityData = CreateOrLoadAsset<UEnemyAbilityMontageData>(AlarmBellJailerAbilityDataPath, bDryRun, ReportLines, DirtyPackages);
		UEnemyAbilityMontageData* GuardCaptainAbilityData = CreateOrLoadAsset<UEnemyAbilityMontageData>(GuardCaptainAbilityDataPath, bDryRun, ReportLines, DirtyPackages);

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Enemy Data"));
		AssignAIAssetsToEnemyData(RatDataPath, StateTree, Blackboard, EDefaultEnemyProfile::Rat, nullptr, nullptr, false, bDryRun, ReportLines, DirtyPackages);
		AssignAIAssetsToEnemyData(RottenGuardDataPath, StateTree, Blackboard, EDefaultEnemyProfile::RottenGuard, nullptr, nullptr, false, bDryRun, ReportLines, DirtyPackages);
		AssignAIAssetsToEnemyData(AlarmBellJailerDataPath, StateTree, Blackboard, EDefaultEnemyProfile::AlarmBellJailer, AlarmBellAbilityData, EnemyGASTemplate, true, bDryRun, ReportLines, DirtyPackages);
		AssignAIAssetsToEnemyData(GuardCaptainDataPath, StateTree, Blackboard, EDefaultEnemyProfile::GuardCaptain, GuardCaptainAbilityData, EnemyGASTemplate, true, bDryRun, ReportLines, DirtyPackages);
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(TEXT("EnemyAITemplateGeneratorReport.md"), ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Enemy AI template generator finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return (bPresetBoss || bPresetDebugger || bPresetDefaultMelee) ? 0 : 1;
}
