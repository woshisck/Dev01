#include "BuffFlow/Nodes/BFNode_ApplyGEInRadius.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Types/FlowDataPinResults.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

UBFNode_ApplyGEInRadius::UBFNode_ApplyGEInRadius(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	Radius = FFlowDataPinInputProperty_Float(300.f);

	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("NoHit")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyGEInRadius::ExecuteInput(const FName& PinName)
{
	if (!Effect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_ApplyGEInRadius] Effect 未设置"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UYogAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!OwnerASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AYogCharacterBase* Owner = GetBuffOwner();

	// ── 解析中心位置 ──────────────────────────────────────────────
	FVector Center = LocationOffset;
	AActor* LocationSourceActor = nullptr;

	if (bUseKillLocation)
	{
		UBuffFlowComponent* BFC = GetBuffFlowComponent();
		if (BFC)
		{
			Center = BFC->LastKillLocation + LocationOffset;
		}
	}
	else
	{
		AActor* SourceActor = ResolveTarget(LocationSource);
		if (!SourceActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_ApplyGEInRadius] LocationSource 解析为 null"));
			TriggerOutput(TEXT("Failed"), true);
			return;
		}
		LocationSourceActor = SourceActor;
		Center = SourceActor->GetActorLocation() + LocationOffset;
	}

	// ── 解析半径 ──────────────────────────────────────────────────
	float ResolvedRadius = Radius.Value;
	{
		FFlowDataPinResult_Float Res = TryResolveDataPinAsFloat(
			GET_MEMBER_NAME_CHECKED(UBFNode_ApplyGEInRadius, Radius));
		if (Res.Result == EFlowDataPinResolveResult::Success)
		{
			ResolvedRadius = Res.Value;
		}
	}

	// ── 球形重叠检测 ──────────────────────────────────────────────
	UWorld* World = GetWorld();
	if (!World)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = false;
	if (bExcludeSelf && Owner)
	{
		QueryParams.AddIgnoredActor(Owner);
	}
	if (bExcludeLocationSourceActor && LocationSourceActor)
	{
		QueryParams.AddIgnoredActor(LocationSourceActor);
	}

	World->OverlapMultiByObjectType(
		Overlaps, Center, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(ResolvedRadius), QueryParams);

	// ── 构建 GE Spec ─────────────────────────────────────────────
	FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = OwnerASC->MakeOutgoingSpec(Effect, 1.f, Context);
	if (!Spec.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// SetByCaller 填充
	auto ApplySetByCaller = [&](const FGameplayTag& Tag, const FName& Member,
	                            const FFlowDataPinInputProperty_Float& Default)
	{
		if (!Tag.IsValid()) return;
		FFlowDataPinResult_Float Res = TryResolveDataPinAsFloat(Member);
		const float Val = (Res.Result == EFlowDataPinResolveResult::Success) ? Res.Value : Default.Value;
		Spec.Data->SetSetByCallerMagnitude(Tag, Val);
	};
	ApplySetByCaller(SetByCallerTag1, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyGEInRadius, SetByCallerValue1), SetByCallerValue1);
	ApplySetByCaller(SetByCallerTag2, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyGEInRadius, SetByCallerValue2), SetByCallerValue2);

	// ── 对每个有效目标施加 GE ────────────────────────────────────
	TSet<AActor*> ProcessedActors;
	TArray<TPair<float, AActor*>> CandidateActors;
	CandidateActors.Reserve(Overlaps.Num());

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || ProcessedActors.Contains(HitActor))
		{
			continue;
		}
		ProcessedActors.Add(HitActor);

		if (bExcludeLocationSourceActor && HitActor == LocationSourceActor)
		{
			continue;
		}

		if (bEnemyOnly && !Cast<AEnemyCharacterBase>(HitActor))
		{
			continue;
		}

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC)
		{
			continue;
		}

		CandidateActors.Add(TPair<float, AActor*>(FVector::DistSquared(Center, HitActor->GetActorLocation()), HitActor));
	}

	CandidateActors.Sort([](const TPair<float, AActor*>& A, const TPair<float, AActor*>& B)
	{
		return A.Key < B.Key;
	});

	int32 Count = 0;
	const int32 MaxResolvedTargets = MaxTargets > 0 ? MaxTargets : MAX_int32;
	const int32 ResolvedApplicationCount = FMath::Clamp(ApplicationCount, 1, 20);

	for (const TPair<float, AActor*>& Candidate : CandidateActors)
	{
		if (Count >= MaxResolvedTargets)
		{
			break;
		}

		AActor* HitActor = Candidate.Value;
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC)
		{
			continue;
		}

		for (int32 ApplyIndex = 0; ApplyIndex < ResolvedApplicationCount; ++ApplyIndex)
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
		++Count;
	}

	HitCount = FFlowDataPinOutputProperty_Int32(Count);

	if (Count > 0)
	{
		TriggerOutput(TEXT("Out"), true);
	}
	else
	{
		TriggerOutput(TEXT("NoHit"), true);
	}
}
