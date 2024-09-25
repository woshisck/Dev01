// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "AI/GSCBTTask_BlueprintBase.h"
#include "Engine/BlueprintGeneratedClass.h"

UGSCBTTask_BlueprintBase::UGSCBTTask_BlueprintBase()
{
	auto ImplementedInBlueprint = [](const UFunction* Func) -> bool
	{
		return Func && ensure(Func->GetOuter())
			&& Func->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	};

	static FName FuncName = FName(TEXT("K2_GetStaticDescription"));
	UFunction* StaticDescriptionFunction = GetClass()->FindFunctionByName(FuncName);
	bHasBlueprintStaticDescription = ImplementedInBlueprint(StaticDescriptionFunction);
}

FString UGSCBTTask_BlueprintBase::GetStaticDescription() const
{
	FString ReturnDesc = Super::GetStaticDescription();

	if (bHasBlueprintStaticDescription)
	{
		const FString Description = K2_GetStaticDescription();
		if (!Description.IsEmpty())
		{
			ReturnDesc = Description;
		}
	}

	return ReturnDesc;
}
