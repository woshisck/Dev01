#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystem/Abilities/GA_ActiveSkill_ShieldBurst.h"
#include "AbilitySystem/Abilities/GA_HitReaction.h"
#include "AbilitySystem/Abilities/GA_Knockback.h"
#include "AbilitySystem/Abilities/GA_KnockbackDebuff.h"
#include "AbilitySystem/Abilities/GA_Rend.h"
#include "Character/PlayerCharacterBase.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Projectile/SlashWaveProjectile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatResponseAbilitiesBlockWhileDeadTest,
	"DevKit.Knockback.CombatResponseAbilitiesBlockWhileDead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatResponseAbilitiesBlockWhileDeadTest::RunTest(const FString& Parameters)
{
	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead"));

	UGA_Knockback* KnockbackAbility = NewObject<UGA_Knockback>();
	UGA_HitReaction* HitReactionAbility = NewObject<UGA_HitReaction>();
	UGA_Rend* RendAbility = NewObject<UGA_Rend>();

	TestTrue(TEXT("Knockback cannot activate while dead"),
		KnockbackAbility->GetActivationBlockedTags().HasTagExact(DeadTag));
	TestTrue(TEXT("Hit reaction cannot activate while dead"),
		HitReactionAbility->GetActivationBlockedTags().HasTagExact(DeadTag));
	TestTrue(TEXT("Rend cannot activate while dead"),
		RendAbility->GetActivationBlockedTags().HasTagExact(DeadTag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackUsesExplicitAttackDirectionTest,
	"DevKit.Knockback.UsesExplicitAttackDirectionTargetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockbackUsesExplicitAttackDirectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for knockback direction test"), World);
	if (!World)
	{
		return false;
	}

	ACharacter* Target = World->SpawnActor<ACharacter>();
	ACharacter* Attacker = World->SpawnActor<ACharacter>();
	TestNotNull(TEXT("Target character spawned"), Target);
	TestNotNull(TEXT("Attacker character spawned"), Attacker);
	if (!Target || !Attacker)
	{
		if (Target)
		{
			Target->Destroy();
		}
		if (Attacker)
		{
			Attacker->Destroy();
		}
		return false;
	}

	Target->SetActorLocation(FVector::ZeroVector);
	Target->SetActorRotation(FRotator::ZeroRotator);
	Attacker->SetActorLocation(FVector::ZeroVector);
	Attacker->SetActorRotation(FRotator(0.f, 90.f, 0.f));

	FGameplayEventData Payload;
	Payload.Instigator = Attacker;
	Payload.Target = Target;

	FGameplayAbilityTargetData_LocationInfo* DirectionData = new FGameplayAbilityTargetData_LocationInfo();
	DirectionData->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	DirectionData->SourceLocation.LiteralTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	DirectionData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	DirectionData->TargetLocation.LiteralTransform = FTransform(FRotator(0.f, 90.f, 0.f), FVector::YAxisVector);
	Payload.TargetData.Add(DirectionData);

	const FVector Direction = UGA_Knockback::ResolveKnockbackDirection(Target, &Payload);

	TestTrue(TEXT("Knockback follows the explicit attack direction"),
		FVector::DotProduct(Direction, FVector::YAxisVector) > 0.99f);
	TestTrue(TEXT("Knockback should not fall back to the target's X-axis facing"),
		FMath::Abs(Direction.X) < 0.01f);

	Target->Destroy();
	Attacker->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackFallsBackToInstigatorPositionTest,
	"DevKit.Knockback.FallsBackToInstigatorPositionWithoutExplicitDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockbackFallsBackToInstigatorPositionTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for knockback fallback test"), World);
	if (!World)
	{
		return false;
	}

	ACharacter* Target = World->SpawnActor<ACharacter>();
	ACharacter* Attacker = World->SpawnActor<ACharacter>();
	TestNotNull(TEXT("Target character spawned"), Target);
	TestNotNull(TEXT("Attacker character spawned"), Attacker);
	if (!Target || !Attacker)
	{
		if (Target)
		{
			Target->Destroy();
		}
		if (Attacker)
		{
			Attacker->Destroy();
		}
		return false;
	}

	Target->SetActorLocation(FVector(100.f, 0.f, 0.f));
	Target->SetActorRotation(FRotator::ZeroRotator);
	Attacker->SetActorLocation(FVector::ZeroVector);
	Attacker->SetActorRotation(FRotator(0.f, 90.f, 0.f));

	FGameplayEventData Payload;
	Payload.Instigator = Attacker;
	Payload.Target = Target;

	const FVector Direction = UGA_Knockback::ResolveKnockbackDirection(Target, &Payload);
	TestTrue(TEXT("Fallback stays based on instigator-to-target position"),
		FVector::DotProduct(Direction, FVector::XAxisVector) > 0.99f);

	Target->Destroy();
	Attacker->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackDebuffForwardsAttackDirectionTest,
	"DevKit.Knockback.DebuffForwardsDamageInstigatorAttackDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockbackDebuffForwardsAttackDirectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for knockback debuff direction test"), World);
	if (!World)
	{
		return false;
	}

	ACharacter* Target = World->SpawnActor<ACharacter>();
	ACharacter* Attacker = World->SpawnActor<ACharacter>();
	TestNotNull(TEXT("Target character spawned"), Target);
	TestNotNull(TEXT("Attacker character spawned"), Attacker);
	if (!Target || !Attacker)
	{
		if (Target)
		{
			Target->Destroy();
		}
		if (Attacker)
		{
			Attacker->Destroy();
		}
		return false;
	}

	Target->SetActorLocation(FVector::ZeroVector);
	Target->SetActorRotation(FRotator::ZeroRotator);
	Attacker->SetActorLocation(FVector::ZeroVector);
	Attacker->SetActorRotation(FRotator(0.f, 90.f, 0.f));

	FGameplayEventData DamagePayload;
	DamagePayload.Instigator = Attacker;
	DamagePayload.Target = Target;
	DamagePayload.EventMagnitude = 120.f;

	const FGameplayEventData KnockbackPayload =
		UGA_KnockbackDebuff::MakeKnockbackPayloadFromDamageEvent(Target, DamagePayload);
	const FVector Direction = UGA_Knockback::ResolveKnockbackDirection(Target, &KnockbackPayload);

	TestTrue(TEXT("Debuff forwarded knockback follows the damage instigator's attack direction"),
		FVector::DotProduct(Direction, FVector::YAxisVector) > 0.99f);
	TestTrue(TEXT("Debuff forwarded knockback should not fall back to the target's X-axis facing"),
		FMath::Abs(Direction.X) < 0.01f);

	Target->Destroy();
	Attacker->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackDebuffPrefersDamageContextInstigatorTest,
	"DevKit.Knockback.DebuffPrefersDamageContextInstigatorDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockbackDebuffPrefersDamageContextInstigatorTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for knockback context direction test"), World);
	if (!World)
	{
		return false;
	}

	ACharacter* Target = World->SpawnActor<ACharacter>();
	ACharacter* Attacker = World->SpawnActor<ACharacter>();
	AActor* EffectCauser = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Target character spawned"), Target);
	TestNotNull(TEXT("Attacker character spawned"), Attacker);
	TestNotNull(TEXT("Effect causer actor spawned"), EffectCauser);
	if (!Target || !Attacker || !EffectCauser)
	{
		if (Target)
		{
			Target->Destroy();
		}
		if (Attacker)
		{
			Attacker->Destroy();
		}
		if (EffectCauser)
		{
			EffectCauser->Destroy();
		}
		return false;
	}

	Target->SetActorLocation(FVector::ZeroVector);
	Target->SetActorRotation(FRotator::ZeroRotator);
	Attacker->SetActorLocation(FVector::ZeroVector);
	Attacker->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	EffectCauser->SetActorLocation(FVector::ZeroVector);
	EffectCauser->SetActorRotation(FRotator::ZeroRotator);

	FGameplayEventData DamagePayload;
	DamagePayload.Instigator = EffectCauser;
	DamagePayload.Target = Target;
	DamagePayload.EventMagnitude = 120.f;
	DamagePayload.ContextHandle = FGameplayEffectContextHandle(new FGameplayEffectContext());
	DamagePayload.ContextHandle.AddInstigator(Attacker, EffectCauser);

	const FGameplayEventData KnockbackPayload =
		UGA_KnockbackDebuff::MakeKnockbackPayloadFromDamageEvent(Target, DamagePayload);
	const FVector Direction = UGA_Knockback::ResolveKnockbackDirection(Target, &KnockbackPayload);

	TestTrue(TEXT("Debuff forwarded knockback prefers the damage context's player instigator direction"),
		FVector::DotProduct(Direction, FVector::YAxisVector) > 0.99f);
	TestTrue(TEXT("Debuff forwarded knockback should ignore an X-facing effect causer"),
		FMath::Abs(Direction.X) < 0.01f);

	Target->Destroy();
	Attacker->Destroy();
	EffectCauser->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackDebuffUsesPlayerAttackInputDirectionTest,
	"DevKit.Knockback.DebuffUsesPlayerAttackInputDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockbackDebuffUsesPlayerAttackInputDirectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for knockback player input direction test"), World);
	if (!World)
	{
		return false;
	}

	ACharacter* Target = World->SpawnActor<ACharacter>();
	APlayerCharacterBase* Attacker = World->SpawnActor<APlayerCharacterBase>();
	AActor* EffectCauser = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Target character spawned"), Target);
	TestNotNull(TEXT("Player attacker spawned"), Attacker);
	TestNotNull(TEXT("Effect causer actor spawned"), EffectCauser);
	if (!Target || !Attacker || !EffectCauser)
	{
		if (Target)
		{
			Target->Destroy();
		}
		if (Attacker)
		{
			Attacker->Destroy();
		}
		if (EffectCauser)
		{
			EffectCauser->Destroy();
		}
		return false;
	}

	Target->SetActorLocation(FVector::ZeroVector);
	Target->SetActorRotation(FRotator::ZeroRotator);
	Attacker->SetActorLocation(FVector::ZeroVector);
	Attacker->SetActorRotation(FRotator::ZeroRotator);
	Attacker->LastInputDirection = FVector::YAxisVector;
	EffectCauser->SetActorLocation(FVector::ZeroVector);
	EffectCauser->SetActorRotation(FRotator::ZeroRotator);

	FGameplayEventData DamagePayload;
	DamagePayload.Instigator = EffectCauser;
	DamagePayload.Target = Target;
	DamagePayload.EventMagnitude = 120.f;
	DamagePayload.ContextHandle = FGameplayEffectContextHandle(new FGameplayEffectContext());
	DamagePayload.ContextHandle.AddInstigator(Attacker, EffectCauser);

	const FGameplayEventData KnockbackPayload =
		UGA_KnockbackDebuff::MakeKnockbackPayloadFromDamageEvent(Target, DamagePayload);
	const FVector Direction = UGA_Knockback::ResolveKnockbackDirection(Target, &KnockbackPayload);

	TestTrue(TEXT("Player knockback follows the stored attack input direction"),
		FVector::DotProduct(Direction, FVector::YAxisVector) > 0.99f);
	TestTrue(TEXT("Player knockback should not use the X-facing actor rotation"),
		FMath::Abs(Direction.X) < 0.01f);

	Target->Destroy();
	Attacker->Destroy();
	EffectCauser->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackDebuffUsesProjectileDirectionContextTest,
	"DevKit.Knockback.DebuffUsesProjectileDirectionContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockbackDebuffUsesProjectileDirectionContextTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for projectile knockback context test"), World);
	if (!World)
	{
		return false;
	}

	ACharacter* Target = World->SpawnActor<ACharacter>();
	APlayerCharacterBase* Attacker = World->SpawnActor<APlayerCharacterBase>();
	ASlashWaveProjectile* Projectile = World->SpawnActor<ASlashWaveProjectile>();
	TestNotNull(TEXT("Target character spawned"), Target);
	TestNotNull(TEXT("Player attacker spawned"), Attacker);
	TestNotNull(TEXT("Slash wave projectile spawned"), Projectile);
	if (!Target || !Attacker || !Projectile)
	{
		if (Target)
		{
			Target->Destroy();
		}
		if (Attacker)
		{
			Attacker->Destroy();
		}
		if (Projectile)
		{
			Projectile->Destroy();
		}
		return false;
	}

	Target->SetActorLocation(FVector::ZeroVector);
	Attacker->SetActorLocation(FVector::ZeroVector);
	Attacker->LastInputDirection = FVector::XAxisVector;
	Projectile->SetActorLocation(FVector::ZeroVector);
	Projectile->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	FSlashWaveProjectileRuntimeConfig ProjectileConfig;
	ProjectileConfig.bEnableTargetedBounce = true;
	Projectile->InitProjectileWithConfig(Attacker, ProjectileConfig);

	FGameplayEventData DamagePayload;
	DamagePayload.Instigator = Projectile;
	DamagePayload.Target = Target;
	DamagePayload.EventMagnitude = 120.f;
	DamagePayload.ContextHandle = FGameplayEffectContextHandle(new FGameplayEffectContext());
	DamagePayload.ContextHandle.AddInstigator(Attacker, Projectile);
	DamagePayload.ContextHandle.AddSourceObject(Projectile);

	const FGameplayEventData KnockbackPayload =
		UGA_KnockbackDebuff::MakeKnockbackPayloadFromDamageEvent(Target, DamagePayload);
	const FVector Direction = UGA_Knockback::ResolveKnockbackDirection(Target, &KnockbackPayload);

	TestTrue(TEXT("Projectile-context knockback follows projectile travel direction"),
		FVector::DotProduct(Direction, FVector::YAxisVector) > 0.99f);
	TestTrue(TEXT("Projectile-context knockback ignores player input direction"),
		FMath::Abs(Direction.X) < 0.01f);

	Target->Destroy();
	Attacker->Destroy();
	Projectile->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldBurstUsesProjectileDirectionContextTest,
	"DevKit.Knockback.ShieldBurstUsesProjectileDirectionContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShieldBurstUsesProjectileDirectionContextTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for shield burst projectile context test"), World);
	if (!World)
	{
		return false;
	}

	ACharacter* Target = World->SpawnActor<ACharacter>();
	APlayerCharacterBase* Attacker = World->SpawnActor<APlayerCharacterBase>();
	ASlashWaveProjectile* Projectile = World->SpawnActor<ASlashWaveProjectile>();
	TestNotNull(TEXT("Target character spawned"), Target);
	TestNotNull(TEXT("Player attacker spawned"), Attacker);
	TestNotNull(TEXT("Slash wave projectile spawned"), Projectile);
	if (!Target || !Attacker || !Projectile)
	{
		if (Target)
		{
			Target->Destroy();
		}
		if (Attacker)
		{
			Attacker->Destroy();
		}
		if (Projectile)
		{
			Projectile->Destroy();
		}
		return false;
	}

	Target->SetActorLocation(FVector::ZeroVector);
	Attacker->SetActorLocation(FVector::ZeroVector);
	Attacker->LastInputDirection = FVector::XAxisVector;
	Projectile->SetActorLocation(FVector::ZeroVector);
	Projectile->SetActorRotation(FRotator(0.f, 90.f, 0.f));
	FSlashWaveProjectileRuntimeConfig ProjectileConfig;
	ProjectileConfig.bEnableTargetedBounce = true;
	Projectile->InitProjectileWithConfig(Attacker, ProjectileConfig);

	FGameplayEffectContextHandle DamageContext(new FGameplayEffectContext());
	DamageContext.AddInstigator(Attacker, Projectile);
	DamageContext.AddSourceObject(Projectile);

	const FGameplayEventData KnockbackPayload =
		UGA_ActiveSkill_ShieldBurst::MakeKnockbackPayload(Attacker, Target, 500.f, DamageContext);
	const FVector Direction = UGA_Knockback::ResolveKnockbackDirection(Target, &KnockbackPayload);

	TestTrue(TEXT("Shield Burst projectile-context knockback follows projectile travel direction"),
		FVector::DotProduct(Direction, FVector::YAxisVector) > 0.99f);
	TestEqual(TEXT("Shield Burst payload keeps knockback distance magnitude"), KnockbackPayload.EventMagnitude, 500.f);

	Target->Destroy();
	Attacker->Destroy();
	Projectile->Destroy();
	return true;
}

#endif
