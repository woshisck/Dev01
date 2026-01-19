// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include <NiagaraSystem.h>
#include "AbilityData.h"
#include "DevKit/Animation/YogAnimInstance.h"
#include "WeaponData.generated.h"
//D :\Fab\Meta\VTSH_LASE_Dev\VTSH_ShaderLib_Test\VTSH_ShaderDev_Test\Source\DevKit\Animation\YogAnimInstance.cpp
class AWeaponInstance;
class UYogGameplayAbility;
class AYogCharacterBase;
class UYogAnimInstance;

USTRUCT()
struct FWPN_DamageStateData
{
	GENERATED_BODY()

	bool bTriggered;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FVector RelativeLocation;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FQuat RelativeRotation;

};



USTRUCT()
struct FWeaponHoldData
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<AWeaponInstance> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FTransform AttachTransform;
};




USTRUCT(BlueprintType)
struct FWeaponAttributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FWeaponAttributeData() {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAtk = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAtkPower = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponAtkRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weapon_CritRate = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weapon_CritDmg = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DashDistance = 1;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TObjectPtr<AWeaponInstance> WeaponInstance;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<UYogGameplayAbility*> Actions;

};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//Get the item price info
	UFUNCTION(BlueprintPure)
	const FWeaponAttributeData& GetWeaponData() const;


	UFUNCTION(BlueprintCallable)
	void GrantAbilityToOwner(AYogCharacterBase* Owner);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "WeaponAttributeData"))
	FDataTableRowHandle WeaponAttributeRow;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<TObjectPtr<UAbilityData>> Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "ActionData"))
	TArray<FDataTableRowHandle> ActionRows;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UAnimInstance>> WeaponAmineLayer;


	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<FWeaponHoldData> ActorsToSpawn;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FRotator WeaponRotation;

	//Sets the height of the display mesh above the Weapon spawner
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	FVector WeaponMeshScale = FVector(1.0f, 1.0f, 1.0f);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Mesh")
	TObjectPtr<UStaticMesh> DisplayMesh;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Anime")
	TSubclassOf<UYogAnimInstance> WeaponLayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Anime")
	TSubclassOf<UYogAnimInstance> DashMontage;


	UFUNCTION()
	void GiveWeapon(UWorld* World, USkeletalMeshComponent* AttachTarget, AYogCharacterBase* ReceivingChar);


	inline static const FWeaponAttributeData DefaultWPNData;



};
