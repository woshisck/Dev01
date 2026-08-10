#include "Component/HitImpactVisualComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystem/GameplayCue/BuffHitFeedbackRow.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Character/YogCharacterBase.h"
#include "Data/EnemyHitImpactData.h"
#include "System/YogSettings.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UHitImpactVisualComponent::UHitImpactVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHitImpactVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(false);
}

void UHitImpactVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearVisualOffset();

	Super::EndPlay(EndPlayReason);
}

void UHitImpactVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHitPushActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float SafeOutDuration = FMath::Max(0.001f, PushOutDuration);
	const float SafeReturnDuration = FMath::Max(0.001f, ReturnDuration);

	ElapsedTime += FMath::Max(0.0f, DeltaTime);

	if (ElapsedTime <= SafeOutDuration)
	{
		const float Alpha = FMath::Clamp(ElapsedTime / SafeOutDuration, 0.0f, 1.0f);
		const float EaseAlpha = 1.0f - FMath::Square(1.0f - Alpha);
		ApplyVisualOffset(FMath::Lerp(StartOffset, PeakOffset, EaseAlpha));
		return;
	}

	const float ReturnAlpha = FMath::Clamp((ElapsedTime - SafeOutDuration) / SafeReturnDuration, 0.0f, 1.0f);
	const float EaseAlpha = ReturnAlpha * ReturnAlpha * (3.0f - 2.0f * ReturnAlpha);
	ApplyVisualOffset(FMath::Lerp(PeakOffset, FVector::ZeroVector, EaseAlpha));

	if (ReturnAlpha >= 1.0f)
	{
		ClearVisualOffset();
	}
}

const FHitImpactTierFX* UHitImpactVisualComponent::ResolveTierFX() const
{
	// Armor promotes any base tier to Hard while ArmorHP > 0.
	EHitReactTier Tier = HitReactTier;
	if (const AYogCharacterBase* OwnerChar = Cast<AYogCharacterBase>(GetOwner()))
	{
		if (const UYogAbilitySystemComponent* ASC = OwnerChar->GetASC())
		{
			if (ASC->GetNumericAttribute(UBaseAttributeSet::GetArmorHPAttribute()) > 0.f)
			{
				Tier = EHitReactTier::Hard;
			}
		}
	}

	const UYogSettings* YogSettings = UYogSettings::Get();
	const UEnemyHitImpactData* Data = YogSettings ? YogSettings->EnemyHitImpactData.LoadSynchronous() : nullptr;
	return Data ? &Data->GetTierFX(Tier) : nullptr;
}

const FBuffHitFeedbackRow* UHitImpactVisualComponent::ResolveBuffHitFeedback() const
{
	const AYogCharacterBase* OwnerChar = Cast<AYogCharacterBase>(GetOwner());
	const UYogAbilitySystemComponent* ASC = OwnerChar ? OwnerChar->GetASC() : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	const UYogSettings* YogSettings = UYogSettings::Get();
	UDataTable* Table = YogSettings ? YogSettings->BuffHitFeedbackTable.LoadSynchronous() : nullptr;
	if (!Table)
	{
		return nullptr;
	}

	static const FString ContextString(TEXT("HitImpactVisual_BuffHitFeedback"));
	TArray<FBuffHitFeedbackRow*> Rows;
	Table->GetAllRows(ContextString, Rows);

	const FBuffHitFeedbackRow* Best = nullptr;
	for (const FBuffHitFeedbackRow* Row : Rows)
	{
		if (Row && Row->BuffTag.IsValid() && ASC->HasMatchingGameplayTag(Row->BuffTag))
		{
			if (!Best || Row->Priority > Best->Priority)
			{
				Best = Row;
			}
		}
	}

	return Best;
}

int32 UHitImpactVisualComponent::PlayHitFeedback(const FVector& HitLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	const FBuffHitFeedbackRow* Row = ResolveBuffHitFeedback();
	const FHitImpactTierFX* TierFX = ResolveTierFX();

	// VFX: buff-row VFX takes precedence over the tier default.
	UNiagaraSystem* EffectiveVFX = (Row && Row->ImpactVFX) ? Row->ImpactVFX.Get()
		: (TierFX ? TierFX->VFX.Get() : nullptr);
	if (EffectiveVFX)
	{
		const bool bUseRowVFX = (Row && Row->ImpactVFX);
		const FVector EffectiveScale = bUseRowVFX ? Row->VFXScale : TierFX->VFXScale;
		const FRotator EffectiveRotation = bUseRowVFX ? Row->VFXRotationOffset : FRotator::ZeroRotator;
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, EffectiveVFX, HitLocation, EffectiveRotation, EffectiveScale);
	}

	// SFX: a buff-row sound replaces the tier default unless bReplaceDefaultSound is false.
	bool bBuffReplacedSound = false;
	if (Row && Row->ImpactSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			World, Row->ImpactSound, HitLocation, FRotator::ZeroRotator, Row->SoundVolume, Row->SoundPitch);
		bBuffReplacedSound = Row->bReplaceDefaultSound;
	}

	if (!bBuffReplacedSound && TierFX && TierFX->Sound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			World, TierFX->Sound, HitLocation, FRotator::ZeroRotator, TierFX->SoundVolume, TierFX->SoundPitch);
	}

	return Row ? Row->CameraShakeLevel : 0;
}

void UHitImpactVisualComponent::PlayHitPush(AActor* SourceActor, float Strength)
{
	if (!SourceActor)
	{
		return;
	}

	PlayHitPushFromLocation(SourceActor->GetActorLocation(), Strength);
}

void UHitImpactVisualComponent::PlayHitPushFromLocation(FVector SourceLocation, float Strength)
{
	AActor* Owner = GetOwner();
	USkeletalMeshComponent* Mesh = ResolveMesh();
	if (!bEnableHitPush || !Owner || !Mesh)
	{
		return;
	}

	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.0f;
	if (bHitPushActive && MinRefreshInterval > 0.0f && Now - LastPushStartTime < MinRefreshInterval)
	{
		return;
	}

	FVector PushDirection = Owner->GetActorLocation() - SourceLocation;
	PushDirection.Z = 0.0f;
	if (!PushDirection.Normalize())
	{
		PushDirection = -Owner->GetActorForwardVector();
		PushDirection.Z = 0.0f;
		if (!PushDirection.Normalize())
		{
			return;
		}
	}

	const float Distance = FMath::Max(0.0f, PushDistance * FMath::Max(0.0f, Strength));
	if (Distance <= KINDA_SMALL_NUMBER && FMath::IsNearlyZero(VerticalLift))
	{
		return;
	}

	const FVector WorldOffset = PushDirection * Distance + FVector::UpVector * VerticalLift;
	const FVector LocalOffset = ConvertWorldOffsetToMeshParentSpace(Mesh, WorldOffset);
	const float MaxDistance = FMath::Max(0.0f, MaxAccumulatedPushDistance);

	StartOffset = AppliedOffset;
	PeakOffset = MaxDistance > 0.0f
		? (AppliedOffset + LocalOffset).GetClampedToMaxSize(MaxDistance)
		: FVector::ZeroVector;

	ElapsedTime = 0.0f;
	LastPushStartTime = Now;
	bHitPushActive = true;
	SetComponentTickEnabled(true);

	if (PushOutDuration <= 0.0f)
	{
		ApplyVisualOffset(PeakOffset);
	}
}

void UHitImpactVisualComponent::StopHitPush()
{
	ClearVisualOffset();
}

USkeletalMeshComponent* UHitImpactVisualComponent::ResolveMesh() const
{
	if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		return CharacterOwner->GetMesh();
	}

	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

FVector UHitImpactVisualComponent::ConvertWorldOffsetToMeshParentSpace(const USkeletalMeshComponent* Mesh, const FVector& WorldOffset) const
{
	if (!Mesh)
	{
		return FVector::ZeroVector;
	}

	if (const USceneComponent* Parent = Mesh->GetAttachParent())
	{
		return Parent->GetComponentTransform().InverseTransformVectorNoScale(WorldOffset);
	}

	return Mesh->GetComponentTransform().InverseTransformVectorNoScale(WorldOffset);
}

void UHitImpactVisualComponent::ApplyVisualOffset(const FVector& NewOffset)
{
	USkeletalMeshComponent* Mesh = ResolveMesh();
	if (!Mesh)
	{
		AppliedOffset = FVector::ZeroVector;
		return;
	}

	const FVector DeltaOffset = NewOffset - AppliedOffset;
	if (!DeltaOffset.IsNearlyZero())
	{
		Mesh->SetRelativeLocation(Mesh->GetRelativeLocation() + DeltaOffset, false, nullptr, ETeleportType::TeleportPhysics);
		AppliedOffset = NewOffset;
	}
}

void UHitImpactVisualComponent::ClearVisualOffset()
{
	ApplyVisualOffset(FVector::ZeroVector);

	AppliedOffset = FVector::ZeroVector;
	StartOffset = FVector::ZeroVector;
	PeakOffset = FVector::ZeroVector;
	ElapsedTime = 0.0f;
	bHitPushActive = false;
	SetComponentTickEnabled(false);
}
