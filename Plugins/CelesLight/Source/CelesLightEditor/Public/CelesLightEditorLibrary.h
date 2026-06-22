#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CelesLightEditorLibrary.generated.h"

class ACelesLightCaptureBox;
class ACelesPointLight;

UCLASS()
class CELESLIGHTEDITOR_API UCelesLightEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static int32 ManualUpdateCelesLights(UWorld* World = nullptr);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static ACelesLightCaptureBox* CreateCelesLightCaptureBox(UWorld* World = nullptr);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static ACelesPointLight* CreateCelesPointLight(UWorld* World = nullptr);
};
