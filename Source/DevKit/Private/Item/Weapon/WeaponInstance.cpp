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
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);

}

void AWeaponInstance::BeginPlay()
{
	Super::BeginPlay();
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
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
			FString::Printf(TEXT("[WeaponInstance] Heat Phase: %d"), Phase));
	}

	// 获取本 Actor 上所有 MeshComponent（蓝图里添加的武器网格）
	TArray<UMeshComponent*> Meshes;
	GetComponents<UMeshComponent>(Meshes);

	if (Phase == 0 || !HeatOverlayMaterial)
	{
		// 关闭发光
		for (UMeshComponent* Mesh : Meshes)
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
		return;
	}

	// 各阶段 Emissive 颜色（线性空间，HDR 亮度）
	static const FLinearColor PhaseColors[] =
	{
		FLinearColor(0.f,  0.f,  0.f),   // 0: 无（占位）
		FLinearColor(2.f,  2.f,  2.f),   // 1: 白光
		FLinearColor(0.f,  3.f,  0.f),   // 2: 绿光
		FLinearColor(4.f,  2.f,  0.f),   // 3: 橙黄
		FLinearColor(5.f,  0.f,  0.f),   // 4: 过热红光
	};

	const int32 ColorIdx = FMath::Clamp(Phase, 0, 4);

	// 首次调用时创建动态材质实例
	if (!HeatOverlayDynMat)
	{
		HeatOverlayDynMat = UMaterialInstanceDynamic::Create(HeatOverlayMaterial, this);
	}

	HeatOverlayDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), PhaseColors[ColorIdx]);

	for (UMeshComponent* Mesh : Meshes)
	{
		Mesh->SetOverlayMaterial(HeatOverlayDynMat);
	}
}
