// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Components/ComboGraphCollisionComponent.h"

#include "ComboGraphLog.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Utils/ComboGraphUtils.h"

UComboGraphCollisionComponent::UComboGraphCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
	bAutoActivate = true;

	// Debug colors
	TraceColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("A8A3FF")));
	TraceHitColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("BADA55")));

	// Setup default collision profile names to include "Pawn", thus ignore collision capsule, and collide with Character Mesh which seems to be sensible defaults
	CollisionProfilesToIgnore.Add(TEXT("Pawn"));
}

void UComboGraphCollisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCanTrace)
	{
		TraceTick();
	}
}

void UComboGraphCollisionComponent::StartTrace()
{
	bCanTrace = true;
	ResetHitActors();
	ResetSocketLocations();
	OnTraceStart.Broadcast();
}

void UComboGraphCollisionComponent::EndTrace()
{
	bCanTrace = false;
	bIsTraceRunning = false;
	OnTraceEnd.Broadcast();
}

void UComboGraphCollisionComponent::RegisterCollisionMesh(UPrimitiveComponent* InMesh)
{
	if (InMesh)
	{
		RegisteredMeshes.AddUnique(InMesh);
	}
}

void UComboGraphCollisionComponent::UnregisterCollisionMesh(UPrimitiveComponent* InMesh)
{
	if (InMesh)
	{
		RegisteredMeshes.Remove(InMesh);
	}
}

void UComboGraphCollisionComponent::TraceTick()
{
	if (bIsTraceRunning)
	{
		DoTraceCheckForRegisteredMeshes();
	}

	bIsTraceRunning = true;
	UpdateSocketLocations();
}

void UComboGraphCollisionComponent::DoTraceCheckForRegisteredMeshes()
{
	for (UPrimitiveComponent* Mesh : RegisteredMeshes)
	{
		if (!Mesh)
		{
			continue;
		}

		TArray<FName> Sockets = Mesh->GetAllSocketNames();
		for (const FName Socket : Sockets)
		{
			DoTraceCheckForSocket(Mesh, Socket);
		}
	}
}

void UComboGraphCollisionComponent::DoTraceCheckForSocket(UPrimitiveComponent* InMesh, const FName InSocketName)
{
	// Calculate Previous / Current Socket location for trace
	const FName SocketUID = CreateSocketUID(InMesh, InSocketName);
	const FVector CurrentLocation = InMesh->GetSocketLocation(InSocketName);
	const FVector* PreviousLocation = LastFrameSocketLocations.Find(SocketUID);

	if (!PreviousLocation)
	{
		// CG_RUNTIME_LOG(Warning, TEXT("UComboGraphCollisionComponent::DoTraceCheckForSocket - Something went wrong, couldn't get back Previous Location for %s. This should not happen."), *SocketUID.ToString());
		return;
	}

	const EDrawDebugTrace::Type DrawDebugType = bDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	TArray<FHitResult> HitResults;
	// Perform Trace by Channel
	const bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		this,
		*PreviousLocation,
		CurrentLocation,
		TraceRadius,
		CollisionTraceChannel,
		bTraceComplex,
		HitActors,
		DrawDebugType,
		HitResults,
		true,
		TraceColor,
		TraceHitColor,
		DebugDrawTime
	);

	if (!bHit)
	{
		// No hit, do nothing
		return;
	}


	for (FHitResult HitResult : HitResults)
	{

		AActor* HitActor = HitResult.GetActor();
		UPrimitiveComponent* HitComponent = HitResult.GetComponent();

		FString FailReason;
		if (IsSuccessfulHit(HitActor, HitComponent, FailReason))
		{
			HitActors.Add(HitActor);
			HandleSuccessfulHit(HitResult, HitActor, HitComponent, InMesh);
		}
		else if (bShouldLogHits)
		{
			CG_RUNTIME_LOG(Display, TEXT("UComboGraphCollisionComponent::DoTraceCheckForSocket - Got a hit for Mesh %s but was ignored. Reason: %s"), *GetNameSafe(InMesh), *FailReason);
		}
	}
}

void UComboGraphCollisionComponent::HandleSuccessfulHit(const FHitResult InHitResult, AActor* HitActor, UPrimitiveComponent* HitComponent, UPrimitiveComponent* InMesh)
{
	if (bShouldLogHits)
	{
		CG_RUNTIME_LOG(
			Display,
			TEXT("UComboGraphCollisionComponent::HandleSuccessfulHit for Mesh: %s. Hit Actor: %s (Type: %s), Hit Component: %s (Profile: %s)"),
			*GetNameSafe(InMesh),
			*GetNameSafe(HitActor),
			// Hit Actor should be valid if it passed IsSuccessfulHit checks
			*GetNameSafe(HitActor->GetClass()),
			*GetNameSafe(HitComponent),
			HitComponent ? *HitComponent->GetCollisionProfileName().ToString() : TEXT("NONE")
		);
	}
	
	// This should only happen on server as Notifies only activate collision for owners with authority
	OnHitRegistered.Broadcast(InHitResult, InMesh);
}

void UComboGraphCollisionComponent::UpdateSocketLocations()
{
	for (UPrimitiveComponent* Mesh : RegisteredMeshes)
	{
		if (!Mesh)
		{
			continue;
		}

		TArray<FName> Sockets = Mesh->GetAllSocketNames();
		for (const FName Socket : Sockets)
		{
			FName SocketUID = CreateSocketUID(Mesh, Socket);
			FVector SocketLocation = Mesh->GetSocketLocation(Socket);
			LastFrameSocketLocations.Add(SocketUID, SocketLocation);
		}
	}
}

FName UComboGraphCollisionComponent::CreateSocketUID(UPrimitiveComponent* InMesh, FName InSocketName)
{
	if (!InMesh)
	{
		return NAME_None;
	}

	const FString MeshID = FString::Printf(TEXT("%s.%s"), *InMesh->GetName(), *InSocketName.ToString());
	return FName(*MeshID);
}

void UComboGraphCollisionComponent::ResetHitActors()
{
	HitActors.Empty();
	HitActors.Add(GetOwner());
}

void UComboGraphCollisionComponent::ResetSocketLocations()
{
	LastFrameSocketLocations.Reset();
}

bool UComboGraphCollisionComponent::IsIgnoredActorType(const TSubclassOf<AActor> InActorType) const
{
	if (!InActorType)
	{
		return true;
	}

	bool bIgnored = false;
	for (TSubclassOf<AActor> IgnoredActorType : ActorTypesToIgnore)
	{
		if (IgnoredActorType != nullptr && InActorType->IsChildOf(IgnoredActorType))
		{
			bIgnored = true;
			break;
		}
	}

	return bIgnored;
}

bool UComboGraphCollisionComponent::IsSuccessfulHit(AActor* HitActor, UPrimitiveComponent* HitComponent, FString& FailReason) const
{
	if (!HitActor || !HitComponent)
	{
		FailReason = TEXT("Either Hit Actor or Hit Component is invalid");
		return false;
	}

	// Hit Actor was not already hit for this collision
	if (HitActors.Contains(HitActor))
	{
		FailReason = FString::Printf(TEXT("Actor %s was already hit in previous collision checks."), *GetNameSafe(HitActor));
		return false;
	}

	// Hit Actor Class is not part of the ignored classes
	if (IsIgnoredActorType(HitActor->GetClass()))
	{
		FailReason = FString::Printf(TEXT("Actor %s of type %s was ignored because of ActorTypesToIgnore configuration."), *GetNameSafe(HitActor), *GetNameSafe(HitActor->GetClass()));
		return false;
	}

	// Hit Component collision profile is not part of ignored profiles
	if (CollisionProfilesToIgnore.Contains(HitComponent->GetCollisionProfileName()))
	{
		FailReason = FString::Printf(TEXT("Hit Component %s with profile %s was ignored because of CollisionProfilesToIgnore configuration."), *GetNameSafe(HitComponent), *HitComponent->GetCollisionProfileName().ToString());
		return false;
	}

	return true;
}

bool UComboGraphCollisionComponent::HasAuthority() const
{
	return GetOwnerRole() == ROLE_Authority;
}

