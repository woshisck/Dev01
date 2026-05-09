#include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"

#include "AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Projectile/MusketBullet.h"
#include "Types/FlowDataPinResults.h"

UBFNode_SpawnRangedProjectiles::UBFNode_SpawnRangedProjectiles(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Projectile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
	YawOffsets = { -8.f, 8.f };
	Damage = FFlowDataPinInputProperty_Float(0.f);
	HitGameplayEventMagnitude = FFlowDataPinInputProperty_Float(0.f);
	RequiredWeaponTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
}

void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(ResolveTarget(SourceSelector));
	if (!SourceCharacter || !SourceCharacter->GetWorld())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	if (bRequireRangedWeaponTag)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
		const FGameplayTag RangedTag = RequiredWeaponTag.IsValid()
			? RequiredWeaponTag
			: FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
		if (!ASC || !RangedTag.IsValid() || !ASC->HasMatchingGameplayTag(RangedTag))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[SpawnRangedProjectiles] Skip: Source=%s missing required weapon tag %s."),
				*GetNameSafe(SourceCharacter),
				*RangedTag.ToString());
			TriggerOutput(TEXT("Failed"), true);
			return;
		}
	}

	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC || !BFC->HasCombatCardEffectContext())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const FCombatCardEffectContext& CardContext = BFC->GetLastCombatCardEffectContext();
	const FCombatDeckActionContext& ActionContext = CardContext.ActionContext;
	// 解析数量 / 锥角数据引脚（若已连线则覆盖节点字段值）
	int32 ResolvedProjectileCount = ProjectileCount;
	{
		const FFlowDataPinResult_Int CountResult = TryResolveDataPinAsInt(
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRangedProjectiles, ProjectileCountPin));
		if (CountResult.Result == EFlowDataPinResolveResult::Success && CountResult.Value > 0)
		{
			ResolvedProjectileCount = static_cast<int32>(CountResult.Value);
		}
	}
	float ResolvedConeAngle = ProjectileConeAngleDegrees;
	{
		const FFlowDataPinResult_Float ConeResult = TryResolveDataPinAsFloat(
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRangedProjectiles, ProjectileConeAnglePinDegrees));
		if (ConeResult.Result == EFlowDataPinResolveResult::Success)
		{
			ResolvedConeAngle = ConeResult.Value;
		}
	}
	const TArray<float> ResolvedYawOffsets = BuildResolvedYawOffsets(CardContext.ComboBonusStacks, ResolvedProjectileCount, ResolvedConeAngle);

	float ResolvedDamage = ActionContext.AttackDamage;
	if (!bUseCombatCardAttackDamage || ResolvedDamage <= 0.f)
	{
		ResolvedDamage = Damage.Value;
		const FFlowDataPinResult_Float DamageResult = TryResolveDataPinAsFloat(
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRangedProjectiles, Damage));
		if (DamageResult.Result == EFlowDataPinResolveResult::Success)
		{
			ResolvedDamage = DamageResult.Value;
		}
	}

	TSubclassOf<AMusketBullet> ResolvedBulletClass = bPreferCombatCardProjectileClass && ActionContext.RangedProjectileClass
		? ActionContext.RangedProjectileClass
		: BulletClass;
	if (!ResolvedBulletClass)
	{
		ResolvedBulletClass = AMusketBullet::StaticClass();
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffect = bPreferCombatCardDamageEffectClass && ActionContext.RangedDamageEffectClass
		? ActionContext.RangedDamageEffectClass
		: DamageEffectClass;
	if (!ResolvedDamageEffect)
	{
		ResolvedDamageEffect = UGE_MusketBullet_Damage::StaticClass();
	}

	const FVector SpawnLocation = ResolveMuzzleLocation(SourceCharacter);
	const float BaseYaw = SourceCharacter->GetActorRotation().Yaw + ActionContext.RangedBaseYawOffsetDeg;
	const FGuid SharedGuid = bShareAttackInstanceGuid ? ActionContext.AttackInstanceGuid : FGuid();

	FActorSpawnParameters Params;
	Params.Instigator = Cast<APawn>(SourceCharacter);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	int32 SpawnedCount = 0;
	float ResolvedHitEventMagnitude = HitGameplayEventMagnitude.Value;
	if (HitGameplayEventTag.IsValid() && !bUseDamageAsHitGameplayEventMagnitude)
	{
		const FFlowDataPinResult_Float EventMagnitudeResult = TryResolveDataPinAsFloat(
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRangedProjectiles, HitGameplayEventMagnitude));
		if (EventMagnitudeResult.Result == EFlowDataPinResolveResult::Success)
		{
			ResolvedHitEventMagnitude = EventMagnitudeResult.Value;
		}
	}

	for (const float YawOffset : ResolvedYawOffsets)
	{
		const FRotator SpawnRotation(0.f, BaseYaw + YawOffset, 0.f);
		AMusketBullet* Bullet = SourceCharacter->GetWorld()->SpawnActor<AMusketBullet>(
			ResolvedBulletClass,
			SpawnLocation,
			SpawnRotation,
			Params);

		if (!Bullet)
		{
			continue;
		}

		Bullet->InitBullet(SourceCharacter, ResolvedDamage, ResolvedDamageEffect);
		Bullet->SetCombatDeckContextWithGuid(
			ActionContext.ActionType,
			ActionContext.bIsComboFinisher,
			ActionContext.bFromDashSave,
			SharedGuid,
			ResolvedDamage);
		Bullet->SetHitGameplayEvent(
			HitGameplayEventTag,
			bSendHitGameplayEventToSourceASC,
			bUseDamageAsHitGameplayEventMagnitude,
			ResolvedHitEventMagnitude);
		++SpawnedCount;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[SpawnRangedProjectiles] Source=%s Count=%d Damage=%.1f BaseYaw=%.1f Guid=%s Bullet=%s"),
		*GetNameSafe(SourceCharacter),
		SpawnedCount,
		ResolvedDamage,
		BaseYaw,
		*SharedGuid.ToString(),
		*GetNameSafe(ResolvedBulletClass.Get()));

	TriggerOutput(SpawnedCount > 0 ? TEXT("Out") : TEXT("Failed"), true);
}

TArray<float> UBFNode_SpawnRangedProjectiles::BuildResolvedYawOffsets(int32 ComboBonusStacks, int32 OverrideCount, float OverrideConeAngle) const
{
	int32 BonusProjectileCount = 0;
	if (bAddComboStacksToProjectileCount)
	{
		BonusProjectileCount = FMath::Min(
			FMath::Max(0, MaxBonusProjectiles),
			FMath::Max(0, ComboBonusStacks) * FMath::Max(0, ProjectilesPerComboStack));
	}

	TArray<float> Result;
	if (!bUseProjectileCountPattern && BonusProjectileCount <= 0)
	{
		Result = YawOffsets;
		if (Result.Num() == 0)
		{
			Result.Add(0.f);
		}
		return Result;
	}

	const int32 BaseProjectileCount = bUseProjectileCountPattern
		? FMath::Max(1, OverrideCount)
		: FMath::Max(1, YawOffsets.Num());
	const int32 FinalProjectileCount = FMath::Max(1, BaseProjectileCount + BonusProjectileCount);
	Result.Reserve(FinalProjectileCount);

	if (FinalProjectileCount == 1 || OverrideConeAngle <= KINDA_SMALL_NUMBER)
	{
		for (int32 Index = 0; Index < FinalProjectileCount; ++Index)
		{
			Result.Add(0.f);
		}
		return Result;
	}

	const float Step = OverrideConeAngle / static_cast<float>(FinalProjectileCount - 1);
	const float StartYaw = -OverrideConeAngle * 0.5f;
	for (int32 Index = 0; Index < FinalProjectileCount; ++Index)
	{
		Result.Add(StartYaw + Step * static_cast<float>(Index));
	}
	return Result;
}

FVector UBFNode_SpawnRangedProjectiles::ResolveMuzzleLocation(ACharacter* SourceCharacter) const
{
	if (!SourceCharacter)
	{
		return FVector::ZeroVector;
	}

	if (USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
	{
		if (Mesh->DoesSocketExist(MuzzleSocketName))
		{
			return Mesh->GetSocketLocation(MuzzleSocketName);
		}
	}

	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(SourceCharacter))
	{
		if (Player->EquippedWeaponInstance)
		{
			TArray<USkeletalMeshComponent*> Meshes;
			Player->EquippedWeaponInstance->GetComponents<USkeletalMeshComponent>(Meshes);
			for (USkeletalMeshComponent* WeaponMesh : Meshes)
			{
				if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
				{
					return WeaponMesh->GetSocketLocation(MuzzleSocketName);
				}
			}
		}
	}

	return SourceCharacter->GetActorLocation()
		+ SourceCharacter->GetActorForwardVector() * 80.f
		+ FVector(0.f, 0.f, 60.f);
}
