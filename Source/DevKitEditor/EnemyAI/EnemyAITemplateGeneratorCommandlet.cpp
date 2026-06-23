#include "DevKitEditor/EnemyAI/EnemyAITemplateGeneratorCommandlet.h"

#include "AI/BTDecorator_EnemyAIState.h"
#include "AI/BTDecorator_EnemyPostAttackReposition.h"
#include "AI/BTService_UpdateEnemyAwareness.h"
#include "AbilitySystem/Abilities/GA_EnemyMeleeAttacks.h"
#include "AbilitySystem/Abilities/GA_EnemyWeaponSkills.h"
#include "Commandlets/CommandletReportUtils.h"
#include "AI/BTService_UpdateEnemyCombatMove.h"
#include "AI/BTTask_EnemyCombatMove.h"
#include "AI/BTTask_EnemyAttackByProfile.h"
#include "AI/BTTask_EnemyPatrolWait.h"
#include "AI/BTTask_UpdateEnemyPatrolTarget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "BehaviorTreeGraphNode_Service.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "Character/EnemyCharacterBase.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "Data/GasTemplate.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_BehaviorTree.h"
#include "FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace EnemyAITemplateGenerator
{
	const FString BlackboardPath = TEXT("/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee");
	const FString BehaviorTreePath = TEXT("/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee");
	const FString EnemyGASTemplatePath = TEXT("/Game/Docs/Data/Enemy/DA_Enemy_GASTemplate");
	const FString RatDataPath = TEXT("/Game/Docs/Data/Enemy/Rat/DA_Rat");
	const FString RottenGuardDataPath = TEXT("/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard");
	const FString AlarmBellJailerDataPath = TEXT("/Game/Docs/Data/Enemy/AlarmBellJailer/DA_AlarmBellJailer");
	const FString AlarmBellJailerAbilityDataPath = TEXT("/Game/Docs/Data/Enemy/AlarmBellJailer/DA_AbilityMontage_AlarmBellJailer_01");
	const FString GuardCaptainDataPath = TEXT("/Game/Docs/Data/Enemy/GuardCaptain/DA_GuardCaptain");
	const FString GuardCaptainAbilityDataPath = TEXT("/Game/Docs/Data/Enemy/GuardCaptain/DA_AbilityMontage_GuardCaptain_01");

	enum class EDefaultEnemyProfile : uint8
	{
		Rat,
		RottenGuard,
		AlarmBellJailer,
		GuardCaptain,
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
		}

		SyncAttackProfileFromAbilityData(EnemyData);
	}

	void SetMoveToBlackboardKey(UBTTask_MoveTo& MoveToTask, FName KeyName)
	{
		FStructProperty* BlackboardKeyProperty = FindFProperty<FStructProperty>(UBTTask_BlackboardBase::StaticClass(), TEXT("BlackboardKey"));
		if (!BlackboardKeyProperty)
		{
			return;
		}

		FBlackboardKeySelector* Selector = BlackboardKeyProperty->ContainerPtrToValuePtr<FBlackboardKeySelector>(&MoveToTask);
		if (Selector)
		{
			Selector->SelectedKeyName = KeyName;
		}
	}

	template <typename GraphNodeType, typename NodeInstanceType>
	GraphNodeType* AddNode(UBehaviorTree& BehaviorTree, UBehaviorTreeGraph& Graph, int32 X, int32 Y)
	{
		GraphNodeType* GraphNode = NewObject<GraphNodeType>(&Graph);
		GraphNode->NodeInstance = NewObject<NodeInstanceType>(&BehaviorTree);
		GraphNode->SetFlags(RF_Transactional);
		GraphNode->NodeInstance->SetFlags(RF_Transactional);
		GraphNode->NodePosX = X;
		GraphNode->NodePosY = Y;
		GraphNode->CreateNewGuid();
		GraphNode->UpdateNodeClassData();
		GraphNode->PostPlacedNewNode();
		GraphNode->AllocateDefaultPins();
		Graph.AddNode(GraphNode, false, false);
		return GraphNode;
	}

	void Connect(UBehaviorTreeGraphNode* Parent, UBehaviorTreeGraphNode* Child)
	{
		if (!Parent || !Child || !Parent->GetOutputPin() || !Child->GetInputPin())
		{
			return;
		}

		Parent->GetOutputPin()->MakeLinkTo(Child->GetInputPin());
	}

	template <typename ServiceType>
	void AddService(UBehaviorTree& BehaviorTree, UBehaviorTreeGraph& Graph, UBehaviorTreeGraphNode& Parent)
	{
		UBehaviorTreeGraphNode_Service* ServiceNode = NewObject<UBehaviorTreeGraphNode_Service>(&Graph);
		ServiceNode->NodeInstance = NewObject<ServiceType>(&BehaviorTree);
		ServiceNode->SetFlags(RF_Transactional);
		ServiceNode->NodeInstance->SetFlags(RF_Transactional);
		ServiceNode->CreateNewGuid();
		ServiceNode->UpdateNodeClassData();
		Parent.AddSubNode(ServiceNode, &Graph);
	}

	void AddStateDecorator(UBehaviorTree& BehaviorTree, UBehaviorTreeGraph& Graph, UBehaviorTreeGraphNode& DecoratedNode, EEnemyAIState State)
	{
		UBehaviorTreeGraphNode_Decorator* DecoratorNode = NewObject<UBehaviorTreeGraphNode_Decorator>(&Graph);
		UBTDecorator_EnemyAIState* Decorator = NewObject<UBTDecorator_EnemyAIState>(&BehaviorTree);
		Decorator->SetRequiredState(State);
		DecoratorNode->NodeInstance = Decorator;
		DecoratorNode->SetFlags(RF_Transactional);
		Decorator->SetFlags(RF_Transactional);
		DecoratorNode->CreateNewGuid();
		DecoratorNode->UpdateNodeClassData();
		DecoratedNode.AddSubNode(DecoratorNode, &Graph);
	}

	void AddPostAttackRepositionDecorator(UBehaviorTree& BehaviorTree, UBehaviorTreeGraph& Graph, UBehaviorTreeGraphNode& DecoratedNode)
	{
		UBehaviorTreeGraphNode_Decorator* DecoratorNode = NewObject<UBehaviorTreeGraphNode_Decorator>(&Graph);
		UBTDecorator_EnemyPostAttackReposition* Decorator = NewObject<UBTDecorator_EnemyPostAttackReposition>(&BehaviorTree);
		DecoratorNode->NodeInstance = Decorator;
		DecoratorNode->SetFlags(RF_Transactional);
		Decorator->SetFlags(RF_Transactional);
		DecoratorNode->CreateNewGuid();
		DecoratorNode->UpdateNodeClassData();
		DecoratedNode.AddSubNode(DecoratorNode, &Graph);
	}

	UBehaviorTreeGraphNode_Root* FindOrCreateRootNode(UBehaviorTreeGraph& Graph, UBlackboardData& Blackboard)
	{
		for (UEdGraphNode* Node : Graph.Nodes)
		{
			if (UBehaviorTreeGraphNode_Root* RootNode = Cast<UBehaviorTreeGraphNode_Root>(Node))
			{
				RootNode->BlackboardAsset = &Blackboard;
				RootNode->UpdateBlackboard();
				return RootNode;
			}
		}

		UBehaviorTreeGraphNode_Root* RootNode = NewObject<UBehaviorTreeGraphNode_Root>(&Graph);
		RootNode->BlackboardAsset = &Blackboard;
		RootNode->NodePosX = 0;
		RootNode->NodePosY = 0;
		RootNode->CreateNewGuid();
		RootNode->PostPlacedNewNode();
		RootNode->AllocateDefaultPins();
		RootNode->BlackboardAsset = &Blackboard;
		RootNode->UpdateBlackboard();
		Graph.AddNode(RootNode, false, false);
		return RootNode;
	}

	void RebuildBehaviorTreeGraph(UBehaviorTree& BehaviorTree, UBlackboardData& Blackboard)
	{
		BehaviorTree.BlackboardAsset = &Blackboard;

		UBehaviorTreeGraph* Graph = Cast<UBehaviorTreeGraph>(BehaviorTree.BTGraph);
		if (!Graph)
		{
			Graph = NewObject<UBehaviorTreeGraph>(&BehaviorTree, TEXT("BTGraph"), RF_Transactional);
			BehaviorTree.BTGraph = Graph;
		}

		Graph->Modify();
		Graph->Nodes.Reset();

		if (const UEdGraphSchema* Schema = Graph->GetSchema())
		{
			Schema->CreateDefaultNodesForGraph(*Graph);
		}

		UBehaviorTreeGraphNode_Root* RootNode = FindOrCreateRootNode(*Graph, Blackboard);
		RootNode->BlackboardAsset = &Blackboard;
		RootNode->UpdateBlackboard();

		UBehaviorTreeGraphNode_Composite* MainSelector = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Selector>(BehaviorTree, *Graph, 0, 160);
		AddService<UBTService_UpdateEnemyAwareness>(BehaviorTree, *Graph, *MainSelector);
		AddService<UBTService_UpdateEnemyCombatMove>(BehaviorTree, *Graph, *MainSelector);
		Connect(RootNode, MainSelector);

		UBehaviorTreeGraphNode_Composite* CombatSelector = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Selector>(BehaviorTree, *Graph, -450, 360);
		AddStateDecorator(BehaviorTree, *Graph, *CombatSelector, EEnemyAIState::Combat);
		Connect(MainSelector, CombatSelector);

		UBehaviorTreeGraphNode_Task* RepositionTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -1050, 560);
		if (UBTTask_EnemyAttackByProfile* AttackTask = Cast<UBTTask_EnemyAttackByProfile>(RepositionTask->NodeInstance))
		{
			AttackTask->SetRequiredAttackRole(EEnemyAIAttackRole::Reposition);
		}
		AddPostAttackRepositionDecorator(BehaviorTree, *Graph, *RepositionTask);
		Connect(CombatSelector, RepositionTask);

		UBehaviorTreeGraphNode_Task* SkillTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -850, 560);
		if (UBTTask_EnemyAttackByProfile* AttackTask = Cast<UBTTask_EnemyAttackByProfile>(SkillTask->NodeInstance))
		{
			AttackTask->SetRequiredAttackRole(EEnemyAIAttackRole::Skill);
		}
		Connect(CombatSelector, SkillTask);

		UBehaviorTreeGraphNode_Task* SpecialMovementTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -650, 560);
		if (UBTTask_EnemyAttackByProfile* AttackTask = Cast<UBTTask_EnemyAttackByProfile>(SpecialMovementTask->NodeInstance))
		{
			AttackTask->SetRequiredAttackRole(EEnemyAIAttackRole::SpecialMovement);
		}
		Connect(CombatSelector, SpecialMovementTask);

		UBehaviorTreeGraphNode_Task* CloseMeleeTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -450, 560);
		if (UBTTask_EnemyAttackByProfile* AttackTask = Cast<UBTTask_EnemyAttackByProfile>(CloseMeleeTask->NodeInstance))
		{
			AttackTask->SetRequiredAttackRole(EEnemyAIAttackRole::CloseMelee);
		}
		Connect(CombatSelector, CloseMeleeTask);

		UBehaviorTreeGraphNode_Task* CombatMoveTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyCombatMove>(BehaviorTree, *Graph, -250, 560);
		Connect(CombatSelector, CombatMoveTask);

		UBehaviorTreeGraphNode_Composite* AlertSequence = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Sequence>(BehaviorTree, *Graph, 0, 360);
		AddStateDecorator(BehaviorTree, *Graph, *AlertSequence, EEnemyAIState::Alert);
		Connect(MainSelector, AlertSequence);

		UBehaviorTreeGraphNode_Task* AlertMoveTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_MoveTo>(BehaviorTree, *Graph, -80, 560);
		if (UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(AlertMoveTask->NodeInstance))
		{
			SetMoveToBlackboardKey(*MoveTo, TEXT("LastKnownTargetLocation"));
			MoveTo->AcceptableRadius = 100.0f;
			MoveTo->bObserveBlackboardValue = true;
			MoveTo->bAllowPartialPath = true;
			MoveTo->bProjectGoalLocation = true;
		}
		Connect(AlertSequence, AlertMoveTask);

		UBehaviorTreeGraphNode_Task* AlertWaitTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_Wait>(BehaviorTree, *Graph, 120, 560);
		if (UBTTask_Wait* Wait = Cast<UBTTask_Wait>(AlertWaitTask->NodeInstance))
		{
			Wait->WaitTime = 0.3f;
			Wait->RandomDeviation = 0.15f;
		}
		Connect(AlertSequence, AlertWaitTask);

		UBehaviorTreeGraphNode_Composite* PatrolSequence = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Sequence>(BehaviorTree, *Graph, 450, 360);
		AddStateDecorator(BehaviorTree, *Graph, *PatrolSequence, EEnemyAIState::Patrol);
		Connect(MainSelector, PatrolSequence);

		UBehaviorTreeGraphNode_Task* PatrolTargetTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_UpdateEnemyPatrolTarget>(BehaviorTree, *Graph, 250, 560);
		Connect(PatrolSequence, PatrolTargetTask);

		UBehaviorTreeGraphNode_Task* PatrolMoveTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_MoveTo>(BehaviorTree, *Graph, 450, 560);
		if (UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(PatrolMoveTask->NodeInstance))
		{
			SetMoveToBlackboardKey(*MoveTo, TEXT("PatrolTargetLocation"));
			MoveTo->AcceptableRadius = 120.0f;
			MoveTo->bAllowPartialPath = true;
			MoveTo->bProjectGoalLocation = true;
		}
		Connect(PatrolSequence, PatrolMoveTask);

		UBehaviorTreeGraphNode_Task* PatrolWaitTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyPatrolWait>(BehaviorTree, *Graph, 650, 560);
		Connect(PatrolSequence, PatrolWaitTask);

		Graph->UpdateAsset();
		BehaviorTree.MarkPackageDirty();
	}

	void AssignBehaviorTreeToEnemyData(
		const FString& EnemyDataPath,
		UBehaviorTree* BehaviorTree,
		EDefaultEnemyProfile Profile,
		UAbilityData* AbilityData,
		UGASTemplate* GASTemplate,
		bool bCreateIfMissing,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- %s `%s`.BehaviorTree -> `%s`."),
				PackageExists(EnemyDataPath) ? TEXT("Would set") : bCreateIfMissing ? TEXT("Would create enemy DA and set") : TEXT("Missing enemy DA"),
				*EnemyDataPath,
				*BehaviorTreePath));
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

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`.BehaviorTree -> `%s`."),
			bDryRun ? TEXT("Would set") : TEXT("Set"),
			*EnemyDataPath,
			*BehaviorTreePath));

		if (!BehaviorTree)
		{
			return;
		}

		EnemyData->Modify();
		EnemyData->BehaviorTree = BehaviorTree;
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
	const bool bPresetDefaultMelee = Params.Contains(TEXT("Preset=DefaultMelee"), ESearchCase::IgnoreCase)
		|| !Params.Contains(TEXT("Preset="), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Enemy AI Template Generator Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Preset: %s"), bPresetDefaultMelee ? TEXT("DefaultMelee") : TEXT("Unsupported")));
	ReportLines.Add(TEXT(""));

	if (!bPresetDefaultMelee)
	{
		ReportLines.Add(TEXT("- Unsupported preset. Use `-Preset=DefaultMelee`."));
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
		ReportLines.Add(TEXT("## Behavior Tree"));
		ReportLines.Add(TEXT("- Combat branch order: PostAttackReposition -> Skill -> SpecialMovement -> CloseMelee -> MoveToCombatSlot."));
		UBehaviorTree* BehaviorTree = CreateOrLoadAsset<UBehaviorTree>(BehaviorTreePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && BehaviorTree && Blackboard)
		{
			RebuildBehaviorTreeGraph(*BehaviorTree, *Blackboard);
			DirtyPackages.AddUnique(BehaviorTree->GetPackage());
			ReportLines.Add(TEXT("- Rebuilt visual graph and runtime tree."));
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
		AssignBehaviorTreeToEnemyData(RatDataPath, BehaviorTree, EDefaultEnemyProfile::Rat, nullptr, nullptr, false, bDryRun, ReportLines, DirtyPackages);
		AssignBehaviorTreeToEnemyData(RottenGuardDataPath, BehaviorTree, EDefaultEnemyProfile::RottenGuard, nullptr, nullptr, false, bDryRun, ReportLines, DirtyPackages);
		AssignBehaviorTreeToEnemyData(AlarmBellJailerDataPath, BehaviorTree, EDefaultEnemyProfile::AlarmBellJailer, AlarmBellAbilityData, EnemyGASTemplate, true, bDryRun, ReportLines, DirtyPackages);
		AssignBehaviorTreeToEnemyData(GuardCaptainDataPath, BehaviorTree, EDefaultEnemyProfile::GuardCaptain, GuardCaptainAbilityData, EnemyGASTemplate, true, bDryRun, ReportLines, DirtyPackages);
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(TEXT("EnemyAITemplateGeneratorReport.md"), ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Enemy AI template generator finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return bPresetDefaultMelee ? 0 : 1;
}
