#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "RuneGroundPathEffectActor.generated.h"

class UAbilitySystemComponent;
class UBoxComponent;
class UDecalComponent;
class UGameplayEffect;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class ERuneGroundPathTargetPolicy : uint8
{
	EnemiesOnly UMETA(DisplayName = "Enemies Only"),
	OwnerOnly UMETA(DisplayName = "Owner Only")
};

UENUM(BlueprintType)
enum class ERuneGroundPathShape : uint8
{
	Rectangle UMETA(DisplayName = "Rectangle"),
	Fan UMETA(DisplayName = "Fan")
};

UENUM(BlueprintType)
enum class ERuneGroundPathFacingMode : uint8
{
	SourceActorForward UMETA(DisplayName = "Source Actor Forward"),
	ControllerRotation UMETA(DisplayName = "Controller Rotation"),
	ToLastDamageTarget UMETA(DisplayName = "To Last Damage Target")
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneGroundPathEffectConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	ERuneGroundPathTargetPolicy TargetPolicy = ERuneGroundPathTargetPolicy::EnemiesOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	ERuneGroundPathShape Shape = ERuneGroundPathShape::Rectangle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01"))
	float Duration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01"))
	float TickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
	float Length = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
	float Width = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
	float Height = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1.0", ToolTip = "Decal projection depth in cm. Keep this shallow so the path decal stays on the floor instead of projecting up onto characters."))
	float DecalProjectionDepth = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ToolTip = "Rotates the decal texture/mask on the ground plane without changing the damage area. Use this when the decal UV forward does not match the path forward."))
	float DecalPlaneRotationDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	FVector SpawnOffset = FVector(45.0f, 0.0f, 6.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
	FVector NiagaraScale = FVector(0.5f, 0.5f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1", ClampMax = "12", ToolTip = "Number of Niagara instances distributed along the path. Fire paths use multiple small instances to read as a ground strip."))
	int32 NiagaraInstanceCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "Damage Tag 1 (SetByCaller)", ToolTip = "GameplayTag used by the GameplayEffect execution. Burn paths should use Data.Damage.Burn."))
	FGameplayTag SetByCallerTag1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "Damage Value 1 / Tick", ToolTip = "Designer-facing damage value passed to SetByCallerTag1. For UGE_RuneBurn this is the burn damage per periodic tick."))
	float SetByCallerValue1 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
	FGameplayTag SetByCallerTag2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
	float SetByCallerValue2 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (ClampMin = "1"))
	int32 ApplicationCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
	bool bApplyOncePerTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	bool bPlayTargetVFXOnApply = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	TObjectPtr<UNiagaraSystem> TargetNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	FName TargetAttachSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	TArray<FName> TargetAttachSocketFallbackNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	bool bAttachTargetVFXToTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	FVector TargetVFXLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	FRotator TargetVFXRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	FVector TargetVFXScale = FVector(1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX", meta = (ClampMin = "0.0"))
	float TargetVFXLifetime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Target VFX")
	bool bDestroyTargetVFXWithArea = false;
};

UCLASS()
class DEVKIT_API ARuneGroundPathEffectActor : public AActor
{
	GENERATED_BODY()

public:
	ARuneGroundPathEffectActor();

	void InitializeGroundPath(AActor* InSourceActor, UAbilitySystemComponent* InSourceASC, const FRuneGroundPathEffectConfig& InConfig);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Ground Path")
	TObjectPtr<UBoxComponent> AreaBox;

	UPROPERTY(VisibleAnywhere, Category = "Ground Path")
	TObjectPtr<UDecalComponent> PathDecal;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> NiagaraComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> TargetNiagaraComponents;

	UPROPERTY(Transient)
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	FRuneGroundPathEffectConfig Config;
	FTimerHandle TickTimerHandle;
	FTimerHandle DestroyTimerHandle;
	TSet<TObjectKey<AActor>> OnceAppliedTargets;

	void ApplyPathTick();
	void SpawnPathNiagara(float Length, float Width);
	bool ShouldAffectActor(AActor* Candidate) const;
	bool IsActorInsidePathShape(AActor* TargetActor) const;
	bool ApplyEffectToActor(AActor* TargetActor);
	void SpawnTargetNiagara(AActor* TargetActor);
	FVector FindGroundLocation(const FVector& DesiredLocation) const;
};
