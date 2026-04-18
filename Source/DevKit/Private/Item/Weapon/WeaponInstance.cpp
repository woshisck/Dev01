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

	static const FLinearColor PhaseColors[] =
	{
		FLinearColor(0.f, 0.f, 0.f),
		FLinearColor(2.f, 2.f, 2.f),
		FLinearColor(0.f, 3.f, 0.f),
		FLinearColor(4.f, 2.f, 0.f),
		FLinearColor(5.f, 0.f, 0.f),
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
