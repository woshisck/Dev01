#include "Component/DataCacheComponent.h"



UDataCacheComponent::UDataCacheComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


void UDataCacheComponent::BeginPlay()
{
	Super::BeginPlay();

	// UAbilitySystemComponent* abilitySystemComponent = GetOwner()->FindComponentByClass<UAbilitySystemComponent>();

	// abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute()).AddUObject(this, &UDataCacheComponent::HandleHealthChange);

}

void UDataCacheComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);
}


