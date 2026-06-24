#include "GameplayTags/GameplayTagAssetMigrationCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "FileHelpers.h"
#include "GameplayTagContainer.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace GameplayTagAssetMigration
{
	const TCHAR* ReportFileName = TEXT("GameplayTagAssetMigrationReport.md");

	struct FTagMigration
	{
		FName OldTag;
		FName NewTag;
		bool bReportOnly = false;
		FString Reason;
	};

	struct FTagMigrationContext
	{
		bool bApply = false;
		bool bIncludeDeprecated = false;
		TArray<FString> ReportLines;
		TArray<UPackage*> DirtyPackages;
		int32 AssetsVisited = 0;
		int32 AssetsWithHits = 0;
		int32 TagsWouldChange = 0;
		int32 TagsChanged = 0;
		int32 ReportOnlyHits = 0;
		int32 SkippedWorldAssets = 0;
		int32 ReadOnlyContainerDepth = 0;
		FString ReadOnlyContainerReason;
	};

	bool IsDeprecatedPackage(const FName PackageName)
	{
		return PackageName.ToString().Contains(TEXT("_Deprecated"));
	}

	void AddExact(TArray<FTagMigration>& Migrations, const TCHAR* OldTag, const TCHAR* NewTag)
	{
		FTagMigration Migration;
		Migration.OldTag = FName(OldTag);
		Migration.NewTag = FName(NewTag);
		Migrations.Add(Migration);
	}

	void AddReportOnly(TArray<FTagMigration>& Migrations, const TCHAR* OldTag, const TCHAR* Reason)
	{
		FTagMigration Migration;
		Migration.OldTag = FName(OldTag);
		Migration.bReportOnly = true;
		Migration.Reason = Reason;
		Migrations.Add(Migration);
	}

	FString BuffLeafFromLegacyLeaf(const FString& Leaf, const bool bIdentityTag)
	{
		if (Leaf == TEXT("Buff.AttackUp") || Leaf == TEXT("AttackUp"))
		{
			return TEXT("AttackUp");
		}
		if (Leaf == TEXT("Defense.ReduceDamage") || Leaf == TEXT("ReduceDamage"))
		{
			return TEXT("ReduceDamage");
		}
		if (Leaf == TEXT("Burn"))
		{
			return TEXT("Fire");
		}
		if (Leaf == TEXT("Burning"))
		{
			return TEXT("Fire");
		}
		if (Leaf == TEXT("Poisoned"))
		{
			return TEXT("Poison");
		}
		if (Leaf == TEXT("Bleeding"))
		{
			return TEXT("Bleed");
		}
		if (Leaf == TEXT("Frozen"))
		{
			return TEXT("Freeze");
		}
		if (Leaf == TEXT("Stunned"))
		{
			return TEXT("Stun");
		}
		if (Leaf == TEXT("Rended"))
		{
			return TEXT("Rend");
		}
		if (Leaf == TEXT("Wounded"))
		{
			return TEXT("Wound");
		}
		if (Leaf == TEXT("Feared"))
		{
			return TEXT("Fear");
		}
		if (Leaf == TEXT("Cursed"))
		{
			return TEXT("Curse");
		}
		if (Leaf == TEXT("Shielded"))
		{
			return TEXT("Shield");
		}
		if (Leaf == TEXT("Heavy"))
		{
			return bIdentityTag ? TEXT("WeaponSkillFinisher") : TEXT("Detonate");
		}
		return Leaf;
	}

	bool TryBuildBuffMigrationTarget(const FString& OldTagString, FString& OutNewTagString)
	{
		struct FLegacyBuffPrefix
		{
			const TCHAR* Prefix;
			bool bIdentityTag;
		};

		static const FLegacyBuffPrefix Prefixes[] =
		{
			{ TEXT("Card.ID."), true },
			{ TEXT("Rune.ID."), true },
			{ TEXT("Card.Effect."), false },
			{ TEXT("Rune.Effect."), false },
		};

		for (const FLegacyBuffPrefix& Prefix : Prefixes)
		{
			if (OldTagString.StartsWith(Prefix.Prefix))
			{
				const FString Leaf = OldTagString.RightChop(FCString::Strlen(Prefix.Prefix));
				OutNewTagString = FString(TEXT("Buff.")) + BuffLeafFromLegacyLeaf(Leaf, Prefix.bIdentityTag);
				return true;
			}
		}

		if (OldTagString.StartsWith(TEXT("Buff.Status.")))
		{
			const FString Leaf = OldTagString.RightChop(FCString::Strlen(TEXT("Buff.Status.")));
			OutNewTagString = FString(TEXT("Buff.")) + BuffLeafFromLegacyLeaf(Leaf, false);
			return true;
		}

		return false;
	}

	const TArray<FTagMigration>& ExactMigrations()
	{
		static TArray<FTagMigration> Migrations;
		if (Migrations.Num() == 0)
		{
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Attack"), TEXT("Character.State.Skill.Attack"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Attack.Combo1"), TEXT("Character.State.Skill.Attack.Combo1"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Attack.Combo2"), TEXT("Character.State.Skill.Attack.Combo2"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Attack.Combo3"), TEXT("Character.State.Skill.Attack.Combo3"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Attack.Combo4"), TEXT("Character.State.Skill.Attack.Combo4"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.LightAtk.Combo1"), TEXT("Character.State.Skill.Attack.Combo1"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.LightAtk.Combo2"), TEXT("Character.State.Skill.Attack.Combo2"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.LightAtk.Combo3"), TEXT("Character.State.Skill.Attack.Combo3"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.LightAtk.Combo4"), TEXT("Character.State.Skill.Attack.Combo4"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.WeaponSkill"), TEXT("Character.State.Skill.WeaponSkill"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"), TEXT("Character.State.Skill.WeaponSkill.Combo1"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"), TEXT("Character.State.Skill.WeaponSkill.Combo2"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.WeaponSkill.Combo3"), TEXT("Character.State.Skill.WeaponSkill.Combo3"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.WeaponSkill.Combo4"), TEXT("Character.State.Skill.WeaponSkill.Combo4"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1"), TEXT("Character.State.Skill.WeaponSkill.Combo1"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"), TEXT("Character.State.Skill.WeaponSkill.Combo2"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"), TEXT("Character.State.Skill.WeaponSkill.Combo3"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"), TEXT("Character.State.Skill.WeaponSkill.Combo4"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Skill"), TEXT("Character.State.Skill.Active"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Skill.Skill1"), TEXT("Character.State.Skill.Active.Skill1"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Skill.Skill2"), TEXT("Character.State.Skill.Active.Skill2"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Dash"), TEXT("Character.State.Movement.Dash"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Dash.Combo1"), TEXT("Character.State.Movement.Dash.Combo1"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Dash.Combo2"), TEXT("Character.State.Movement.Dash.Combo2"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Dash.Combo3"), TEXT("Character.State.Movement.Dash.Combo3"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Dash.Combo4"), TEXT("Character.State.Movement.Dash.Combo4"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Dash.Dash1"), TEXT("Character.State.Movement.Dash"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Dash.DashATK1"), TEXT("Character.State.Movement.Dash"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.DashAtk"), TEXT("Character.State.Movement.Dash"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.Reload"), TEXT("Character.State.Skill.Reload"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.SwitchWeapon"), TEXT("Character.State.Equipment.SwitchWeapon"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.CanCombo"), TEXT("Character.State.Window.CanCombo"));
			AddExact(Migrations, TEXT("PlayerState.AbilityCast.PostAttackRecovery"), TEXT("Character.State.Window.PostAttackRecovery"));
			AddExact(Migrations, TEXT("PlayerState.Block.Idle"), TEXT("Character.State.Block.Idle"));
			AddExact(Migrations, TEXT("PlayerState.Block.Start"), TEXT("Character.State.Block.Start"));
			AddExact(Migrations, TEXT("Character.State.Feared"), TEXT("Buff.Fear"));
			AddExact(Migrations, TEXT("Character.State.Frozen"), TEXT("Buff.Freeze"));
			AddExact(Migrations, TEXT("Character.State.Stunned"), TEXT("Buff.Stun"));
			AddExact(Migrations, TEXT("Character.State.SuperArmor"), TEXT("Buff.SuperArmor"));

			AddExact(Migrations, TEXT("Tutorial.Hint.HeavyCard"), TEXT("Tutorial.Hint.WeaponSkillFinisher"));
			AddExact(Migrations, TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.heavy_card_obtained"), TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.weapon_skill_finisher_obtained"));
			AddExact(Migrations, TEXT("Card.ID.Heavy"), TEXT("Buff.WeaponSkillFinisher"));
			AddExact(Migrations, TEXT("Card.Effect.Heavy"), TEXT("Buff.Detonate"));
			AddExact(Migrations, TEXT("Rune.ID.Heavy"), TEXT("Buff.WeaponSkillFinisher"));
			AddExact(Migrations, TEXT("Rune.Effect.Heavy"), TEXT("Buff.Detonate"));
			AddExact(Migrations, TEXT("Action.Rune.KnockbackApplied"), TEXT("Buff.Event.KnockbackApplied"));
			AddExact(Migrations, TEXT("Action.Rune.SlashWaveHit"), TEXT("Buff.Event.SlashWaveHit"));
			AddExact(Migrations, TEXT("Action.Rune.MoonlightBurnHit"), TEXT("Buff.Event.Moonlight.BurnHit"));
			AddExact(Migrations, TEXT("Action.Rune.MoonlightPoisonHit"), TEXT("Buff.Event.Moonlight.PoisonHit"));
			AddExact(Migrations, TEXT("Action.Rune.MoonlightPoisonExpired"), TEXT("Buff.Event.Moonlight.PoisonExpired"));
			AddExact(Migrations, TEXT("Action.Rune.MoonlightShield"), TEXT("Buff.Event.Moonlight.Shield"));
			AddExact(Migrations, TEXT("Action.Rune.MoonlightSlash"), TEXT("Buff.Event.Moonlight.Slash"));
			AddExact(Migrations, TEXT("Action.Rune.MoonlightSplit"), TEXT("Buff.Event.Moonlight.Split"));
			AddExact(Migrations, TEXT("Action.Rune.ShadowMarkDetonate"), TEXT("Buff.Event.ShadowMarkDetonate"));
			AddExact(Migrations, TEXT("Event.Rune.KnockbackApplied"), TEXT("Buff.Event.KnockbackApplied"));
			AddExact(Migrations, TEXT("Event.Rune.BleedApplied"), TEXT("Buff.Event.Bleed"));
			AddExact(Migrations, TEXT("Event.Rune.KillConfirmed"), TEXT("Buff.Event.KillConfirmed"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.AtkUp"), TEXT("GameplayCue.Buff.AttackUp"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.Burn"), TEXT("GameplayCue.Buff.Fire"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.Burn.Vfx"), TEXT("GameplayCue.Buff.Fire.Vfx"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.Cursed"), TEXT("GameplayCue.Buff.Curse"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.DeathPoison"), TEXT("GameplayCue.Buff.DeathPoison"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.Fearless"), TEXT("GameplayCue.Buff.Fearless"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.FinisherCharge"), TEXT("GameplayCue.Buff.FinisherCharge"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.KillExplosion"), TEXT("GameplayCue.Buff.KillExplosion"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.MoonlightSlash.Fire"), TEXT("GameplayCue.Buff.MoonlightSlash.Fire"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.MoonlightSlash.Hit"), TEXT("GameplayCue.Buff.MoonlightSlash.Hit"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.MoonlightSlash.Pierce"), TEXT("GameplayCue.Buff.MoonlightSlash.Pierce"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.MoonlightSlash.Split"), TEXT("GameplayCue.Buff.MoonlightSlash.Split"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.Poison.Vfx"), TEXT("GameplayCue.Buff.Poison.Vfx"));
			AddExact(Migrations, TEXT("GameplayCue.Rune.Shield"), TEXT("GameplayCue.Buff.Shield"));

			AddReportOnly(Migrations, TEXT("PlayerState.AbilityCast.Special"), TEXT("Legacy special system; confirm whether the asset now belongs to Character.State.Skill.Active."));
			AddReportOnly(Migrations, TEXT("PlayerState.AbilityCast.SpecialAttack"), TEXT("Deprecated SpecialAttack path; do not auto-enable in the active Skill path."));
			AddReportOnly(Migrations, TEXT("PlayerState.AbilityCast.Finisher"), TEXT("Deprecated QTE finisher path; keep disabled unless explicitly restored."));
			AddReportOnly(Migrations, TEXT("PlayerState.AbilityCast.FinisherCharge"), TEXT("Deprecated QTE finisher path; keep disabled unless explicitly restored."));
			AddReportOnly(Migrations, TEXT("PlayerState.AbilityCast"), TEXT("Legacy action-state root with no single formal leaf target. Replace by asset intent or remove if it was only a category tag."));
			AddReportOnly(Migrations, TEXT("Combo.CombatDeck.ActionSlot.Attack"), TEXT("Action slot should be represented by CombatDeck slot/DA fields, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.CombatDeck.ActionSlot.Skill"), TEXT("Action slot should be represented by CombatDeck slot/DA fields, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.CombatDeck.ActionSlot.WeaponSkill"), TEXT("Action slot should be represented by CombatDeck slot/DA fields, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.CombatDeck.ActionSlot.Dash"), TEXT("Action slot should be represented by CombatDeck slot/DA fields, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.CombatDeck.FlowRole.Starter"), TEXT("Flow role should be represented by the card DA enum, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.CombatDeck.FlowRole.Catalyst"), TEXT("Flow role should be represented by the card DA enum, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.CombatDeck.FlowRole.Finisher"), TEXT("Flow role should be represented by the card DA enum, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.TriggerTiming.OnCommit"), TEXT("Trigger timing should be represented by the card DA trigger enum, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Combo.TriggerTiming.OnHit"), TEXT("Trigger timing should be represented by the card DA trigger enum, not a formal GameplayTag."));
			AddReportOnly(Migrations, TEXT("Card.ID.Finisher"), TEXT("Deprecated QTE finisher card. Keep disabled as compatibility data instead of auto-migrating it to the formal Buff runtime path."));
			AddReportOnly(Migrations, TEXT("Card.Effect.Finisher"), TEXT("Deprecated QTE finisher effect. Keep disabled as compatibility data instead of auto-migrating it to the formal Buff runtime path."));
			AddReportOnly(Migrations, TEXT("Rune.ID.Finisher"), TEXT("Deprecated QTE finisher rune id. Keep disabled as compatibility data instead of auto-migrating it to the formal Buff runtime path."));
			AddReportOnly(Migrations, TEXT("Rune.Effect.Finisher"), TEXT("Deprecated QTE finisher rune effect. Keep disabled as compatibility data instead of auto-migrating it to the formal Buff runtime path."));
		}
		return Migrations;
	}

	bool StartsWithTagPath(const FString& TagString, const TCHAR* Prefix)
	{
		return TagString == Prefix || TagString.StartsWith(FString(Prefix) + TEXT("."));
	}

	bool ResolveMigration(const FName OldTagName, FTagMigration& OutMigration)
	{
		const FString OldTagString = OldTagName.ToString();
		for (const FTagMigration& Migration : ExactMigrations())
		{
			if (Migration.OldTag == OldTagName)
			{
				OutMigration = Migration;
				return true;
			}
		}

		FString BuffMigrationTarget;
		if (TryBuildBuffMigrationTarget(OldTagString, BuffMigrationTarget))
		{
			OutMigration.OldTag = OldTagName;
			OutMigration.NewTag = FName(*BuffMigrationTarget);
			return true;
		}

		if (StartsWithTagPath(OldTagString, TEXT("Action.Rune")))
		{
			const FString Leaf = OldTagString == TEXT("Action.Rune")
				? FString()
				: OldTagString.RightChop(FCString::Strlen(TEXT("Action.Rune.")));
			OutMigration.OldTag = OldTagName;
			OutMigration.NewTag = Leaf.IsEmpty() ? FName(TEXT("Buff.Event")) : FName(*(FString(TEXT("Buff.Event.")) + Leaf));
			return true;
		}

		if (StartsWithTagPath(OldTagString, TEXT("Event.Rune")))
		{
			const FString Leaf = OldTagString == TEXT("Event.Rune")
				? FString()
				: OldTagString.RightChop(FCString::Strlen(TEXT("Event.Rune.")));
			OutMigration.OldTag = OldTagName;
			OutMigration.NewTag = Leaf.IsEmpty() ? FName(TEXT("Buff.Event")) : FName(*(FString(TEXT("Buff.Event.")) + Leaf));
			return true;
		}

		if (StartsWithTagPath(OldTagString, TEXT("GameplayCue.Rune")))
		{
			const FString Leaf = OldTagString == TEXT("GameplayCue.Rune")
				? FString()
				: OldTagString.RightChop(FCString::Strlen(TEXT("GameplayCue.Rune.")));
			OutMigration.OldTag = OldTagName;
			OutMigration.NewTag = Leaf.IsEmpty() ? FName(TEXT("GameplayCue.Buff")) : FName(*(FString(TEXT("GameplayCue.Buff.")) + Leaf));
			return true;
		}

		if (StartsWithTagPath(OldTagString, TEXT("Rune.Card")))
		{
			OutMigration.OldTag = OldTagName;
			OutMigration.bReportOnly = true;
			OutMigration.Reason = TEXT("Rune.Card.* is deprecated. Use Buff.* only when this is a runtime/link effect; otherwise move identity/binding/role data to the DA fields.");
			return true;
		}

		if (StartsWithTagPath(OldTagString, TEXT("Rune.Binding"))
			|| StartsWithTagPath(OldTagString, TEXT("Rune.FlowRole"))
			|| StartsWithTagPath(OldTagString, TEXT("Rune.Trigger"))
			|| StartsWithTagPath(OldTagString, TEXT("Rune.Activation"))
			|| StartsWithTagPath(OldTagString, TEXT("Rune.Type"))
			|| StartsWithTagPath(OldTagString, TEXT("Rune.Rarity"))
			|| StartsWithTagPath(OldTagString, TEXT("Rune.Element"))
			|| StartsWithTagPath(OldTagString, TEXT("Buff.Rune")))
		{
			OutMigration.OldTag = OldTagName;
			OutMigration.bReportOnly = true;
			OutMigration.Reason = TEXT("Deprecated Rune metadata tag. New content should express this through Rune/Card DA fields or deck slot data, not a formal GameplayTag.");
			return true;
		}

		if (StartsWithTagPath(OldTagString, TEXT("PlayerState.AbilityCast.Special"))
			|| StartsWithTagPath(OldTagString, TEXT("PlayerState.AbilityCast.Finisher")))
		{
			OutMigration.OldTag = OldTagName;
			OutMigration.bReportOnly = true;
			OutMigration.Reason = TEXT("Deprecated combat path. Report only so old assets do not become active runtime entries by accident.");
			return true;
		}

		if (StartsWithTagPath(OldTagString, TEXT("PlayerState.AbilityCast.LightAtk"))
			|| StartsWithTagPath(OldTagString, TEXT("PlayerState.AbilityCast.HeavyAtk"))
			|| StartsWithTagPath(OldTagString, TEXT("PlayerState.AbilityCast.Cooldown"))
			|| StartsWithTagPath(OldTagString, TEXT("PlayerState.AbilityCast.Cooling")))
		{
			OutMigration.OldTag = OldTagName;
			OutMigration.bReportOnly = true;
			OutMigration.Reason = TEXT("Legacy action/cooldown branch with no exact formal equivalent in this commandlet. Confirm the asset intent before migration.");
			return true;
		}

		return false;
	}

	FString PropertyPath(const FString& ObjectPath, const FString& Prefix, const FProperty* Property)
	{
		if (!Property)
		{
			return ObjectPath;
		}

		return Prefix.IsEmpty()
			? FString::Printf(TEXT("%s.%s"), *ObjectPath, *Property->GetName())
			: FString::Printf(TEXT("%s.%s.%s"), *ObjectPath, *Prefix, *Property->GetName());
	}

	bool MigrateTag(FGameplayTag& Tag, const FString& Path, FTagMigrationContext& Context)
	{
		if (!Tag.IsValid())
		{
			return false;
		}

		FTagMigration Migration;
		if (!ResolveMigration(Tag.GetTagName(), Migration))
		{
			return false;
		}

		if (Migration.bReportOnly)
		{
			++Context.ReportOnlyHits;
			Context.ReportLines.Add(FString::Printf(
				TEXT("- REPORT_ONLY `%s` at `%s`: %s"),
				*Tag.ToString(),
				*Path,
				*Migration.Reason));
			return false;
		}

		const FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(Migration.NewTag, false);
		if (!NewTag.IsValid())
		{
			++Context.ReportOnlyHits;
			Context.ReportLines.Add(FString::Printf(
				TEXT("- MISSING_TARGET `%s` -> `%s` at `%s`"),
				*Tag.ToString(),
				*Migration.NewTag.ToString(),
				*Path));
			return false;
		}

		if (Context.ReadOnlyContainerDepth > 0)
		{
			++Context.ReportOnlyHits;
			Context.ReportLines.Add(FString::Printf(
				TEXT("- REPORT_ONLY `%s` -> `%s` at `%s`: %s"),
				*Tag.ToString(),
				*NewTag.ToString(),
				*Path,
				*Context.ReadOnlyContainerReason));
			return false;
		}

		Context.ReportLines.Add(FString::Printf(
			TEXT("- %s `%s` -> `%s` at `%s`"),
			Context.bApply ? TEXT("CHANGED") : TEXT("WOULD_CHANGE"),
			*Tag.ToString(),
			*NewTag.ToString(),
			*Path));
		++Context.TagsWouldChange;

		if (Context.bApply)
		{
			Tag = NewTag;
			++Context.TagsChanged;
			return true;
		}

		return false;
	}

	bool MigrateProperty(FProperty* Property, void* ValuePtr, const FString& ObjectPath, const FString& Prefix, FTagMigrationContext& Context);

	bool MigrateStructProperty(FStructProperty* StructProperty, void* ValuePtr, const FString& ObjectPath, const FString& Prefix, FTagMigrationContext& Context)
	{
		if (StructProperty->Struct == FGameplayTag::StaticStruct())
		{
			FGameplayTag* Tag = reinterpret_cast<FGameplayTag*>(ValuePtr);
			return Tag && MigrateTag(*Tag, PropertyPath(ObjectPath, Prefix, StructProperty), Context);
		}

		if (StructProperty->Struct == FGameplayTagContainer::StaticStruct())
		{
			FGameplayTagContainer* Container = reinterpret_cast<FGameplayTagContainer*>(ValuePtr);
			if (!Container)
			{
				return false;
			}

			bool bChanged = false;
			TArray<FGameplayTag> Tags;
			Container->GetGameplayTagArray(Tags);
			for (const FGameplayTag& OldTag : Tags)
			{
				FGameplayTag MutableTag = OldTag;
				if (MigrateTag(MutableTag, PropertyPath(ObjectPath, Prefix, StructProperty), Context))
				{
					Container->RemoveTag(OldTag);
					Container->AddTag(MutableTag);
					bChanged = true;
				}
			}
			return bChanged;
		}

		bool bChanged = false;
		const FString StructPrefix = Prefix.IsEmpty() ? StructProperty->GetName() : Prefix + TEXT(".") + StructProperty->GetName();
		for (TFieldIterator<FProperty> It(StructProperty->Struct); It; ++It)
		{
			FProperty* ChildProperty = *It;
			void* ChildValuePtr = ChildProperty->ContainerPtrToValuePtr<void>(ValuePtr);
			bChanged |= MigrateProperty(ChildProperty, ChildValuePtr, ObjectPath, StructPrefix, Context);
		}
		return bChanged;
	}

	bool MigrateProperty(FProperty* Property, void* ValuePtr, const FString& ObjectPath, const FString& Prefix, FTagMigrationContext& Context)
	{
		if (!Property || !ValuePtr)
		{
			return false;
		}

		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			return MigrateStructProperty(StructProperty, ValuePtr, ObjectPath, Prefix, Context);
		}

		if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			bool bChanged = false;
			FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
			const FString ArrayPrefix = Prefix.IsEmpty() ? Property->GetName() : Prefix + TEXT(".") + Property->GetName();
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				bChanged |= MigrateProperty(
					ArrayProperty->Inner,
					Helper.GetRawPtr(Index),
					ObjectPath,
					FString::Printf(TEXT("%s[%d]"), *ArrayPrefix, Index),
					Context);
			}
			return bChanged;
		}

		if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			FScriptSetHelper Helper(SetProperty, ValuePtr);
			const FString SetPrefix = Prefix.IsEmpty() ? Property->GetName() : Prefix + TEXT(".") + Property->GetName();
			++Context.ReadOnlyContainerDepth;
			const FString PreviousReason = Context.ReadOnlyContainerReason;
			Context.ReadOnlyContainerReason = TEXT("set elements are not rewritten in place because the set hash must be rebuilt explicitly.");
			for (int32 Index = 0; Index < Helper.GetMaxIndex(); ++Index)
			{
				if (Helper.IsValidIndex(Index))
				{
					MigrateProperty(
						SetProperty->ElementProp,
						Helper.GetElementPtr(Index),
						ObjectPath,
						FString::Printf(TEXT("%s[%d]"), *SetPrefix, Index),
						Context);
				}
			}
			Context.ReadOnlyContainerReason = PreviousReason;
			--Context.ReadOnlyContainerDepth;
			return false;
		}

		if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			bool bChanged = false;
			FScriptMapHelper Helper(MapProperty, ValuePtr);
			const FString MapPrefix = Prefix.IsEmpty() ? Property->GetName() : Prefix + TEXT(".") + Property->GetName();
			for (int32 Index = 0; Index < Helper.GetMaxIndex(); ++Index)
			{
				if (Helper.IsValidIndex(Index))
				{
					++Context.ReadOnlyContainerDepth;
					const FString PreviousReason = Context.ReadOnlyContainerReason;
					Context.ReadOnlyContainerReason = TEXT("map keys are not rewritten in place because the map hash must be rebuilt explicitly.");
					MigrateProperty(
						MapProperty->KeyProp,
						Helper.GetKeyPtr(Index),
						ObjectPath,
						FString::Printf(TEXT("%s[%d].Key"), *MapPrefix, Index),
						Context);
					Context.ReadOnlyContainerReason = PreviousReason;
					--Context.ReadOnlyContainerDepth;
					bChanged |= MigrateProperty(
						MapProperty->ValueProp,
						Helper.GetValuePtr(Index),
						ObjectPath,
						FString::Printf(TEXT("%s[%d].Value"), *MapPrefix, Index),
						Context);
				}
			}
			return bChanged;
		}

		return false;
	}

	bool MigrateObject(UObject* Object, FTagMigrationContext& Context)
	{
		if (!Object)
		{
			return false;
		}

		bool bChanged = false;
		const FString ObjectPath = Object->GetPathName();
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
			bChanged |= MigrateProperty(Property, ValuePtr, ObjectPath, FString(), Context);
		}
		return bChanged;
	}
}

UGameplayTagAssetMigrationCommandlet::UGameplayTagAssetMigrationCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGameplayTagAssetMigrationCommandlet::Main(const FString& Params)
{
	using namespace GameplayTagAssetMigration;

	FTagMigrationContext Context;
	Context.bApply = FParse::Param(*Params, TEXT("Apply"));
	Context.bIncludeDeprecated = FParse::Param(*Params, TEXT("IncludeDeprecated"));

	FString RootPath = TEXT("/Game");
	FParse::Value(*Params, TEXT("Root="), RootPath);
	if (!RootPath.StartsWith(TEXT("/Game")))
	{
		UE_LOG(LogTemp, Error, TEXT("Root must be a /Game package path. Received: %s"), *RootPath);
		return 1;
	}

	Context.ReportLines.Add(TEXT("# GameplayTag Asset Migration Report"));
	Context.ReportLines.Add(FString::Printf(TEXT("- Mode: `%s`"), Context.bApply ? TEXT("Apply") : TEXT("DryRun")));
	Context.ReportLines.Add(FString::Printf(TEXT("- Root: `%s`"), *RootPath));
	Context.ReportLines.Add(FString::Printf(TEXT("- IncludeDeprecated: `%s`"), Context.bIncludeDeprecated ? TEXT("true") : TEXT("false")));
	Context.ReportLines.Add(TEXT(""));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().SearchAllAssets(true);
	AssetRegistryModule.Get().WaitForCompletion();

	TArray<FString> PathsToScan;
	PathsToScan.Add(RootPath);
	AssetRegistryModule.Get().ScanPathsSynchronous(PathsToScan, true);

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(FName(*RootPath), AssetDataList, true);

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (!Context.bIncludeDeprecated && IsDeprecatedPackage(AssetData.PackageName))
		{
			continue;
		}

		const FName AssetClassName = AssetData.AssetClassPath.GetAssetName();
		if (AssetClassName == TEXT("World"))
		{
			++Context.SkippedWorldAssets;
			continue;
		}

		UObject* Asset = AssetData.GetAsset();
		if (!Asset)
		{
			continue;
		}

		++Context.AssetsVisited;
		bool bAssetChanged = false;
		const int32 HitsBefore = Context.TagsWouldChange + Context.ReportOnlyHits;
		UPackage* Package = Asset->GetOutermost();
		bAssetChanged |= MigrateObject(Asset, Context);
		ForEachObjectWithOuter(Package, [Asset, &bAssetChanged, &Context](UObject* Object)
		{
			if (Object == Asset)
			{
				return;
			}
			bAssetChanged |= GameplayTagAssetMigration::MigrateObject(Object, Context);
		}, true);

		const int32 HitsAfter = Context.TagsWouldChange + Context.ReportOnlyHits;
		if (HitsAfter > HitsBefore)
		{
			++Context.AssetsWithHits;
		}

		if (bAssetChanged)
		{
			if (Context.bApply && Package)
			{
				Package->MarkPackageDirty();
				Context.DirtyPackages.AddUnique(Package);
			}
		}
	}

	if (Context.bApply && Context.DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(Context.DirtyPackages, true);
	}

	Context.ReportLines.Insert(FString::Printf(TEXT("- Assets visited: `%d`"), Context.AssetsVisited), 4);
	Context.ReportLines.Insert(FString::Printf(TEXT("- Assets with migration hits: `%d`"), Context.AssetsWithHits), 5);
	Context.ReportLines.Insert(FString::Printf(TEXT("- Tags would change: `%d`"), Context.TagsWouldChange), 6);
	Context.ReportLines.Insert(FString::Printf(TEXT("- Tags changed: `%d`"), Context.TagsChanged), 7);
	Context.ReportLines.Insert(FString::Printf(TEXT("- Report-only hits: `%d`"), Context.ReportOnlyHits), 8);
	Context.ReportLines.Insert(FString::Printf(TEXT("- Skipped World assets: `%d`"), Context.SkippedWorldAssets), 9);
	Context.ReportLines.Insert(TEXT(""), 10);

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ReportFileName, Context.ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("GameplayTag asset migration %s. Report: %s"),
		Context.bApply ? TEXT("applied") : TEXT("dry-run complete"),
		*SharedReportPath);

	return 0;
}
