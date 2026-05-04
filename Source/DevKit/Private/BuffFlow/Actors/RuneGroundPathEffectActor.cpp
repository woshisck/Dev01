#include "BuffFlow/Actors/RuneGroundPathEffectActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

ARuneGroundPathEffectActor::ARuneGroundPathEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	AreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaBox"));
	SetRootComponent(AreaBox);
	AreaBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaBox->SetCollisionObjectType(ECC_WorldDynamic);
	AreaBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AreaBox->SetGenerateOverlapEvents(true);

	PathDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("PathDecal"));
	PathDecal->SetupAttachment(AreaBox);
	PathDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	PathDecal->SetVisibility(false);
}

void ARuneGroundPathEffectActor::InitializeGroundPath(AActor* InSourceActor, UAbilitySystemComponent* InSourceASC, const FRuneGroundPathEffectConfig& InConfig)
{
	SourceActor = InSourceActor;
	SourceASC = InSourceASC;
	Config = InConfig;

	const float Length = FMath::Max(1.0f, Config.Length);
	const float Width = FMath::Max(1.0f, Config.Width);
	const float Height = FMath::Max(1.0f, Config.Height);
	const float DecalProjectionDepth = FMath::Clamp(Config.DecalProjectionDepth, 1.0f, Height);
	AreaBox->SetBoxExtent(FVector(Length * 0.5f, Width * 0.5f, Height * 0.5f), true);

	if (PathDecal)
	{
		PathDecal->DecalSize = FVector(DecalProjectionDepth, Width, Length);
		PathDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, Config.DecalPlaneRotationDegrees));
		PathDecal->SetRelativeLocation(FVector(0.0f, 0.0f, DecalProjectionDepth * 0.5f));
		PathDecal->SetDecalMaterial(Config.DecalMaterial);
		if (UMaterialInstanceDynamic* DecalMID = PathDecal->CreateDynamicMaterialInstance())
		{
			DecalMID->SetScalarParameterValue(TEXT("FanMask"), Config.Shape == ERuneGroundPathShape::Fan ? 1.0f : 0.0f);
		}
		PathDecal->SortOrder = 25;
		PathDecal->FadeScreenSize = 0.001f;
		PathDecal->SetVisibility(Config.DecalMaterial != nullptr);
	}

	if (Config.NiagaraSystem)
	{
		SpawnPathNiagara(Length, Width);
	}

	ApplyPathTick();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TickTimerHandle,
			this,
			&ARuneGroundPathEffectActor::ApplyPathTick,
			FMath::Max(0.01f, Config.TickInterval),
			true);

		World->GetTimerManager().SetTimer(
			DestroyTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				Destroy();
			}),
			FMath::Max(0.01f, Config.Duration),
			false);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RuneGroundPath] Spawned Source=%s Effect=%s Policy=%d Shape=%d Length=%.1f Width=%.1f Height=%.1f Duration=%.2f Tick=%.2f Once=%d BurnDamage=%.2f Decal=%s DecalSize=%s DecalPlaneRot=%.1f Niagara=%s NiagaraCount=%d"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(Config.Effect),
		static_cast<int32>(Config.TargetPolicy),
		static_cast<int32>(Config.Shape),
		Length,
		Width,
		Height,
		Config.Duration,
		Config.TickInterval,
		Config.bApplyOncePerTarget ? 1 : 0,
		Config.SetByCallerValue1,
		*GetNameSafe(Config.DecalMaterial),
		PathDecal ? *PathDecal->DecalSize.ToCompactString() : TEXT("None"),
		Config.DecalPlaneRotationDegrees,
		*GetNameSafe(Config.NiagaraSystem),
		Config.NiagaraInstanceCount);
}

void ARuneGroundPathEffectActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
		World->GetTimerManager().ClearTimer(DestroyTimerHandle);
	}

	for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
	{
		if (NiagaraComponent)
		{
			NiagaraComponent->Deactivate();
		}
	}
	NiagaraComponents.Reset();

	Super::EndPlay(EndPlayReason);
}

void ARuneGroundPathEffectActor::SpawnPathNiagara(const float Length, const float Width)
{
	if (!Config.NiagaraSystem)
	{
		return;
	}

	const int32 InstanceCount = FMath::Clamp(Config.NiagaraInstanceCount, 1, 12);
	for (int32 Index = 0; Index < InstanceCount; ++Index)
	{
		const float Alpha = InstanceCount == 1 ? 0.5f : (static_cast<float>(Index) + 0.5f) / static_cast<float>(InstanceCount);
		const float LocalX = FMath::Lerp(-Length * 0.42f, Length * 0.42f, Alpha);
		float LocalY = 0.0f;
		if (Config.Shape == ERuneGroundPathShape::Fan)
		{
			const float Progress = FMath::Clamp(Alpha, 0.0f, 1.0f);
			const float HalfWidthAtDistance = FMath::Lerp(FMath::Min(30.0f, Width * 0.15f), Width * 0.42f, Progress);
			const int32 Pattern = Index % 3;
			LocalY = Pattern == 0 ? 0.0f : (Pattern == 1 ? -HalfWidthAtDistance * 0.38f : HalfWidthAtDistance * 0.38f);
		}

		UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Config.NiagaraSystem,
			AreaBox,
			NAME_None,
			FVector(LocalX, LocalY, 8.0f),
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true,
			true,
			ENCPoolMethod::AutoRelease);
		if (SpawnedComponent)
		{
			SpawnedComponent->SetRelativeScale3D(Config.NiagaraScale);
			NiagaraComponents.Add(SpawnedComponent);
		}
	}
}

void ARuneGroundPathEffectActor::ApplyPathTick()
{
	if (!Config.Effect || !SourceASC)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RuneGroundPathEffect), false, SourceActor);
	World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		GetActorQuat(),
		ObjectQueryParams,
		FCollisionShape::MakeBox(AreaBox ? AreaBox->GetScaledBoxExtent() : FVector::ZeroVector),
		QueryParams);

	TSet<AActor*> AppliedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || AppliedActors.Contains(TargetActor) || !ShouldAffectActor(TargetActor) || !IsActorInsidePathShape(TargetActor))
		{
			continue;
		}

		const TObjectKey<AActor> TargetKey(TargetActor);
		if (Config.bApplyOncePerTarget && OnceAppliedTargets.Contains(TargetKey))
		{
			continue;
		}

		AppliedActors.Add(TargetActor);
		if (ApplyEffectToActor(TargetActor) && Config.bApplyOncePerTarget)
		{
			OnceAppliedTargets.Add(TargetKey);
		}
	}
}

bool ARuneGroundPathEffectActor::ShouldAffectActor(AActor* Candidate) const
{
	if (!Candidate)
	{
		return false;
	}

	switch (Config.TargetPolicy)
	{
	case ERuneGroundPathTargetPolicy::EnemiesOnly:
		return Candidate != SourceActor && Cast<AEnemyCharacterBase>(Candidate) != nullptr;
	case ERuneGroundPathTargetPolicy::OwnerOnly:
		return Candidate == SourceActor;
	default:
		return false;
	}
}

bool ARuneGroundPathEffectActor::IsActorInsidePathShape(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	if (Config.Shape != ERuneGroundPathShape::Fan)
	{
		return true;
	}

	const float Length = FMath::Max(1.0f, Config.Length);
	const float Width = FMath::Max(1.0f, Config.Width);
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(TargetActor->GetActorLocation());
	const float ForwardDistance = LocalLocation.X + Length * 0.5f;
	if (ForwardDistance < 0.0f || ForwardDistance > Length)
	{
		return false;
	}

	const float Progress = FMath::Clamp(ForwardDistance / Length, 0.0f, 1.0f);
	const float MinHalfWidth = FMath::Min(30.0f, Width * 0.15f);
	const float HalfWidthAtDistance = FMath::Lerp(MinHalfWidth, Width * 0.5f, Progress);
	return FMath::Abs(LocalLocation.Y) <= HalfWidthAtDistance;
}

bool ARuneGroundPathEffectActor::ApplyEffectToActor(AActor* TargetActor)
{
	if (!TargetActor || !Config.Effect || !SourceASC)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	if (SourceActor)
	{
		Context.AddInstigator(SourceActor, SourceActor);
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Config.Effect, 1.0f, Context);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	if (Config.SetByCallerTag1.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Config.SetByCallerTag1, Config.SetByCallerValue1);
	}
	if (Config.SetByCallerTag2.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Config.SetByCallerTag2, Config.SetByCallerValue2);
	}

	for (int32 Index = 0; Index < FMath::Max(1, Config.ApplicationCount); ++Index)
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}

	UE_LOG(LogTemp, Warning, TEXT("[RuneGroundPath] Applied Effect=%s Target=%s Count=%d"),
		*GetNameSafe(Config.Effect),
		*GetNameSafe(TargetActor),
		FMath::Max(1, Config.ApplicationCount));
	return true;
}

FVector ARuneGroundPathEffectActor::FindGroundLocation(const FVector& DesiredLocation) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return DesiredLocation;
	}

	FHitResult Hit;
	const FVector Start = DesiredLocation + FVector(0.0f, 0.0f, 240.0f);
	const FVector End = DesiredLocation - FVector(0.0f, 0.0f, 900.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RuneGroundPathGroundTrace), false, SourceActor);
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
	{
		return Hit.Location + FVector(0.0f, 0.0f, 3.0f);
	}

	return DesiredLocation;
}
