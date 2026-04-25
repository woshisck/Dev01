#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "InputActionRichTextDecorator.generated.h"

class UInputAction;

UCLASS(Blueprintable)
class DEVKIT_API UInputActionRichTextDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "InputAction")
	TMap<FName, TSoftObjectPtr<UInputAction>> ActionMap;

	UPROPERTY(EditAnywhere, Category = "InputAction")
	FVector2D IconSize = FVector2D(24.f, 24.f);

	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
};
