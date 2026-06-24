#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RuneModules.generated.h"

class AMusketBullet;
class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneModuleFlags
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modules")
	bool bProjectile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modules")
	bool bAura = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modules")
	bool bStatus = false;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneProjectileModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	TSubclassOf<AMusketBullet> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "100.0"))
	float Speed = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ConeAngleDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool bSweepCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FGameplayTag HitEventTag;
};

UENUM(BlueprintType)
enum class ERuneGroundAreaShape : uint8
{
	Rectangle UMETA(DisplayName = "Rectangle"),
	Sector UMETA(DisplayName = "Sector"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneAuraModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
	ERuneGroundAreaShape Shape = ERuneGroundAreaShape::Rectangle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "1.0"))
	float Length = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "1.0"))
	float Width = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "1.0"))
	float Height = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "0.01"))
	float Duration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura", meta = (ClampMin = "0.01"))
	float TickInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Visual")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura|Visual")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneStatusModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	TSubclassOf<UGameplayEffect> StatusGE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status", meta = (ClampMin = "0.0"))
	float Duration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status", meta = (ClampMin = "1"))
	int32 StackLimit = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FGameplayTag StatusTag;
};
