#include "Tools/SCharacterBalanceWidget.h"

#include "AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h"
#include "AbilitySystem/Abilities/Musket/GA_Musket_LightAttack.h"
#include "AbilitySystem/Abilities/Musket/GA_Musket_SprintAttack.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimMontage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/MontageNotifyEntry.h"
#include "Data/MusketActionTuningDataAsset.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "HAL/PlatformFileManager.h"
#include "IAssetTools.h"
#include "IDetailsView.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SCharacterBalanceWidget"

namespace
{
	template <typename T>
	TArray<T*> CollectAssetsOfClass()
	{
		TArray<T*> Out;
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		if (AssetRegistry.IsLoadingAssets())
		{
			AssetRegistry.SearchAllAssets(true);
		}

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (T* Loaded = Cast<T>(Asset.GetAsset()))
			{
				Out.Add(Loaded);
			}
		}
		return Out;
	}

	FYogBaseAttributeData* GetBaseRow(UCharacterData* Character)
	{
		return Character ? Character->YogBaseAttributeDataRow.GetRow<FYogBaseAttributeData>(TEXT("CharacterDataWorkbench")) : nullptr;
	}

	FMovementData* GetMovementRow(UCharacterData* Character)
	{
		return Character ? Character->MovementDataRow.GetRow<FMovementData>(TEXT("CharacterDataWorkbench")) : nullptr;
	}

	UDataTable* GetBaseTable(UCharacterData* Character)
	{
		return Character ? const_cast<UDataTable*>(Character->YogBaseAttributeDataRow.DataTable.Get()) : nullptr;
	}

	UDataTable* GetMovementTable(UCharacterData* Character)
	{
		return Character ? const_cast<UDataTable*>(Character->MovementDataRow.DataTable.Get()) : nullptr;
	}

	FString RowNameToString(const FDataTableRowHandle& Handle)
	{
		return Handle.RowName.IsNone() ? TEXT("-") : Handle.RowName.ToString();
	}

	FString ObjectName(const UObject* Object)
	{
		return Object ? Object->GetName() : TEXT("-");
	}

	FString ObjectPath(const UObject* Object)
	{
		return Object ? Object->GetPathName() : TEXT("-");
	}

	FString TagsToString(const FGameplayTagContainer& Tags)
	{
		return Tags.IsEmpty() ? TEXT("-") : Tags.ToStringSimple();
	}

	FString CsvEscape(const FString& In)
	{
		FString Out = In;
		const bool bNeedsQuotes = Out.Contains(TEXT(",")) || Out.Contains(TEXT("\"")) || Out.Contains(TEXT("\n"));
		Out.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return bNeedsQuotes ? FString::Printf(TEXT("\"%s\""), *Out) : Out;
	}

	FString SanitizeForAssetName(FString In)
	{
		In.ReplaceInline(TEXT("."), TEXT("_"));
		In.ReplaceInline(TEXT(":"), TEXT("_"));
		In.ReplaceInline(TEXT("/"), TEXT("_"));
		In.ReplaceInline(TEXT("\\"), TEXT("_"));
		In.ReplaceInline(TEXT(" "), TEXT("_"));
		return In;
	}

	const TArray<FCharacterFieldDef>& GetBaseFieldDefs()
	{
		static const TArray<FCharacterFieldDef> Fields =
		{
			{ TEXT("Attack"),       TEXT("Attack"),       TEXT("攻击力"),       TEXT("Base attack value used by damage calculations."),          TEXT("[f]Attack"),       TEXT("all"), 0.f,   0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("AttackPower"),  TEXT("AttackPower"),  TEXT("攻击力加成"),   TEXT("Attack multiplier / power scalar."),                       TEXT("[f]AttackPower"),  TEXT("all"), 1.f,   0.f,    10.f,   true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("MaxHealth"),    TEXT("MaxHealth"),    TEXT("生命值上限"),   TEXT("Maximum health."),                                        TEXT("[f]MaxHealth"),    TEXT("all"), 100.f, 0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("MaxHeat"),      TEXT("MaxHeat"),      TEXT("热量上限"),     TEXT("Maximum heat resource."),                                 TEXT("[f]MaxHeat"),      TEXT("all"), 100.f, 0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("Shield"),       TEXT("Shield"),       TEXT("护盾"),         TEXT("Initial shield value."),                                  TEXT("[f]Shield"),       TEXT("all"), 0.f,   0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("AttackSpeed"),  TEXT("AttackSpeed"),  TEXT("攻击速度"),     TEXT("Attack speed rate."),                                     TEXT("[f]AttackSpeed"),  TEXT("all"), 1.f,   0.1f,   10.f,   true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("AttackRange"),  TEXT("AttackRange"),  TEXT("攻击距离"),     TEXT("Default attack range fallback."),                         TEXT("[f]AttackRange"),  TEXT("all"), 150.f, 0.f,    4000.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("MoveSpeed"),    TEXT("MoveSpeed"),    TEXT("属性移动速度"), TEXT("Attribute-side move speed value."),                       TEXT("[f]MoveSpeed"),    TEXT("all"), 600.f, 0.f,    3000.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("CritRate"),     TEXT("CritRate"),     TEXT("暴击率"),       TEXT("Critical hit chance."),                                   TEXT("[f]CritRate"),     TEXT("all"), 0.f,   0.f,    1.f,    true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("CritDamage"),   TEXT("CritDamage"),   TEXT("暴击伤害"),     TEXT("Critical damage multiplier."),                            TEXT("[f]CritDamage"),   TEXT("all"), 1.f,   1.f,    10.f,   true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("Dodge"),        TEXT("Dodge"),        TEXT("闪避"),         TEXT("Dodge attribute."),                                       TEXT("[f]Dodge"),        TEXT("all"), 0.f,   0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("Resilience"),   TEXT("Resilience"),   TEXT("韧性"),         TEXT("Resilience / poise attribute."),                          TEXT("[f]Resilience"),   TEXT("all"), 0.f,   0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("Resist"),       TEXT("Resist"),       TEXT("抗性"),         TEXT("Resistance attribute."),                                  TEXT("[f]Resist"),       TEXT("all"), 0.f,   0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("DmgTaken"),     TEXT("DmgTaken"),     TEXT("受伤倍率"),     TEXT("Incoming damage scalar."),                                TEXT("[f]DmgTaken"),     TEXT("all"), 1.f,   0.f,    10.f,   true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("MaxArmorHP"),   TEXT("MaxArmorHP"),   TEXT("护甲生命上限"), TEXT("Maximum armor HP."),                                      TEXT("[f]MaxArmorHP"),   TEXT("all"), 0.f,   0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
			{ TEXT("Sanity"),       TEXT("Sanity"),       TEXT("精神"),         TEXT("Sanity value."),                                          TEXT("[f]Sanity"),       TEXT("all"), 0.f,   0.f,    9999.f, true, ECharacterFieldSource::BaseAttributes },
		};
		return Fields;
	}

	const TArray<FCharacterFieldDef>& GetMovementFieldDefs()
	{
		static const TArray<FCharacterFieldDef> Fields =
		{
			{ TEXT("MaxWalkSpeed"),         TEXT("MaxWalkSpeed"),         TEXT("最大移动速度"), TEXT("Character movement component max walk speed."),        TEXT("[f]MaxWalkSpeed"),         TEXT("all"), 600.f, 0.f, 3000.f,  true, ECharacterFieldSource::Movement },
			{ TEXT("GroundFriction"),       TEXT("GroundFriction"),       TEXT("地面摩擦"),     TEXT("Ground friction."),                                      TEXT("[f]GroundFriction"),       TEXT("all"), 8.f,   0.f, 100000.f, true, ECharacterFieldSource::Movement },
			{ TEXT("BreakingDeceleration"), TEXT("BreakingDeceleration"), TEXT("制动减速度"),   TEXT("Braking deceleration walking."),                        TEXT("[f]BreakingDeceleration"), TEXT("all"), 2048.f,0.f, 100000.f, true, ECharacterFieldSource::Movement },
			{ TEXT("MaxAcceleration"),      TEXT("MaxAcceleration"),      TEXT("最大加速度"),   TEXT("Maximum acceleration."),                                  TEXT("[f]MaxAcceleration"),      TEXT("all"), 2048.f,0.f, 100000.f, true, ECharacterFieldSource::Movement },
			{ TEXT("RotationYaw"),          TEXT("RotationYaw"),          TEXT("转向速度Yaw"),  TEXT("Rotation rate yaw."),                                     TEXT("[f]RotationYaw"),          TEXT("all"), 600.f, 0.f, 3600.f,  true, ECharacterFieldSource::Movement },
		};
		return Fields;
	}

	const FCharacterFieldDef* FindFieldDef(FName ColumnName)
	{
		for (const FCharacterFieldDef& Field : GetBaseFieldDefs())
		{
			if (Field.ColumnName == ColumnName)
			{
				return &Field;
			}
		}
		for (const FCharacterFieldDef& Field : GetMovementFieldDefs())
		{
			if (Field.ColumnName == ColumnName)
			{
				return &Field;
			}
		}
		return nullptr;
	}

	bool IsBaseColumn(FName ColumnName)
	{
		const FCharacterFieldDef* Field = FindFieldDef(ColumnName);
		return Field && Field->Source == ECharacterFieldSource::BaseAttributes;
	}

	float ReadCharacterFloat(UCharacterData* Character, FName ColumnName)
	{
		if (FYogBaseAttributeData* Attr = GetBaseRow(Character))
		{
			if (ColumnName == TEXT("Attack")) return Attr->Attack;
			if (ColumnName == TEXT("AttackPower")) return Attr->AttackPower;
			if (ColumnName == TEXT("MaxHealth")) return Attr->MaxHealth;
			if (ColumnName == TEXT("MaxHeat")) return Attr->MaxHeat;
			if (ColumnName == TEXT("Shield")) return Attr->Shield;
			if (ColumnName == TEXT("AttackSpeed")) return Attr->AttackSpeed;
			if (ColumnName == TEXT("AttackRange")) return Attr->AttackRange;
			if (ColumnName == TEXT("MoveSpeed")) return Attr->MoveSpeed;
			if (ColumnName == TEXT("Dodge")) return Attr->Dodge;
			if (ColumnName == TEXT("Resilience")) return Attr->Resilience;
			if (ColumnName == TEXT("Resist")) return Attr->Resist;
			if (ColumnName == TEXT("DmgTaken")) return Attr->DmgTaken;
			if (ColumnName == TEXT("CritRate")) return Attr->Crit_Rate;
			if (ColumnName == TEXT("CritDamage")) return Attr->Crit_Damage;
			if (ColumnName == TEXT("MaxArmorHP")) return Attr->MaxArmorHP;
			if (ColumnName == TEXT("Sanity")) return Attr->Sanity;
		}

		if (FMovementData* Move = GetMovementRow(Character))
		{
			if (ColumnName == TEXT("MaxWalkSpeed")) return Move->MaxWalkSpeed;
			if (ColumnName == TEXT("GroundFriction")) return Move->GroundFriction;
			if (ColumnName == TEXT("BreakingDeceleration")) return Move->BreakingDeceleration;
			if (ColumnName == TEXT("MaxAcceleration")) return Move->MaxAcceleration;
			if (ColumnName == TEXT("RotationYaw")) return Move->RotationRate.Yaw;
		}

		return 0.f;
	}

	bool WriteCharacterFloat(UCharacterData* Character, FName ColumnName, float NewValue)
	{
		if (!Character)
		{
			return false;
		}

		if (IsBaseColumn(ColumnName))
		{
			FYogBaseAttributeData* Attr = GetBaseRow(Character);
			UDataTable* DataTable = GetBaseTable(Character);
			if (!Attr || !DataTable)
			{
				return false;
			}

			if (ColumnName == TEXT("Attack")) { Attr->Attack = NewValue; }
			else if (ColumnName == TEXT("AttackPower")) { Attr->AttackPower = NewValue; }
			else if (ColumnName == TEXT("MaxHealth")) { Attr->MaxHealth = NewValue; }
			else if (ColumnName == TEXT("MaxHeat")) { Attr->MaxHeat = NewValue; }
			else if (ColumnName == TEXT("Shield")) { Attr->Shield = NewValue; }
			else if (ColumnName == TEXT("AttackSpeed")) { Attr->AttackSpeed = NewValue; }
			else if (ColumnName == TEXT("AttackRange")) { Attr->AttackRange = NewValue; }
			else if (ColumnName == TEXT("MoveSpeed")) { Attr->MoveSpeed = NewValue; }
			else if (ColumnName == TEXT("Dodge")) { Attr->Dodge = NewValue; }
			else if (ColumnName == TEXT("Resilience")) { Attr->Resilience = NewValue; }
			else if (ColumnName == TEXT("Resist")) { Attr->Resist = NewValue; }
			else if (ColumnName == TEXT("DmgTaken")) { Attr->DmgTaken = NewValue; }
			else if (ColumnName == TEXT("CritRate")) { Attr->Crit_Rate = NewValue; }
			else if (ColumnName == TEXT("CritDamage")) { Attr->Crit_Damage = NewValue; }
			else if (ColumnName == TEXT("MaxArmorHP")) { Attr->MaxArmorHP = NewValue; }
			else if (ColumnName == TEXT("Sanity")) { Attr->Sanity = NewValue; }
			else { return false; }

			DataTable->MarkPackageDirty();
			return true;
		}

		if (FMovementData* Move = GetMovementRow(Character))
		{
			UDataTable* DataTable = GetMovementTable(Character);
			if (!DataTable)
			{
				return false;
			}

			if (ColumnName == TEXT("MaxWalkSpeed")) { Move->MaxWalkSpeed = NewValue; }
			else if (ColumnName == TEXT("GroundFriction")) { Move->GroundFriction = NewValue; }
			else if (ColumnName == TEXT("BreakingDeceleration")) { Move->BreakingDeceleration = NewValue; }
			else if (ColumnName == TEXT("MaxAcceleration")) { Move->MaxAcceleration = NewValue; }
			else if (ColumnName == TEXT("RotationYaw")) { Move->RotationRate.Yaw = NewValue; }
			else { return false; }

			DataTable->MarkPackageDirty();
			return true;
		}

		return false;
	}

	UMusketActionTuningDataAsset* GetMusketTuning(const FActDataRow& Row)
	{
		return Row.MusketCDO.IsValid() ? Row.MusketCDO->TuningData.Get() : nullptr;
	}

	float ReadMusketFloat(const FActDataRow& Row, FName ColumnName)
	{
		if (UMusketActionTuningDataAsset* Tuning = GetMusketTuning(Row))
		{
			if (ColumnName == TEXT("LightDamageMultiplier")) return Tuning->LightDamageMultiplier;
			if (ColumnName == TEXT("LightHalfAngleDeg")) return Tuning->LightHalfAngleDeg;
			if (ColumnName == TEXT("HeavyChargeTime")) return Tuning->HeavyChargeTime;
			if (ColumnName == TEXT("HeavyStartHalfAngle")) return Tuning->HeavyStartHalfAngle;
			if (ColumnName == TEXT("HeavyEndHalfAngle")) return Tuning->HeavyEndHalfAngle;
			if (ColumnName == TEXT("HeavyStartRadius")) return Tuning->HeavyStartRadius;
			if (ColumnName == TEXT("HeavyEndRadius")) return Tuning->HeavyEndRadius;
			if (ColumnName == TEXT("HeavyBaseDamageMultiplier")) return Tuning->HeavyBaseDamageMultiplier;
			if (ColumnName == TEXT("HeavyFullChargeMultiplier")) return Tuning->HeavyFullChargeMultiplier;
			if (ColumnName == TEXT("SprintDamageMultiplier")) return Tuning->SprintDamageMultiplier;
			if (ColumnName == TEXT("SprintHalfFanAngle")) return Tuning->SprintHalfFanAngle;
		}

		if (UGA_Musket_LightAttack* Light = Cast<UGA_Musket_LightAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("LightDamageMultiplier")) return Light->DamageMultiplier;
			if (ColumnName == TEXT("LightHalfAngleDeg")) return Light->HalfAngleDeg;
		}
		if (UGA_Musket_HeavyAttack* Heavy = Cast<UGA_Musket_HeavyAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("HeavyChargeTime")) return Heavy->ChargeTime;
			if (ColumnName == TEXT("HeavyStartHalfAngle")) return Heavy->StartHalfAngle;
			if (ColumnName == TEXT("HeavyEndHalfAngle")) return Heavy->EndHalfAngle;
			if (ColumnName == TEXT("HeavyStartRadius")) return Heavy->StartRadius;
			if (ColumnName == TEXT("HeavyEndRadius")) return Heavy->EndRadius;
			if (ColumnName == TEXT("HeavyBaseDamageMultiplier")) return Heavy->BaseDamageMultiplier;
			if (ColumnName == TEXT("HeavyFullChargeMultiplier")) return Heavy->FullChargeMultiplier;
		}
		if (UGA_Musket_SprintAttack* Sprint = Cast<UGA_Musket_SprintAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("SprintDamageMultiplier")) return Sprint->DamageMultiplier;
			if (ColumnName == TEXT("SprintHalfFanAngle")) return Sprint->HalfFanAngle;
		}

		return 0.f;
	}

	bool WriteMusketFloat(const FActDataRow& Row, FName ColumnName, float NewValue)
	{
		if (UMusketActionTuningDataAsset* Tuning = GetMusketTuning(Row))
		{
			if (ColumnName == TEXT("LightDamageMultiplier")) { Tuning->LightDamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("LightHalfAngleDeg")) { Tuning->LightHalfAngleDeg = NewValue; }
			else if (ColumnName == TEXT("HeavyChargeTime")) { Tuning->HeavyChargeTime = NewValue; }
			else if (ColumnName == TEXT("HeavyStartHalfAngle")) { Tuning->HeavyStartHalfAngle = NewValue; }
			else if (ColumnName == TEXT("HeavyEndHalfAngle")) { Tuning->HeavyEndHalfAngle = NewValue; }
			else if (ColumnName == TEXT("HeavyStartRadius")) { Tuning->HeavyStartRadius = NewValue; }
			else if (ColumnName == TEXT("HeavyEndRadius")) { Tuning->HeavyEndRadius = NewValue; }
			else if (ColumnName == TEXT("HeavyBaseDamageMultiplier")) { Tuning->HeavyBaseDamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("HeavyFullChargeMultiplier")) { Tuning->HeavyFullChargeMultiplier = NewValue; }
			else if (ColumnName == TEXT("SprintDamageMultiplier")) { Tuning->SprintDamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("SprintHalfFanAngle")) { Tuning->SprintHalfFanAngle = NewValue; }
			else { return false; }

			Tuning->MarkPackageDirty();
			return true;
		}

		if (UGA_Musket_LightAttack* Light = Cast<UGA_Musket_LightAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("LightDamageMultiplier")) { Light->DamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("LightHalfAngleDeg")) { Light->HalfAngleDeg = NewValue; }
			else { return false; }
		}
		else if (UGA_Musket_HeavyAttack* Heavy = Cast<UGA_Musket_HeavyAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("HeavyChargeTime")) { Heavy->ChargeTime = NewValue; }
			else if (ColumnName == TEXT("HeavyStartHalfAngle")) { Heavy->StartHalfAngle = NewValue; }
			else if (ColumnName == TEXT("HeavyEndHalfAngle")) { Heavy->EndHalfAngle = NewValue; }
			else if (ColumnName == TEXT("HeavyStartRadius")) { Heavy->StartRadius = NewValue; }
			else if (ColumnName == TEXT("HeavyEndRadius")) { Heavy->EndRadius = NewValue; }
			else if (ColumnName == TEXT("HeavyBaseDamageMultiplier")) { Heavy->BaseDamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("HeavyFullChargeMultiplier")) { Heavy->FullChargeMultiplier = NewValue; }
			else { return false; }
		}
		else if (UGA_Musket_SprintAttack* Sprint = Cast<UGA_Musket_SprintAttack>(Row.MusketCDO.Get()))
		{
			if (ColumnName == TEXT("SprintDamageMultiplier")) { Sprint->DamageMultiplier = NewValue; }
			else if (ColumnName == TEXT("SprintHalfFanAngle")) { Sprint->HalfFanAngle = NewValue; }
			else { return false; }
		}
		else
		{
			return false;
		}

		if (Row.BlueprintAsset.IsValid())
		{
			Row.BlueprintAsset->MarkPackageDirty();
		}
		return true;
	}

	float ReadActFloat(const FActDataRow& Row, FName ColumnName)
	{
		if (UMontageAttackDataAsset* AttackData = Row.AttackData.Get())
		{
			if (ColumnName == TEXT("ActDamage")) return AttackData->ActDamage;
			if (ColumnName == TEXT("ActRange")) return AttackData->ActRange;
			if (ColumnName == TEXT("ActResilience")) return AttackData->ActResilience;
			if (ColumnName == TEXT("ActDmgReduce")) return AttackData->ActDmgReduce;
		}
		if (UAN_MeleeDamage* Notify = Row.DamageNotify.Get())
		{
			if (ColumnName == TEXT("ActDamage")) return Notify->ActDamage;
			if (ColumnName == TEXT("ActRange")) return Notify->ActRange;
			if (ColumnName == TEXT("ActResilience")) return Notify->ActResilience;
			if (ColumnName == TEXT("ActDmgReduce")) return Notify->ActDmgReduce;
		}
		if (UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("StartFrame")) return HitWindow->StartFrame;
			if (ColumnName == TEXT("EndFrame")) return HitWindow->EndFrame;
		}
		if (UMNE_ComboWindow* ComboWindow = Cast<UMNE_ComboWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("StartFrame")) return ComboWindow->StartFrame;
			if (ColumnName == TEXT("EndFrame")) return ComboWindow->EndFrame;
		}
		if (UMNE_TagWindow* TagWindow = Cast<UMNE_TagWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("StartFrame")) return TagWindow->StartFrame;
			if (ColumnName == TEXT("EndFrame")) return TagWindow->EndFrame;
		}
		if (UMNE_EarlyExit* EarlyExit = Cast<UMNE_EarlyExit>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("Frame")) return EarlyExit->Frame;
			if (ColumnName == TEXT("BlendTime")) return EarlyExit->BlendTime;
		}
		if (UMNE_GameplayEvent* GameplayEvent = Cast<UMNE_GameplayEvent>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("Frame")) return GameplayEvent->Frame;
		}
		return ReadMusketFloat(Row, ColumnName);
	}

	bool WriteActFloat(const FActDataRow& Row, FName ColumnName, float NewValue)
	{
		const bool bAttackDataField = ColumnName == TEXT("ActDamage")
			|| ColumnName == TEXT("ActRange")
			|| ColumnName == TEXT("ActResilience")
			|| ColumnName == TEXT("ActDmgReduce");
		if (bAttackDataField)
		{
			UMontageAttackDataAsset* AttackData = Row.AttackData.Get();
			if (!AttackData)
			{
				return false;
			}

			if (ColumnName == TEXT("ActDamage")) { AttackData->ActDamage = NewValue; }
			else if (ColumnName == TEXT("ActRange")) { AttackData->ActRange = NewValue; }
			else if (ColumnName == TEXT("ActResilience")) { AttackData->ActResilience = NewValue; }
			else if (ColumnName == TEXT("ActDmgReduce")) { AttackData->ActDmgReduce = NewValue; }

			AttackData->MarkPackageDirty();
			return true;
		}

		if (UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("StartFrame")) { HitWindow->StartFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else if (ColumnName == TEXT("EndFrame")) { HitWindow->EndFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else { return false; }
		}
		else if (UMNE_ComboWindow* ComboWindow = Cast<UMNE_ComboWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("StartFrame")) { ComboWindow->StartFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else if (ColumnName == TEXT("EndFrame")) { ComboWindow->EndFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else { return false; }
		}
		else if (UMNE_TagWindow* TagWindow = Cast<UMNE_TagWindow>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("StartFrame")) { TagWindow->StartFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else if (ColumnName == TEXT("EndFrame")) { TagWindow->EndFrame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else { return false; }
		}
		else if (UMNE_EarlyExit* EarlyExit = Cast<UMNE_EarlyExit>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("Frame")) { EarlyExit->Frame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else if (ColumnName == TEXT("BlendTime")) { EarlyExit->BlendTime = FMath::Max(0.f, NewValue); }
			else { return false; }
		}
		else if (UMNE_GameplayEvent* GameplayEvent = Cast<UMNE_GameplayEvent>(Row.MontageEntry.Get()))
		{
			if (ColumnName == TEXT("Frame")) { GameplayEvent->Frame = FMath::Max(0, FMath::RoundToInt(NewValue)); }
			else { return false; }
		}
		else
		{
			return WriteMusketFloat(Row, ColumnName, NewValue);
		}

		if (Row.MontageConfig.IsValid())
		{
			Row.MontageConfig->MarkPackageDirty();
		}
		return true;
	}

	FString ActRowAttackDataName(const FActDataRow& Row)
	{
		if (Row.AttackData.IsValid())
		{
			return Row.AttackData->GetName();
		}
		if (Row.DamageNotify.IsValid())
		{
			return TEXT("AN_MeleeDamage fallback");
		}
		if (Row.MusketCDO.IsValid())
		{
			return GetMusketTuning(Row) ? GetMusketTuning(Row)->GetName() : TEXT("Class defaults");
		}
		return TEXT("-");
	}

	FString EntryLabel(const UMontageNotifyEntry* Entry)
	{
		if (!Entry)
		{
			return TEXT("-");
		}
		if (const UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Entry))
		{
			return FString::Printf(TEXT("HitWindow %d-%d"), HitWindow->StartFrame, HitWindow->EndFrame);
		}
		if (const UMNE_ComboWindow* ComboWindow = Cast<UMNE_ComboWindow>(Entry))
		{
			return FString::Printf(TEXT("ComboWindow %d-%d"), ComboWindow->StartFrame, ComboWindow->EndFrame);
		}
		if (const UMNE_EarlyExit* EarlyExit = Cast<UMNE_EarlyExit>(Entry))
		{
			return FString::Printf(TEXT("EarlyExit %d"), EarlyExit->Frame);
		}
		if (const UMNE_TagWindow* TagWindow = Cast<UMNE_TagWindow>(Entry))
		{
			return FString::Printf(TEXT("TagWindow %d-%d"), TagWindow->StartFrame, TagWindow->EndFrame);
		}
		if (const UMNE_GameplayEvent* Event = Cast<UMNE_GameplayEvent>(Entry))
		{
			return FString::Printf(TEXT("GameplayEvent %d"), Event->Frame);
		}
		return Entry->GetClass()->GetName();
	}

	FString CharacterRowStatus(UCharacterData* Character)
	{
		TArray<FString> Issues;
		if (!Character)
		{
			return TEXT("Invalid");
		}
		if (!GetBaseRow(Character))
		{
			Issues.Add(TEXT("Missing Base"));
		}
		if (!GetMovementRow(Character))
		{
			Issues.Add(TEXT("Missing Move"));
		}
		if (!Character->AbilityData)
		{
			Issues.Add(TEXT("Missing Ability"));
		}
		if (!Character->GasTemplate)
		{
			Issues.Add(TEXT("No GasTemplate"));
		}
		return Issues.IsEmpty() ? TEXT("OK") : FString::Join(Issues, TEXT(", "));
	}

	TSharedRef<SWidget> MakeTextCell(const FString& Text, const FString& ToolTip = FString())
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ToolTipText(FText::FromString(ToolTip));
	}

	TSharedRef<SWidget> MakeSectionTitle(const FText& Text)
	{
		return SNew(STextBlock)
			.Text(Text)
			.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"));
	}
}

class SCharacterWorkbenchTableRow : public SMultiColumnTableRow<TSharedPtr<FCharacterWorkbenchRow>>
{
public:
	SLATE_BEGIN_ARGS(SCharacterWorkbenchTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FCharacterWorkbenchRow>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FCharacterWorkbenchRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FCharacterWorkbenchRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		UCharacterData* Character = Item.IsValid() ? Item->Asset.Get() : nullptr;
		if (!Character)
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Character"))
		{
			return MakeTextCell(Character->GetName(), Character->GetPathName());
		}
		if (ColumnName == TEXT("BaseRow"))
		{
			return MakeTextCell(RowNameToString(Character->YogBaseAttributeDataRow), ObjectPath(GetBaseTable(Character)));
		}
		if (ColumnName == TEXT("MoveRow"))
		{
			return MakeTextCell(RowNameToString(Character->MovementDataRow), ObjectPath(GetMovementTable(Character)));
		}
		if (ColumnName == TEXT("AbilityData"))
		{
			return MakeTextCell(ObjectName(Character->AbilityData.Get()), ObjectPath(Character->AbilityData.Get()));
		}
		if (ColumnName == TEXT("Status"))
		{
			return MakeTextCell(Item->Status);
		}

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FCharacterWorkbenchRow> Item;
};

class SActDataTableRow : public SMultiColumnTableRow<TSharedPtr<FActDataRow>>
{
public:
	SLATE_BEGIN_ARGS(SActDataTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<FActDataRow>, Item)
		SLATE_ARGUMENT(TWeakPtr<SCharacterBalanceWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		SMultiColumnTableRow<TSharedPtr<FActDataRow>>::Construct(
			SMultiColumnTableRow<TSharedPtr<FActDataRow>>::FArguments().Padding(FMargin(3.f, 2.f)),
			OwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Item.IsValid())
		{
			return MakeTextCell(TEXT("-"));
		}

		if (ColumnName == TEXT("Ability"))
		{
			return MakeTextCell(Item->AbilityTag.IsValid() ? Item->AbilityTag.ToString() : TEXT("-"));
		}
		if (ColumnName == TEXT("Type"))
		{
			return MakeTextCell(Item->TypeLabel);
		}
		if (ColumnName == TEXT("Source"))
		{
			return MakeTextCell(Item->SourceLabel);
		}
		if (ColumnName == TEXT("AttackData"))
		{
			return MakeTextCell(ActRowAttackDataName(*Item));
		}
		if (ColumnName == TEXT("Status"))
		{
			return MakeTextCell(Item->Status);
		}
		if (ColumnName == TEXT("Actions"))
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenActAsset", "Open"))
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SCharacterBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->OpenActPrimaryAsset(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CreateAttackData", "Create AttackData"))
					.Visibility_Lambda([Row = Item]()
					{
						return Row.IsValid()
							&& Row->Kind == EActDataRowKind::HitWindow
							&& !Row->AttackData.IsValid()
							? EVisibility::Visible
							: EVisibility::Collapsed;
					})
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SCharacterBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->CreateAttackDataForActRow(Row);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("CreateMusketTuning", "Create Tuning"))
					.Visibility_Lambda([Row = Item]()
					{
						return Row.IsValid()
							&& Row->MusketCDO.IsValid()
							&& !GetMusketTuning(*Row)
							? EVisibility::Visible
							: EVisibility::Collapsed;
					})
					.OnClicked_Lambda([Owner = OwnerWidget, Row = Item]()
					{
						if (TSharedPtr<SCharacterBalanceWidget> Pinned = Owner.Pin())
						{
							Pinned->CreateMusketTuningData(Row);
						}
						return FReply::Handled();
					})
				];
		}

		return MakeTextCell(TEXT("-"));
	}

private:
	TSharedPtr<FActDataRow> Item;
	TWeakPtr<SCharacterBalanceWidget> OwnerWidget;
};

void SCharacterBalanceWidget::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("CharacterReady", "Ready");

	FDetailsViewArgs CharacterDetailsArgs;
	CharacterDetailsArgs.bHideSelectionTip = true;
	CharacterDetailsArgs.bAllowSearch = true;
	CharacterDetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FDetailsViewArgs ActDetailsArgs;
	ActDetailsArgs.bHideSelectionTip = true;
	ActDetailsArgs.bAllowSearch = true;
	ActDetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	CharacterDetailsView = PropertyEditor.CreateDetailView(CharacterDetailsArgs);
	ActDetailsView = PropertyEditor.CreateDetailView(ActDetailsArgs);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.31f)
			[
				BuildCharacterListPanel()
			]
			+ SSplitter::Slot().Value(0.69f)
			[
				SNew(SBorder).Padding(8.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(STextBlock)
						.Text(this, &SCharacterBalanceWidget::GetSelectedCharacterNameText)
						.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							BuildPageButton(LOCTEXT("PageSummary", "角色信息"), ECharacterWorkbenchPage::Summary)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							BuildPageButton(LOCTEXT("PageBase", "基础属性"), ECharacterWorkbenchPage::BaseAttributes)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							BuildPageButton(LOCTEXT("PageMove", "移动属性"), ECharacterWorkbenchPage::Movement)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							BuildPageButton(LOCTEXT("PageAbility", "Ability / Montage"), ECharacterWorkbenchPage::AbilityMontage)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							BuildPageButton(LOCTEXT("PageAct", "Act 数据"), ECharacterWorkbenchPage::ActData)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							BuildPageButton(LOCTEXT("PageValidation", "校验 / 导出"), ECharacterWorkbenchPage::ValidationExport)
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SWidgetSwitcher)
						.WidgetIndex(this, &SCharacterBalanceWidget::GetActivePageIndex)
						+ SWidgetSwitcher::Slot()
						[
							BuildSummaryPage()
						]
						+ SWidgetSwitcher::Slot()
						[
							BuildFieldsPage(ECharacterFieldSource::BaseAttributes)
						]
						+ SWidgetSwitcher::Slot()
						[
							BuildFieldsPage(ECharacterFieldSource::Movement)
						]
						+ SWidgetSwitcher::Slot()
						[
							BuildAbilityMontagePage()
						]
						+ SWidgetSwitcher::Slot()
						[
							BuildActDataPage()
						]
						+ SWidgetSwitcher::Slot()
						[
							BuildValidationExportPage()
						]
					]
				]
			]
		]
	];

	RefreshData(LOCTEXT("InitialStatus", "Character Data Workbench refreshed."));
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildToolbar()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WorkbenchTitle", "Character Data Workbench"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Refresh", "Refresh")).OnClicked(this, &SCharacterBalanceWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("Validate", "Validate All")).OnClicked(this, &SCharacterBalanceWidget::OnValidateClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("ExportCsv", "Export CSV")).OnClicked(this, &SCharacterBalanceWidget::OnExportCsvClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Text(this, &SCharacterBalanceWidget::GetStatsText)
			]
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildCharacterListPanel()
{
	return SNew(SBorder).Padding(6.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SSearchBox)
				.HintText(LOCTEXT("SearchCharacter", "Search character / AbilityData"))
				.OnTextChanged(this, &SCharacterBalanceWidget::OnSearchTextChanged)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(CharacterListView, SListView<FCharacterRowPtr>)
				.ListItemsSource(&FilteredRows)
				.OnGenerateRow(this, &SCharacterBalanceWidget::GenerateCharacterRow)
				.OnSelectionChanged(this, &SCharacterBalanceWidget::OnCharacterSelectionChanged)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("Character")).DefaultLabel(LOCTEXT("CharacterColumn", "Character")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("BaseRow")).DefaultLabel(LOCTEXT("BaseRowColumn", "Base")).FixedWidth(70.f)
					+ SHeaderRow::Column(TEXT("MoveRow")).DefaultLabel(LOCTEXT("MoveRowColumn", "Move")).FixedWidth(70.f)
					+ SHeaderRow::Column(TEXT("AbilityData")).DefaultLabel(LOCTEXT("AbilityColumn", "Ability")).FillWidth(0.9f)
					+ SHeaderRow::Column(TEXT("Status")).DefaultLabel(LOCTEXT("StatusColumn", "Status")).FixedWidth(110.f)
				)
			]
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildPageButton(const FText& Label, ECharacterWorkbenchPage Page)
{
	return SNew(SButton)
		.Text(Label)
		.IsEnabled_Lambda([this, Page]() { return ActivePage != Page; })
		.OnClicked(this, &SCharacterBalanceWidget::OnPageButtonClicked, Page);
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildSummaryPage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			MakeSectionTitle(LOCTEXT("CharacterInfoSection", "角色基础信息"))
		]
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(this, &SCharacterBalanceWidget::GetSelectedCharacterSummaryText)
			.AutoWrapText(true)
		]
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenCharacterAsset", "Open Character DA"))
				.OnClicked_Lambda([this]()
				{
					OpenAsset(SelectedRow);
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenBaseDt", "Open Base DT"))
				.OnClicked_Lambda([this]()
				{
					OpenBaseTable(SelectedRow);
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenMoveDt", "Open Move DT"))
				.OnClicked_Lambda([this]()
				{
					OpenMovementTable(SelectedRow);
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenAbilityData", "Open AbilityData"))
				.OnClicked_Lambda([this]()
				{
					OpenSelectedAbilityData();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenGasTemplate", "Open GasTemplate"))
				.OnClicked_Lambda([this]()
				{
					OpenSelectedGasTemplate();
					return FReply::Handled();
				})
			]
		]
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			MakeSectionTitle(LOCTEXT("CharacterDetailsSection", "Character DA Details"))
		]
		+ SScrollBox::Slot()
		[
			SNew(SBox)
			.MinDesiredHeight(420.f)
			[
				CharacterDetailsView.ToSharedRef()
			]
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildFieldsPage(ECharacterFieldSource Source)
{
	const TArray<FCharacterFieldDef>& Fields = Source == ECharacterFieldSource::BaseAttributes
		? GetBaseFieldDefs()
		: GetMovementFieldDefs();

	TSharedRef<SScrollBox> Scroll = SNew(SScrollBox);
	Scroll->AddSlot()
	.Padding(0.f, 0.f, 0.f, 8.f)
	[
		MakeSectionTitle(Source == ECharacterFieldSource::BaseAttributes
			? LOCTEXT("BaseAttributesSection", "基础属性")
			: LOCTEXT("MovementSection", "移动属性"))
	];
	Scroll->AddSlot()
	.Padding(0.f, 0.f, 0.f, 8.f)
	[
		SNew(STextBlock)
		.Text(Source == ECharacterFieldSource::BaseAttributes
			? LOCTEXT("BaseAttributesHelp", "Writes to the selected CharacterData Base DataTable row. Assets are marked dirty but not auto-saved.")
			: LOCTEXT("MovementHelp", "Writes to the selected CharacterData Movement DataTable row. Assets are marked dirty but not auto-saved."))
		.AutoWrapText(true)
	];

	for (const FCharacterFieldDef& Field : Fields)
	{
		Scroll->AddSlot().Padding(0.f, 0.f, 0.f, 4.f)
		[
			BuildCharacterNumberRow(Field)
		];
	}
	return Scroll;
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildCharacterNumberRow(const FCharacterFieldDef& Field)
{
	const FName ColumnName = Field.ColumnName;
	const FString RangeText = Field.bHasRange
		? FString::Printf(TEXT("Default %.3f | Range %.3f - %.3f"), Field.DefaultValue, Field.MinValue, Field.MaxValue)
		: FString::Printf(TEXT("Default %.3f"), Field.DefaultValue);

	return SNew(SBorder).Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.23f).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s\n%s"), *Field.DisplayName, *Field.ChineseName)))
				.ToolTipText(FText::FromString(Field.Description))
			]
			+ SHorizontalBox::Slot().FillWidth(0.18f).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Field.TypeToken))
				.ToolTipText(FText::FromString(Field.Platform))
			]
			+ SHorizontalBox::Slot().FillWidth(0.32f).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Field.Description))
				.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot().FillWidth(0.18f).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(RangeText))
				.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SNumericEntryBox<float>)
				.MinDesiredValueWidth(110.f)
				.Value_Lambda([this, ColumnName]()
				{
					if (UCharacterData* Character = GetSelectedCharacter())
					{
						return TOptional<float>(ReadCharacterFloat(Character, ColumnName));
					}
					return TOptional<float>();
				})
				.OnValueCommitted_Lambda([this, ColumnName](float NewValue, ETextCommit::Type)
				{
					CommitFloat(SelectedRow, ColumnName, NewValue);
				})
			]
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildAbilityMontagePage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			MakeSectionTitle(LOCTEXT("AbilityMontageSection", "Ability / Montage 链路"))
		]
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("AbilityMontageHelp", "This view follows CharacterData -> AbilityData -> MontageMap / MontageConfigMap. Act rows are edited in the Act 数据 page."))
			.AutoWrapText(true)
		]
		+ SScrollBox::Slot()
		[
			SAssignNew(AbilityMontageContent, SVerticalBox)
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildActDataPage()
{
	return SNew(SSplitter)
		+ SSplitter::Slot().Value(0.58f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text(this, &SCharacterBalanceWidget::GetSelectedActSummaryText)
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(ActListView, SListView<FActRowPtr>)
				.ListItemsSource(&ActRows)
				.OnGenerateRow(this, &SCharacterBalanceWidget::GenerateActRow)
				.OnSelectionChanged(this, &SCharacterBalanceWidget::OnActSelectionChanged)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("Ability")).DefaultLabel(LOCTEXT("ActAbilityColumn", "AbilityTag")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("Type")).DefaultLabel(LOCTEXT("ActTypeColumn", "Type")).FixedWidth(120.f)
					+ SHeaderRow::Column(TEXT("Source")).DefaultLabel(LOCTEXT("ActSourceColumn", "Source")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("AttackData")).DefaultLabel(LOCTEXT("ActAttackDataColumn", "AttackData / Tuning")).FillWidth(1.0f)
					+ SHeaderRow::Column(TEXT("Status")).DefaultLabel(LOCTEXT("ActStatusColumn", "Status")).FixedWidth(130.f)
					+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("ActActionsColumn", "Actions")).FixedWidth(260.f)
				)
			]
		]
		+ SSplitter::Slot().Value(0.42f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
			[
				MakeSectionTitle(LOCTEXT("SelectedActSection", "Selected Act"))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(this, &SCharacterBalanceWidget::GetSelectedActDetailsText)
				.AutoWrapText(true)
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("ActDamage"), TEXT("ActDamage"), TEXT("External MontageAttackData damage. AN_MeleeDamage fallback is read-only in this workbench."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("ActRange"), TEXT("ActRange"), TEXT("Attack range / target query range."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("ActResilience"), TEXT("ActResilience"), TEXT("Action poise / resilience contribution."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
			[
				BuildActNumberRow(TEXT("ActDmgReduce"), TEXT("ActDmgReduce"), TEXT("Action damage reduction value."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("StartFrame"), TEXT("StartFrame"), TEXT("Hit / combo / tag window start frame."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("EndFrame"), TEXT("EndFrame"), TEXT("Hit / combo / tag window end frame."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("Frame"), TEXT("Frame"), TEXT("Single frame entry such as EarlyExit or GameplayEvent."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
			[
				BuildActNumberRow(TEXT("BlendTime"), TEXT("BlendTime"), TEXT("EarlyExit blend time."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
			[
				MakeSectionTitle(LOCTEXT("MusketQuickFields", "Ranged / Musket Act"))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("LightDamageMultiplier"), TEXT("LightDamageMultiplier"), TEXT("Musket light attack damage multiplier."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("LightHalfAngleDeg"), TEXT("LightHalfAngleDeg"), TEXT("Musket light spread half angle."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("HeavyChargeTime"), TEXT("HeavyChargeTime"), TEXT("Musket heavy charge time."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("HeavyBaseDamageMultiplier"), TEXT("HeavyBaseDamageMultiplier"), TEXT("Heavy attack base damage multiplier."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("HeavyFullChargeMultiplier"), TEXT("HeavyFullChargeMultiplier"), TEXT("Heavy attack full-charge damage multiplier."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("HeavyEndRadius"), TEXT("HeavyEndRadius"), TEXT("Heavy attack final radius."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 4.f)
			[
				BuildActNumberRow(TEXT("SprintDamageMultiplier"), TEXT("SprintDamageMultiplier"), TEXT("Sprint attack damage multiplier."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
			[
				BuildActNumberRow(TEXT("SprintHalfFanAngle"), TEXT("SprintHalfFanAngle"), TEXT("Sprint attack fan half angle."))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 8.f)
			[
				MakeSectionTitle(LOCTEXT("ActDetailsSection", "Details: tags, hitboxes, hit stop, effects"))
			]
			+ SScrollBox::Slot().Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox)
				.MinDesiredHeight(420.f)
				[
					ActDetailsView.ToSharedRef()
				]
			]
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildActNumberRow(FName ColumnName, const FString& Label, const FString& Description)
{
	return SNew(SBorder).Padding(4.f)
		.Visibility_Lambda([this, ColumnName]()
		{
			return SelectedActRow.IsValid() && (CanEditActField(*SelectedActRow, ColumnName) || ReadActFloat(*SelectedActRow, ColumnName) != 0.f)
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ToolTipText(FText::FromString(Description))
			]
			+ SHorizontalBox::Slot().FillWidth(0.55f)
			[
				SNew(SNumericEntryBox<float>)
				.MinDesiredValueWidth(120.f)
				.IsEnabled_Lambda([this, ColumnName]()
				{
					return SelectedActRow.IsValid() && CanEditActField(*SelectedActRow, ColumnName);
				})
				.Value_Lambda([this, ColumnName]()
				{
					return SelectedActRow.IsValid()
						? TOptional<float>(ReadActFloat(*SelectedActRow, ColumnName))
						: TOptional<float>();
				})
				.OnValueCommitted_Lambda([this, ColumnName](float NewValue, ETextCommit::Type)
				{
					CommitActFloat(SelectedActRow, ColumnName, NewValue);
				})
			]
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildValidationExportPage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			MakeSectionTitle(LOCTEXT("ValidationExportSection", "校验 / 导出"))
		]
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ValidationHelp", "Validate All checks CharacterData row references, AbilityData montage links, HitWindow frames, and missing AttackData. Export CSV writes Character and Act snapshots under Saved/Balance."))
			.AutoWrapText(true)
		]
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton).Text(LOCTEXT("ValidateAgain", "Validate All")).OnClicked(this, &SCharacterBalanceWidget::OnValidateClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("ExportAgain", "Export CSV")).OnClicked(this, &SCharacterBalanceWidget::OnExportCsvClicked)
			]
		]
		+ SScrollBox::Slot().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock).Text(this, &SCharacterBalanceWidget::GetStatsText).AutoWrapText(true)
		]
		+ SScrollBox::Slot()
		[
			BuildFieldDictionarySection()
		];
}

TSharedRef<SWidget> SCharacterBalanceWidget::BuildFieldDictionarySection()
{
	TSharedRef<SGridPanel> Grid = SNew(SGridPanel);
	Grid->AddSlot(0, 0).Padding(2.f)[MakeTextCell(TEXT("Field"))];
	Grid->AddSlot(1, 0).Padding(2.f)[MakeTextCell(TEXT("Type"))];
	Grid->AddSlot(2, 0).Padding(2.f)[MakeTextCell(TEXT("Default"))];
	Grid->AddSlot(3, 0).Padding(2.f)[MakeTextCell(TEXT("Min"))];
	Grid->AddSlot(4, 0).Padding(2.f)[MakeTextCell(TEXT("Max"))];
	Grid->AddSlot(5, 0).Padding(2.f)[MakeTextCell(TEXT("Description"))];

	int32 RowIndex = 1;
	auto AddField = [&Grid, &RowIndex](const FCharacterFieldDef& Field)
	{
		Grid->AddSlot(0, RowIndex).Padding(2.f)[MakeTextCell(FString::Printf(TEXT("%s / %s"), *Field.DisplayName, *Field.ChineseName))];
		Grid->AddSlot(1, RowIndex).Padding(2.f)[MakeTextCell(Field.TypeToken)];
		Grid->AddSlot(2, RowIndex).Padding(2.f)[MakeTextCell(FString::Printf(TEXT("%.3f"), Field.DefaultValue))];
		Grid->AddSlot(3, RowIndex).Padding(2.f)[MakeTextCell(Field.bHasRange ? FString::Printf(TEXT("%.3f"), Field.MinValue) : TEXT("-"))];
		Grid->AddSlot(4, RowIndex).Padding(2.f)[MakeTextCell(Field.bHasRange ? FString::Printf(TEXT("%.3f"), Field.MaxValue) : TEXT("-"))];
		Grid->AddSlot(5, RowIndex).Padding(2.f)[MakeTextCell(Field.Description)];
		++RowIndex;
	};

	for (const FCharacterFieldDef& Field : GetBaseFieldDefs())
	{
		AddField(Field);
	}
	for (const FCharacterFieldDef& Field : GetMovementFieldDefs())
	{
		AddField(Field);
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			MakeSectionTitle(LOCTEXT("FieldDictionarySection", "字段字典视图"))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Grid
		];
}

void SCharacterBalanceWidget::RefreshData(const FText& NewStatus)
{
	const FString PreviousSelection = SelectedRow.IsValid() && SelectedRow->Asset.IsValid()
		? SelectedRow->Asset->GetPathName()
		: FString();

	TArray<UCharacterData*> Characters = CollectAssetsOfClass<UCharacterData>();
	Characters.Sort([](const UCharacterData& A, const UCharacterData& B)
	{
		return A.GetName() < B.GetName();
	});

	Rows.Reset();
	for (UCharacterData* Character : Characters)
	{
		TSharedRef<FCharacterWorkbenchRow> Row = MakeShared<FCharacterWorkbenchRow>(Character);
		Row->Status = CharacterRowStatus(Character);
		Rows.Add(Row);
	}

	RebuildFilteredRows();

	FCharacterRowPtr NewSelection;
	for (const FCharacterRowPtr& Row : FilteredRows)
	{
		if (Row.IsValid() && Row->Asset.IsValid() && Row->Asset->GetPathName() == PreviousSelection)
		{
			NewSelection = Row;
			break;
		}
	}
	if (!NewSelection.IsValid() && FilteredRows.Num() > 0)
	{
		NewSelection = FilteredRows[0];
	}

	if (CharacterListView.IsValid())
	{
		CharacterListView->RequestListRefresh();
		if (NewSelection.IsValid())
		{
			CharacterListView->SetSelection(NewSelection);
		}
	}
	OnCharacterSelectionChanged(NewSelection, ESelectInfo::Direct);

	if (!NewStatus.IsEmpty())
	{
		StatusText = NewStatus;
	}
}

void SCharacterBalanceWidget::RebuildFilteredRows()
{
	FilteredRows.Reset();
	const FString Filter = SearchText.ToString();
	for (const FCharacterRowPtr& Row : Rows)
	{
		UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
		if (!Character)
		{
			continue;
		}

		if (Filter.IsEmpty()
			|| Character->GetName().Contains(Filter, ESearchCase::IgnoreCase)
			|| ObjectName(Character->AbilityData.Get()).Contains(Filter, ESearchCase::IgnoreCase)
			|| Row->Status.Contains(Filter, ESearchCase::IgnoreCase))
		{
			FilteredRows.Add(Row);
		}
	}

	if (CharacterListView.IsValid())
	{
		CharacterListView->RequestListRefresh();
	}
}

void SCharacterBalanceWidget::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText;
	RebuildFilteredRows();
	if (!FilteredRows.Contains(SelectedRow))
	{
		const FCharacterRowPtr NewSelection = FilteredRows.Num() > 0 ? FilteredRows[0] : nullptr;
		if (CharacterListView.IsValid() && NewSelection.IsValid())
		{
			CharacterListView->SetSelection(NewSelection);
		}
		OnCharacterSelectionChanged(NewSelection, ESelectInfo::Direct);
	}
}

TSharedRef<ITableRow> SCharacterBalanceWidget::GenerateCharacterRow(FCharacterRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SCharacterWorkbenchTableRow, OwnerTable)
		.Item(Row);
}

void SCharacterBalanceWidget::OnCharacterSelectionChanged(FCharacterRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedRow = Row;
	if (CharacterDetailsView.IsValid())
	{
		CharacterDetailsView->SetObject(GetSelectedCharacter());
	}
	CollectActRowsForSelectedCharacter();
	RebuildAbilityMontageContent();
}

TSharedRef<ITableRow> SCharacterBalanceWidget::GenerateActRow(FActRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SActDataTableRow, OwnerTable)
		.Item(Row)
		.OwnerWidget(SharedThis(this));
}

void SCharacterBalanceWidget::OnActSelectionChanged(FActRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedActRow = Row;
	if (ActDetailsView.IsValid())
	{
		ActDetailsView->SetObject(Row.IsValid() ? GetActDetailsObject(*Row) : nullptr);
	}
}

void SCharacterBalanceWidget::CollectActRowsForSelectedCharacter()
{
	ActRows.Reset();
	SelectedActRow.Reset();

	UCharacterData* Character = GetSelectedCharacter();
	UAbilityData* AbilityData = Character ? Character->AbilityData.Get() : nullptr;
	if (Character && AbilityData)
	{
		auto AddNotifyFallbackRows = [this, Character](const FGameplayTag& AbilityTag, UMontageConfigDA* Config, UAnimMontage* Montage, const FString& SourceLabel)
		{
			if (!Montage)
			{
				return;
			}
			for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
			{
				if (UAN_MeleeDamage* DamageNotify = Cast<UAN_MeleeDamage>(NotifyEvent.Notify))
				{
					TSharedRef<FActDataRow> Row = MakeShared<FActDataRow>();
					Row->Kind = EActDataRowKind::NotifyFallback;
					Row->Character = Character;
					Row->AbilityTag = AbilityTag;
					Row->TypeLabel = TEXT("AN Notify Fallback");
					Row->SourceLabel = SourceLabel;
					Row->Montage = Montage;
					Row->MontageConfig = Config;
					Row->DamageNotify = DamageNotify;
					Row->Status = DamageNotify->AttackDataOverride ? TEXT("Uses AttackDataOverride") : TEXT("Fallback only");
					if (DamageNotify->AttackDataOverride)
					{
						Row->AttackData = DamageNotify->AttackDataOverride;
					}
					ActRows.Add(Row);
				}
			}
		};

		for (const TPair<FGameplayTag, TObjectPtr<UAnimMontage>>& Pair : AbilityData->MontageMap)
		{
			TSharedRef<FActDataRow> Row = MakeShared<FActDataRow>();
			Row->Kind = EActDataRowKind::MontageMap;
			Row->Character = Character;
			Row->AbilityTag = Pair.Key;
			Row->TypeLabel = TEXT("MontageMap");
			Row->SourceLabel = ObjectName(Pair.Value.Get());
			Row->Montage = Pair.Value;
			Row->Status = Pair.Value ? TEXT("Fallback montage") : TEXT("Missing montage");
			ActRows.Add(Row);
			AddNotifyFallbackRows(Pair.Key, nullptr, Pair.Value.Get(), TEXT("MontageMap"));
		}

		for (const TPair<FGameplayTag, FAbilityMontageConfigList>& Pair : AbilityData->MontageConfigMap)
		{
			for (const FTaggedMontageConfig& ConfigCandidate : Pair.Value.Configs)
			{
				UMontageConfigDA* Config = ConfigCandidate.MontageConfig.Get();
				TSharedRef<FActDataRow> ConfigRow = MakeShared<FActDataRow>();
				ConfigRow->Kind = EActDataRowKind::MontageConfig;
				ConfigRow->Character = Character;
				ConfigRow->AbilityTag = Pair.Key;
				ConfigRow->TypeLabel = TEXT("MontageConfig");
				ConfigRow->SourceLabel = ObjectName(Config);
				ConfigRow->MontageConfig = Config;
				ConfigRow->Montage = Config ? Config->Montage.Get() : nullptr;
				ConfigRow->RequiredTags = ConfigCandidate.RequiredTags;
				ConfigRow->BlockedTags = ConfigCandidate.BlockedTags;
				ConfigRow->Priority = ConfigCandidate.Priority;
				ConfigRow->Status = Config ? TEXT("OK") : TEXT("Missing MontageConfig");
				ActRows.Add(ConfigRow);

				if (!Config)
				{
					continue;
				}

				AddNotifyFallbackRows(Pair.Key, Config, Config->Montage.Get(), FString::Printf(TEXT("Config:%s"), *Config->GetName()));
				for (UMontageNotifyEntry* Entry : Config->Entries)
				{
					if (!Entry)
					{
						continue;
					}

					if (UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Entry))
					{
						TSharedRef<FActDataRow> WindowRow = MakeShared<FActDataRow>();
						WindowRow->Kind = EActDataRowKind::HitWindow;
						WindowRow->Character = Character;
						WindowRow->AbilityTag = Pair.Key;
						WindowRow->TypeLabel = TEXT("HitWindow");
						WindowRow->SourceLabel = FString::Printf(TEXT("%s / %s"), *ObjectName(Config), *EntryLabel(Entry));
						WindowRow->MontageConfig = Config;
						WindowRow->Montage = Config->Montage;
						WindowRow->MontageEntry = Entry;
						WindowRow->Status = HitWindow->AttackDataCandidates.IsEmpty() ? TEXT("Missing AttackData") : TEXT("Has candidates");
						ActRows.Add(WindowRow);

						for (int32 CandidateIndex = 0; CandidateIndex < HitWindow->AttackDataCandidates.Num(); ++CandidateIndex)
						{
							const FTaggedMontageAttackData& AttackCandidate = HitWindow->AttackDataCandidates[CandidateIndex];
							TSharedRef<FActDataRow> CandidateRow = MakeShared<FActDataRow>();
							CandidateRow->Kind = EActDataRowKind::AttackDataCandidate;
							CandidateRow->Character = Character;
							CandidateRow->AbilityTag = Pair.Key;
							CandidateRow->TypeLabel = TEXT("AttackData");
							CandidateRow->SourceLabel = FString::Printf(TEXT("%s / %s"), *ObjectName(Config), *EntryLabel(Entry));
							CandidateRow->MontageConfig = Config;
							CandidateRow->Montage = Config->Montage;
							CandidateRow->MontageEntry = Entry;
							CandidateRow->AttackData = AttackCandidate.AttackData;
							CandidateRow->RequiredTags = AttackCandidate.RequiredTags;
							CandidateRow->BlockedTags = AttackCandidate.BlockedTags;
							CandidateRow->Priority = AttackCandidate.Priority;
							CandidateRow->CandidateIndex = CandidateIndex;
							CandidateRow->Status = AttackCandidate.AttackData ? TEXT("Editable") : TEXT("Missing AttackData");
							ActRows.Add(CandidateRow);
						}
					}
					else
					{
						TSharedRef<FActDataRow> EntryRow = MakeShared<FActDataRow>();
						EntryRow->Character = Character;
						EntryRow->AbilityTag = Pair.Key;
						EntryRow->SourceLabel = FString::Printf(TEXT("%s / %s"), *ObjectName(Config), *EntryLabel(Entry));
						EntryRow->MontageConfig = Config;
						EntryRow->Montage = Config->Montage;
						EntryRow->MontageEntry = Entry;
						EntryRow->Status = TEXT("Window data");

						if (Cast<UMNE_ComboWindow>(Entry))
						{
							EntryRow->Kind = EActDataRowKind::ComboWindow;
							EntryRow->TypeLabel = TEXT("ComboWindow");
						}
						else if (Cast<UMNE_EarlyExit>(Entry))
						{
							EntryRow->Kind = EActDataRowKind::EarlyExit;
							EntryRow->TypeLabel = TEXT("EarlyExit");
						}
						else if (Cast<UMNE_TagWindow>(Entry))
						{
							EntryRow->Kind = EActDataRowKind::TagWindow;
							EntryRow->TypeLabel = TEXT("TagWindow");
						}
						else if (Cast<UMNE_GameplayEvent>(Entry))
						{
							EntryRow->Kind = EActDataRowKind::GameplayEvent;
							EntryRow->TypeLabel = TEXT("GameplayEvent");
						}
						else
						{
							EntryRow->TypeLabel = Entry->GetClass()->GetName();
						}
						ActRows.Add(EntryRow);
					}
				}
			}
		}
	}

	CollectMusketActRows();

	ActRows.Sort([](const FActRowPtr& A, const FActRowPtr& B)
	{
		const FString AKey = A.IsValid() ? FString::Printf(TEXT("%s|%s|%s"), *A->AbilityTag.ToString(), *A->TypeLabel, *A->SourceLabel) : FString();
		const FString BKey = B.IsValid() ? FString::Printf(TEXT("%s|%s|%s"), *B->AbilityTag.ToString(), *B->TypeLabel, *B->SourceLabel) : FString();
		return AKey < BKey;
	});

	if (ActListView.IsValid())
	{
		ActListView->RequestListRefresh();
		if (ActRows.Num() > 0)
		{
			ActListView->SetSelection(ActRows[0]);
		}
	}
	OnActSelectionChanged(ActRows.Num() > 0 ? ActRows[0] : nullptr, ESelectInfo::Direct);
}

void SCharacterBalanceWidget::CollectMusketActRows()
{
	MusketBlueprintCount = 0;
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> BlueprintAssets;
	FARFilter BlueprintFilter;
	BlueprintFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	BlueprintFilter.PackagePaths.Add(FName(TEXT("/Game/Code/GAS/Abilities")));
	BlueprintFilter.bRecursiveClasses = true;
	BlueprintFilter.bRecursivePaths = true;
	AssetRegistry.GetAssets(BlueprintFilter, BlueprintAssets);

	for (const FAssetData& Asset : BlueprintAssets)
	{
		const FString PackageName = Asset.PackageName.ToString();
		const FString AssetName = Asset.AssetName.ToString();
		const bool bLooksLikeMusketAbility =
			AssetName.Contains(TEXT("Musket"), ESearchCase::IgnoreCase) ||
			PackageName.Contains(TEXT("/Musket/"), ESearchCase::IgnoreCase);
		if (!bLooksLikeMusketAbility)
		{
			continue;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			continue;
		}

		EActDataRowKind Kind;
		FString TypeLabel;
		if (Blueprint->GeneratedClass->IsChildOf(UGA_Musket_LightAttack::StaticClass()))
		{
			Kind = EActDataRowKind::MusketLight;
			TypeLabel = TEXT("Musket Light");
		}
		else if (Blueprint->GeneratedClass->IsChildOf(UGA_Musket_HeavyAttack::StaticClass()))
		{
			Kind = EActDataRowKind::MusketHeavy;
			TypeLabel = TEXT("Musket Heavy");
		}
		else if (Blueprint->GeneratedClass->IsChildOf(UGA_Musket_SprintAttack::StaticClass()))
		{
			Kind = EActDataRowKind::MusketSprint;
			TypeLabel = TEXT("Musket Sprint");
		}
		else
		{
			continue;
		}

		TSharedRef<FActDataRow> Row = MakeShared<FActDataRow>();
		Row->Kind = Kind;
		Row->TypeLabel = TypeLabel;
		Row->SourceLabel = FString::Printf(TEXT("Ranged / Musket Act / %s"), *AssetName);
		Row->BlueprintAsset = Blueprint;
		Row->MusketCDO = Cast<UGA_MusketBase>(Blueprint->GeneratedClass->GetDefaultObject());
		Row->Status = GetMusketTuning(*Row) ? TEXT("TuningData") : TEXT("Class defaults");
		ActRows.Add(Row);
		++MusketBlueprintCount;
	}
}

void SCharacterBalanceWidget::RebuildAbilityMontageContent()
{
	if (!AbilityMontageContent.IsValid())
	{
		return;
	}

	AbilityMontageContent->ClearChildren();
	UCharacterData* Character = GetSelectedCharacter();
	UAbilityData* AbilityData = Character ? Character->AbilityData.Get() : nullptr;
	if (!AbilityData)
	{
		AbilityMontageContent->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NoAbilityData", "No AbilityData assigned."))
		];
		return;
	}

	AbilityMontageContent->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
	[
		SNew(STextBlock)
		.Text(FText::Format(LOCTEXT("AbilityDataSummary", "AbilityData: {0} | MontageMap: {1} | MontageConfigMap: {2}"),
			FText::FromString(AbilityData->GetName()),
			FText::AsNumber(AbilityData->MontageMap.Num()),
			FText::AsNumber(AbilityData->MontageConfigMap.Num())))
	];

	AbilityMontageContent->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
	[
		MakeSectionTitle(LOCTEXT("MontageMapSection", "MontageMap"))
	];
	for (const TPair<FGameplayTag, TObjectPtr<UAnimMontage>>& Pair : AbilityData->MontageMap)
	{
		AbilityMontageContent->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s -> %s"), *Pair.Key.ToString(), *ObjectPath(Pair.Value.Get()))))
			.AutoWrapText(true)
		];
	}

	AbilityMontageContent->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)
	[
		MakeSectionTitle(LOCTEXT("MontageConfigMapSection", "MontageConfigMap"))
	];
	for (const TPair<FGameplayTag, FAbilityMontageConfigList>& Pair : AbilityData->MontageConfigMap)
	{
		for (const FTaggedMontageConfig& Candidate : Pair.Value.Configs)
		{
			const UMontageConfigDA* Config = Candidate.MontageConfig.Get();
			AbilityMontageContent->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s -> %s | Priority %d | Required %s | Blocked %s"),
					*Pair.Key.ToString(),
					*ObjectPath(Config),
					Candidate.Priority,
					*TagsToString(Candidate.RequiredTags),
					*TagsToString(Candidate.BlockedTags))))
				.AutoWrapText(true)
			];
		}
	}
}

FText SCharacterBalanceWidget::GetStatsText() const
{
	return FText::Format(
		LOCTEXT("Stats", "Characters: {0} ({1} visible) | Act rows: {2} | Musket BP: {3} | {4}"),
		FText::AsNumber(Rows.Num()),
		FText::AsNumber(FilteredRows.Num()),
		FText::AsNumber(ActRows.Num()),
		FText::AsNumber(MusketBlueprintCount),
		StatusText);
}

FText SCharacterBalanceWidget::GetSelectedCharacterNameText() const
{
	UCharacterData* Character = GetSelectedCharacter();
	return Character
		? FText::FromString(FString::Printf(TEXT("%s  |  %s"), *Character->GetName(), *ObjectPath(Character)))
		: LOCTEXT("NoCharacterSelected", "No character selected");
}

FText SCharacterBalanceWidget::GetSelectedCharacterSummaryText() const
{
	UCharacterData* Character = GetSelectedCharacter();
	if (!Character)
	{
		return LOCTEXT("NoCharacterSummary", "Select a CharacterData asset.");
	}

	return FText::FromString(FString::Printf(
		TEXT("Base Row: %s (%s)\nMove Row: %s (%s)\nAbilityData: %s\nGasTemplate: %s\nDefault Anim Layers: %d\nStatus: %s"),
		*RowNameToString(Character->YogBaseAttributeDataRow),
		*ObjectPath(GetBaseTable(Character)),
		*RowNameToString(Character->MovementDataRow),
		*ObjectPath(GetMovementTable(Character)),
		*ObjectPath(Character->AbilityData.Get()),
		*ObjectPath(Character->GasTemplate.Get()),
		Character->DefaultAnimeLayers.Num(),
		SelectedRow.IsValid() ? *SelectedRow->Status : TEXT("-")));
}

FText SCharacterBalanceWidget::GetSelectedActSummaryText() const
{
	UCharacterData* Character = GetSelectedCharacter();
	if (!Character)
	{
		return LOCTEXT("NoActCharacter", "Select a character to inspect AbilityData and Act rows.");
	}
	return FText::Format(LOCTEXT("ActSummary", "{0}: {1} Act rows from AbilityData plus Ranged / Musket Act rows."),
		FText::FromString(Character->GetName()),
		FText::AsNumber(ActRows.Num()));
}

FText SCharacterBalanceWidget::GetSelectedActDetailsText() const
{
	if (!SelectedActRow.IsValid())
	{
		return LOCTEXT("NoActSelected", "Select an Act row.");
	}

	return FText::FromString(FString::Printf(
		TEXT("AbilityTag: %s\nType: %s\nSource: %s\nAttackData / Tuning: %s\nRequiredTags: %s\nBlockedTags: %s\nPriority: %d\nStatus: %s"),
		SelectedActRow->AbilityTag.IsValid() ? *SelectedActRow->AbilityTag.ToString() : TEXT("-"),
		*SelectedActRow->TypeLabel,
		*SelectedActRow->SourceLabel,
		*ActRowAttackDataName(*SelectedActRow),
		*TagsToString(SelectedActRow->RequiredTags),
		*TagsToString(SelectedActRow->BlockedTags),
		SelectedActRow->Priority,
		*SelectedActRow->Status));
}

int32 SCharacterBalanceWidget::GetActivePageIndex() const
{
	return static_cast<int32>(ActivePage);
}

FReply SCharacterBalanceWidget::OnPageButtonClicked(ECharacterWorkbenchPage Page)
{
	ActivePage = Page;
	return FReply::Handled();
}

FReply SCharacterBalanceWidget::OnRefreshClicked()
{
	RefreshData(LOCTEXT("Refreshed", "Character Data Workbench refreshed."));
	return FReply::Handled();
}

FReply SCharacterBalanceWidget::OnValidateClicked()
{
	int32 Errors = 0;
	int32 Warnings = 0;
	TSet<FString> SeenBaseRows;
	for (const FCharacterRowPtr& Row : Rows)
	{
		UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
		if (!Character)
		{
			continue;
		}

		if (!GetBaseRow(Character))
		{
			++Errors;
			UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: missing base row on %s"), *GetNameSafe(Character));
		}
		if (!GetMovementRow(Character))
		{
			++Warnings;
			UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: missing movement row on %s"), *GetNameSafe(Character));
		}
		if (!Character->AbilityData)
		{
			++Warnings;
			UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: missing AbilityData on %s"), *GetNameSafe(Character));
		}

		const FString BaseKey = FString::Printf(TEXT("%s:%s"), *ObjectPath(GetBaseTable(Character)), *RowNameToString(Character->YogBaseAttributeDataRow));
		if (SeenBaseRows.Contains(BaseKey))
		{
			++Warnings;
			UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: shared base row %s"), *BaseKey);
		}
		SeenBaseRows.Add(BaseKey);

		UAbilityData* AbilityData = Character->AbilityData.Get();
		if (!AbilityData)
		{
			continue;
		}

		TSet<FGameplayTag> AbilityTags;
		for (const TPair<FGameplayTag, TObjectPtr<UAnimMontage>>& Pair : AbilityData->MontageMap)
		{
			AbilityTags.Add(Pair.Key);
			if (!Pair.Value)
			{
				++Errors;
				UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: %s MontageMap %s has no montage"), *GetNameSafe(Character), *Pair.Key.ToString());
			}
		}
		for (const TPair<FGameplayTag, FAbilityMontageConfigList>& Pair : AbilityData->MontageConfigMap)
		{
			AbilityTags.Add(Pair.Key);
			if (Pair.Value.Configs.IsEmpty())
			{
				++Warnings;
				UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: %s MontageConfigMap %s has no configs"), *GetNameSafe(Character), *Pair.Key.ToString());
			}
			for (const FTaggedMontageConfig& Candidate : Pair.Value.Configs)
			{
				UMontageConfigDA* Config = Candidate.MontageConfig.Get();
				if (!Config)
				{
					++Errors;
					UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: %s MontageConfigMap %s has a missing MontageConfig"), *GetNameSafe(Character), *Pair.Key.ToString());
					continue;
				}
				if (!Config->Montage)
				{
					++Warnings;
					UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: %s MontageConfig %s has no montage"), *GetNameSafe(Character), *GetNameSafe(Config));
				}
				for (UMontageNotifyEntry* Entry : Config->Entries)
				{
					if (UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Entry))
					{
						if (HitWindow->StartFrame > HitWindow->EndFrame)
						{
							++Errors;
							UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: invalid HitWindow frames on %s"), *GetNameSafe(Config));
						}
						if (HitWindow->AttackDataCandidates.IsEmpty())
						{
							++Warnings;
							UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: HitWindow on %s has no AttackData"), *GetNameSafe(Config));
						}
						for (const FTaggedMontageAttackData& AttackCandidate : HitWindow->AttackDataCandidates)
						{
							if (!AttackCandidate.AttackData)
							{
								++Errors;
								UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: HitWindow on %s has missing AttackData candidate"), *GetNameSafe(Config));
							}
						}
					}
				}
			}
		}

		for (const FGameplayTag& AbilityTag : AbilityTags)
		{
			const bool bHasMontage = AbilityData->MontageMap.Contains(AbilityTag) && AbilityData->MontageMap.FindRef(AbilityTag) != nullptr;
			const FAbilityMontageConfigList* ConfigList = AbilityData->MontageConfigMap.Find(AbilityTag);
			const bool bHasConfig = ConfigList && ConfigList->Configs.ContainsByPredicate([](const FTaggedMontageConfig& Candidate)
			{
				return Candidate.MontageConfig != nullptr;
			});
			if (!bHasMontage && !bHasConfig)
			{
				++Errors;
				UE_LOG(LogTemp, Warning, TEXT("CharacterWorkbench: %s AbilityTag %s has no valid Montage or MontageConfig"), *GetNameSafe(Character), *AbilityTag.ToString());
			}
		}
	}

	StatusText = FText::Format(LOCTEXT("ValidateStatus", "Validate All: {0} errors, {1} warnings. See Output Log."),
		FText::AsNumber(Errors),
		FText::AsNumber(Warnings));
	return FReply::Handled();
}

FReply SCharacterBalanceWidget::OnExportCsvClicked()
{
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString BalanceDir = FPaths::ProjectSavedDir() / TEXT("Balance");
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*BalanceDir);
	const FString CharacterPath = BalanceDir / FString::Printf(TEXT("CharacterDataWorkbench_%s.csv"), *Stamp);
	const FString ActPath = BalanceDir / FString::Printf(TEXT("CharacterActData_%s.csv"), *Stamp);

	TArray<FCharacterFieldDef> AllFields = GetBaseFieldDefs();
	AllFields.Append(GetMovementFieldDefs());

	FString CharacterCsv;
	CharacterCsv += TEXT("角色资产,Base Row,Move Row,AbilityData,GasTemplate,Status");
	for (const FCharacterFieldDef& Field : AllFields)
	{
		CharacterCsv += FString::Printf(TEXT(",%s"), *CsvEscape(Field.ChineseName));
	}
	CharacterCsv += TEXT("\n");

	CharacterCsv += TEXT("[s]Asset,[s]BaseRow,[s]MoveRow,[s]AbilityData,[s]GasTemplate,[s]Status");
	for (const FCharacterFieldDef& Field : AllFields)
	{
		CharacterCsv += FString::Printf(TEXT(",%s"), *CsvEscape(Field.TypeToken));
	}
	CharacterCsv += TEXT("\n");

	CharacterCsv += TEXT("client,all,all,all,all,client");
	for (const FCharacterFieldDef& Field : AllFields)
	{
		CharacterCsv += FString::Printf(TEXT(",%s"), *CsvEscape(Field.Platform));
	}
	CharacterCsv += TEXT("\n");

	CharacterCsv += TEXT("0,0,0,0,0,0");
	for (const FCharacterFieldDef& Field : AllFields)
	{
		CharacterCsv += FString::Printf(TEXT(",%.3f"), Field.DefaultValue);
	}
	CharacterCsv += TEXT("\n");

	for (const FCharacterRowPtr& Row : Rows)
	{
		UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
		if (!Character)
		{
			continue;
		}
		CharacterCsv += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s"),
			*CsvEscape(Character->GetName()),
			*CsvEscape(RowNameToString(Character->YogBaseAttributeDataRow)),
			*CsvEscape(RowNameToString(Character->MovementDataRow)),
			*CsvEscape(ObjectPath(Character->AbilityData.Get())),
			*CsvEscape(ObjectPath(Character->GasTemplate.Get())),
			*CsvEscape(Row->Status));
		for (const FCharacterFieldDef& Field : AllFields)
		{
			CharacterCsv += FString::Printf(TEXT(",%.3f"), ReadCharacterFloat(Character, Field.ColumnName));
		}
		CharacterCsv += TEXT("\n");
	}

	FString ActCsv;
	ActCsv += TEXT("角色资产,AbilityTag,类型,来源,MontageConfig,Montage,Entry,AttackData,RequiredTags,BlockedTags,Priority,StartFrame,EndFrame,Frame,BlendTime,ActDamage,ActRange,ActResilience,ActDmgReduce,Status\n");
	ActCsv += TEXT("[s]Character,[s]AbilityTag,[s]Type,[s]Source,[s]MontageConfig,[s]Montage,[s]Entry,[s]AttackData,[s]RequiredTags,[s]BlockedTags,[i]Priority,[f]StartFrame,[f]EndFrame,[f]Frame,[f]BlendTime,[f]ActDamage,[f]ActRange,[f]ActResilience,[f]ActDmgReduce,[s]Status\n");
	ActCsv += TEXT("client,all,client,client,all,all,all,all,all,all,all,all,all,all,all,all,all,all,all,client\n");
	ActCsv += TEXT("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n");
	for (const FActRowPtr& Row : ActRows)
	{
		if (!Row.IsValid())
		{
			continue;
		}
		ActCsv += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%s\n"),
			*CsvEscape(ObjectName(Row->Character.Get())),
			*CsvEscape(Row->AbilityTag.IsValid() ? Row->AbilityTag.ToString() : TEXT("-")),
			*CsvEscape(Row->TypeLabel),
			*CsvEscape(Row->SourceLabel),
			*CsvEscape(ObjectPath(Row->MontageConfig.Get())),
			*CsvEscape(ObjectPath(Row->Montage.Get())),
			*CsvEscape(EntryLabel(Row->MontageEntry.Get())),
			*CsvEscape(ObjectPath(Row->AttackData.Get())),
			*CsvEscape(TagsToString(Row->RequiredTags)),
			*CsvEscape(TagsToString(Row->BlockedTags)),
			Row->Priority,
			ReadActFloat(*Row, TEXT("StartFrame")),
			ReadActFloat(*Row, TEXT("EndFrame")),
			ReadActFloat(*Row, TEXT("Frame")),
			ReadActFloat(*Row, TEXT("BlendTime")),
			ReadActFloat(*Row, TEXT("ActDamage")),
			ReadActFloat(*Row, TEXT("ActRange")),
			ReadActFloat(*Row, TEXT("ActResilience")),
			ReadActFloat(*Row, TEXT("ActDmgReduce")),
			*CsvEscape(Row->Status));
	}

	FFileHelper::SaveStringToFile(CharacterCsv, *CharacterPath, FFileHelper::EEncodingOptions::ForceUTF8);
	FFileHelper::SaveStringToFile(ActCsv, *ActPath, FFileHelper::EEncodingOptions::ForceUTF8);
	StatusText = FText::Format(LOCTEXT("Exported", "Exported {0} and {1}"),
		FText::FromString(FPaths::GetCleanFilename(CharacterPath)),
		FText::FromString(FPaths::GetCleanFilename(ActPath)));
	return FReply::Handled();
}

void SCharacterBalanceWidget::OpenAsset(TSharedPtr<FCharacterWorkbenchRow> Row) const
{
	OpenObject(Row.IsValid() ? Row->Asset.Get() : nullptr);
}

void SCharacterBalanceWidget::OpenBaseTable(TSharedPtr<FCharacterWorkbenchRow> Row) const
{
	UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
	OpenObject(GetBaseTable(Character));
}

void SCharacterBalanceWidget::OpenMovementTable(TSharedPtr<FCharacterWorkbenchRow> Row) const
{
	UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
	OpenObject(GetMovementTable(Character));
}

void SCharacterBalanceWidget::OpenSelectedAbilityData() const
{
	UCharacterData* Character = GetSelectedCharacter();
	OpenObject(Character ? Character->AbilityData.Get() : nullptr);
}

void SCharacterBalanceWidget::OpenSelectedGasTemplate() const
{
	UCharacterData* Character = GetSelectedCharacter();
	OpenObject(Character ? Character->GasTemplate.Get() : nullptr);
}

void SCharacterBalanceWidget::OpenActPrimaryAsset(TSharedPtr<FActDataRow> Row) const
{
	if (!Row.IsValid())
	{
		return;
	}

	if (UMontageAttackDataAsset* AttackData = Row->AttackData.Get())
	{
		OpenObject(AttackData);
		return;
	}
	if (UMusketActionTuningDataAsset* Tuning = GetMusketTuning(*Row))
	{
		OpenObject(Tuning);
		return;
	}
	if (Row->MontageConfig.IsValid())
	{
		OpenObject(Row->MontageConfig.Get());
		return;
	}
	if (Row->Montage.IsValid())
	{
		OpenObject(Row->Montage.Get());
		return;
	}
	if (Row->BlueprintAsset.IsValid())
	{
		OpenObject(Row->BlueprintAsset.Get());
	}
}

void SCharacterBalanceWidget::OpenObject(UObject* Object) const
{
	if (GEditor && Object)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(Object);
		}
	}
}

UObject* SCharacterBalanceWidget::GetActDetailsObject(const FActDataRow& Row) const
{
	if (Row.AttackData.IsValid())
	{
		return Row.AttackData.Get();
	}
	if (UMusketActionTuningDataAsset* Tuning = GetMusketTuning(Row))
	{
		return Tuning;
	}
	if (Row.MontageEntry.IsValid())
	{
		return Row.MontageEntry.Get();
	}
	if (Row.DamageNotify.IsValid())
	{
		return Row.DamageNotify.Get();
	}
	if (Row.MusketCDO.IsValid())
	{
		return Row.MusketCDO.Get();
	}
	return Row.MontageConfig.Get();
}

bool SCharacterBalanceWidget::CanEditActField(const FActDataRow& Row, FName ColumnName) const
{
	if (ColumnName == TEXT("ActDamage") || ColumnName == TEXT("ActRange") || ColumnName == TEXT("ActResilience") || ColumnName == TEXT("ActDmgReduce"))
	{
		return Row.AttackData.IsValid();
	}
	if (ColumnName == TEXT("StartFrame") || ColumnName == TEXT("EndFrame"))
	{
		return Cast<UMNE_HitWindow>(Row.MontageEntry.Get())
			|| Cast<UMNE_ComboWindow>(Row.MontageEntry.Get())
			|| Cast<UMNE_TagWindow>(Row.MontageEntry.Get());
	}
	if (ColumnName == TEXT("Frame"))
	{
		return Cast<UMNE_EarlyExit>(Row.MontageEntry.Get()) || Cast<UMNE_GameplayEvent>(Row.MontageEntry.Get());
	}
	if (ColumnName == TEXT("BlendTime"))
	{
		return Cast<UMNE_EarlyExit>(Row.MontageEntry.Get()) != nullptr;
	}
	if (Row.MusketCDO.IsValid())
	{
		if (Row.Kind == EActDataRowKind::MusketLight)
		{
			return ColumnName == TEXT("LightDamageMultiplier") || ColumnName == TEXT("LightHalfAngleDeg");
		}
		if (Row.Kind == EActDataRowKind::MusketHeavy)
		{
			return ColumnName == TEXT("HeavyChargeTime")
				|| ColumnName == TEXT("HeavyStartHalfAngle")
				|| ColumnName == TEXT("HeavyEndHalfAngle")
				|| ColumnName == TEXT("HeavyStartRadius")
				|| ColumnName == TEXT("HeavyEndRadius")
				|| ColumnName == TEXT("HeavyBaseDamageMultiplier")
				|| ColumnName == TEXT("HeavyFullChargeMultiplier");
		}
		if (Row.Kind == EActDataRowKind::MusketSprint)
		{
			return ColumnName == TEXT("SprintDamageMultiplier") || ColumnName == TEXT("SprintHalfFanAngle");
		}
	}
	return false;
}

void SCharacterBalanceWidget::CommitFloat(TSharedPtr<FCharacterWorkbenchRow> Row, FName ColumnName, float NewValue)
{
	UCharacterData* Character = Row.IsValid() ? Row->Asset.Get() : nullptr;
	if (!Character)
	{
		return;
	}

	UDataTable* DataTable = IsBaseColumn(ColumnName) ? GetBaseTable(Character) : GetMovementTable(Character);
	const FScopedTransaction Transaction(LOCTEXT("EditCharacterBalance", "Edit Character Workbench Value"));
	if (DataTable)
	{
		DataTable->Modify();
	}
	Character->Modify();

	if (WriteCharacterFloat(Character, ColumnName, NewValue))
	{
		Row->Status = CharacterRowStatus(Character);
		StatusText = FText::Format(LOCTEXT("CommitStatus", "Set {0}.{1} = {2}. DataTable dirty, not auto-saved."),
			FText::FromString(Character->GetName()), FText::FromName(ColumnName), FText::AsNumber(NewValue));
		if (CharacterListView.IsValid())
		{
			CharacterListView->RequestListRefresh();
		}
	}
}

void SCharacterBalanceWidget::CommitActFloat(TSharedPtr<FActDataRow> Row, FName ColumnName, float NewValue)
{
	if (!Row.IsValid() || !CanEditActField(*Row, ColumnName))
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("EditActBalance", "Edit Character Act Value"));
	if (Row->AttackData.IsValid())
	{
		Row->AttackData->Modify();
	}
	if (Row->MontageEntry.IsValid())
	{
		Row->MontageEntry->Modify();
	}
	if (Row->MontageConfig.IsValid())
	{
		Row->MontageConfig->Modify();
	}
	if (UMusketActionTuningDataAsset* Tuning = GetMusketTuning(*Row))
	{
		Tuning->Modify();
	}
	if (Row->MusketCDO.IsValid())
	{
		Row->MusketCDO->Modify();
	}
	if (Row->BlueprintAsset.IsValid())
	{
		Row->BlueprintAsset->Modify();
	}

	if (WriteActFloat(*Row, ColumnName, NewValue))
	{
		StatusText = FText::Format(LOCTEXT("CommitActStatus", "Set {0}.{1} = {2}. Asset dirty, not auto-saved."),
			FText::FromString(Row->SourceLabel), FText::FromName(ColumnName), FText::AsNumber(NewValue));
		if (ActListView.IsValid())
		{
			ActListView->RequestListRefresh();
		}
	}
}

void SCharacterBalanceWidget::CreateAttackDataForActRow(TSharedPtr<FActDataRow> Row)
{
	if (!Row.IsValid())
	{
		return;
	}

	UMNE_HitWindow* HitWindow = Cast<UMNE_HitWindow>(Row->MontageEntry.Get());
	UMontageConfigDA* Config = Row->MontageConfig.Get();
	if (!HitWindow || !Config)
	{
		return;
	}

	FString PackageName;
	FString AssetName;
	const FString BasePackageName = FString::Printf(
		TEXT("/Game/Docs/Data/Melee/AttackData/DA_%s_%s_AttackData"),
		*SanitizeForAssetName(Config->GetName()),
		*SanitizeForAssetName(Row->AbilityTag.IsValid() ? Row->AbilityTag.ToString() : TEXT("Act")));
	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateUniqueAssetName(BasePackageName, TEXT(""), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	UMontageAttackDataAsset* NewAsset = NewObject<UMontageAttackDataAsset>(Package, UMontageAttackDataAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	FAssetRegistryModule::AssetCreated(NewAsset);

	const FScopedTransaction Transaction(LOCTEXT("CreateAttackDataTx", "Create Montage Attack Data"));
	Config->Modify();
	HitWindow->Modify();
	FTaggedMontageAttackData Candidate;
	Candidate.AttackData = NewAsset;
	HitWindow->AttackDataCandidates.Add(Candidate);
	Config->MarkPackageDirty();
	NewAsset->MarkPackageDirty();

	StatusText = FText::Format(LOCTEXT("CreatedAttackData", "Created {0} and assigned it to {1}. Save assets when ready."),
		FText::FromString(AssetName), FText::FromString(Config->GetName()));
	CollectActRowsForSelectedCharacter();
}

void SCharacterBalanceWidget::CreateMusketTuningData(TSharedPtr<FActDataRow> Row)
{
	if (!Row.IsValid() || !Row->MusketCDO.IsValid())
	{
		return;
	}

	const FString SourceName = Row->BlueprintAsset.IsValid() ? Row->BlueprintAsset->GetName() : Row->SourceLabel;
	FString PackageName;
	FString AssetName;
	const FString BasePackageName = FString::Printf(TEXT("/Game/Docs/Data/Musket/Tuning/DA_%s_Tuning"), *SanitizeForAssetName(SourceName));
	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateUniqueAssetName(BasePackageName, TEXT(""), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	UMusketActionTuningDataAsset* NewAsset = NewObject<UMusketActionTuningDataAsset>(Package, UMusketActionTuningDataAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);

	if (UGA_Musket_LightAttack* Light = Cast<UGA_Musket_LightAttack>(Row->MusketCDO.Get()))
	{
		NewAsset->LightDamageMultiplier = Light->DamageMultiplier;
		NewAsset->LightHalfAngleDeg = Light->HalfAngleDeg;
	}
	if (UGA_Musket_HeavyAttack* Heavy = Cast<UGA_Musket_HeavyAttack>(Row->MusketCDO.Get()))
	{
		NewAsset->HeavyChargeTime = Heavy->ChargeTime;
		NewAsset->HeavyStartHalfAngle = Heavy->StartHalfAngle;
		NewAsset->HeavyEndHalfAngle = Heavy->EndHalfAngle;
		NewAsset->HeavyStartRadius = Heavy->StartRadius;
		NewAsset->HeavyEndRadius = Heavy->EndRadius;
		NewAsset->HeavyBaseDamageMultiplier = Heavy->BaseDamageMultiplier;
		NewAsset->HeavyFullChargeMultiplier = Heavy->FullChargeMultiplier;
	}
	if (UGA_Musket_SprintAttack* Sprint = Cast<UGA_Musket_SprintAttack>(Row->MusketCDO.Get()))
	{
		NewAsset->SprintDamageMultiplier = Sprint->DamageMultiplier;
		NewAsset->SprintHalfFanAngle = Sprint->HalfFanAngle;
	}

	FAssetRegistryModule::AssetCreated(NewAsset);

	const FScopedTransaction Transaction(LOCTEXT("CreateMusketTuningTx", "Create Musket Tuning Data"));
	Row->MusketCDO->Modify();
	Row->MusketCDO->TuningData = NewAsset;
	if (Row->BlueprintAsset.IsValid())
	{
		Row->BlueprintAsset->Modify();
		Row->BlueprintAsset->MarkPackageDirty();
	}
	NewAsset->MarkPackageDirty();

	StatusText = FText::Format(LOCTEXT("CreatedMusketTuning", "Created {0} and assigned it to {1}. Save assets when ready."),
		FText::FromString(AssetName), FText::FromString(SourceName));
	CollectActRowsForSelectedCharacter();
}

UCharacterData* SCharacterBalanceWidget::GetSelectedCharacter() const
{
	return SelectedRow.IsValid() ? SelectedRow->Asset.Get() : nullptr;
}

#undef LOCTEXT_NAMESPACE
