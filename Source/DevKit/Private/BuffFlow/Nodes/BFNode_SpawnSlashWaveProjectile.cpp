#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"

#include "GameFramework/Character.h"
#include "Projectile/SlashWaveProjectile.h"

UBFNode_SpawnSlashWaveProjectile::UBFNode_SpawnSlashWaveProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Projectile");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SpawnSlashWaveProjectile::ExecuteInput(const FName& PinName)
{
	if (!ProjectileClass)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* SourceActor = ResolveTarget(SourceSelector);
	ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
	if (!SourceCharacter || !SourceCharacter->GetWorld())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	const FVector Forward = SourceCharacter->GetActorForwardVector();
	const FVector Right = SourceCharacter->GetActorRightVector();
	const FVector Up = FVector::UpVector;
	const FVector SpawnLocation = SourceCharacter->GetActorLocation()
		+ Forward * SpawnOffset.X
		+ Right * SpawnOffset.Y
		+ Up * SpawnOffset.Z;
	const FRotator SpawnRotation = Forward.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceCharacter;
	SpawnParams.Instigator = SourceCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASlashWaveProjectile* Projectile = SourceCharacter->GetWorld()->SpawnActor<ASlashWaveProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);

	if (!Projectile)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	Projectile->InitProjectileAdvanced(
		SourceCharacter,
		Damage,
		DamageEffect,
		Speed,
		MaxDistance,
		MaxHitCount,
		CollisionBoxExtent);

	TriggerOutput(TEXT("Out"), true);
}
