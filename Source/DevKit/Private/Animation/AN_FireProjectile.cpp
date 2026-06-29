#include "Animation/AN_FireProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UAN_FireProjectile::UAN_FireProjectile()
{
	EventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.RangeAttack.Fire"));
}

void UAN_FireProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag   = EventTag;
	EventData.Instigator = Owner;
	// GA_RangeAttack casts this back to UAN_FireProjectile to read MuzzleSocketName.
	EventData.OptionalObject = this;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, EventData);
}

FString UAN_FireProjectile::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Fire Projectile | %s | %s"),
		*EventTag.ToString(),
		MuzzleSocketName.IsNone() ? TEXT("root") : *MuzzleSocketName.ToString());
}
