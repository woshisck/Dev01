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
			cache_Weapon->IgnoreActorList.Empty();
			//IgnoreActorList
		}

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NotifyBegin -> MeshComp Character NOT AVALIABLE"));

	}

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
			cache_Weapon->IgnoreActorList.Empty();
		}

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NotifyEnd -> MeshComp NOT AVALIABLE"));

	}
}

void UYogAnimNotifyState_Damage::Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const
{
	Super::Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	//TODO: called when received tick, NOT CALL WITH EVERY TICK
		
}

void UYogAnimNotifyState_Damage::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (MeshComp)
	{
		if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
		{
			AWeaponInstance* cache_Weapon = Character->Weapon;
			FWeaponSocketLoc wpnSocket;
			wpnSocket.DmgBox_End = cache_Weapon->point_DamageEnd;
			wpnSocket.DmgBox_Start = cache_Weapon->point_DamageStart;
			cache_Weapon->Array_damageBox.Insert(wpnSocket, 0);
			UE_LOG(LogTemp, Warning, TEXT("Owner_weapon->Array_damageBox.Num(): %d"), cache_Weapon->Array_damageBox.Num());
			
			if (cache_Weapon->Array_damageBox.Num() > 0)
			{
				cache_Weapon->CreateDamageBox();

				FWeaponSocketLoc current_socket_loc = cache_Weapon->Array_damageBox[0];
				//FWeaponSocketLoc last_socket_loc = cache_Weapon->Array_damageBox[1];

				//FVector current_midpoint = current_socket_loc.DmgBox_Start->GetComponentLocation() + ((current_socket_loc.DmgBox_End->GetComponentLocation() - current_socket_loc.DmgBox_Start->GetComponentLocation()) / 2);

			}


		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NotifyTick -> MeshComp NOT AVALIABLE"));

	}
}




