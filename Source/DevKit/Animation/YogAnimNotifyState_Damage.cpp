// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAnimNotifyState_Damage.h"
#include <DevKit/Character/YogCharacterBase.h>
#include "../Item/Weapon/WeaponInstance.h"





void UYogAnimNotifyState_Damage::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (MeshComp)
	{
		if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
		{
			AWeaponInstance* cache_Weapon = Character->Weapon;
			cache_Weapon->Array_damageBox.Empty();
		}

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NotifyBegin Character NOT AVALIABLE"));

	}


	//if(!MeshComp)
	//{
	//	return;


	//}
	//else
	//{
	//	if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
	//	{
	//		if (AWeaponInstance* cache_Weapon = Character->Weapon)
	//		{
	//			UE_LOG(LogTemp, Warning, TEXT("NotifyBegin cache_Weapon AVALIABLE"));
	//		}
	//		else 
	//		{
	//			UE_LOG(LogTemp, Warning, TEXT("NotifyBegin cache_Weapon NOT AVALIABLE"));
	//		}
	//		UE_LOG(LogTemp, Warning, TEXT("NotifyBegin Character AVALIABLE"));
	//	}
	//	else
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("NotifyBegin Character NOT AVALIABLE"));
	//	}

	//}

}

void UYogAnimNotifyState_Damage::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (MeshComp)
	{
		if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
		{
			AWeaponInstance* cache_Weapon = Character->Weapon;
			cache_Weapon->Array_damageBox.Empty();
		}

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NotifyEnd NOT AVALIABLE"));

	}
}

void UYogAnimNotifyState_Damage::Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const
{
	Super::Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);


	

	//AYogCharacterBase* Player = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	//AWeaponInstance* Weapon = Player->Weapon;



	//AYogCharacterBase* Owner = Cast<AYogCharacterBase>(MeshComp->GetOwner());


	//TODO: check weapon stats, need to add weapon class


	
}

void UYogAnimNotifyState_Damage::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (MeshComp)
	{
		if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
		{
			AWeaponInstance* cache_Weapon = Character->Weapon;
			FWeaponSocketLoc tempWeaponSL;
			tempWeaponSL.DmgBox_End = cache_Weapon->point_DamageEnd;
			tempWeaponSL.DmgBox_Start = cache_Weapon->point_DamageStart;
			cache_Weapon->Array_damageBox.Insert(tempWeaponSL, 0);
			UE_LOG(LogTemp, Warning, TEXT("Owner_weapon->Array_damageBox.Num(): %d"), cache_Weapon->Array_damageBox.Num());
		}

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Received_NotifyTick NOT AVALIABLE"));

	}
}



