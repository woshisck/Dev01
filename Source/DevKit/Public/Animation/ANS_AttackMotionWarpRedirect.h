#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_MotionWarping.h"
#include "ANS_AttackMotionWarpRedirect.generated.h"

class APlayerCharacterBase;
class AYogPlayerControllerBase;
class UMotionWarpingComponent;

UCLASS(meta = (DisplayName = "Attack Motion Warp Redirect"))
class DEVKIT_API UANS_AttackMotionWarpRedirect : public UAnimNotifyState_MotionWarping
{
	GENERATED_BODY()

public:
	UANS_AttackMotionWarpRedirect(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Redirect")
	FName WarpTargetName = TEXT("AttackTarget");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Redirect")
	bool bEnableRedirect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Redirect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RedirectDistance = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Redirect", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "90.0"))
	float MaxAngleDegrees = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Redirect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float InputGraceTime = 0.25f;

	virtual URootMotionModifier* AddRootMotionModifier_Implementation(
		UMotionWarpingComponent* MotionWarpingComp,
		const UAnimSequenceBase* Animation,
		float StartTime,
		float EndTime) const override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	bool RefreshRedirectTarget(UMotionWarpingComponent* MotionWarpingComp) const;
	bool ResolveAttackDirection(const APlayerCharacterBase* PlayerCharacter, FVector& OutDirection) const;
	static bool DirectionsMatch(const FVector& MoveDirection, const FVector& AttackDirection, float MaxAngle);
};
