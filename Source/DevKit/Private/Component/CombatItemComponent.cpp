#include "Component/CombatItemComponent.h"

#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GameplayEffect.h"

static FGameplayTag CombatItem_TagNoHitReactDamage()
{
	return FGameplayTag::RequestGameplayTag(TEXT("Item.Damage.NoHitReact"), false);
}

static bool CombatItem_SpecHasTag(const FGameplayEffectSpec& Spec, const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (Spec.GetDynamicAssetTags().HasTag(Tag))
	{
		return true;
	}

	if (Spec.Def)
	{
		if (Spec.Def->GetAssetTags().HasTag(Tag) || Spec.Def->GetGrantedTags().HasTag(Tag))
		{
			return true;
		}
	}

	return false;
}

UCombatItemComponent::UCombatItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatItemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultSlots();
}

void UCombatItemComponent::InitializeDefaultSlots()
{
	Slots.Reset();
	for (UCombatItemDataAsset* Asset : DefaultItemAssets)
	{
		if (!Asset)
		{
			continue;
		}

		FRuntimeSlot& Slot = Slots.AddDefaulted_GetRef();
		Slot.Config = Asset->Config;
	}

	ActiveSlotIndex = Slots.IsValidIndex(ActiveSlotIndex) ? ActiveSlotIndex : 0;
	BroadcastSlotsChanged();
}

bool UCombatItemComponent::UseActiveItem()
{
	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		OnItemUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("No item slot")));
		return false;
	}

	const FCombatItemConfig Config = Slots[ActiveSlotIndex].Config;
	if (!Config.ActivationTag.IsValid())
	{
		OnItemUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("Item has no activation tag")));
		return false;
	}

	UYogAbilitySystemComponent* ASC = GetOwnerYogASC();
	if (!ASC)
	{
		OnItemUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("No ability system")));
		return false;
	}

	FGameplayEventData Payload;
	Payload.EventTag = Config.ActivationTag;
	Payload.Instigator = GetOwner();
	Payload.Target = GetOwner();
	Payload.OptionalObject = Config.Montage;

	// Consume only once an ability has answered the event; a missing or mis-tagged GA would
	// otherwise silently eat a single-use item.
	if (ASC->HandleGameplayEvent(Config.ActivationTag, &Payload) <= 0)
	{
		OnItemUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("No ability handled the item")));
		return false;
	}

	const int32 UsedIndex = ActiveSlotIndex;
	const TArray<FCombatItemSlotView> Views = GetSlotViews();
	const FCombatItemSlotView UsedSlot = Views.IsValidIndex(UsedIndex)
		? Views[UsedIndex]
		: FCombatItemSlotView();

	Slots.RemoveAt(UsedIndex);
	ActiveSlotIndex = Slots.IsEmpty() ? 0 : FMath::Clamp(UsedIndex, 0, Slots.Num() - 1);

	OnItemUsed.Broadcast(UsedIndex, UsedSlot);
	BroadcastSlotsChanged();
	return true;
}

void UCombatItemComponent::SelectNextItem()
{
	if (Slots.IsEmpty())
	{
		return;
	}

	SetActiveSlotIndex((ActiveSlotIndex + 1) % Slots.Num());
}

void UCombatItemComponent::SelectPreviousItem()
{
	if (Slots.IsEmpty())
	{
		return;
	}

	SetActiveSlotIndex((ActiveSlotIndex - 1 + Slots.Num()) % Slots.Num());
}

void UCombatItemComponent::SetActiveSlotIndex(int32 NewIndex)
{
	if (!Slots.IsValidIndex(NewIndex) || ActiveSlotIndex == NewIndex)
	{
		return;
	}

	ActiveSlotIndex = NewIndex;
	BroadcastSlotsChanged();
}

TArray<FCombatItemSlotView> UCombatItemComponent::GetSlotViews() const
{
	TArray<FCombatItemSlotView> Views;
	Views.Reserve(Slots.Num());

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FRuntimeSlot& Slot = Slots[Index];
		FCombatItemSlotView& View = Views.AddDefaulted_GetRef();
		View.ItemId = Slot.Config.ItemId;
		View.DisplayName = Slot.Config.DisplayName;
		View.Icon = Slot.Config.Icon;
		View.bSelected = Index == ActiveSlotIndex;
	}

	return Views;
}

bool UCombatItemComponent::IsNoHitReactItemDamage(const FGameplayEffectSpec& Spec)
{
	return CombatItem_SpecHasTag(Spec, CombatItem_TagNoHitReactDamage());
}

void UCombatItemComponent::ApplyItemPureDamage(AActor* SourceActor, AActor* TargetActor, float Damage, FName DamageType, bool bSuppressHitReact)
{
	if (!SourceActor || !TargetActor || Damage <= 0.0f)
	{
		return;
	}

	UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(SourceActor->FindComponentByClass<UAbilitySystemComponent>());
	UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetActor->FindComponentByClass<UAbilitySystemComponent>());
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	UGameplayEffect* DamageGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	DamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UDamageAttributeSet::GetDamagePureAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Damage));
	DamageGE->Modifiers.Add(ModInfo);

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(SourceActor, SourceActor);
	Context.AddSourceObject(SourceActor);
	FGameplayEffectSpec Spec(DamageGE, Context, 1.0f);
	if (bSuppressHitReact)
	{
		const FGameplayTag NoHitReactTag = CombatItem_TagNoHitReactDamage();
		if (NoHitReactTag.IsValid())
		{
			Spec.AddDynamicAssetTag(NoHitReactTag);
		}
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	SourceASC->LogDamageDealt(TargetActor, Damage, DamageType);
}

void UCombatItemComponent::BroadcastSlotsChanged() const
{
	OnItemSlotsChanged.Broadcast(GetSlotViews());
}

UYogAbilitySystemComponent* UCombatItemComponent::GetOwnerYogASC() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? Cast<UYogAbilitySystemComponent>(OwnerActor->FindComponentByClass<UAbilitySystemComponent>()) : nullptr;
}

#if WITH_DEV_AUTOMATION_TESTS
void UCombatItemComponent::SetSlotsForTest(const TArray<FCombatItemConfig>& InConfigs)
{
	Slots.Reset();
	for (const FCombatItemConfig& Config : InConfigs)
	{
		FRuntimeSlot& Slot = Slots.AddDefaulted_GetRef();
		Slot.Config = Config;
	}
	ActiveSlotIndex = Slots.IsValidIndex(ActiveSlotIndex) ? ActiveSlotIndex : 0;
	BroadcastSlotsChanged();
}
#endif
