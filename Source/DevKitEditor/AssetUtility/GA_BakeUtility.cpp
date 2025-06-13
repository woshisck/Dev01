// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_BakeUtility.h"
#include "../DevKit/AbilitySystem/Abilities/YogGameplayAbility.h"
#include <EditorUtilityLibrary.h>


UGA_BakeUtility::UGA_BakeUtility()
{
    SupportedClasses.Add(UYogGameplayAbility::StaticClass());
    SupportedClasses.Add(UBlueprint::StaticClass());
}

void UGA_BakeUtility::BakeData()
{
    TArray<UObject*> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();


    for (UObject* Asset : SelectedAssets)
    {
        if (Asset->IsA(UYogGameplayAbility::StaticClass()))
        {
            UE_LOG(LogTemp, Warning, TEXT("FIND IT!: %s"), *Asset->GetName());
            // Perform actions on the Static Mesh
        
            // ...
        }
    }

}
