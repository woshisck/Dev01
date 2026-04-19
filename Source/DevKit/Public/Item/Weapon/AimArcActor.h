// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AimArcActor.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;

/**
 * 火绳枪瞄准扇形指示器。
 *
 * 用法（GA 内 Blueprint）：
 *   1. SpawnActorDeferred<AYogAimArcActor>，FinishSpawning 于角色脚下
 *   2. 每 Tick 调用 UpdateArc(HalfAngleDeg, RadiusCm)
 *   3. 攻击发射后 Destroy 此 Actor
 *
 * 材质要求（BP_AimArcActor 的 ArcDecal 材质须包含以下 Scalar Parameter）：
 *   - "HalfAngle"  — 半角（度数）
 *   - "ArcRadius"  — 扇形半径（以 Decal Size Y 为 1 归一化，通常直接传 cm 值）
 *   - "EdgeSoftness" — 边缘柔化程度（0~1）
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API AYogAimArcActor : public AActor
{
    GENERATED_BODY()

public:
    AYogAimArcActor();

    /**
     * 更新扇形参数。每帧由蓄力 GA 调用。
     * @param HalfAngleDeg   半角（度），蓄力期间从 ArcStartHalfAngle 插值到 ArcEndHalfAngle
     * @param RadiusCm       扇形半径（厘米）
     */
    UFUNCTION(BlueprintCallable, Category = "AimArc")
    void UpdateArc(float HalfAngleDeg, float RadiusCm);

    /** 设置扇形颜色（蓄力满时可切换为暴击色） */
    UFUNCTION(BlueprintCallable, Category = "AimArc")
    void SetArcColor(FLinearColor Color);

    /** 开始显示（蓄力开始） */
    UFUNCTION(BlueprintCallable, Category = "AimArc")
    void ShowArc();

    /** 隐藏（保持 Actor 复用，发射后再隐藏再销毁） */
    UFUNCTION(BlueprintCallable, Category = "AimArc")
    void HideArc();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UDecalComponent> ArcDecal;

    /** 在 BP 子类中设置材质 */
    UPROPERTY(EditDefaultsOnly, Category = "AimArc")
    TObjectPtr<UMaterialInterface> ArcMaterial;

    UPROPERTY(BlueprintReadOnly, Category = "AimArc")
    TObjectPtr<UMaterialInstanceDynamic> DynMaterial;

private:
    static const FName ParamHalfAngle;
    static const FName ParamArcRadius;
    static const FName ParamColor;
};
