// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBlueprintFunctionLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "Animation/YogAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <DevKit/Buff/Aura/AuraBase.h>
#include <DevKit/Buff/Aura/AuraDefinition.h>
#include "System/YogGameInstanceBase.h"
#include "DevKit/Data/WeaponData.h"
#include "NavigationSystem.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Engine/OverlapResult.h"


//UYogBlueprintFunctionLibrary::UYogBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer)
//	: Super(ObjectInitializer)
//{
//}


bool UYogBlueprintFunctionLibrary::IsInEditor()
{

	return GIsEditor;
}


FName UYogBlueprintFunctionLibrary::GetDTRow(FString AssetName, int32 rowNum)
{
	FString result;
	result.Append(AssetName);
	result.Append(TEXT("Lvl_"));
	result += FString::Printf(TEXT("%u"), rowNum);

	return FName(result);
}

void UYogBlueprintFunctionLibrary::PrintGameplayTagContainer(FGameplayTagContainer gameplaytag_container)
{
	UE_LOG(LogTemp, Warning, TEXT("Yog TagContainer Contents: %s"), *gameplaytag_container.ToString());
}

TArray<int> UYogBlueprintFunctionLibrary::RandomSplitInteger(int Total, int NumberParts)
{
	TArray<int32> Result;

	// 1. VALIDATE INPUTS FIRST
	if (NumberParts <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("RandomSplitInteger: NumberParts must be positive (got %d)"), NumberParts);
		return Result;
	}

	if (Total <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RandomSplitInteger: Total must be positive (got %d)"), Total);
		// Return array of zeros
		for (int32 i = 0; i < NumberParts; i++)
		{
			Result.Add(0);
		}
		return Result;
	}

	// 2. INITIALIZE the Result array FIRST!
	Result.Init(0, NumberParts);  // Initialize all to zero

	// 3. Create random stream
	FRandomStream RandomStream;
	RandomStream.GenerateNewSeed();

	// 4. Distribute the total
	for (int32 i = 0; i < Total; i++)
	{
		// SAFE: Use RandRange for integer range
		int32 RandomIndex = RandomStream.RandRange(0, NumberParts - 1);
		Result[RandomIndex]++;
	}

	return Result;

}

TArray<AActor*> UYogBlueprintFunctionLibrary::GetAllActorsFromRange(UObject* WorldContextObject, AActor* OriginActor, float Range, bool parallel, TSubclassOf<AActor> ActorClass)
{
	TArray<AActor*> Result;
	TArray<FOverlapResult> Overlaps;

	// Create collision shape (sphere)
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Range);

	// Perform overlap check
	bool bHit = OriginActor->GetWorld()->OverlapMultiByChannel(
		Overlaps,
		OriginActor->GetActorLocation(),
		FQuat::Identity,
		ECC_WorldDynamic, // Use appropriate collision channel
		Sphere
	);

	// Filter by class
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (Actor && Actor->IsA(ActorClass))
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

/*parallel is for many actors select at the same time */
TArray<AActor*> UYogBlueprintFunctionLibrary::ASync_GetAllActorsFromRange(UObject* WorldContextObject, const FVector& Center, float Radius, TSubclassOf<AActor> ActorClass)
{
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ActorClass, AllActors);

	TQueue<AActor*> ThreadSafeQueue;
	const float RadiusSquared = Radius * Radius;

	ParallelFor(AllActors.Num(), [&](int32 Index)
		{
			AActor* Actor = AllActors[Index];
			if (Actor && FVector::DistSquared(Center, Actor->GetActorLocation()) <= RadiusSquared)
			{
				ThreadSafeQueue.Enqueue(Actor);
			}
		});

	// Convert queue to array
	TArray<AActor*> FilteredActors;
	AActor* Actor = nullptr;
	while (ThreadSafeQueue.Dequeue(Actor))
	{
		FilteredActors.Add(Actor);
	}

	return FilteredActors;
}



void UYogBlueprintFunctionLibrary::GiveWeaponToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition, UWeaponData* WeaponData)
{
	check(WorldContextObject && ReceivingChar && WeaponDefinition && WeaponData);

	UE_LOG(LogTemp, Warning, TEXT("GiveWeaponToCharacter IS DEPRECATED, NOT USED ANYMORE!!"));


		//UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		//USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
	
		//WeaponDefinition->SetupWeaponToCharacter(World, AttachTarget, ReceivingChar);

		//WeaponData->GrantAbilityToOwner(ReceivingChar);

}

void UYogBlueprintFunctionLibrary::EquipWeapon(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponData* WeaponData)
{
	
	ensure(WorldContextObject && ReceivingChar && WeaponData);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();

	WeaponData->GiveWeapon(World, AttachTarget, ReceivingChar);


}

void UYogBlueprintFunctionLibrary::GiveAbilityToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UAbilityData* AbilityData)
{
	if (WorldContextObject)
	{
		//UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
		ReceivingChar->AbilityData = AbilityData;		
	}

}
bool UYogBlueprintFunctionLibrary::GiveEffectToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition)
{
	return false;
}

UAnimSequence* UYogBlueprintFunctionLibrary::GetCurrentSlotAnimation(USkeletalMeshComponent* MeshComp, FName SlotName)
{
	if (!MeshComp || !MeshComp->GetAnimInstance()) return nullptr;

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();

	if (!MontageInstance || !MontageInstance->Montage) return nullptr;

	// Get the current position in the montage
	float CurrentPosition = MontageInstance->GetPosition();

	// Find the slot track
	const FSlotAnimationTrack* SlotTrack = MontageInstance->Montage->SlotAnimTracks.FindByPredicate(
		[SlotName](const FSlotAnimationTrack& Track) { return Track.SlotName == SlotName; });

	if (SlotTrack)
	{
		// Find which segment is currently playing
		const FAnimSegment* CurrentSegment = SlotTrack->AnimTrack.GetSegmentAtTime(CurrentPosition);
		if (CurrentSegment && CurrentSegment->GetAnimReference())
		{
			return Cast<UAnimSequence>(CurrentSegment->GetAnimReference());
		}
	}

	return nullptr;
}

UAnimSequence* UYogBlueprintFunctionLibrary::SetSequenceRootMotion(UAnimSequence* sequence, bool enableRoot)
{
	sequence->bRootMotionSettingsCopiedFromMontage = false;
	sequence->EnableRootMotionSettingFromMontage(enableRoot ,  ERootMotionRootLock::RefPose);
	return sequence;
}

EYogCharacterState UYogBlueprintFunctionLibrary::GetCharacterState(AYogCharacterBase* character)
{

	if (character)
	{
		return character->GetCurrentState();
	}
	else
	{
		return EYogCharacterState::Idle;
	}
}

bool UYogBlueprintFunctionLibrary::LaunchCharacterWithDist(AYogCharacterBase* character, FVector Direction, float Distance, bool bHorizontalOnly, bool bVerticalOnl)
{
	if (character)
	{
		float Speed = FMath::Sqrt(2.f * character->GetCharacterMovement()->GetGravityZ() * Distance);
		character->GetCharacterMovement()->Launch(Direction * Speed);
		return true;
	}
	else
	{
		return false;
	}

}


bool UYogBlueprintFunctionLibrary::SpawnAura(UObject* WorldContextObject, AYogCharacterBase* character, UAuraDefinition* auraDefinition)
{
	if (WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		TSubclassOf<AAuraBase> AuraSpawnClass = auraDefinition->AuraClass;

		//FActorSpawnParameters SpawnParams;
		//SpawnParams.Owner = character; // Set owner if needed
		//SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


		AAuraBase* newAura = World->SpawnActorDeferred<AAuraBase>(AuraSpawnClass, FTransform::Identity, character);


		newAura->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		newAura->AttachToActor(character, FAttachmentTransformRules::KeepRelativeTransform);

		return true;
	
	}
	else
	{
		return false;
	}




}

FVector2D UYogBlueprintFunctionLibrary::PerpendicularClockWise(FVector2D vector)
{
	return FVector2D(vector.Y, -vector.X);
}

FVector2D UYogBlueprintFunctionLibrary::PerpendicularAntiClockWise(FVector2D vector)
{
	return FVector2D(-vector.Y, -vector.X);
}

bool UYogBlueprintFunctionLibrary::TargetLocIsInTriangle(FVector targetLoc, FVector pointA, FVector pointB, FVector pointC)
{
	//https://www.baeldung.com/cs/check-if-point-is-in-2d-triangle

	FVector Normal = FVector(0, 0, -1);

	FVector vec_BA = pointA - pointB;
	FVector vec_CB = pointB - pointC;
	FVector vec_AC = pointC - pointA;

	FVector vec_Bp = targetLoc - pointB;
	FVector vec_Cp = targetLoc - pointC;
	FVector vec_Ap = targetLoc - pointA;


	UE_LOG(LogTemp, Warning, TEXT("FVector::CrossProduct(vec_BA, vec_Bp): %f, %f, %f"), FVector::CrossProduct(vec_BA, vec_Bp).X, FVector::CrossProduct(vec_BA, vec_Bp).Y, FVector::CrossProduct(vec_BA, vec_Bp).Z);
	UE_LOG(LogTemp, Warning, TEXT("FVector::CrossProduct(vec_BA, vec_Bp): %f, %f, %f"), FVector::CrossProduct(vec_CB, vec_Cp).X, FVector::CrossProduct(vec_CB, vec_Cp).Y, FVector::CrossProduct(vec_CB, vec_Cp).Z);
	UE_LOG(LogTemp, Warning, TEXT("FVector::CrossProduct(vec_BA, vec_Bp): %f, %f, %f"), FVector::CrossProduct(vec_AC, vec_Ap).X, FVector::CrossProduct(vec_AC, vec_Ap).Y, FVector::CrossProduct(vec_AC, vec_Ap).Z);

	if (FVector::DotProduct(Normal, FVector::CrossProduct(vec_BA, vec_Bp)) > 0 &&
		FVector::DotProduct(Normal, FVector::CrossProduct(vec_CB, vec_Cp)) > 0 &&
		FVector::DotProduct(Normal, FVector::CrossProduct(vec_AC, vec_Ap)) > 0)
	{ 
		return true;
	}
	else
	{
		return false;
	}

}


void UYogBlueprintFunctionLibrary::DrawDebugAnnulusSector(UObject* WorldContextObject, FVector Center, float InnerRadius, float OuterRadius, float StartAngleDeg, float EndAngleDeg, FColor Color)
{

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);


	const int32 NumSegments = 36;
	float StartAngle = FMath::DegreesToRadians(StartAngleDeg);
	float EndAngle = FMath::DegreesToRadians(EndAngleDeg);

	// Helper to draw an arc
	auto DrawArc = [&](float Radius) {
		FVector LastVertex = Center + FVector(FMath::Cos(StartAngle), FMath::Sin(StartAngle), 0) * Radius;
		for (int32 i = 1; i <= NumSegments; i++) {
			float Angle = StartAngle + (EndAngle - StartAngle) * i / NumSegments;
			FVector ThisVertex = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
			DrawDebugLine(World, LastVertex, ThisVertex, Color, true, 99.9f, 0, 4.0f);
			LastVertex = ThisVertex;
		}
		};

	DrawArc(InnerRadius);
	DrawArc(OuterRadius);

	// Draw the two radial lines connecting the arcs
	FVector InnerStart = Center + FVector(FMath::Cos(StartAngle), FMath::Sin(StartAngle), 0) * InnerRadius;
	FVector OuterStart = Center + FVector(FMath::Cos(StartAngle), FMath::Sin(StartAngle), 0) * OuterRadius;
	FVector InnerEnd = Center + FVector(FMath::Cos(EndAngle), FMath::Sin(EndAngle), 0) * InnerRadius;
	FVector OuterEnd = Center + FVector(FMath::Cos(EndAngle), FMath::Sin(EndAngle), 0) * OuterRadius;


	DrawDebugLine(World, InnerStart, OuterStart, Color, true, 99.9f, 0, 4.0f);
	DrawDebugLine(World, InnerEnd, OuterEnd, Color, true, 99.9f, 0, 4.0f);
}

bool UYogBlueprintFunctionLibrary::DrawDebugTriangle(UObject* WorldContextObject, FVector pointA, FVector pointB, FVector pointC)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		FColor lineColor = FColor(1, 0, 0);
		DrawDebugLine(World, pointA, pointB, lineColor, false, 15.f, 0, 5.f);
		return true;
	}
	else
	{
		return false;
	}

}


FVector UYogBlueprintFunctionLibrary::DistFromPointToPoint(UObject* WorldContextObject, FVector pointA, FVector pointB)
{
	int x1 = pointA.X;
	int y1 = pointA.Y;

	int x2 = pointB.X;
	int y2 = pointB.Y;



	float A = y2 - y1;
	float B = x1 - x2;
	float C = x2 * y1 - x1 * y2;

	return FVector(A, B, C);
}

float UYogBlueprintFunctionLibrary::DistFromPointToLine(UObject* WorldContextObject, FVector pointA, FVector Coef)
{
	float A = Coef.X;
	float B = Coef.Y;
	float C = Coef.Z;
	float x = pointA.X;
	float y = pointA.Y;

	float Distance = abs((A * x + B * y + C) / FMath::Sqrt(pow(A, 2) + pow(B, 2)));
	return Distance;
}

TArray<FYogCollisionTriangle> UYogBlueprintFunctionLibrary::MakeTriangleArray(UObject* WorldContextObject, TArray<FVector> point_array, FVector Playerloc)
{
	TArray<FYogCollisionTriangle> result;
	for (int i = 0; i < point_array.Num(); i++)
	{
		if (i > 0)
		{
			FVector current_Point = point_array[i];
			FVector last_Point = point_array[i - 1];
			FYogCollisionTriangle triangle = FYogCollisionTriangle(last_Point, current_Point, Playerloc);
			result.Add(triangle);
		}
	}

	return result;
}

bool UYogBlueprintFunctionLibrary::DrawTriangle(UObject* WorldContextObject, FYogCollisionTriangle triangle)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		FColor lineColor = FColor(1, 0, 0);

		DrawDebugLine(World, triangle.PointA, triangle.PointB, lineColor, false, 15.f, 0, 5.f);
		DrawDebugLine(World, triangle.PointB, triangle.PointC, lineColor, false, 15.f, 0, 5.f);
		DrawDebugLine(World, triangle.PointC, triangle.PointA, lineColor, false, 15.f, 0, 5.f);
		return true;
	}
	else
	{
		return false;
	}
}

UYogGameInstanceBase* UYogBlueprintFunctionLibrary::GetYogGameInstance(UObject* WorldContextObject)
{

	return  Cast<UYogGameInstanceBase>(WorldContextObject->GetWorld()->GetGameInstance());

}

TArray<FYogApplyEffect> UYogBlueprintFunctionLibrary::MergeApplyEffects(const TArray<FYogApplyEffect>& SourceArray)
{
	TMap<FYogApplyEffect, int32> LevelAccumulator;

	// First pass: accumulate levels
	for (const FYogApplyEffect& Effect : SourceArray)
	{
		// Add or update the accumulated level
		LevelAccumulator.FindOrAdd(Effect) += Effect.level;
	}

	// Second pass: create result array with accumulated levels
	TArray<FYogApplyEffect> Result;
	for (const auto& Pair : LevelAccumulator)
	{
		FYogApplyEffect MergedEffect = Pair.Key; // Copy the base effect
		MergedEffect.level = Pair.Value; // Set the accumulated level
		Result.Add(MergedEffect);
	}

	return Result;
}


float UYogBlueprintFunctionLibrary::DistFromPointToTriangle(UObject* WorldContextObject, FVector target_point, FYogCollisionTriangle triangle)
{
	//DONT FORGET TO INCLUDE CAPSULE COMPONENT RADIUS FOR CHECK
	// FVector pointA, FVector pointB, FVector pointC
	FVector pointA = triangle.PointA;
	FVector pointB = triangle.PointB;
	FVector pointC = triangle.PointC;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		// Compute vectors
		FVector AB = pointB - pointA;
		FVector AC = pointC - pointA;
		FVector AP = target_point - pointA;

		// Compute dot products
		float d1 = FVector::DotProduct(AB, AP);
		float d2 = FVector::DotProduct(AC, AP);

		// Check if target_point is vertex region outside pointA
		if (d1 <= 0.0f && d2 <= 0.0f)
			return FVector::Dist(target_point, pointA);

		// Check if target_point is in vertex region outside pointB
		FVector BP = target_point - pointB;
		float d3 = FVector::DotProduct(AB, BP);
		float d4 = FVector::DotProduct(AC, BP);
		if (d3 >= 0.0f && d4 <= d3)
			return FVector::Dist(target_point, pointB);

		// Check if target_point is in edge region of AB, if so return distance to AB
		float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
			float v = d1 / (d1 - d3);
			FVector Closesttarget_point = pointA + v * AB;
			return FVector::Dist(target_point, Closesttarget_point);
		}

		// Check if target_point is in vertex region outside pointC
		FVector CP = target_point - pointC;
		float d5 = FVector::DotProduct(AB, CP);
		float d6 = FVector::DotProduct(AC, CP);
		if (d6 >= 0.0f && d5 <= d6)
			return FVector::Dist(target_point, pointC);

		// Check if target_point is in edge region of AC, if so return distance to AC
		float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
			float w = d2 / (d2 - d6);
			FVector Closesttarget_point = pointA + w * AC;
			return FVector::Dist(target_point, Closesttarget_point);
		}

		// Check if target_point is in edge region of BC, if so return distance to BC
		float va = d3 * d6 - d5 * d4;
		if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
			float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			FVector BC = pointC - pointB;
			FVector Closesttarget_point = pointB + w * BC;
			return FVector::Dist(target_point, Closesttarget_point);
		}

		// target_point is inside face region. Compute distance to plane.
		FVector Normal = FVector::CrossProduct(AB, AC).GetSafeNormal();
		float Distance = FVector::DotProduct(target_point - pointA, Normal);
		return FMath::Abs(Distance);
	}
	else
	{
		return BIG_NUMBER;
	}
}

float UYogBlueprintFunctionLibrary::DistFromPointToAnnulus(UObject* WorldContextObject, FVector target_point, FYogCollisionAnnulus annulus)
{
	// Calculate 2D vector from center to target point
	FVector2D CenterToPoint = FVector2D(target_point - annulus.Center);
	float DistanceFromCenter = CenterToPoint.Size();

	// Calculate angle in degrees (0-360)
	float PointAngle = FMath::RadiansToDegrees(FMath::Atan2(CenterToPoint.Y, CenterToPoint.X));
	PointAngle = FMath::Fmod(PointAngle + 360.0f, 360.0f);

	// Normalize angles
	float StartAngle = FMath::Fmod(annulus.start_angle_degree + 360.0f, 360.0f);
	float EndAngle = FMath::Fmod(annulus.end_angle_degree + 360.0f, 360.0f);

	// Helper function to calculate minimal angular distance
	auto GetMinAngularDistance = [](float Angle1, float Angle2) -> float
		{
			float Diff = FMath::Abs(Angle1 - Angle2);
			return FMath::Min(Diff, 360.0f - Diff);
		};

	// Check if in angular sector
	bool bInAngularSector = false;
	if (StartAngle <= EndAngle)
	{
		bInAngularSector = (PointAngle >= StartAngle && PointAngle <= EndAngle);
	}
	else
	{
		bInAngularSector = (PointAngle >= StartAngle || PointAngle <= EndAngle);
	}

	if (bInAngularSector)
	{
		// Within angular sector, check radial distance
		if (DistanceFromCenter < annulus.inner_radius)
		{
			return annulus.inner_radius - DistanceFromCenter;
		}
		else if (DistanceFromCenter > annulus.outer_radius)
		{
			return DistanceFromCenter - annulus.outer_radius;
		}
		else
		{
			return 0.0f; // Inside annulus
		}
	}
	else
	{
		// Outside angular sector - find closest point on annulus
		// Calculate distances to sector edges
		float DistToStart = GetMinAngularDistance(PointAngle, StartAngle);
		float DistToEnd = GetMinAngularDistance(PointAngle, EndAngle);
		float MinAngularDist = FMath::Min(DistToStart, DistToEnd);

		// Find the closest radial distance
		float RadialDist = 0.0f;
		if (DistanceFromCenter < annulus.inner_radius)
		{
			RadialDist = annulus.inner_radius - DistanceFromCenter;
		}
		else if (DistanceFromCenter > annulus.outer_radius)
		{
			RadialDist = DistanceFromCenter - annulus.outer_radius;
		}
		else
		{
			// Between inner and outer radius but wrong angle
			// Closest point is at same radius but at sector edge
			RadialDist = 0.0f;
		}

		// For points outside angular sector, the closest point is either:
		// 1. On the radial edge (if radial distance > 0)
		// 2. On the angular edge (if between radii)
		// We take the minimum of these possibilities

		// Distance to angular edge at current radius
		float AngularEdgeDist = DistanceFromCenter * FMath::Tan(FMath::DegreesToRadians(MinAngularDist * 0.5f)) * 2.0f;

		return FMath::Sqrt(FMath::Square(RadialDist) + FMath::Square(AngularEdgeDist));
	}
}
float UYogBlueprintFunctionLibrary::DistFromPointToSquare(UObject* WorldContextObject, FVector target_point, FYogCollisionSquare square)
{
	return 0.0f;
}



