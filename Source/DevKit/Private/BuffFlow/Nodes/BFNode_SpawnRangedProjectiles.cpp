#include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"

#include "AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h"
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
}

void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(ResolveTarget(SourceSelector));
	if (!SourceCharacter || !SourceCharacter->GetWorld())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC || !BFC->HasCombatCardEffectContext())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const FCombatCardEffectContext& CardContext = BFC->GetLastCombatCardEffectContext();
	const FCombatDeckActionContext& ActionContext = CardContext.ActionContext;

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

	TSubclassOf<AMusketBullet> ResolvedBulletClass = ActionContext.RangedProjectileClass
		? ActionContext.RangedProjectileClass
		: BulletClass;
	if (!ResolvedBulletClass)
	{
		ResolvedBulletClass = AMusketBullet::StaticClass();
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffect = ActionContext.RangedDamageEffectClass
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
	for (const float YawOffset : YawOffsets)
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
