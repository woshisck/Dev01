#pragma once

#include "CoreMinimal.h"
#include "WaterInteractionTypes.generated.h"

UENUM(BlueprintType)
enum class EWaterImpulseType : uint8
{
	Footstep UMETA(DisplayName = "Footstep"),
	RollOrDash UMETA(DisplayName = "Roll Or Dash"),
	Explosion UMETA(DisplayName = "Explosion"),
	FallingMesh UMETA(DisplayName = "Falling Mesh"),
	MagicOrVFX UMETA(DisplayName = "Magic Or VFX")
};

UENUM(BlueprintType)
enum class EWaterPerformanceTier : uint8
{
	Low UMETA(DisplayName = "Low / Deck / NS"),
	High UMETA(DisplayName = "High / PC")
};

USTRUCT(BlueprintType)
struct FWaterImpulseRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float Radius = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float Strength = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float MassScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float Age = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	float Lifetime = 2.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	EWaterImpulseType Type = EWaterImpulseType::Footstep;

	float GetNormalizedAge() const
	{
		return Lifetime > 0.0f ? FMath::Clamp(Age / Lifetime, 0.0f, 1.0f) : 1.0f;
	}

	float GetFade() const
	{
		return 1.0f - GetNormalizedAge();
	}
};

USTRUCT(BlueprintType)
struct FWaterSurfaceAttachmentLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	TObjectPtr<UTexture> Texture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0"))
	float Intensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FVector2D Tiling = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FVector2D ScrollSpeed = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoughnessInfluence = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	bool bDisplacedByInteraction = true;
};

USTRUCT(BlueprintType)
struct FWaterVisualTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FLinearColor BaseWaterColor = FLinearColor(0.10f, 0.16f, 0.14f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FLinearColor DepthColor = FLinearColor(0.05f, 0.08f, 0.06f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Roughness = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0"))
	float SpecularIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0"))
	float WaveScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FLinearColor FoamColor = FLinearColor(0.82f, 0.82f, 0.74f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0"))
	float FoamStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.01"))
	float FoamDecay = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FLinearColor TurbidityColor = FLinearColor(0.32f, 0.22f, 0.12f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0"))
	float TurbidityStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.01"))
	float TurbidityClearTime = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0"))
	float GlintIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlintThreshold = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0.01"))
	float GlintLifetime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FWaterSurfaceAttachmentLayer AttachmentLayer1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FWaterSurfaceAttachmentLayer AttachmentLayer2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FWaterSurfaceAttachmentLayer AttachmentLayer3;
};
