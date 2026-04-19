// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Weapon/WeaponInstance.h"
#include "Item/Weapon/WeaponSpawner.h"
#include "SaveGame/YogSaveGame.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
AWeaponInstance::AWeaponInstance()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);

}

void AWeaponInstance::BeginPlay()
{
	Super::BeginPlay();
}

void AWeaponInstance::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GlowElapsed < 0.f) return;

	GlowElapsed += DeltaTime;

	const float HoldEnd       = GlowSweepDuration + GlowHoldDuration;
	const float TotalDuration = HoldEnd + GlowFadeDuration;

	TArray<UMeshComponent*> Meshes;
	GetComponents<UMeshComponent>(Meshes);

	float SweepProgress, GlowAlpha;

	if (GlowElapsed < GlowSweepDuration)
	{
		SweepProgress = GlowElapsed / GlowSweepDuration;
		GlowAlpha     = 1.f;
	}
	else if (GlowElapsed < HoldEnd)
	{
		SweepProgress = 1.f;
		GlowAlpha     = 1.f;
	}
	else if (GlowElapsed < TotalDuration)
	{
		SweepProgress = 1.f;
		GlowAlpha     = 1.f - (GlowElapsed - HoldEnd) / GlowFadeDuration;
	}
	else
	{
		for (UMeshComponent* Mesh : Meshes) { Mesh->SetOverlayMaterial(nullptr); }
		GlowElapsed = -1.f;
		return;
	}

	if (HeatOverlayDynMat)
	{
		HeatOverlayDynMat->SetScalarParameterValue(TEXT("SweepProgress"), SweepProgress);
		HeatOverlayDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), GlowAlpha);
	}
}

void AWeaponInstance::PostActorCreated()
{
	Super::PostActorCreated();

	
}


void AWeaponInstance::InitializeWeapon()
{
	AActor* owner = this->GetOwner();
	if (Cast<AWeaponSpawner>(owner))
	{
		//AttachSocket = 
		//AttachTransform
		//WeaponLayer
		//WeaponAbilities
	}


}

void AWeaponInstance::EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar)
{
	this->SetActorRelativeTransform(AttachTransform);
	this->AttachToComponent(ReceivingChar->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);

	if (WeaponLayer)
	{
		ReceivingChar->GetMesh()->GetAnimInstance()->LinkAnimClassLayers(this->WeaponLayer);
	}
}

void AWeaponInstance::OnHeatPhaseChanged(int32 Phase)
{
	TArray<UMeshComponent*> Meshes;
	GetComponents<UMeshComponent>(Meshes);

	if (Phase == 0 || !HeatOverlayMaterial)
	{
		// 立即关闭（Phase=0 或无材质）
		for (UMeshComponent* Mesh : Meshes) { Mesh->SetOverlayMaterial(nullptr); }
		GlowElapsed = -1.f;
		return;
	}

	// Phase 1=冷白/淡蓝(0.3强) Phase 2=暖橙(0.7强) Phase 3=金色(1.5强)
	static const FLinearColor PhaseColors[] =
	{
		FLinearColor(0.f,   0.f,   0.f  ),  // 0: 无
		FLinearColor(0.9f,  1.0f,  1.8f ),  // 1: 冷白/淡蓝
		FLinearColor(2.5f,  1.0f,  0.08f),  // 2: 暖橙
		FLinearColor(5.5f,  4.0f,  0.3f ),  // 3: 金色
		FLinearColor(5.5f,  4.0f,  0.3f ),  // 4: 占位
	};

	if (!HeatOverlayDynMat)
	{
		HeatOverlayDynMat = UMaterialInstanceDynamic::Create(HeatOverlayMaterial, this);
	}

	const int32 Idx = FMath::Clamp(Phase, 0, 4);
	HeatOverlayDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), PhaseColors[Idx]);
	HeatOverlayDynMat->SetScalarParameterValue(TEXT("SweepProgress"), 0.f);
	HeatOverlayDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), 1.f);

	for (UMeshComponent* Mesh : Meshes) { Mesh->SetOverlayMaterial(HeatOverlayDynMat); }

	// 启动动画计时
	GlowElapsed = 0.f;
}
