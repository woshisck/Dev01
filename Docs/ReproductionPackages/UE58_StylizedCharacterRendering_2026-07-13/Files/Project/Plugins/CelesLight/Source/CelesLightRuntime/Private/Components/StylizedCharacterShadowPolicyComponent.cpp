#include "Components/StylizedCharacterShadowPolicyComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/Actor.h"

UStylizedCharacterShadowPolicyComponent::UStylizedCharacterShadowPolicyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStylizedCharacterShadowPolicyComponent::OnRegister()
{
	Super::OnRegister();
	ApplyShadowPolicy();
}

void UStylizedCharacterShadowPolicyComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyShadowPolicy();
}

#if WITH_EDITOR
void UStylizedCharacterShadowPolicyComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyShadowPolicy();
}
#endif

void UStylizedCharacterShadowPolicyComponent::ApplyShadowPolicy()
{
	TArray<UPrimitiveComponent*> Targets;
	for (UPrimitiveComponent* ExplicitTarget : ExplicitTargetComponents)
	{
		if (IsValid(ExplicitTarget))
		{
			Targets.AddUnique(ExplicitTarget);
		}
	}

	if (Targets.IsEmpty() && bAutoFindSkinnedMeshComponents)
	{
		if (AActor* Owner = GetOwner())
		{
			TInlineComponentArray<USkinnedMeshComponent*> SkinnedMeshes(Owner);
			for (USkinnedMeshComponent* SkinnedMesh : SkinnedMeshes)
			{
				Targets.Add(SkinnedMesh);
			}
		}
	}

	for (UPrimitiveComponent* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		if (bEnvironmentOnlyNativeShadow)
		{
			Target->SetCastShadow(true);
		}
		Target->SetCastShadowOnOtherObjectsOnly(bEnvironmentOnlyNativeShadow);

		if (bEnvironmentOnlyNativeShadow && bDisableContactShadows)
		{
			Target->SetCastContactShadow(false);
		}

		if (bEnvironmentOnlyNativeShadow && bDisableCapsuleDirectShadows)
		{
			if (USkinnedMeshComponent* SkinnedMesh = Cast<USkinnedMeshComponent>(Target))
			{
				SkinnedMesh->SetCastCapsuleDirectShadow(false);
			}
		}
	}
}
