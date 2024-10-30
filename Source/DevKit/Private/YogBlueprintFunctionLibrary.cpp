// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBlueprintFunctionLibrary.h"



// Copyright Epic Games, Inc. All Rights Reserved.


//#include "ActionRPGLoadingScreen.h"



//void UYogBlueprintFunctionLibrary::PlayLoadingScreen(bool bPlayUntilStopped, float PlayTime)
//{
//	IActionRPGLoadingScreenModule& LoadingScreenModule = IActionRPGLoadingScreenModule::Get();
//	LoadingScreenModule.StartInGameLoadingScreen(bPlayUntilStopped, PlayTime);
//}
//
//void UYogBlueprintFunctionLibrary::StopLoadingScreen()
//{
//	IActionRPGLoadingScreenModule& LoadingScreenModule = IActionRPGLoadingScreenModule::Get();
//	LoadingScreenModule.StopInGameLoadingScreen();
//}

bool UYogBlueprintFunctionLibrary::IsInEditor()
{
	return GIsEditor;
}

//bool UYogBlueprintFunctionLibrary::EqualEqual_RPGItemSlot(const FRPGItemSlot& A, const FRPGItemSlot& B)
//{
//	return A == B;
//}
//
//bool UYogBlueprintFunctionLibrary::NotEqual_RPGItemSlot(const FRPGItemSlot& A, const FRPGItemSlot& B)
//{
//	return A != B;
//}
//
//bool UYogBlueprintFunctionLibrary::IsValidItemSlot(const FRPGItemSlot& ItemSlot)
//{
//	return ItemSlot.IsValid();
//}

//bool UYogBlueprintFunctionLibrary::DoesEffectContainerSpecHaveEffects(const FRPGGameplayEffectContainerSpec& ContainerSpec)
//{
//	return ContainerSpec.HasValidEffects();
//}
//
//bool UYogBlueprintFunctionLibrary::DoesEffectContainerSpecHaveTargets(const FRPGGameplayEffectContainerSpec& ContainerSpec)
//{
//	return ContainerSpec.HasValidTargets();
//}

//FRPGGameplayEffectContainerSpec UYogBlueprintFunctionLibrary::AddTargetsToEffectContainerSpec(const FRPGGameplayEffectContainerSpec& ContainerSpec, const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors)
//{
//	FRPGGameplayEffectContainerSpec NewSpec = ContainerSpec;
//	NewSpec.AddTargets(HitResults, TargetActors);
//	return NewSpec;
//}
//
//TArray<FActiveGameplayEffectHandle> UYogBlueprintFunctionLibrary::ApplyExternalEffectContainerSpec(const FRPGGameplayEffectContainerSpec& ContainerSpec)
//{
//	TArray<FActiveGameplayEffectHandle> AllEffects;
//
//	// Iterate list of gameplay effects
//	for (const FGameplayEffectSpecHandle& SpecHandle : ContainerSpec.TargetGameplayEffectSpecs)
//	{
//		if (SpecHandle.IsValid())
//		{
//			// If effect is valid, iterate list of targets and apply to all
//			for (TSharedPtr<FGameplayAbilityTargetData> Data : ContainerSpec.TargetData.Data)
//			{
//				AllEffects.Append(Data->ApplyGameplayEffectSpec(*SpecHandle.Data.Get()));
//			}
//		}
//	}
//	return AllEffects;
//}

