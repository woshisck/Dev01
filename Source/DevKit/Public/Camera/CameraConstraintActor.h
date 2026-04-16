// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraConstraintActor.generated.h"

/**
 * ACameraConstraintActor
 *
 * 在关卡中放置此 Actor 并在 Details 面板里编辑 BoundaryPoints 顶点列表，
 * 即可形成一个凸/凹多边形，CameraPawn 会将自己的 XY 位置限定在该多边形内部。
 *
 * 使用方式：
 *   1. 拖入关卡
 *   2. 在 Details > Camera Constraint 下填写 BoundaryPoints（世界坐标，Z 值忽略）
 *   3. 运行时 CameraPawn 会自动查找并使用第一个此类 Actor
 *
 * 编辑器中顶点之间会以青色线段连接，方便预览边界形状。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API ACameraConstraintActor : public AActor
{
	GENERATED_BODY()

public:
	ACameraConstraintActor();

	/**
	 * 多边形顶点列表（世界坐标）。
	 * meta = (MakeEditWidget = true) 可在编辑器视口中直接拖拽每个点。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Constraint",
		meta = (MakeEditWidget = true))
	TArray<FVector> BoundaryPoints;

	/**
	 * 将 XY 平面上的一点约束到多边形内部。
	 * 若点在多边形内，直接返回原点；
	 * 若在外部，返回距该点最近的边上的点。
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Constraint")
	FVector2D ConstrainPosition(FVector2D InPos) const;

	/** 用射线法判断点是否在多边形内部（XY 平面） */
	UFUNCTION(BlueprintPure, Category = "Camera Constraint")
	bool IsPointInsidePolygon(FVector2D Point) const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	/** 遍历所有边，返回距 Point 最近的边上的点 */
	FVector2D GetClosestBoundaryPoint(FVector2D Point) const;
};
