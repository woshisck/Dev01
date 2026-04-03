#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_GetAttribute.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "获取属性值", Category = "BuffFlow|属性"))
class DEVKIT_API UBFNode_GetAttribute : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** 获取到的属性值（供后续节点读取） */
	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow")
	float CachedValue = 0.f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
