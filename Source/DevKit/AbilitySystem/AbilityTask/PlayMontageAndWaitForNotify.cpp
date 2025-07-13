// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayMontageAndWaitForNotify.h"

/*
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"

// Function to play montage and wait for anim notify
void UYourComponent::PlayMontageAndWaitForNotify(UAnimMontage* MontageToPlay, FName NotifyName, float PlayRate, FName StartSection)
{
    if (!MontageToPlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("MontageToPlay is null!"));
        return;
    }

    USkeletalMeshComponent* SkelMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
    if (!SkelMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("No skeletal mesh component found!"));
        return;
    }

    UAnimInstance* AnimInstance = SkelMesh->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("No anim instance found!"));
        return;
    }

    // Play the montage
    float MontageLength = AnimInstance->Montage_Play(MontageToPlay, PlayRate, EMontagePlayReturnType::MontageLength, 0.0f, false);

    // Bind to notify delegate
    FOnMontageBlendingOutStarted BlendingOutDelegate;
    FOnMontageEnded MontageEndedDelegate;
    FDelegateHandle NotifyHandle;

    // This is where we'll handle the notify
    auto OnNotify = [this, NotifyName](FName Notify, const FBranchingPointNotifyPayload& BranchingPointPayload)
    {
        if (Notify == NotifyName)
        {
            // Notify received - do something
            OnAnimNotifyReceived(NotifyName);

            // Unbind the delegate
            if (USkeletalMeshComponent* SkelMeshComp = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
            {
                if (UAnimInstance* AnimInst = SkelMeshComp->GetAnimInstance())
                {
                    AnimInst->OnPlayMontageNotifyBegin.Remove(NotifyHandle);
                }
            }
        }
    };

    // Bind the notify delegate
    NotifyHandle = AnimInstance->OnPlayMontageNotifyBegin.AddLambda(OnNotify);

    // Optional: Handle montage ending prematurely
    BlendingOutDelegate.BindLambda([this, NotifyHandle](UAnimMontage* Montage, bool bInterrupted)
    {
        if (USkeletalMeshComponent* SkelMeshComp = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (UAnimInstance* AnimInst = SkelMeshComp->GetAnimInstance())
            {
                AnimInst->OnPlayMontageNotifyBegin.Remove(NotifyHandle);
            }
        }

        if (bInterrupted)
        {
            OnMontageInterrupted();
        }
    });

    MontageEndedDelegate.BindLambda([this, NotifyHandle](UAnimMontage* Montage, bool bInterrupted)
    {
        if (USkeletalMeshComponent* SkelMeshComp = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (UAnimInstance* AnimInst = SkelMeshComp->GetAnimInstance())
            {
                AnimInst->OnPlayMontageNotifyBegin.Remove(NotifyHandle);
            }
        }

        OnMontageEnded(bInterrupted);
    });

    AnimInstance->OnMontageBlendingOut.Add(BlendingOutDelegate);
    AnimInstance->OnMontageEnded.Add(MontageEndedDelegate);
}

// Called when the anim notify is received
void UYourComponent::OnAnimNotifyReceived(FName NotifyName)
{
    // Implement your logic here when the notify is received
    UE_LOG(LogTemp, Log, TEXT("Received anim notify: %s"), *NotifyName.ToString());
}

// Called if montage ends prematurely
void UYourComponent::OnMontageEnded(bool bInterrupted)
{
    UE_LOG(LogTemp, Warning, TEXT("Montage ended before notify was received!"));
}

// Called if montage is interrupted
void UYourComponent::OnMontageInterrupted()
{
    UE_LOG(LogTemp, Warning, TEXT("Montage was interrupted!"));
}
*/