#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h"
#include "GlobalHitShakeData.generated.h"

class UCameraShakeBase;

/**
 * Global camera-shake config for the player-hits-enemy impact cue.
 * One asset for the whole game, referenced from Project Settings (UYogSettings).
 * The shake magnitude scales with the swing's final HP damage, so heavier hits
 * wobble the camera more. Individual weapons no longer carry their own shake.
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGlobalHitShakeData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	// Flat multiplier applied on top of the damage curve result.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake", meta = (ClampMin = "0.0"))
	float CameraShakeScale = 1.f;

	// Maps final HP removed (post armor/shield) to a shake scale. When empty, scale is 1.
	// X = damage, Y = scale. Tunes the light-hit vs heavy-hit falloff globally.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake")
	FRuntimeFloatCurve DamageToShakeScale;

	// Resolves the final shake scale for a given amount of damage dealt.
	float ResolveShakeScale(float Damage) const
	{
		float DamageScale = 1.f;
		const FRichCurve* Curve = DamageToShakeScale.GetRichCurveConst();
		if (Curve && Curve->GetNumKeys() > 0)
		{
			DamageScale = Curve->Eval(Damage);
		}
		return CameraShakeScale * DamageScale;
	}
};
