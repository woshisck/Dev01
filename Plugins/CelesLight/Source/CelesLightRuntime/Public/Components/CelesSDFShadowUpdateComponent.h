#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CelesSDFShadowUpdateComponent.generated.h"

class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent))
class CELESLIGHTRUNTIME_API UCelesSDFShadowUpdateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCelesSDFShadowUpdateComponent();

	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Celes Light|SDF Shadow")
	void InitSDF();

	UFUNCTION(BlueprintCallable, Category = "Celes Light|SDF Shadow")
	void SDFShadowTick();

	UFUNCTION(BlueprintPure, Category = "Celes Light|SDF Shadow")
	static FVector GetDirectionByAxis(FRotator Rot, TEnumAsByte<EAxis::Type> Axis, bool bInvert);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow", meta = (ClampMin = "0"))
	int32 FaceMaterialID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	FName TraceBoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	TEnumAsByte<EAxis::Type> TraceBoneFwdAxis = EAxis::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	bool bInvertFwd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	TEnumAsByte<EAxis::Type> TraceBoneRtAxis = EAxis::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	bool bInvertRt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow", meta = (ClampMin = "0.01"))
	float TickTimeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	FName ForwardParameterName = TEXT("SDFCustomDirectionFwd");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|SDF Shadow")
	FName RightParameterName = TEXT("SDFCustomDirectionRt");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|SDF Shadow")
	TObjectPtr<UMaterialInstanceDynamic> SDFDynMaterial = nullptr;

private:
	FRotator ResolveTraceRotation() const;
};
