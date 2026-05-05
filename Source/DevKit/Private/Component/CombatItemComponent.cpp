#include "Component/CombatItemComponent.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Actors/CombatItemAreaActor.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"

namespace
{
	TMap<TObjectKey<AActor>, float> GStickyOilTurnMultipliers;
	TMap<TObjectKey<AActor>, float> GStickyOilMoveMultipliers;

	FGameplayTag TagOilBlade()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.OilBlade"), false);
	}

	FGameplayTag TagStickyOil()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.StickyOil"), false);
	}

	FGameplayTag TagInSmoke()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.InSmoke"), false);
	}

	FGameplayTag TagNoHitReactItemDamage()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Item.Damage.NoHitReact"), false);
	}

	FGameplayTag TagOilFireBonusDamage()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Item.Damage.OilFireBonus"), false);
	}

	FGameplayTag TagFireDamage()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.DamageType.Fire"), false);
	}

	FGameplayTag TagBurning()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Burning"), false);
	}

	FGameplayTag TagBurnDamage()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.Burn"), false);
	}

	FGameplayTag TagBurnCardEffect()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Burn"), false);
	}

	bool SpecHasTag(const FGameplayEffectSpec& Spec, const FGameplayTag& Tag)
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

	bool IsFireDamageSpec(const FGameplayEffectSpec& Spec)
	{
		if (SpecHasTag(Spec, TagFireDamage()) || SpecHasTag(Spec, TagBurning()) || SpecHasTag(Spec, TagBurnCardEffect()))
		{
			return true;
		}

		const FGameplayTag BurnDamageTag = TagBurnDamage();
		return BurnDamageTag.IsValid() && Spec.GetSetByCallerMagnitude(BurnDamageTag, false, 0.0f) > 0.0f;
	}

	FCombatItemConfig MakeBuiltInOilItem()
	{
		FCombatItemConfig Config;
		Config.ItemId = TEXT("OilBottle");
		Config.DisplayName = FText::FromString(TEXT("Oil"));
		Config.EffectType = ECombatItemEffectType::OilBottle;
		Config.MaxCharges = 2;
		Config.InitialCharges = 2;
		Config.Cooldown = 12.0f;
		return Config;
	}

	FCombatItemConfig MakeBuiltInThunderItem()
	{
		FCombatItemConfig Config;
		Config.ItemId = TEXT("ThunderStone");
		Config.DisplayName = FText::FromString(TEXT("Thunder"));
		Config.EffectType = ECombatItemEffectType::ThunderStone;
		Config.MaxCharges = 2;
		Config.InitialCharges = 2;
		Config.Cooldown = 16.0f;
		return Config;
	}

	FCombatItemConfig MakeBuiltInSmokeItem()
	{
		FCombatItemConfig Config;
		Config.ItemId = TEXT("SmokeBomb");
		Config.DisplayName = FText::FromString(TEXT("Smoke"));
		Config.EffectType = ECombatItemEffectType::SmokeBomb;
		Config.MaxCharges = 2;
		Config.InitialCharges = 2;
		Config.Cooldown = 18.0f;
		return Config;
	}
}

UCombatItemComponent::UCombatItemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatItemComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultSlots();
}

void UCombatItemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndOilBlade();
	SetPlayerInsideSmoke(false, 0.0f);
	ActiveSmokeAreaCount = 0;
	ClearAllStickyOil();
	Super::EndPlay(EndPlayReason);
}

void UCombatItemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bChanged = false;
	for (FRuntimeSlot& Slot : Slots)
	{
		if (Slot.CooldownRemaining > 0.0f)
		{
			Slot.CooldownRemaining = FMath::Max(0.0f, Slot.CooldownRemaining - FMath::Max(0.0f, DeltaTime));
			bChanged = true;
		}
	}

	if (bChanged)
	{
		BroadcastSlotsChanged();
	}
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
		Slot.Charges = FMath::Clamp(Slot.Config.InitialCharges, 0, FMath::Max(Slot.Config.MaxCharges, 0));
	}

	if (Slots.IsEmpty() && bUseBuiltInDefaultItemsWhenEmpty)
	{
		const FCombatItemConfig BuiltInItems[] =
		{
			MakeBuiltInOilItem(),
			MakeBuiltInThunderItem(),
			MakeBuiltInSmokeItem(),
		};
		for (const FCombatItemConfig& Config : BuiltInItems)
		{
			FRuntimeSlot& Slot = Slots.AddDefaulted_GetRef();
			Slot.Config = Config;
			Slot.Charges = FMath::Clamp(Config.InitialCharges, 0, FMath::Max(Config.MaxCharges, 0));
		}
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

	FRuntimeSlot& Slot = Slots[ActiveSlotIndex];
	if (Slot.Charges <= 0)
	{
		OnItemUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("No charges")));
		return false;
	}

	if (Slot.CooldownRemaining > 0.0f)
	{
		OnItemUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("Cooling down")));
		return false;
	}

	if (!ExecuteItemEffect(Slot))
	{
		OnItemUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("Cannot use item")));
		return false;
	}

	Slot.Charges = FMath::Max(0, Slot.Charges - 1);
	Slot.CooldownRemaining = FMath::Max(0.0f, Slot.Config.Cooldown);
	const TArray<FCombatItemSlotView> Views = GetSlotViews();
	const FCombatItemSlotView UsedSlot = Views.IsValidIndex(ActiveSlotIndex)
		? Views[ActiveSlotIndex]
		: FCombatItemSlotView();
	OnItemUsed.Broadcast(ActiveSlotIndex, UsedSlot);
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
		View.EffectType = Slot.Config.EffectType;
		View.Charges = Slot.Charges;
		View.MaxCharges = Slot.Config.MaxCharges;
		View.CooldownRemaining = Slot.CooldownRemaining;
		View.CooldownDuration = Slot.Config.Cooldown;
		View.bSelected = Index == ActiveSlotIndex;
	}

	return Views;
}

bool UCombatItemComponent::ExecuteItemEffect(const FRuntimeSlot& Slot)
{
	switch (Slot.Config.EffectType)
	{
	case ECombatItemEffectType::OilBottle:
		return ExecuteOilBottle(Slot.Config);
	case ECombatItemEffectType::ThunderStone:
		return ExecuteThunderStone(Slot.Config);
	case ECombatItemEffectType::SmokeBomb:
		return ExecuteSmokeBomb(Slot.Config);
	default:
		return false;
	}
}

bool UCombatItemComponent::ExecuteOilBottle(const FCombatItemConfig& Config)
{
	UYogAbilitySystemComponent* ASC = GetOwnerYogASC();
	const FGameplayTag OilBladeTag = TagOilBlade();
	if (!ASC || !OilBladeTag.IsValid())
	{
		return false;
	}

	ASC->SetLooseGameplayTagCount(OilBladeTag, 1);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OilBladeTimer);
		World->GetTimerManager().SetTimer(
			OilBladeTimer,
			this,
			&UCombatItemComponent::EndOilBlade,
			FMath::Max(0.01f, Config.Oil.OilBladeDuration),
			false);
	}

	return true;
}

bool UCombatItemComponent::ExecuteThunderStone(const FCombatItemConfig& Config)
{
	APlayerCharacterBase* Player = GetPlayerOwner();
	UWorld* World = GetWorld();
	if (!Player || !World)
	{
		return false;
	}

	TSubclassOf<ACombatItemAreaActor> SpawnClass = AreaActorClass;
	if (!SpawnClass)
	{
		SpawnClass = ACombatItemAreaActor::StaticClass();
	}
	FActorSpawnParameters Params;
	Params.Owner = Player;
	Params.Instigator = Player;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACombatItemAreaActor* Area = World->SpawnActor<ACombatItemAreaActor>(SpawnClass, Player->GetActorTransform(), Params);
	if (!Area)
	{
		return false;
	}

	Area->InitializeThunderArea(Player, Config);
	return true;
}

bool UCombatItemComponent::ExecuteSmokeBomb(const FCombatItemConfig& Config)
{
	APlayerCharacterBase* Player = GetPlayerOwner();
	UWorld* World = GetWorld();
	if (!Player || !World)
	{
		return false;
	}

	TSubclassOf<ACombatItemAreaActor> SpawnClass = AreaActorClass;
	if (!SpawnClass)
	{
		SpawnClass = ACombatItemAreaActor::StaticClass();
	}
	FActorSpawnParameters Params;
	Params.Owner = Player;
	Params.Instigator = Player;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACombatItemAreaActor* Area = World->SpawnActor<ACombatItemAreaActor>(SpawnClass, Player->GetActorTransform(), Params);
	if (!Area)
	{
		return false;
	}

	Area->InitializeSmokeArea(Player, Config);
	return true;
}

void UCombatItemComponent::EndOilBlade()
{
	if (UYogAbilitySystemComponent* ASC = GetOwnerYogASC())
	{
		const FGameplayTag OilBladeTag = TagOilBlade();
		if (OilBladeTag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(OilBladeTag, 0);
		}
	}
}

bool UCombatItemComponent::ApplyOilBladeHitToTarget(AActor* TargetActor)
{
	UYogAbilitySystemComponent* OwnerASC = GetOwnerYogASC();
	const FGameplayTag OilBladeTag = TagOilBlade();
	if (!TargetActor || !OwnerASC || !OilBladeTag.IsValid() || !OwnerASC->HasMatchingGameplayTag(OilBladeTag))
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC || TargetASC == OwnerASC)
	{
		return false;
	}

	FCombatItemOilConfig OilConfig;
	for (const FRuntimeSlot& Slot : Slots)
	{
		if (Slot.Config.EffectType == ECombatItemEffectType::OilBottle)
		{
			OilConfig = Slot.Config.Oil;
			break;
		}
	}

	TObjectKey<AActor> Key(TargetActor);
	FStickyOilRuntimeState& State = StickyOilStates.FindOrAdd(Key);
	State.TargetActor = TargetActor;
	State.ASC = TargetASC;
	State.Stacks = FMath::Clamp(State.Stacks + 1, 1, FMath::Max(1, OilConfig.MaxStickyStacks));

	ApplyStickyOilAttributeDeltas(State, OilConfig);

	const FGameplayTag StickyTag = TagStickyOil();
	if (StickyTag.IsValid())
	{
		TargetASC->SetLooseGameplayTagCount(StickyTag, State.Stacks);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(State.ExpireTimer);
		FTimerDelegate ExpireDelegate;
		ExpireDelegate.BindUObject(this, &UCombatItemComponent::ExpireStickyOil, TargetActor);
		World->GetTimerManager().SetTimer(
			State.ExpireTimer,
			ExpireDelegate,
			FMath::Max(0.01f, OilConfig.StickyDuration),
			false);
	}

	return true;
}

void UCombatItemComponent::ApplyStickyOilAttributeDeltas(FStickyOilRuntimeState& State, const FCombatItemOilConfig& OilConfig)
{
	UAbilitySystemComponent* ASC = State.ASC.Get();
	if (!ASC)
	{
		return;
	}

	const float MoveSlow = FMath::Clamp(OilConfig.MoveSpeedSlowPerStack * State.Stacks, 0.0f, 0.95f);
	const float AttackSlow = FMath::Clamp(OilConfig.AttackSpeedSlowPerStack * State.Stacks, 0.0f, 0.95f);
	const float TurnSlow = FMath::Clamp(OilConfig.TurnSpeedSlowPerStack * State.Stacks, 0.0f, 0.95f);

	const float MoveBase = ASC->GetNumericAttribute(UBaseAttributeSet::GetMoveSpeedAttribute()) - State.MoveSpeedDelta;
	const float AttackBase = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackSpeedAttribute()) - State.AttackSpeedDelta;
	const float NewMoveDelta = MoveBase * -MoveSlow;
	const float NewAttackDelta = AttackBase * -AttackSlow;

	ASC->ApplyModToAttributeUnsafe(UBaseAttributeSet::GetMoveSpeedAttribute(), EGameplayModOp::Additive, NewMoveDelta - State.MoveSpeedDelta);
	ASC->ApplyModToAttributeUnsafe(UBaseAttributeSet::GetAttackSpeedAttribute(), EGameplayModOp::Additive, NewAttackDelta - State.AttackSpeedDelta);

	State.MoveSpeedDelta = NewMoveDelta;
	State.AttackSpeedDelta = NewAttackDelta;
	if (AActor* TargetActor = State.TargetActor.Get())
	{
		GStickyOilMoveMultipliers.FindOrAdd(TObjectKey<AActor>(TargetActor)) = FMath::Clamp(1.0f - MoveSlow, 0.05f, 1.0f);
		GStickyOilTurnMultipliers.FindOrAdd(TObjectKey<AActor>(TargetActor)) = FMath::Clamp(1.0f - TurnSlow, 0.05f, 1.0f);
	}
}

void UCombatItemComponent::RemoveStickyOilAttributeDeltas(FStickyOilRuntimeState& State)
{
	if (UAbilitySystemComponent* ASC = State.ASC.Get())
	{
		if (!FMath::IsNearlyZero(State.MoveSpeedDelta))
		{
			ASC->ApplyModToAttributeUnsafe(UBaseAttributeSet::GetMoveSpeedAttribute(), EGameplayModOp::Additive, -State.MoveSpeedDelta);
		}
		if (!FMath::IsNearlyZero(State.AttackSpeedDelta))
		{
			ASC->ApplyModToAttributeUnsafe(UBaseAttributeSet::GetAttackSpeedAttribute(), EGameplayModOp::Additive, -State.AttackSpeedDelta);
		}

		const FGameplayTag StickyTag = TagStickyOil();
		if (StickyTag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(StickyTag, 0);
		}
	}

	if (AActor* TargetActor = State.TargetActor.Get())
	{
		GStickyOilMoveMultipliers.Remove(TObjectKey<AActor>(TargetActor));
		GStickyOilTurnMultipliers.Remove(TObjectKey<AActor>(TargetActor));
	}

	State.MoveSpeedDelta = 0.0f;
	State.AttackSpeedDelta = 0.0f;
	State.Stacks = 0;
}

void UCombatItemComponent::ExpireStickyOil(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	const TObjectKey<AActor> Key(TargetActor);
	if (FStickyOilRuntimeState* State = StickyOilStates.Find(Key))
	{
		RemoveStickyOilAttributeDeltas(*State);
		StickyOilStates.Remove(Key);
	}
}

void UCombatItemComponent::ClearAllStickyOil()
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<TObjectKey<AActor>, FStickyOilRuntimeState>& Pair : StickyOilStates)
		{
			World->GetTimerManager().ClearTimer(Pair.Value.ExpireTimer);
			RemoveStickyOilAttributeDeltas(Pair.Value);
		}
	}
	else
	{
		for (TPair<TObjectKey<AActor>, FStickyOilRuntimeState>& Pair : StickyOilStates)
		{
			RemoveStickyOilAttributeDeltas(Pair.Value);
		}
	}

	StickyOilStates.Reset();
}

void UCombatItemComponent::SetPlayerInsideSmoke(bool bInside, float DodgeBonus)
{
	UYogAbilitySystemComponent* ASC = GetOwnerYogASC();
	const FGameplayTag SmokeTag = TagInSmoke();
	if (!ASC || !SmokeTag.IsValid())
	{
		return;
	}

	if (bInside)
	{
		++ActiveSmokeAreaCount;
		if (!bSmokeDodgeApplied)
		{
			SmokeDodgeAppliedDelta = FMath::Max(0.0f, DodgeBonus);
			ASC->ApplyModToAttributeUnsafe(UBaseAttributeSet::GetDodgeAttribute(), EGameplayModOp::Additive, SmokeDodgeAppliedDelta);
			ASC->SetLooseGameplayTagCount(SmokeTag, 1);
			bSmokeDodgeApplied = true;
		}
		return;
	}

	ActiveSmokeAreaCount = FMath::Max(0, ActiveSmokeAreaCount - 1);
	if (ActiveSmokeAreaCount == 0 && bSmokeDodgeApplied)
	{
		ASC->ApplyModToAttributeUnsafe(UBaseAttributeSet::GetDodgeAttribute(), EGameplayModOp::Additive, -SmokeDodgeAppliedDelta);
		ASC->SetLooseGameplayTagCount(SmokeTag, 0);
		SmokeDodgeAppliedDelta = 0.0f;
		bSmokeDodgeApplied = false;
	}
}

int32 UCombatItemComponent::GetStickyOilStackCount(const AActor* TargetActor)
{
	const UAbilitySystemComponent* ASC = TargetActor ? TargetActor->FindComponentByClass<UAbilitySystemComponent>() : nullptr;
	const FGameplayTag StickyTag = TagStickyOil();
	return ASC && StickyTag.IsValid() ? ASC->GetTagCount(StickyTag) : 0;
}

float UCombatItemComponent::GetStickyOilTurnSpeedMultiplier(const AActor* TargetActor)
{
	if (TargetActor)
	{
		if (const float* Multiplier = GStickyOilTurnMultipliers.Find(TObjectKey<AActor>(const_cast<AActor*>(TargetActor))))
		{
			return *Multiplier;
		}
	}

	const int32 Stacks = GetStickyOilStackCount(TargetActor);
	return FMath::Clamp(1.0f - (0.04f * Stacks), 0.05f, 1.0f);
}

float UCombatItemComponent::GetStickyOilMoveSpeedMultiplier(const AActor* TargetActor)
{
	if (TargetActor)
	{
		if (const float* Multiplier = GStickyOilMoveMultipliers.Find(TObjectKey<AActor>(const_cast<AActor*>(TargetActor))))
		{
			return *Multiplier;
		}
	}

	const int32 Stacks = GetStickyOilStackCount(TargetActor);
	return FMath::Clamp(1.0f - (0.03f * Stacks), 0.05f, 1.0f);
}

void UCombatItemComponent::TryApplyOilFireBonus(UYogAbilitySystemComponent* SourceASC, UYogAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& Spec)
{
	if (!SourceASC || !TargetASC || !IsFireDamageSpec(Spec) || SpecHasTag(Spec, TagOilFireBonusDamage()))
	{
		return;
	}

	AActor* SourceActor = SourceASC->GetAvatarActor();
	const APawn* SourcePawn = Cast<APawn>(SourceActor);
	if (!SourcePawn || !SourcePawn->IsPlayerControlled())
	{
		return;
	}

	AActor* TargetActor = TargetASC->GetAvatarActor();
	const int32 StickyStacks = GetStickyOilStackCount(TargetActor);
	if (StickyStacks <= 0)
	{
		return;
	}

	FCombatItemOilConfig OilConfig;
	if (const UCombatItemComponent* SourceItemComponent = SourceActor->FindComponentByClass<UCombatItemComponent>())
	{
		for (const FRuntimeSlot& Slot : SourceItemComponent->Slots)
		{
			if (Slot.Config.EffectType == ECombatItemEffectType::OilBottle)
			{
				OilConfig = Slot.Config.Oil;
				break;
			}
		}
	}
	const float BonusDamage = FMath::Min(OilConfig.FireBonusDamageCap, OilConfig.FireBonusDamagePerStack * StickyStacks);
	if (BonusDamage <= 0.0f)
	{
		return;
	}

	ApplyItemPureDamage(SourceActor, TargetActor, BonusDamage, TEXT("Item_OilFireBonus"), true);
}

bool UCombatItemComponent::IsNoHitReactItemDamage(const FGameplayEffectSpec& Spec)
{
	return SpecHasTag(Spec, TagNoHitReactItemDamage());
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
		const FGameplayTag NoHitReactTag = TagNoHitReactItemDamage();
		if (NoHitReactTag.IsValid())
		{
			Spec.AddDynamicAssetTag(NoHitReactTag);
		}
	}
	if (DamageType == FName(TEXT("Item_OilFireBonus")))
	{
		const FGameplayTag OilBonusTag = TagOilFireBonusDamage();
		const FGameplayTag FireTag = TagFireDamage();
		if (OilBonusTag.IsValid())
		{
			Spec.AddDynamicAssetTag(OilBonusTag);
		}
		if (FireTag.IsValid())
		{
			Spec.AddDynamicAssetTag(FireTag);
		}
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	SourceASC->LogDamageDealt(TargetActor, Damage, DamageType);
}

void UCombatItemComponent::BroadcastSlotsChanged() const
{
	OnItemSlotsChanged.Broadcast(GetSlotViews());
}

APlayerCharacterBase* UCombatItemComponent::GetPlayerOwner() const
{
	return Cast<APlayerCharacterBase>(GetOwner());
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
		Slot.Charges = FMath::Clamp(Config.InitialCharges, 0, FMath::Max(Config.MaxCharges, 0));
	}
	ActiveSlotIndex = Slots.IsValidIndex(ActiveSlotIndex) ? ActiveSlotIndex : 0;
	BroadcastSlotsChanged();
}

void UCombatItemComponent::AdvanceCooldownsForTest(float DeltaTime)
{
	TickComponent(DeltaTime, LEVELTICK_All, nullptr);
}
#endif
