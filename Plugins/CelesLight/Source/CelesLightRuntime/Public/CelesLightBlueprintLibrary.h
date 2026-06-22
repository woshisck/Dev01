#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CelesLightBlueprintLibrary.generated.h"

class UCelesLightReceiveComponent;
class UTextureRenderTarget2D;

UCLASS()
class CELESLIGHTRUNTIME_API UCelesLightBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Celes Light", meta = (WorldContext = "WorldContextObject"))
	static void CelesLightReceiveTrace(
		const UObject* WorldContextObject,
		FVector Origin,
		float Radius,
		const TArray<FName>& FilterActorTags,
		TArray<AActor*>& ReceiveActors);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	static UTextureRenderTarget2D* CreateLightInfoRenderTarget(UObject* Outer, int32 LightInfoCount);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	static bool ConfigureLightInfoRenderTarget(UTextureRenderTarget2D* RenderTarget, int32 LightInfoCount);
};
