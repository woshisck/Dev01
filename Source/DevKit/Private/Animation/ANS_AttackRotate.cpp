// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/ANS_AttackRotate.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UANS_AttackRotate::NotifyTick(
	USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (RotateSpeed <= 0.f || !MeshComp) return;

	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
	if (!Character) return;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement) return;

	// 取最后一帧移动输入向量（已归一化到 [-1,1]，由 Controller 输入轴写入）
	FVector InputDir = Movement->GetLastInputVector();
	InputDir.Z = 0.f;
	if (InputDir.IsNearlyZero()) return; // 无输入：保持当前朝向，不旋转

	InputDir.Normalize();

	const float TargetYaw  = FMath::RadiansToDegrees(FMath::Atan2(InputDir.Y, InputDir.X));
	const float CurrentYaw = Character->GetActorRotation().Yaw;

	// 按 RotateSpeed 度/秒限幅旋转，避免瞬间跳转朝向
	const float MaxDelta  = RotateSpeed * FrameDeltaTime;
	const float DeltaYaw  = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
	const float ClampedDelta = FMath::Clamp(DeltaYaw, -MaxDelta, MaxDelta);

	Character->SetActorRotation(FRotator(0.f, CurrentYaw + ClampedDelta, 0.f));
}

FString UANS_AttackRotate::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Atk Rotate | %.0f°/s"), RotateSpeed);
}
