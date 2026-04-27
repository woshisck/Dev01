// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/Weapon/AimArcActor.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AYogAimArcActor::AYogAimArcActor()
{
    PrimaryActorTick.bCanEverTick = false;

    ArcDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("ArcDecal"));
    SetRootComponent(ArcDecal);

    // 向下投影，覆盖角色半径范围；Y/Z 控制半径，X 控制投影深度
    ArcDecal->DecalSize = FVector(128.f, 600.f, 600.f); // 初始大值，UpdateArc 会动态调整
    ArcDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // 朝向地面
    ArcDecal->SetVisibility(false);
}

void AYogAimArcActor::BeginPlay()
{
    Super::BeginPlay();

    if (ArcMaterial)
    {
        DynMaterial = ArcDecal->CreateDynamicMaterialInstance();
    }
}

void AYogAimArcActor::UpdateArc(float HalfAngleDeg, float RadiusCm)
{
    if (!DynMaterial) return;

    // 调整 Decal 大小以覆盖扇形范围（Y/Z = 半径）
    ArcDecal->DecalSize = FVector(128.f, RadiusCm, RadiusCm);

    DynMaterial->SetScalarParameterValue(FName(TEXT("HalfAngle")), HalfAngleDeg);
    DynMaterial->SetScalarParameterValue(FName(TEXT("ArcRadius")), RadiusCm);
}

void AYogAimArcActor::SetArcColor(FLinearColor Color)
{
    if (DynMaterial)
    {
        DynMaterial->SetVectorParameterValue(FName(TEXT("Color")), Color);
    }
}

void AYogAimArcActor::ShowArc()
{
    ArcDecal->SetVisibility(true);
}

void AYogAimArcActor::HideArc()
{
    ArcDecal->SetVisibility(false);
}
