// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "WeaponInstance.generated.h"


/**
 * 
 */
struct FWeaponActorToSpawn;
class USceneComponent;
class UYogGameplayAbility;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FWeaponSocketLoc
{
public:
	GENERATED_USTRUCT_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* DmgBox_Start = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* DmgBox_End = nullptr;
};



UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AWeaponInstance : public AActor
{
	GENERATED_BODY()
public:
	AWeaponInstance();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AActor*> IgnoreActorList;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<FWeaponSocketLoc> Array_damageBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USceneComponent> point_DamageStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USceneComponent> point_DamageEnd;

	UFUNCTION(BlueprintCallable)
	void ClearArray();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DT")
	TObjectPtr<UDataTable> WeaponAttributeTable;


	UFUNCTION(BlueprintCallable)
	void AddIgnoreActor(AActor* actor);

	UFUNCTION(BlueprintImplementableEvent)
	void CreateDamageEffect(AActor* WeaponOwner, AActor* TargetActor);

	UFUNCTION()
	void Initialize();


	UPROPERTY()
	float AttackPower;

	UPROPERTY()
	float AttackSpeed;

	UPROPERTY()
	float AttackRange;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> PickedUpEffect;

	UPROPERTY()
	float CrticalRate;

	UPROPERTY()
	float CriticalDamage;

	UPROPERTY()
	TArray<UYogGameplayAbility*> Actions;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UGameplayEffect>> WeaponHitEffect;




	////~UObject interface
	//virtual UWorld* GetWorld() const override final;
	////~End of UObject interface

	//UFUNCTION(BlueprintPure, Category = Equipment)
	//UObject* GetInstigator() const { return Instigator; }

	//void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	//UFUNCTION(BlueprintPure, Category = Equipment)
	//APawn* GetPawn() const;

	//UFUNCTION(BlueprintPure, Category = Equipment, meta = (DeterminesOutputType = PawnType))
	//APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	//UFUNCTION(BlueprintPure, Category = Equipment)
	//TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	//void SpawnEquipmentActors(const TArray<FWeaponActorToSpawn>& ActorsToSpawn);
	//void DestroyEquipmentActors();

	//virtual void OnEquipped();
	//virtual void OnUnequipped();


	//UPROPERTY(BlueprintReadWrite, meta = (DeterminesOutputType = PawnType))
	//TObjectPtr<UObject> Instigator;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	//TArray<TObjectPtr<AActor>> SpawnedActors;

	//UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnEquipped"))
	//void K2_OnEquipped();

	//UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnUnequipped"))
	//void K2_OnUnequipped();




};
