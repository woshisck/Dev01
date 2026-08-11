#include "Animation/AN_WeaponWhiffSound.h"

#include "Components/SkeletalMeshComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Kismet/GameplayStatics.h"

void UAN_WeaponWhiffSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(MeshComp->GetOwner());
	if (!Player)
	{
		return;
	}

	const UWeaponDefinition* WeaponDef = Player->GetEffectiveEquippedWeaponDefinition();
	if (!WeaponDef || !WeaponDef->WhiffSound)
	{
		return;
	}

	// Attached rather than at-location so the swoosh tracks the character through the swing, and
	// stops if the character is destroyed mid-arc.
	UGameplayStatics::SpawnSoundAttached(
		WeaponDef->WhiffSound, MeshComp, NAME_None, FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset, true,
		WeaponDef->WhiffSoundVolume, WeaponDef->WhiffSoundPitch);
}

FString UAN_WeaponWhiffSound::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Whiff Sound");
}
