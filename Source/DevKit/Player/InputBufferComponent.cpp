// Fill out your copyright notice in the Description page of Project Settings.


#include "InputBufferComponent.h"


// Sets default values for this component's properties
UInputBufferComponent::UInputBufferComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;


}


// Called when the game starts
void UInputBufferComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UInputBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UInputBufferComponent::UpdateActionBuffer(EPlayerActionInput actionInput)
{
}

void UInputBufferComponent::UpdateMoveBuffer(FVector2D move)
{
	
	

	if (MovementBuffer.Num() < MAX_BUFFER_SIZE)
	{
		MovementBuffer.Add(move);
		/*FVector2D cache_item = MovementBuffer[0];*/

	}

	//if (MovementBuffer.Top() != move) {
	//	//update buffer 
	//	MovementBuffer.Push(move);

	//}
	//else if (MovementBuffer.Num() > MOVE_BUFFER_SIZE) {
	//	MovementBuffer.Empty();
	//	MovementBuffer.Push(move);

	//}

}



//FVector2D UInputBufferComponent::GetLastFrameInput(FVector2D Movement)
//{
//	return buffer.Pop();
//}

void UInputBufferComponent::ClearActionBuffer()
{
	ClearBuffer(ActionBuffer);
}

void UInputBufferComponent::ClearMovementBuffer()
{
	ClearBuffer(MovementBuffer);
}

void UInputBufferComponent::DebugPrintAction()
{
	for (const EPlayerActionInput& Element : ActionBuffer)
	{
		FString result = UEnum::GetDisplayValueAsText(Element).ToString();
		UE_LOG(LogTemp, Warning, TEXT("Array element: %s"), *result);
	}
}

void UInputBufferComponent::DebugPrintMovement()
{
	for (const FVector2D& Element : MovementBuffer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Array element: %s"), *Element.ToString());
	}
}


FVector2D UInputBufferComponent::GetLastMoveInput(FVector2D Movement)
{
	return GetLastItem(MovementBuffer);
}

